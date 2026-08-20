#include <ctype.h>
#include <dirent.h>
#include <netinet/in.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

#include "app_manager.h"
#include "log_capture.h"
#include "web_state.h"
#include "web_utils.h"
#include "websocket.h"

#define PATH_CAPACITY 512
#define APP_JOB_CAPACITY 16
#define APP_STATUS_CAPACITY 8192

typedef struct app_job
{
    int used;
    int reader_done;
    int active;
    int fd;
    unsigned long job_id;
    long pid;
    int state;
    int exit_code;
    int finished;
    char app_id[WEB_APP_ID_CAPACITY];
} app_job_t;

static unsigned long next_job_id = 1;
static unsigned short supervisor_port;
static pthread_mutex_t jobs_mutex = PTHREAD_MUTEX_INITIALIZER;
static app_job_t jobs[APP_JOB_CAPACITY];

static int valid_app_id(const char *id)
{
    const unsigned char *cursor = (const unsigned char *)id;
    if (*cursor == '\0')
        return 0;
    while (*cursor)
    {
        if (!(isalnum(*cursor) || *cursor == '_' || *cursor == '-'))
            return 0;
        cursor++;
    }
    return 1;
}

static int manifest_value(const char *manifest, const char *key, char *value, size_t value_size)
{
    char buffer[4096];
    char needle[80];
    FILE *file = fopen(manifest, "rb");
    if (file == NULL)
        return -1;
    size_t length = fread(buffer, 1, sizeof buffer - 1, file);
    fclose(file);
    buffer[length] = '\0';
    snprintf(needle, sizeof needle, "\"%s\"", key);
    char *cursor = strstr(buffer, needle);
    if (cursor == NULL)
        return -1;
    cursor = strchr(cursor + strlen(needle), ':');
    if (cursor == NULL)
        return -1;
    cursor++;
    while (*cursor && isspace((unsigned char)*cursor))
        cursor++;
    if (*cursor++ != '"')
        return -1;
    char *end = strchr(cursor, '"');
    if (end == NULL || (size_t)(end - cursor) + 1 > value_size)
        return -1;
    memcpy(value, cursor, (size_t)(end - cursor));
    value[end - cursor] = '\0';
    return 0;
}

static enum MHD_Result response(struct MHD_Connection *connection, unsigned status,
                                const char *type, const void *body, size_t length,
                                enum MHD_ResponseMemoryMode mode)
{
    struct MHD_Response *result = MHD_create_response_from_buffer(length, (void *)body, mode);
    if (result == NULL)
    {
        if (mode == MHD_RESPMEM_MUST_FREE)
            free((void *)body);
        return MHD_NO;
    }
    MHD_add_response_header(result, "Content-Type", type);
    MHD_add_response_header(result, "Access-Control-Allow-Origin", "*");
    enum MHD_Result queued = MHD_queue_response(connection, status, result);
    MHD_destroy_response(result);
    return queued;
}

static enum MHD_Result text_response(struct MHD_Connection *connection, unsigned status,
                                     const char *body)
{
    return response(connection, status, "text/plain", body, strlen(body), MHD_RESPMEM_MUST_COPY);
}

enum MHD_Result app_list_response(struct MHD_Connection *connection)
{
    DIR *directory = opendir("/data/python/apps");
    char body[8192];
    size_t at = (size_t)snprintf(body, sizeof body, "[");
    int first = 1;
    if (directory != NULL)
    {
        struct dirent *entry;
        while ((entry = readdir(directory)) != NULL)
        {
            char manifest[PATH_CAPACITY];
            char name[128];
            if (entry->d_name[0] == '.' || !valid_app_id(entry->d_name))
                continue;
            snprintf(manifest, sizeof manifest, "/data/python/apps/%s/app.json", entry->d_name);
            if (manifest_value(manifest, "name", name, sizeof name) != 0)
                snprintf(name, sizeof name, "%s", entry->d_name);
            if (!first)
                at += (size_t)snprintf(body + at, sizeof body - at, ",");
            at += (size_t)snprintf(body + at, sizeof body - at, "{\"id\":\"");
            at = web_json_append(body, at, sizeof body, entry->d_name);
            at += (size_t)snprintf(body + at, sizeof body - at, "\",\"name\":\"");
            at = web_json_append(body, at, sizeof body, name);
            at += (size_t)snprintf(body + at, sizeof body - at, "\"}");
            first = 0;
            if (at + 128 >= sizeof body)
                break;
        }
        closedir(directory);
    }
    at += (size_t)snprintf(body + at, sizeof body - at, "]");
    return response(connection, MHD_HTTP_OK, "application/json", body, at, MHD_RESPMEM_MUST_COPY);
}

static int write_all(int fd, const void *data, size_t length)
{
    const char *cursor = data;
    while (length > 0)
    {
        ssize_t written = send(fd, cursor, length, 0);
        if (written <= 0)
            return -1;
        cursor += written;
        length -= (size_t)written;
    }
    return 0;
}

static int read_line(int fd, char *line, size_t capacity)
{
    size_t at = 0;
    while (at + 1 < capacity)
    {
        char ch;
        ssize_t length = recv(fd, &ch, 1, 0);
        if (length <= 0)
            return -1;
        if (ch == '\n')
        {
            line[at] = '\0';
            return 0;
        }
        if (ch != '\r')
            line[at++] = ch;
    }
    return -1;
}

static int read_bytes(int fd, char *buffer, size_t length)
{
    size_t at = 0;
    while (at < length)
    {
        ssize_t received = recv(fd, buffer + at, length - at, 0);
        if (received <= 0)
            return -1;
        at += (size_t)received;
    }
    return 0;
}

static int connect_supervisor(void)
{
    struct sockaddr_in address;
    int fd = socket(AF_INET, SOCK_STREAM, 0);
    if (fd < 0)
        return -1;
    memset(&address, 0, sizeof address);
    address.sin_family = AF_INET;
    address.sin_port = htons(supervisor_port);
    address.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    if (connect(fd, (struct sockaddr *)&address, sizeof address) != 0)
    {
        close(fd);
        return -1;
    }
    return fd;
}

static unsigned active_job_count_locked(void)
{
    unsigned count = 0;
    for (unsigned i = 0; i < APP_JOB_CAPACITY; i++)
    {
        if (jobs[i].used && jobs[i].active)
            count++;
    }
    return count;
}

static void sync_legacy_state_locked(void)
{
    unsigned active = active_job_count_locked();
    pthread_mutex_lock(&web_state_mutex);
    web_launch_started = active != 0;
    web_app_running = active != 0;
    pthread_mutex_unlock(&web_state_mutex);
}

static app_job_t *find_job_locked(unsigned long job_id)
{
    for (unsigned i = 0; i < APP_JOB_CAPACITY; i++)
    {
        if (jobs[i].used && jobs[i].job_id == job_id)
            return &jobs[i];
    }
    return NULL;
}

static app_job_t *new_job_locked(void)
{
    app_job_t *candidate = NULL;
    for (unsigned i = 0; i < APP_JOB_CAPACITY; i++)
    {
        if (!jobs[i].used)
            return &jobs[i];
        if (jobs[i].reader_done && !jobs[i].active &&
            (candidate == NULL || jobs[i].job_id < candidate->job_id))
            candidate = &jobs[i];
    }
    return candidate;
}

static void fail_job(app_job_t *job, int exit_code)
{
    pthread_mutex_lock(&jobs_mutex);
    job->active = 0;
    job->reader_done = 1;
    job->pid = 0;
    job->fd = -1;
    job->finished = 1;
    job->exit_code = exit_code;
    job->state = WEB_APP_FAILED;
    sync_legacy_state_locked();
    pthread_mutex_unlock(&jobs_mutex);
    websocket_broadcast_status();
}

static void finish_job(app_job_t *job, int result, int stopped)
{
    char line[128];
    int length;
    pthread_mutex_lock(&jobs_mutex);
    if (!job->active)
    {
        pthread_mutex_unlock(&jobs_mutex);
        return;
    }
    job->active = 0;
    job->pid = 0;
    job->fd = -1;
    job->finished = 1;
    job->exit_code = stopped ? 130 : result;
    job->state = stopped ? WEB_APP_STOPPED : result == 0 ? WEB_APP_FINISHED : WEB_APP_FAILED;
    pthread_mutex_lock(&web_state_mutex);
    if (web_app_job_id == job->job_id)
    {
        web_app_pid = 0;
        web_app_finished = 1;
        web_app_exit_code = job->exit_code;
        web_app_state = job->state;
    }
    pthread_mutex_unlock(&web_state_mutex);
    sync_legacy_state_locked();
    pthread_mutex_unlock(&jobs_mutex);
    websocket_broadcast_status();
    length = stopped ? snprintf(line, sizeof line, "\n[launcher] job %lu (%s) stopped\n",
                                job->job_id, job->app_id) :
                      snprintf(line, sizeof line, "\n[launcher] job %lu (%s) exited with code %d\n",
                               job->job_id, job->app_id, result);
    if (length > 0)
        log_append(line, (size_t)length);
}

static void *supervisor_reader(void *data)
{
    app_job_t *job = data;
    int fd;
    char line[128];

    pthread_mutex_lock(&jobs_mutex);
    fd = job->fd;
    pthread_mutex_unlock(&jobs_mutex);
    for (;;)
    {
        char *cursor;
        if (read_line(fd, line, sizeof line) != 0)
            break;
        if (!strncmp(line, "LOG ", 4))
        {
            unsigned long length = strtoul(line + 4, &cursor, 10);
            char buffer[1024];
            if (cursor == line + 4 || *cursor != '\0')
                break;
            while (length > 0)
            {
                size_t chunk = length < sizeof buffer ? (size_t)length : sizeof buffer;
                if (read_bytes(fd, buffer, chunk) != 0)
                    goto disconnected;
                log_append(buffer, chunk);
                length -= chunk;
            }
        }
        else if (!strcmp(line, "STOPPING"))
        {
            pthread_mutex_lock(&jobs_mutex);
            if (job->active)
                job->state = WEB_APP_STOPPING;
            pthread_mutex_unlock(&jobs_mutex);
            websocket_broadcast_status();
        }
        else if (!strncmp(line, "EXIT ", 5))
        {
            int result = (int)strtol(line + 5, &cursor, 10);
            if (cursor == line + 5 || *cursor != '\0')
                break;
            finish_job(job, result, result == 130);
            goto disconnected;
        }
    }

disconnected:
    close(fd);
    pthread_mutex_lock(&jobs_mutex);
    if (job->active)
    {
        pthread_mutex_unlock(&jobs_mutex);
        finish_job(job, 1, 0);
    }
    else
        pthread_mutex_unlock(&jobs_mutex);
    pthread_mutex_lock(&jobs_mutex);
    job->reader_done = 1;
    pthread_mutex_unlock(&jobs_mutex);
    return NULL;
}

int app_manager_status_json(char *body, size_t capacity)
{
    size_t at = 0;
    int written;
    int started;
    int running;
    int finished;
    int exit_code;
    unsigned long selected_job_id;
    long selected_pid;
    int selected_state;
    char selected_app[WEB_APP_ID_CAPACITY];

    pthread_mutex_lock(&jobs_mutex);
    pthread_mutex_lock(&web_state_mutex);
    started = web_launch_started;
    running = web_app_running;
    finished = web_app_finished;
    exit_code = web_app_exit_code;
    selected_job_id = web_app_job_id;
    selected_pid = web_app_pid;
    selected_state = web_app_state;
    snprintf(selected_app, sizeof selected_app, "%s", web_app_id);
    pthread_mutex_unlock(&web_state_mutex);
    written = snprintf(body + at, capacity - at,
                       "{\"type\":\"status\",\"pid\":%ld,\"app_pid\":%ld,\"started\":%s,"
                       "\"running\":%s,\"finished\":%s,\"exit_code\":%d,\"repl_port\":%u,"
                       "\"job_id\":%lu,\"app\":\"",
                       (long)getpid(), selected_pid, started ? "true" : "false",
                       running ? "true" : "false", finished ? "true" : "false", exit_code,
                       (unsigned)tcp_repl_port, selected_job_id);
    if (written < 0 || (size_t)written >= capacity - at)
        goto too_small;
    at += (size_t)written;
    at = web_json_append(body, at, capacity, selected_app);
    written = snprintf(body + at, capacity - at, "\",\"state\":\"%s\",\"jobs\":[",
                       web_app_state_name(selected_state));
    if (written < 0 || (size_t)written >= capacity - at)
        goto too_small;
    at += (size_t)written;
    int first = 1;
    for (unsigned i = 0; i < APP_JOB_CAPACITY; i++)
    {
        app_job_t *job = &jobs[i];
        if (!job->used)
            continue;
        written = snprintf(body + at, capacity - at, "%s{\"job_id\":%lu,\"app\":\"",
                           first ? "" : ",", job->job_id);
        if (written < 0 || (size_t)written >= capacity - at)
            goto too_small;
        at += (size_t)written;
        at = web_json_append(body, at, capacity, job->app_id);
        written = snprintf(body + at, capacity - at,
                           "\",\"app_pid\":%ld,\"running\":%s,\"finished\":%s,"
                           "\"exit_code\":%d,\"state\":\"%s\"}",
                           job->pid, job->active ? "true" : "false",
                           job->finished ? "true" : "false", job->exit_code,
                           web_app_state_name(job->state));
        if (written < 0 || (size_t)written >= capacity - at)
            goto too_small;
        at += (size_t)written;
        first = 0;
    }
    pthread_mutex_unlock(&jobs_mutex);
    written = snprintf(body + at, capacity - at, "]}");
    return written < 0 || (size_t)written >= capacity - at ? -1 : (int)(at + written);

too_small:
    pthread_mutex_unlock(&jobs_mutex);
    return -1;
}

int app_manager_start(unsigned short port)
{
    supervisor_port = port;
    return 0;
}

void app_manager_stop(void)
{
    pthread_mutex_lock(&jobs_mutex);
    for (unsigned i = 0; i < APP_JOB_CAPACITY; i++)
    {
        if (jobs[i].used && jobs[i].active && jobs[i].fd >= 0)
            (void)write_all(jobs[i].fd, "STOP\n", 5);
    }
    pthread_mutex_unlock(&jobs_mutex);

    int fd = connect_supervisor();
    if (fd >= 0)
    {
        (void)write_all(fd, "SHUTDOWN\n", 9);
        close(fd);
    }
}

enum MHD_Result app_launch_response(struct MHD_Connection *connection, const char *query)
{
    char app_id[WEB_APP_ID_CAPACITY];
    char manifest[PATH_CAPACITY];
    char entry[128];
    app_job_t *job;
    if (query == NULL || strncmp(query, "app=", 4) != 0 || strlen(query + 4) >= sizeof app_id ||
        !valid_app_id(query + 4))
        return text_response(connection, MHD_HTTP_BAD_REQUEST, "invalid app");
    snprintf(app_id, sizeof app_id, "%s", query + 4);
    snprintf(manifest, sizeof manifest, "/data/python/apps/%s/app.json", app_id);
    if (manifest_value(manifest, "entry", entry, sizeof entry) != 0 || entry[0] == '/' ||
        strstr(entry, "..") != NULL)
        return text_response(connection, MHD_HTTP_BAD_REQUEST, "invalid manifest");
    char app_root[PATH_CAPACITY];
    char app_lib[PATH_CAPACITY];
    char script_path[PATH_CAPACITY];
    snprintf(app_root, sizeof app_root, "/data/python/apps/%s", app_id);
    snprintf(app_lib, sizeof app_lib, "%s/lib", app_root);
    snprintf(script_path, sizeof script_path, "%s/%s", app_root, entry);
    if (access(script_path, R_OK) != 0)
        return text_response(connection, MHD_HTTP_NOT_FOUND, "entry not found");

    pthread_mutex_lock(&jobs_mutex);
    job = new_job_locked();
    if (job == NULL)
    {
        pthread_mutex_unlock(&jobs_mutex);
        return text_response(connection, MHD_HTTP_CONFLICT, "too many app jobs");
    }
    memset(job, 0, sizeof *job);
    job->used = 1;
    job->active = 1;
    job->fd = -1;
    job->job_id = next_job_id++;
    job->exit_code = -1;
    job->state = WEB_APP_STARTING;
    snprintf(job->app_id, sizeof job->app_id, "%s", app_id);
    pthread_mutex_lock(&web_state_mutex);
    web_launch_started = 1;
    web_app_running = 1;
    web_app_finished = 0;
    web_app_exit_code = -1;
    web_app_job_id = job->job_id;
    web_app_pid = 0;
    snprintf(web_app_id, sizeof web_app_id, "%s", app_id);
    web_app_state = WEB_APP_STARTING;
    pthread_mutex_unlock(&web_state_mutex);
    pthread_mutex_unlock(&jobs_mutex);
    websocket_broadcast_status();

    char line[PATH_CAPACITY * 3 + 16];
    int fd = connect_supervisor();
    if (fd < 0 || snprintf(line, sizeof line, "START\t%s\t%s\t%s\n", script_path,
                           app_root, app_lib) >= (int)sizeof line ||
        write_all(fd, line, strlen(line)) != 0)
    {
        if (fd >= 0)
            close(fd);
        fail_job(job, 1);
        return text_response(connection, MHD_HTTP_SERVICE_UNAVAILABLE, "app supervisor unavailable");
    }
    if (read_line(fd, line, sizeof line) != 0 || strncmp(line, "STARTED ", 8) != 0)
    {
        close(fd);
        fail_job(job, 1);
        return text_response(connection, MHD_HTTP_SERVICE_UNAVAILABLE, "app start failed");
    }
    char *end;
    long child_pid = strtol(line + 8, &end, 10);
    if (end == line + 8 || *end != '\0' || child_pid <= 0)
    {
        close(fd);
        fail_job(job, 1);
        return text_response(connection, MHD_HTTP_SERVICE_UNAVAILABLE, "invalid app pid");
    }
    pthread_mutex_lock(&jobs_mutex);
    job->fd = fd;
    job->pid = child_pid;
    job->state = WEB_APP_RUNNING;
    pthread_mutex_lock(&web_state_mutex);
    web_app_pid = child_pid;
    web_app_state = WEB_APP_RUNNING;
    pthread_mutex_unlock(&web_state_mutex);
    pthread_mutex_unlock(&jobs_mutex);
    websocket_broadcast_status();

    pthread_t thread;
    if (pthread_create(&thread, NULL, supervisor_reader, job) != 0)
    {
        (void)write_all(fd, "STOP\n", 5);
        close(fd);
        pthread_mutex_lock(&jobs_mutex);
        job->reader_done = 1;
        pthread_mutex_unlock(&jobs_mutex);
        fail_job(job, 1);
        return text_response(connection, MHD_HTTP_INTERNAL_SERVER_ERROR, "reader failed");
    }
    pthread_detach(thread);
    char body[256];
    snprintf(body, sizeof body, "{\"started\":true,\"app\":\"%s\",\"job_id\":%lu,\"pid\":%ld}",
             app_id, job->job_id, child_pid);
    return response(connection, MHD_HTTP_OK, "application/json", body, strlen(body),
                    MHD_RESPMEM_MUST_COPY);
}

enum MHD_Result app_stop_response(struct MHD_Connection *connection)
{
    const char *value = MHD_lookup_connection_value(connection, MHD_GET_ARGUMENT_KIND, "job_id");
    unsigned long job_id = value == NULL ? 0 : strtoul(value, NULL, 10);
    app_job_t *job = NULL;
    int fd = -1;

    pthread_mutex_lock(&jobs_mutex);
    if (job_id != 0)
        job = find_job_locked(job_id);
    else
    {
        for (unsigned i = 0; i < APP_JOB_CAPACITY; i++)
        {
            if (jobs[i].used && jobs[i].active &&
                (job == NULL || jobs[i].job_id > job->job_id))
                job = &jobs[i];
        }
    }
    if (job == NULL || !job->active)
    {
        pthread_mutex_unlock(&jobs_mutex);
        return text_response(connection, MHD_HTTP_CONFLICT, "no running app for job");
    }
    job->state = WEB_APP_STOPPING;
    fd = job->fd;
    unsigned long selected_job_id = job->job_id;
    pthread_mutex_unlock(&jobs_mutex);
    websocket_broadcast_status();
    if (fd < 0 || write_all(fd, "STOP\n", 5) != 0)
        return text_response(connection, MHD_HTTP_SERVICE_UNAVAILABLE, "app supervisor unavailable");
    char body[96];
    snprintf(body, sizeof body, "{\"stopping\":true,\"job_id\":%lu}", selected_job_id);
    return response(connection, MHD_HTTP_ACCEPTED, "application/json", body, strlen(body),
                    MHD_RESPMEM_MUST_COPY);
}
