#include <ctype.h>
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

static unsigned long next_job_id = 1;
static unsigned short supervisor_port;
static int supervisor_fd = -1;
static pthread_mutex_t supervisor_mutex = PTHREAD_MUTEX_INITIALIZER;

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

static void app_finished(int result, int stopped)
{
    pthread_mutex_lock(&web_state_mutex);
    web_app_exit_code = stopped ? 130 : result;
    web_app_pid = 0;
    web_app_running = 0;
    web_app_finished = 1;
    web_launch_started = 0;
    web_app_state = stopped ? WEB_APP_STOPPED : result == 0 ? WEB_APP_FINISHED : WEB_APP_FAILED;
    pthread_mutex_unlock(&web_state_mutex);
    websocket_broadcast_status();
    char line[96];
    int length = stopped ? snprintf(line, sizeof line, "\n[launcher] app stopped\n") :
                          snprintf(line, sizeof line, "\n[launcher] app exited with code %d\n", result);
    if (length > 0)
        log_append(line, (size_t)length);
}

static void *supervisor_reader(void *data)
{
    int fd = *(int *)data;
    char line[128];
    free(data);
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
            pthread_mutex_lock(&web_state_mutex);
            if (web_app_running)
                web_app_state = WEB_APP_STOPPING;
            pthread_mutex_unlock(&web_state_mutex);
            websocket_broadcast_status();
        }
        else if (!strncmp(line, "EXIT ", 5))
        {
            int result = (int)strtol(line + 5, &cursor, 10);
            if (cursor == line + 5 || *cursor != '\0')
                break;
            app_finished(result, result == 130);
            goto disconnected;
        }
    }

disconnected:
    pthread_mutex_lock(&supervisor_mutex);
    if (supervisor_fd == fd)
        supervisor_fd = -1;
    pthread_mutex_unlock(&supervisor_mutex);
    close(fd);
    pthread_mutex_lock(&web_state_mutex);
    int was_running = web_app_running;
    pthread_mutex_unlock(&web_state_mutex);
    if (was_running)
        app_finished(1, 0);
    return NULL;
}

int app_manager_start(unsigned short port)
{
    supervisor_port = port;
    return 0;
}

void app_manager_stop(void)
{
    pthread_mutex_lock(&supervisor_mutex);
    if (supervisor_fd >= 0)
    {
        (void)write_all(supervisor_fd, "SHUTDOWN\n", 9);
        shutdown(supervisor_fd, SHUT_RDWR);
        close(supervisor_fd);
        supervisor_fd = -1;
    }
    pthread_mutex_unlock(&supervisor_mutex);

    /* The supervisor is loopback-only; ask it to exit through its control socket. */
    int fd = connect_supervisor();
    if (fd >= 0)
    {
        (void)write_all(fd, "SHUTDOWN\n", 9);
        close(fd);
    }
}

enum MHD_Result app_launch_response(struct MHD_Connection *connection, const char *query)
{
    char app_id[128];
    char manifest[PATH_CAPACITY];
    char entry[128];
    if (query == NULL || strncmp(query, "app=", 4) != 0 || strlen(query + 4) >= sizeof app_id ||
        !valid_app_id(query + 4))
        return text_response(connection, MHD_HTTP_BAD_REQUEST, "invalid app");
    snprintf(app_id, sizeof app_id, "%s", query + 4);
    pthread_mutex_lock(&web_state_mutex);
    if (web_launch_started)
    {
        pthread_mutex_unlock(&web_state_mutex);
        return text_response(connection, MHD_HTTP_CONFLICT, "app already launched");
    }
    pthread_mutex_unlock(&web_state_mutex);
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
    log_clear_broadcast();
    pthread_mutex_lock(&web_state_mutex);
    web_launch_started = 1;
    web_app_running = 1;
    web_app_finished = 0;
    web_app_exit_code = -1;
    web_app_job_id = next_job_id++;
    web_app_pid = 0;
    snprintf(web_app_id, sizeof web_app_id, "%s", app_id);
    web_app_state = WEB_APP_STARTING;
    pthread_mutex_unlock(&web_state_mutex);
    websocket_broadcast_status();
    int fd = connect_supervisor();
    char command[PATH_CAPACITY * 3 + 16];
    if (fd < 0 || snprintf(command, sizeof command, "START\t%s\t%s\t%s\n", script_path,
                           app_root, app_lib) >= (int)sizeof command ||
        write_all(fd, command, strlen(command)) != 0)
    {
        if (fd >= 0)
            close(fd);
        pthread_mutex_lock(&web_state_mutex);
        web_launch_started = 0;
        web_app_running = 0;
        web_app_finished = 1;
        web_app_state = WEB_APP_FAILED;
        pthread_mutex_unlock(&web_state_mutex);
        websocket_broadcast_status();
        return text_response(connection, MHD_HTTP_SERVICE_UNAVAILABLE, "app supervisor unavailable");
    }
    char line[128];
    if (read_line(fd, line, sizeof line) != 0 || strncmp(line, "STARTED ", 8) != 0)
    {
        close(fd);
        pthread_mutex_lock(&web_state_mutex);
        web_launch_started = 0;
        web_app_running = 0;
        web_app_finished = 1;
        web_app_state = WEB_APP_FAILED;
        pthread_mutex_unlock(&web_state_mutex);
        websocket_broadcast_status();
        return text_response(connection, MHD_HTTP_SERVICE_UNAVAILABLE, "app start failed");
    }
    char *end;
    long child_pid = strtol(line + 8, &end, 10);
    if (end == line + 8 || *end != '\0' || child_pid <= 0)
    {
        close(fd);
        return text_response(connection, MHD_HTTP_SERVICE_UNAVAILABLE, "invalid app pid");
    }
    pthread_mutex_lock(&supervisor_mutex);
    supervisor_fd = fd;
    pthread_mutex_unlock(&supervisor_mutex);
    pthread_mutex_lock(&web_state_mutex);
    web_app_pid = child_pid;
    web_app_state = WEB_APP_RUNNING;
    pthread_mutex_unlock(&web_state_mutex);
    websocket_broadcast_status();
    int *reader_fd = malloc(sizeof *reader_fd);
    if (reader_fd == NULL)
    {
        app_manager_stop();
        return text_response(connection, MHD_HTTP_INTERNAL_SERVER_ERROR, "out of memory");
    }
    *reader_fd = fd;
    pthread_t thread;
    if (pthread_create(&thread, NULL, supervisor_reader, reader_fd) != 0)
    {
        free(reader_fd);
        app_manager_stop();
        return text_response(connection, MHD_HTTP_INTERNAL_SERVER_ERROR, "reader failed");
    }
    pthread_detach(thread);
    char body[256];
    snprintf(body, sizeof body, "{\"started\":true,\"app\":\"%s\",\"job_id\":%lu,\"pid\":%ld}",
             app_id, web_app_job_id, child_pid);
    return response(connection, MHD_HTTP_OK, "application/json", body, strlen(body),
                    MHD_RESPMEM_MUST_COPY);
}

enum MHD_Result app_stop_response(struct MHD_Connection *connection)
{
    unsigned long job_id;
    int running;
    int fd;

    pthread_mutex_lock(&supervisor_mutex);
    fd = supervisor_fd;
    pthread_mutex_unlock(&supervisor_mutex);
    pthread_mutex_lock(&web_state_mutex);
    running = web_app_running;
    job_id = web_app_job_id;
    if (running)
        web_app_state = WEB_APP_STOPPING;
    pthread_mutex_unlock(&web_state_mutex);
    if (!running)
        return text_response(connection, MHD_HTTP_CONFLICT, "no running app");

    websocket_broadcast_status();
    if (fd < 0 || write_all(fd, "STOP\n", 5) != 0)
        return text_response(connection, MHD_HTTP_SERVICE_UNAVAILABLE, "app supervisor unavailable");
    char body[96];
    snprintf(body, sizeof body, "{\"stopping\":true,\"job_id\":%lu}", job_id);
    return response(connection, MHD_HTTP_ACCEPTED, "application/json", body, strlen(body),
                    MHD_RESPMEM_MUST_COPY);
}
