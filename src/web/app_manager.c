#include <ctype.h>
#include <dirent.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "app_manager.h"
#include "cpython_runtime.h"
#include "log_capture.h"
#include "web_state.h"
#include "web_utils.h"
#include "websocket.h"

#define PATH_CAPACITY 512

typedef struct app_launch
{
    char script_path[PATH_CAPACITY];
    char app_root[PATH_CAPACITY];
    char app_lib[PATH_CAPACITY];
} app_launch_t;

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

static void *app_worker(void *data)
{
    app_launch_t *launch = data;
    cpython_run_options_t options = {
        .runtime_path = "/data/python/runtime/cpython-lib",
        .app_root_path = launch->app_root,
        .app_lib_path = launch->app_lib,
    };
    int result = cpython_ps5_run_file(launch->script_path, &options);
    pthread_mutex_lock(&web_state_mutex);
    web_app_exit_code = result;
    web_app_running = 0;
    web_app_finished = 1;
    web_launch_started = 0;
    pthread_mutex_unlock(&web_state_mutex);
    websocket_broadcast_status();
    char line[96];
    int length = snprintf(line, sizeof line, "\n[launcher] app exited with code %d\n", result);
    if (length > 0)
        log_append(line, (size_t)length);
    if (result == 0)
        log_reset();
    free(launch);
    return NULL;
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
    app_launch_t *launch = calloc(1, sizeof *launch);
    if (launch == NULL)
        return text_response(connection, MHD_HTTP_INTERNAL_SERVER_ERROR, "out of memory");
    snprintf(launch->app_root, sizeof launch->app_root, "/data/python/apps/%s", app_id);
    snprintf(launch->app_lib, sizeof launch->app_lib, "%s/lib", launch->app_root);
    snprintf(launch->script_path, sizeof launch->script_path, "%s/%s", launch->app_root, entry);
    if (access(launch->script_path, R_OK) != 0)
    {
        free(launch);
        return text_response(connection, MHD_HTTP_NOT_FOUND, "entry not found");
    }
    log_clear_broadcast();
    pthread_mutex_lock(&web_state_mutex);
    web_launch_started = 1;
    web_app_running = 1;
    web_app_finished = 0;
    web_app_exit_code = -1;
    pthread_mutex_unlock(&web_state_mutex);
    websocket_broadcast_status();
    pthread_t thread;
    if (pthread_create(&thread, NULL, app_worker, launch) != 0)
    {
        pthread_mutex_lock(&web_state_mutex);
        web_launch_started = 0;
        web_app_running = 0;
        pthread_mutex_unlock(&web_state_mutex);
        free(launch);
        websocket_broadcast_status();
        return text_response(connection, MHD_HTTP_INTERNAL_SERVER_ERROR, "thread failed");
    }
    pthread_detach(thread);
    char body[160];
    snprintf(body, sizeof body, "{\"started\":true,\"app\":\"%s\"}", app_id);
    return response(connection, MHD_HTTP_OK, "application/json", body, strlen(body),
                    MHD_RESPMEM_MUST_COPY);
}
