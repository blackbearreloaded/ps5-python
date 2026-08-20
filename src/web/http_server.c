#include <ctype.h>
#include <errno.h>
#include <fcntl.h>
#include <microhttpd.h>
#include <netinet/in.h>
#include <pthread.h>
#include <signal.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <sys/socket.h>
#include <unistd.h>

#include "cpython_ps5_host.h"
#include "cpython_runtime.h"
#include "http_server.h"
#include "http_response.h"
#include "app_manager.h"
#include "log_capture.h"
#include "web_state.h"
#include "web_utils.h"
#include "websocket.h"

#define log_mutex web_log_mutex
#define log_buffer web_log_buffer
#define log_length web_log_length
#define log_base web_log_base
#define log_next web_log_next
#define log_pipe web_log_pipe
#define LOG_CAPACITY WEB_LOG_CAPACITY
#define DEFAULT_PORT 8090
#define PATH_CAPACITY 512
#define TCP_REPL_LINE_CAPACITY 65536
#define SCRIPT_SOURCE_CAPACITY 65536
#define SCRIPT_OUTPUT_CAPACITY 16384

typedef struct script_request
{
    char source[SCRIPT_SOURCE_CAPACITY + 1];
    size_t source_length;
    int oversized;
    int invalid;
} script_request_t;

typedef struct sha1_state
{
    uint32_t h[5];
    uint64_t length;
    unsigned char block[64];
    size_t used;
} sha1_state_t;

static uint32_t sha1_rotate(uint32_t value, unsigned count)
{
    return (value << count) | (value >> (32 - count));
}

static void sha1_block(sha1_state_t *state, const unsigned char *block)
{
    uint32_t words[80];
    uint32_t a, b, c, d, e, f, k, temp;
    unsigned i;

    for (i = 0; i < 16; i++)
        words[i] = ((uint32_t)block[i * 4] << 24) | ((uint32_t)block[i * 4 + 1] << 16) |
                   ((uint32_t)block[i * 4 + 2] << 8) | (uint32_t)block[i * 4 + 3];
    for (; i < 80; i++)
        words[i] = sha1_rotate(words[i - 3] ^ words[i - 8] ^ words[i - 14] ^ words[i - 16], 1);

    a = state->h[0];
    b = state->h[1];
    c = state->h[2];
    d = state->h[3];
    e = state->h[4];
    for (i = 0; i < 80; i++)
    {
        if (i < 20)
        {
            f = (b & c) | ((~b) & d);
            k = 0x5a827999;
        }
        else if (i < 40)
        {
            f = b ^ c ^ d;
            k = 0x6ed9eba1;
        }
        else if (i < 60)
        {
            f = (b & c) | (b & d) | (c & d);
            k = 0x8f1bbcdc;
        }
        else
        {
            f = b ^ c ^ d;
            k = 0xca62c1d6;
        }
        temp = sha1_rotate(a, 5) + f + e + k + words[i];
        e = d;
        d = c;
        c = sha1_rotate(b, 30);
        b = a;
        a = temp;
    }
    state->h[0] += a;
    state->h[1] += b;
    state->h[2] += c;
    state->h[3] += d;
    state->h[4] += e;
}

static void sha1_init(sha1_state_t *state)
{
    memset(state, 0, sizeof *state);
    state->h[0] = 0x67452301;
    state->h[1] = 0xefcdab89;
    state->h[2] = 0x98badcfe;
    state->h[3] = 0x10325476;
    state->h[4] = 0xc3d2e1f0;
}

static void sha1_update(sha1_state_t *state, const void *data, size_t length)
{
    const unsigned char *cursor = data;
    size_t amount;

    state->length += (uint64_t)length * 8;
    while (length > 0)
    {
        amount = sizeof state->block - state->used;
        if (amount > length)
            amount = length;
        memcpy(state->block + state->used, cursor, amount);
        state->used += amount;
        cursor += amount;
        length -= amount;
        if (state->used == sizeof state->block)
        {
            sha1_block(state, state->block);
            state->used = 0;
        }
    }
}

static void sha1_final(sha1_state_t *state, unsigned char digest[20])
{
    uint64_t length = state->length;
    unsigned i;

    sha1_update(state, "\x80", 1);
    while (state->used != 56)
        sha1_update(state, "\0", 1);
    for (i = 0; i < 8; i++)
    {
        unsigned char byte = (unsigned char)(length >> (56 - i * 8));
        sha1_update(state, &byte, 1);
    }
    for (i = 0; i < 5; i++)
    {
        digest[i * 4] = (unsigned char)(state->h[i] >> 24);
        digest[i * 4 + 1] = (unsigned char)(state->h[i] >> 16);
        digest[i * 4 + 2] = (unsigned char)(state->h[i] >> 8);
        digest[i * 4 + 3] = (unsigned char)state->h[i];
    }
}

static size_t base64_encode(const unsigned char *input, size_t length, char *output,
                            size_t capacity)
{
    static const char alphabet[] =
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
    size_t at = 0;
    size_t i;
    unsigned value;

    for (i = 0; i < length; i += 3)
    {
        value = (unsigned)input[i] << 16;
        if (i + 1 < length)
            value |= (unsigned)input[i + 1] << 8;
        if (i + 2 < length)
            value |= input[i + 2];
        if (at + 4 >= capacity)
            return 0;
        output[at++] = alphabet[(value >> 18) & 63];
        output[at++] = alphabet[(value >> 12) & 63];
        output[at++] = i + 1 < length ? alphabet[(value >> 6) & 63] : '=';
        output[at++] = i + 2 < length ? alphabet[value & 63] : '=';
    }
    output[at] = '\0';
    return at;
}

static int websocket_accept_key(const char *key, char output[64])
{
    static const char guid[] = "258EAFA5-E914-47DA-95CA-C5AB0DC85B11";
    sha1_state_t state;
    unsigned char digest[20];

    sha1_init(&state);
    sha1_update(&state, key, strlen(key));
    sha1_update(&state, guid, sizeof guid - 1);
    sha1_final(&state, digest);
    return base64_encode(digest, sizeof digest, output, 64) != 0 ? 0 : -1;
}

static ws_client_t *ws_claim_client(void)
{
    unsigned i;
    ws_client_t *client = NULL;

    pthread_mutex_lock(&ws_mutex);
    for (i = 0; i < WS_MAX_CLIENTS; i++)
    {
        if (!ws_clients[i].in_use)
        {
            client = &ws_clients[i];
            if (!client->initialized)
            {
                pthread_mutex_init(&client->send_mutex, NULL);
                client->initialized = 1;
            }
            client->in_use = 1;
            client->active = 1;
            client->initial_length = 0;
            client->initial_offset = 0;
            break;
        }
    }
    pthread_mutex_unlock(&ws_mutex);
    return client;
}

static ssize_t ws_read_bytes(ws_client_t *client, void *output, size_t length)
{
    unsigned char *cursor = output;
    size_t available;
    ssize_t received;

    while (length > 0 && client->initial_offset < client->initial_length)
    {
        available = client->initial_length - client->initial_offset;
        if (available > length)
            available = length;
        memcpy(cursor, client->initial + client->initial_offset, available);
        client->initial_offset += available;
        cursor += available;
        length -= available;
    }
    while (length > 0)
    {
        received = recv(client->fd, cursor, length, 0);
        if (received <= 0)
            return -1;
        cursor += received;
        length -= (size_t)received;
    }
    return 0;
}

static int ws_receive_frame(ws_client_t *client, unsigned *opcode_output,
                            unsigned char **payload_output, size_t *length_output)
{
    unsigned char header[2];
    unsigned char mask[4];
    unsigned char *payload = NULL;
    uint64_t length;
    unsigned opcode = 0;
    uint64_t i;
    int masked;

    if (ws_read_bytes(client, header, sizeof header) != 0)
    {
        return -1;
    }
    opcode = header[0] & 0x0f;
    masked = (header[1] & 0x80) != 0;
    length = header[1] & 0x7f;
    if (length == 126)
    {
        unsigned char extended[2];
        if (ws_read_bytes(client, extended, sizeof extended) != 0)
            return -1;
        length = ((uint64_t)extended[0] << 8) | extended[1];
    }
    else if (length == 127)
    {
        unsigned char extended[8];
        if (ws_read_bytes(client, extended, sizeof extended) != 0)
            return -1;
        length = 0;
        for (i = 0; i < 8; i++)
            length = (length << 8) | extended[i];
    }
    if (length > WS_FRAME_CAPACITY || (!masked && opcode != 8))
    {
        return -1;
    }
    if (masked && ws_read_bytes(client, mask, sizeof mask) != 0)
        return -1;
    if (length > 0)
    {
        payload = malloc((size_t)length);
        if (payload == NULL || ws_read_bytes(client, payload, (size_t)length) != 0)
        {
            free(payload);
            return -1;
        }
        if (masked)
        {
            for (i = 0; i < length; i++)
                payload[i] ^= mask[i & 3];
        }
    }
    if (opcode == 8)
    {
        websocket_send_frame(client, 8, payload, (size_t)length);
        free(payload);
        return 1;
    }
    if (opcode == 9)
    {
        websocket_send_frame(client, 10, payload, (size_t)length);
        free(payload);
        return 0;
    }
    if (opcode_output != NULL)
        *opcode_output = opcode;
    if (payload_output != NULL)
        *payload_output = payload;
    else
        free(payload);
    if (length_output != NULL)
        *length_output = (size_t)length;
    return 0;
}

static void ws_send_repl_result(ws_client_t *client, const unsigned char *source,
                                size_t source_length)
{
    char *source_text;
    char *output;
    char *message;
    size_t at;
    const size_t output_capacity = 8192;
    int result;

    if (source_length > WS_FRAME_CAPACITY)
        return;
    source_text = malloc(source_length + 1);
    output = malloc(output_capacity);
    if (source_text == NULL || output == NULL)
    {
        free(source_text);
        free(output);
        return;
    }
    memcpy(source_text, source, source_length);
    source_text[source_length] = '\0';
    result = cpython_ps5_runtime_eval(source_text, output, output_capacity);
    if (result == CPYTHON_PS5_RUNTIME_RESTARTED)
    {
        websocket_broadcast("{\"type\":\"repl_reset\",\"reason\":\"exit\"}");
        free(source_text);
        free(output);
        return;
    }
    message = malloc(strlen(output) * 2 + 96);
    if (message == NULL)
    {
        free(source_text);
        free(output);
        return;
    }
    at = (size_t)snprintf(message, strlen(output) * 2 + 96,
                          "{\"type\":\"repl\",\"ok\":%s,\"data\":\"",
                          result == 0 ? "true" : "false");
    at = web_json_append(message, at, strlen(output) * 2 + 96, output);
    (void)snprintf(message + at, strlen(output) * 2 + 96 - at, "\"}");
    websocket_send_text(client, message);
    free(message);
    free(source_text);
    free(output);
}

static void *ws_client_worker(void *data)
{
    ws_client_t *client = data;
    int flags;
    char message[8192];
    char *snapshot = NULL;
    size_t snapshot_length;
    unsigned opcode = 0;
    unsigned char *payload;
    size_t payload_length;

    flags = fcntl(client->fd, F_GETFL, 0);
    if (flags >= 0)
        fcntl(client->fd, F_SETFL, flags & ~O_NONBLOCK);
    if (app_manager_status_json(message, sizeof message) >= 0)
        websocket_send_text(client, message);

    pthread_mutex_lock(&log_mutex);
    snapshot_length = log_length;
    snapshot = malloc(snapshot_length ? snapshot_length : 1);
    if (snapshot != NULL && snapshot_length > 0)
        memcpy(snapshot, log_buffer, snapshot_length);
    pthread_mutex_unlock(&log_mutex);
    if (snapshot != NULL && snapshot_length > 0)
    {
        static const char prefix[] = "{\"type\":\"log\",\"data\":\"";
        char *encoded = malloc(snapshot_length * 2 + 32);
        size_t at = sizeof prefix - 1;
        if (encoded != NULL)
        {
            memcpy(encoded, prefix, sizeof prefix - 1);
            at = web_json_append_bytes(encoded, at, snapshot_length * 2 + 32,
                                       (const unsigned char *)snapshot, snapshot_length);
            (void)snprintf(encoded + at, snapshot_length * 2 + 32 - at, "\"}");
            websocket_send_text(client, encoded);
            free(encoded);
        }
    }
    free(snapshot);

    while (client->active && !server_stop)
    {
        payload = NULL;
        payload_length = 0;
        if (ws_receive_frame(client, &opcode, &payload, &payload_length) != 0)
            break;
        if (opcode == 1 && payload != NULL)
        {
            ws_send_repl_result(client, payload, payload_length);
        }
        free(payload);
    }
    pthread_mutex_lock(&ws_mutex);
    client->active = 0;
    pthread_mutex_unlock(&ws_mutex);
    MHD_upgrade_action(client->urh, MHD_UPGRADE_ACTION_CLOSE);
    pthread_mutex_lock(&ws_mutex);
    client->in_use = 0;
    pthread_mutex_unlock(&ws_mutex);
    return NULL;
}

static void ws_upgrade_handler(void *cls, struct MHD_Connection *connection, void *req_cls,
                               const char *extra_in, size_t extra_in_size, MHD_socket sock,
                               struct MHD_UpgradeResponseHandle *urh)
{
    ws_client_t *client;
    pthread_t thread;

    (void)cls;
    (void)connection;
    (void)req_cls;
    client = ws_claim_client();
    if (client == NULL)
    {
        MHD_upgrade_action(urh, MHD_UPGRADE_ACTION_CLOSE);
        return;
    }
    client->fd = sock;
    client->urh = urh;
    if (extra_in_size > sizeof client->initial || (extra_in_size > 0 && extra_in == NULL))
    {
        client->active = 0;
        client->in_use = 0;
        MHD_upgrade_action(urh, MHD_UPGRADE_ACTION_CLOSE);
        return;
    }
    if (extra_in_size > 0)
    {
        memcpy(client->initial, extra_in, extra_in_size);
        client->initial_length = extra_in_size;
    }
    if (pthread_create(&thread, NULL, ws_client_worker, client) != 0)
    {
        client->active = 0;
        client->in_use = 0;
        MHD_upgrade_action(urh, MHD_UPGRADE_ACTION_CLOSE);
        return;
    }
    pthread_detach(thread);
}

static const char *query_value(const char *query, const char *key, char *out, size_t out_size)
{
    size_t key_length = strlen(key);
    const char *cursor = query;
    const char *end;
    size_t length;

    while (cursor != NULL && *cursor)
    {
        if (!strncmp(cursor, key, key_length) && cursor[key_length] == '=')
        {
            cursor += key_length + 1;
            end = strchr(cursor, '&');
            length = end ? (size_t)(end - cursor) : strlen(cursor);
            if (length + 1 > out_size)
                return NULL;
            memcpy(out, cursor, length);
            out[length] = '\0';
            return out;
        }
        cursor = strchr(cursor, '&');
        if (cursor)
            cursor++;
    }
    return NULL;
}

static enum MHD_Result status_response(struct MHD_Connection *connection)
{
    char body[8192];
    int length = app_manager_status_json(body, sizeof body);
    if (length < 0)
        return http_queue_text(connection, MHD_HTTP_INTERNAL_SERVER_ERROR, "text/plain",
                               "status too large");
    return http_queue_response(connection, MHD_HTTP_OK, "application/json", NULL, body,
                               (size_t)length, MHD_RESPMEM_MUST_COPY);
}

static enum MHD_Result repl_reset_response(struct MHD_Connection *connection)
{
    if (cpython_ps5_runtime_reset(NULL) != 0)
        return http_queue_text(connection, MHD_HTTP_INTERNAL_SERVER_ERROR, "text/plain",
                               "interpreter reset failed");
    websocket_broadcast("{\"type\":\"repl_reset\"}");
    return http_queue_text(connection, MHD_HTTP_OK, "application/json", "{\"reset\":true}");
}

static enum MHD_Result script_run_response(struct MHD_Connection *connection,
                                           const script_request_t *request)
{
    char *output = NULL;
    char *body = NULL;
    size_t body_capacity;
    size_t at;
    int result;
    int restarted;

    output = calloc(1, SCRIPT_OUTPUT_CAPACITY);
    if (output == NULL)
        return http_queue_text(connection, MHD_HTTP_INTERNAL_SERVER_ERROR, "text/plain",
                               "out of memory");
    result = cpython_ps5_runtime_eval(request->source, output, SCRIPT_OUTPUT_CAPACITY);
    restarted = result == CPYTHON_PS5_RUNTIME_RESTARTED;
    if (restarted)
        websocket_broadcast("{\"type\":\"repl_reset\",\"reason\":\"exit\"}");
    body_capacity = strlen(output) * 2 + 96;
    body = malloc(body_capacity);
    if (body == NULL)
    {
        free(output);
        return http_queue_text(connection, MHD_HTTP_INTERNAL_SERVER_ERROR, "text/plain",
                               "out of memory");
    }
    at = (size_t)snprintf(body, body_capacity, "{\"ok\":%s,\"restarted\":%s,\"data\":\"",
                          result == 0 ? "true" : "false", restarted ? "true" : "false");
    at = web_json_append(body, at, body_capacity, output);
    (void)snprintf(body + at, body_capacity - at, "\",\"source_bytes\":%llu}",
                   (unsigned long long)request->source_length);
    free(output);
    return http_queue_response(connection, MHD_HTTP_OK, "application/json", NULL, body,
                               strlen(body), MHD_RESPMEM_MUST_FREE);
}

void web_request_completed(void *cls, struct MHD_Connection *connection, void **con_cls,
                           enum MHD_RequestTerminationCode termination_code)
{
    (void)cls;
    (void)connection;
    (void)termination_code;
    if (*con_cls != NULL && *con_cls != (void *)1 && *con_cls != (void *)2)
        free(*con_cls);
    *con_cls = NULL;
}

static enum MHD_Result logs_response(struct MHD_Connection *connection, const char *query)
{
    char since_text[32];
    char next_text[32];
    char *body;
    uint64_t since = 0;
    uint64_t start;
    size_t length;

    if (query_value(query, "since", since_text, sizeof since_text))
        since = strtoull(since_text, NULL, 10);
    body = malloc(LOG_CAPACITY);
    if (body == NULL)
        return http_queue_text(connection, MHD_HTTP_INTERNAL_SERVER_ERROR, "text/plain",
                               "out of memory");
    pthread_mutex_lock(&log_mutex);
    start = since < log_base ? 0 : since - log_base;
    if (start > log_length)
        start = log_length;
    length = log_length - (size_t)start;
    memcpy(body, log_buffer + start, length);
    snprintf(next_text, sizeof next_text, "%llu", (unsigned long long)log_next);
    pthread_mutex_unlock(&log_mutex);
    return http_queue_response(connection, MHD_HTTP_OK, "text/plain; charset=utf-8", next_text,
                               body, length, MHD_RESPMEM_MUST_FREE);
}

static int header_has_token(const char *header, const char *token)
{
    size_t token_length = strlen(token);
    const char *cursor = header;

    while (cursor != NULL && *cursor)
    {
        while (*cursor == ' ' || *cursor == '\t' || *cursor == ',')
            cursor++;
        if (!strncasecmp(cursor, token, token_length) &&
            (cursor[token_length] == '\0' || cursor[token_length] == ',' ||
             cursor[token_length] == ' ' || cursor[token_length] == '\t'))
            return 1;
        cursor = strchr(cursor, ',');
        if (cursor)
            cursor++;
    }
    return 0;
}

static enum MHD_Result websocket_response(struct MHD_Connection *connection)
{
    const char *key;
    const char *upgrade;
    const char *connection_header;
    char accept[64];
    struct MHD_Response *response;
    enum MHD_Result result;

    upgrade = MHD_lookup_connection_value(connection, MHD_HEADER_KIND, "Upgrade");
    connection_header = MHD_lookup_connection_value(connection, MHD_HEADER_KIND, "Connection");
    key = MHD_lookup_connection_value(connection, MHD_HEADER_KIND, "Sec-WebSocket-Key");
    if (upgrade == NULL || strcasecmp(upgrade, "websocket") != 0 || connection_header == NULL ||
        !header_has_token(connection_header, "Upgrade") || key == NULL ||
        websocket_accept_key(key, accept) != 0)
        return http_queue_text(connection, MHD_HTTP_BAD_REQUEST, "text/plain",
                               "WebSocket upgrade required");
    response = MHD_create_response_for_upgrade(ws_upgrade_handler, NULL);
    if (response == NULL)
        return MHD_NO;
    MHD_add_response_header(response, "Upgrade", "websocket");
    MHD_add_response_header(response, "Connection", "Upgrade");
    MHD_add_response_header(response, "Sec-WebSocket-Accept", accept);
    result = MHD_queue_response(connection, MHD_HTTP_SWITCHING_PROTOCOLS, response);
    MHD_destroy_response(response);
    return result;
}

enum MHD_Result web_access_handler(void *cls, struct MHD_Connection *connection, const char *url,
                                   const char *method, const char *version, const char *upload_data,
                                   size_t *upload_data_size, void **con_cls)
{
    script_request_t *script_request;
    char target[1024];
    const char *argument;
    char query[160];

    (void)cls;
    (void)version;
    if (*con_cls == NULL)
    {
        if (!strcmp(method, "POST") && !strcmp(url, "/api/script/run"))
        {
            script_request = calloc(1, sizeof *script_request);
            if (script_request == NULL)
                return http_queue_text(connection, MHD_HTTP_INTERNAL_SERVER_ERROR, "text/plain",
                                       "out of memory");
            *con_cls = script_request;
            return MHD_YES;
        }
        if (!strcmp(method, "POST") && !strcmp(url, "/api/app/stop"))
        {
            *con_cls = (void *)2;
            return MHD_YES;
        }
        *con_cls = (void *)1;
        return MHD_YES;
    }
    if (*con_cls == (void *)2)
    {
        *upload_data_size = 0;
        return app_stop_response(connection);
    }
    if (*con_cls != (void *)1)
    {
        script_request = *con_cls;
        if (*upload_data_size != 0)
        {
            if (script_request->source_length + *upload_data_size > SCRIPT_SOURCE_CAPACITY)
                script_request->oversized = 1;
            else if (memchr(upload_data, '\0', *upload_data_size) != NULL)
                script_request->invalid = 1;
            else
            {
                memcpy(script_request->source + script_request->source_length, upload_data,
                       *upload_data_size);
                script_request->source_length += *upload_data_size;
            }
            *upload_data_size = 0;
            return MHD_YES;
        }
        if (script_request->invalid)
            return http_queue_text(connection, MHD_HTTP_BAD_REQUEST, "text/plain",
                                   "script body contains a NUL byte");
        if (script_request->oversized)
            return http_queue_text(connection, MHD_HTTP_CONTENT_TOO_LARGE, "text/plain",
                                   "script body exceeds 65536 bytes");
        script_request->source[script_request->source_length] = '\0';
        return script_run_response(connection, script_request);
    }
    if (strcmp(method, "GET") != 0)
        return http_queue_text(connection, MHD_HTTP_BAD_REQUEST, "text/plain", "GET required");
    if (*upload_data_size != 0)
    {
        *upload_data_size = 0;
        return MHD_YES;
    }
    if (strlen(url) >= sizeof target)
        return http_queue_text(connection, MHD_HTTP_URI_TOO_LONG, "text/plain", "URI too long");
    memcpy(target, url, strlen(url) + 1);
    if (!strcmp(target, "/ws"))
        return websocket_response(connection);
    if (!strcmp(target, "/"))
        return http_static_file_response(connection, "/data/python/web/index.html",
                                         "text/html; charset=utf-8");
    if (!strcmp(target, "/app.css"))
        return http_static_file_response(connection, "/data/python/web/app.css", "text/css");
    if (!strcmp(target, "/app.js"))
        return http_static_file_response(connection, "/data/python/web/app.js",
                                         "application/javascript");
    if (!strcmp(target, "/vendor/highlight.js/highlight.min.js"))
        return http_static_file_response(connection,
                                         "/data/python/web/vendor/highlight.js/highlight.min.js",
                                         "application/javascript");
    if (!strcmp(target, "/api/apps"))
        return app_list_response(connection);
    if (!strcmp(target, "/api/status"))
        return status_response(connection);
    if (!strcmp(target, "/api/repl/reset"))
        return repl_reset_response(connection);
    if (!strcmp(target, "/api/logs/clear"))
    {
        log_clear_broadcast();
        return http_queue_text(connection, MHD_HTTP_OK, "application/json", "{\"cleared\":true}");
    }
    if (!strcmp(target, "/api/logs"))
    {
        argument = MHD_lookup_connection_value(connection, MHD_GET_ARGUMENT_KIND, "since");
        if (argument != NULL)
            snprintf(query, sizeof query, "since=%s", argument);
        else
            query[0] = '\0';
        return logs_response(connection, query);
    }
    if (!strcmp(target, "/api/launch"))
    {
        argument = MHD_lookup_connection_value(connection, MHD_GET_ARGUMENT_KIND, "app");
        if (argument != NULL)
            snprintf(query, sizeof query, "app=%s", argument);
        else
            query[0] = '\0';
        return app_launch_response(connection, query);
    }
    if (!strcmp(target, "/api/shutdown"))
    {
        server_stop = 1;
        return http_queue_text(connection, MHD_HTTP_OK, "text/plain", "bye\n");
    }
    return http_queue_text(connection, MHD_HTTP_NOT_FOUND, "text/plain", "not found\n");
}
