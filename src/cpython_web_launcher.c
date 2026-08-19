#include <ctype.h>
#include <dirent.h>
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

#define DEFAULT_PORT 8090
#define LOG_CAPACITY 65536
#define PATH_CAPACITY 512
#define WS_MAX_CLIENTS 4
#define WS_INITIAL_CAPACITY 2048
#define WS_FRAME_CAPACITY 65536
#define TCP_REPL_LINE_CAPACITY 65536

static pthread_mutex_t log_mutex = PTHREAD_MUTEX_INITIALIZER;
static char log_buffer[LOG_CAPACITY];
static size_t log_length;
static uint64_t log_base;
static uint64_t log_next;
static int log_pipe[2] = {-1, -1};

static pthread_mutex_t state_mutex = PTHREAD_MUTEX_INITIALIZER;
static int launch_started;
static int app_running;
static int app_finished;
static int app_exit_code;
static volatile sig_atomic_t server_stop;
static unsigned short tcp_repl_port;
static int tcp_repl_fd = -1;
static pthread_t tcp_repl_thread;
static int tcp_repl_started;

typedef struct ws_client {
    int initialized;
    int in_use;
    int active;
    MHD_socket fd;
    struct MHD_UpgradeResponseHandle *urh;
    pthread_mutex_t send_mutex;
    unsigned char initial[WS_INITIAL_CAPACITY];
    size_t initial_length;
    size_t initial_offset;
} ws_client_t;

static pthread_mutex_t ws_mutex = PTHREAD_MUTEX_INITIALIZER;
static ws_client_t ws_clients[WS_MAX_CLIENTS];

typedef struct app_launch {
    char script_path[PATH_CAPACITY];
    char app_root[PATH_CAPACITY];
    char app_lib[PATH_CAPACITY];
} app_launch_t;

static int
send_all(MHD_socket fd, const void *data, size_t length)
{
    const char *cursor = data;
    ssize_t sent;

    while (length > 0) {
        sent = send(fd, cursor, length, 0);
        if (sent <= 0)
            return -1;
        cursor += sent;
        length -= (size_t)sent;
    }
    return 0;
}

static size_t
json_append(char *out, size_t at, size_t capacity, const char *text)
{
    const unsigned char *cursor = (const unsigned char *)text;

    while (*cursor && at + 2 < capacity) {
        if (*cursor == '"' || *cursor == '\\') {
            out[at++] = '\\';
            out[at++] = (char)*cursor++;
        } else if (*cursor == '\n') {
            out[at++] = '\\';
            out[at++] = 'n';
            cursor++;
        } else if (*cursor == '\r') {
            out[at++] = '\\';
            out[at++] = 'r';
            cursor++;
        } else if (*cursor < 0x20) {
            cursor++;
        } else {
            out[at++] = (char)*cursor++;
        }
    }
    out[at] = '\0';
    return at;
}

static size_t
json_append_bytes(char *out, size_t at, size_t capacity,
                  const unsigned char *data, size_t length)
{
    size_t i = 0;
    size_t sequence_length;
    unsigned char value;

    while (i < length && at + 2 < capacity) {
        value = data[i];
        if (value == '"' || value == '\\') {
            out[at++] = '\\';
            out[at++] = (char)value;
            i++;
        } else if (value == '\n' || value == '\r' || value == '\t') {
            out[at++] = '\\';
            out[at++] = value == '\n' ? 'n' : value == '\r' ? 'r' : 't';
            i++;
        } else if (value < 0x20) {
            out[at++] = '?';
            i++;
        } else if (value < 0x80) {
            out[at++] = (char)value;
            i++;
        } else {
            sequence_length = 0;
            if (value >= 0xc2 && value <= 0xdf && i + 1 < length &&
                data[i + 1] >= 0x80 && data[i + 1] <= 0xbf)
                sequence_length = 2;
            else if (value >= 0xe0 && value <= 0xef && i + 2 < length &&
                     data[i + 1] >= 0x80 && data[i + 1] <= 0xbf &&
                     data[i + 2] >= 0x80 && data[i + 2] <= 0xbf)
                sequence_length = 3;
            else if (value >= 0xf0 && value <= 0xf4 && i + 3 < length &&
                     data[i + 1] >= 0x80 && data[i + 1] <= 0xbf &&
                     data[i + 2] >= 0x80 && data[i + 2] <= 0xbf &&
                     data[i + 3] >= 0x80 && data[i + 3] <= 0xbf)
                sequence_length = 4;
            if (sequence_length != 0 && at + sequence_length < capacity) {
                memcpy(out + at, data + i, sequence_length);
                at += sequence_length;
                i += sequence_length;
            } else {
                out[at++] = '?';
                i++;
            }
        }
    }
    out[at] = '\0';
    return at;
}

typedef struct sha1_state {
    uint32_t h[5];
    uint64_t length;
    unsigned char block[64];
    size_t used;
} sha1_state_t;

static uint32_t
sha1_rotate(uint32_t value, unsigned count)
{
    return (value << count) | (value >> (32 - count));
}

static void
sha1_block(sha1_state_t *state, const unsigned char *block)
{
    uint32_t words[80];
    uint32_t a, b, c, d, e, f, k, temp;
    unsigned i;

    for (i = 0; i < 16; i++)
        words[i] = ((uint32_t)block[i * 4] << 24) |
                   ((uint32_t)block[i * 4 + 1] << 16) |
                   ((uint32_t)block[i * 4 + 2] << 8) |
                   (uint32_t)block[i * 4 + 3];
    for (; i < 80; i++)
        words[i] = sha1_rotate(words[i - 3] ^ words[i - 8] ^
                                words[i - 14] ^ words[i - 16], 1);

    a = state->h[0];
    b = state->h[1];
    c = state->h[2];
    d = state->h[3];
    e = state->h[4];
    for (i = 0; i < 80; i++) {
        if (i < 20) {
            f = (b & c) | ((~b) & d);
            k = 0x5a827999;
        } else if (i < 40) {
            f = b ^ c ^ d;
            k = 0x6ed9eba1;
        } else if (i < 60) {
            f = (b & c) | (b & d) | (c & d);
            k = 0x8f1bbcdc;
        } else {
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

static void
sha1_init(sha1_state_t *state)
{
    memset(state, 0, sizeof *state);
    state->h[0] = 0x67452301;
    state->h[1] = 0xefcdab89;
    state->h[2] = 0x98badcfe;
    state->h[3] = 0x10325476;
    state->h[4] = 0xc3d2e1f0;
}

static void
sha1_update(sha1_state_t *state, const void *data, size_t length)
{
    const unsigned char *cursor = data;
    size_t amount;

    state->length += (uint64_t)length * 8;
    while (length > 0) {
        amount = sizeof state->block - state->used;
        if (amount > length)
            amount = length;
        memcpy(state->block + state->used, cursor, amount);
        state->used += amount;
        cursor += amount;
        length -= amount;
        if (state->used == sizeof state->block) {
            sha1_block(state, state->block);
            state->used = 0;
        }
    }
}

static void
sha1_final(sha1_state_t *state, unsigned char digest[20])
{
    uint64_t length = state->length;
    unsigned i;

    sha1_update(state, "\x80", 1);
    while (state->used != 56)
        sha1_update(state, "\0", 1);
    for (i = 0; i < 8; i++) {
        unsigned char byte = (unsigned char)(length >> (56 - i * 8));
        sha1_update(state, &byte, 1);
    }
    for (i = 0; i < 5; i++) {
        digest[i * 4] = (unsigned char)(state->h[i] >> 24);
        digest[i * 4 + 1] = (unsigned char)(state->h[i] >> 16);
        digest[i * 4 + 2] = (unsigned char)(state->h[i] >> 8);
        digest[i * 4 + 3] = (unsigned char)state->h[i];
    }
}

static size_t
base64_encode(const unsigned char *input, size_t length, char *output,
              size_t capacity)
{
    static const char alphabet[] =
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
    size_t at = 0;
    size_t i;
    unsigned value;

    for (i = 0; i < length; i += 3) {
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

static int
websocket_accept_key(const char *key, char output[64])
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

static ws_client_t *
ws_claim_client(void)
{
    unsigned i;
    ws_client_t *client = NULL;

    pthread_mutex_lock(&ws_mutex);
    for (i = 0; i < WS_MAX_CLIENTS; i++) {
        if (!ws_clients[i].in_use) {
            client = &ws_clients[i];
            if (!client->initialized) {
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

static int
ws_send_frame(ws_client_t *client, unsigned opcode, const void *payload,
              size_t length)
{
    unsigned char header[10];
    size_t header_length;

    if (length > WS_FRAME_CAPACITY)
        return -1;
    header[0] = (unsigned char)(0x80 | (opcode & 0x0f));
    if (length < 126) {
        header[1] = (unsigned char)length;
        header_length = 2;
    } else if (length <= 0xffff) {
        header[1] = 126;
        header[2] = (unsigned char)(length >> 8);
        header[3] = (unsigned char)length;
        header_length = 4;
    } else {
        unsigned i;
        header[1] = 127;
        for (i = 0; i < 8; i++)
            header[2 + i] = (unsigned char)(length >> (56 - i * 8));
        header_length = 10;
    }

    pthread_mutex_lock(&client->send_mutex);
    if (send_all(client->fd, header, header_length) != 0 ||
        send_all(client->fd, payload, length) != 0) {
        pthread_mutex_unlock(&client->send_mutex);
        return -1;
    }
    pthread_mutex_unlock(&client->send_mutex);
    return 0;
}

static int
ws_send_text(ws_client_t *client, const char *text)
{
    return ws_send_frame(client, 1, text, strlen(text));
}

static void
ws_broadcast(const char *message)
{
    unsigned i;

    pthread_mutex_lock(&ws_mutex);
    for (i = 0; i < WS_MAX_CLIENTS; i++) {
        if (ws_clients[i].active && ws_send_text(&ws_clients[i], message) != 0)
            ws_clients[i].active = 0;
    }
    pthread_mutex_unlock(&ws_mutex);
}

static void
ws_broadcast_log(const char *data, size_t length)
{
    static const char prefix[] = "{\"type\":\"log\",\"data\":\"";
    char *message;
    size_t at;

    message = malloc(length * 2 + 32);
    if (message == NULL)
        return;
    memcpy(message, prefix, sizeof prefix - 1);
    at = sizeof prefix - 1;
    at = json_append_bytes(message, at, length * 2 + 32,
                           (const unsigned char *)data, length);
    at += (size_t)snprintf(message + at, length * 2 + 32 - at, "\"}");
    ws_broadcast(message);
    free(message);
}

static void
ws_broadcast_status(void)
{
    char message[224];
    int running;
    int finished;
    int exit_code;
    long process_id;

    pthread_mutex_lock(&state_mutex);
    running = app_running;
    finished = app_finished;
    exit_code = app_exit_code;
    pthread_mutex_unlock(&state_mutex);
    process_id = (long)getpid();
    snprintf(message, sizeof message,
             "{\"type\":\"status\",\"pid\":%ld,\"running\":%s,"
             "\"finished\":%s,\"exit_code\":%d,\"repl_port\":%u}",
             process_id, running ? "true" : "false",
             finished ? "true" : "false", exit_code,
             (unsigned)tcp_repl_port);
    ws_broadcast(message);
}

static ssize_t
ws_read_bytes(ws_client_t *client, void *output, size_t length)
{
    unsigned char *cursor = output;
    size_t available;
    ssize_t received;

    while (length > 0 && client->initial_offset < client->initial_length) {
        available = client->initial_length - client->initial_offset;
        if (available > length)
            available = length;
        memcpy(cursor, client->initial + client->initial_offset, available);
        client->initial_offset += available;
        cursor += available;
        length -= available;
    }
    while (length > 0) {
        received = recv(client->fd, cursor, length, 0);
        if (received <= 0)
            return -1;
        cursor += received;
        length -= (size_t)received;
    }
    return 0;
}

static int
ws_receive_frame(ws_client_t *client, unsigned *opcode_output,
                  unsigned char **payload_output, size_t *length_output)
{
    unsigned char header[2];
    unsigned char mask[4];
    unsigned char *payload = NULL;
    uint64_t length;
    unsigned opcode;
    uint64_t i;
    int masked;

    if (ws_read_bytes(client, header, sizeof header) != 0) {
        return -1;
    }
    opcode = header[0] & 0x0f;
    masked = (header[1] & 0x80) != 0;
    length = header[1] & 0x7f;
    if (length == 126) {
        unsigned char extended[2];
        if (ws_read_bytes(client, extended, sizeof extended) != 0)
            return -1;
        length = ((uint64_t)extended[0] << 8) | extended[1];
    } else if (length == 127) {
        unsigned char extended[8];
        if (ws_read_bytes(client, extended, sizeof extended) != 0)
            return -1;
        length = 0;
        for (i = 0; i < 8; i++)
            length = (length << 8) | extended[i];
    }
    if (length > WS_FRAME_CAPACITY || (!masked && opcode != 8)) {
        return -1;
    }
    if (masked && ws_read_bytes(client, mask, sizeof mask) != 0)
        return -1;
    if (length > 0) {
        payload = malloc((size_t)length);
        if (payload == NULL || ws_read_bytes(client, payload, (size_t)length) != 0) {
            free(payload);
            return -1;
        }
        if (masked) {
            for (i = 0; i < length; i++)
                payload[i] ^= mask[i & 3];
        }
    }
    if (opcode == 8) {
        ws_send_frame(client, 8, payload, (size_t)length);
        free(payload);
        return 1;
    }
    if (opcode == 9) {
        ws_send_frame(client, 10, payload, (size_t)length);
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

static void
ws_send_repl_result(ws_client_t *client, const unsigned char *source,
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
    if (source_text == NULL || output == NULL) {
        free(source_text);
        free(output);
        return;
    }
    memcpy(source_text, source, source_length);
    source_text[source_length] = '\0';
    result = cpython_ps5_runtime_eval(source_text, output, output_capacity);
    message = malloc(strlen(output) * 2 + 96);
    if (message == NULL) {
        free(source_text);
        free(output);
        return;
    }
    at = (size_t)snprintf(message, strlen(output) * 2 + 96,
                          "{\"type\":\"repl\",\"ok\":%s,\"data\":\"",
                          result == 0 ? "true" : "false");
    at = json_append(message, at, strlen(output) * 2 + 96, output);
    at += (size_t)snprintf(message + at, strlen(output) * 2 + 96 - at,
                           "\"}");
    ws_send_text(client, message);
    free(message);
    free(source_text);
    free(output);
}

static void *
ws_client_worker(void *data)
{
    ws_client_t *client = data;
    int flags;
    char message[224];
    char *snapshot = NULL;
    size_t snapshot_length;
    unsigned opcode;
    unsigned char *payload;
    size_t payload_length;
    long process_id;

    flags = fcntl(client->fd, F_GETFL, 0);
    if (flags >= 0)
        fcntl(client->fd, F_SETFL, flags & ~O_NONBLOCK);
    pthread_mutex_lock(&state_mutex);
    process_id = (long)getpid();
    snprintf(message, sizeof message,
             "{\"type\":\"status\",\"pid\":%ld,\"running\":%s,"
             "\"finished\":%s,\"exit_code\":%d,\"repl_port\":%u}",
             process_id, app_running ? "true" : "false",
             app_finished ? "true" : "false", app_exit_code,
             (unsigned)tcp_repl_port);
    pthread_mutex_unlock(&state_mutex);
    ws_send_text(client, message);

    pthread_mutex_lock(&log_mutex);
    snapshot_length = log_length;
    snapshot = malloc(snapshot_length ? snapshot_length : 1);
    if (snapshot != NULL && snapshot_length > 0)
        memcpy(snapshot, log_buffer, snapshot_length);
    pthread_mutex_unlock(&log_mutex);
    if (snapshot != NULL && snapshot_length > 0) {
        static const char prefix[] = "{\"type\":\"log\",\"data\":\"";
        char *encoded = malloc(snapshot_length * 2 + 32);
        size_t at = sizeof prefix - 1;
        if (encoded != NULL) {
            memcpy(encoded, prefix, sizeof prefix - 1);
            at = json_append_bytes(encoded, at, snapshot_length * 2 + 32,
                                   (const unsigned char *)snapshot,
                                   snapshot_length);
            at += (size_t)snprintf(encoded + at, snapshot_length * 2 + 32 - at,
                                    "\"}");
            ws_send_text(client, encoded);
            free(encoded);
        }
    }
    free(snapshot);

    while (client->active && !server_stop) {
        payload = NULL;
        payload_length = 0;
        if (ws_receive_frame(client, &opcode, &payload, &payload_length) != 0)
            break;
        if (opcode == 1 && payload != NULL) {
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

static void
ws_upgrade_handler(void *cls, struct MHD_Connection *connection,
                   void *req_cls, const char *extra_in, size_t extra_in_size,
                   MHD_socket sock, struct MHD_UpgradeResponseHandle *urh)
{
    ws_client_t *client;
    pthread_t thread;

    (void)cls;
    (void)connection;
    (void)req_cls;
    client = ws_claim_client();
    if (client == NULL) {
        MHD_upgrade_action(urh, MHD_UPGRADE_ACTION_CLOSE);
        return;
    }
    client->fd = sock;
    client->urh = urh;
    if (extra_in_size > sizeof client->initial ||
        (extra_in_size > 0 && extra_in == NULL)) {
        client->active = 0;
        client->in_use = 0;
        MHD_upgrade_action(urh, MHD_UPGRADE_ACTION_CLOSE);
        return;
    }
    if (extra_in_size > 0) {
        memcpy(client->initial, extra_in, extra_in_size);
        client->initial_length = extra_in_size;
    }
    if (pthread_create(&thread, NULL, ws_client_worker, client) != 0) {
        client->active = 0;
        client->in_use = 0;
        MHD_upgrade_action(urh, MHD_UPGRADE_ACTION_CLOSE);
        return;
    }
    pthread_detach(thread);
}

static int
tcp_repl_send(int fd, const char *text)
{
    return send_all(fd, text, strlen(text));
}

static int
tcp_repl_execute_line(int fd, char *line, size_t *line_length, char *output)
{
    size_t output_length;

    line[*line_length] = '\0';
    (void)cpython_ps5_runtime_eval(line, output, 8192);
    output_length = strlen(output);
    if (output_length > 0 && send_all(fd, output, output_length) != 0)
        return -1;
    if (output_length == 0 || output[output_length - 1] != '\n') {
        if (tcp_repl_send(fd, "\r\n") != 0)
            return -1;
    }
    if (tcp_repl_send(fd, ">>> ") != 0)
        return -1;
    *line_length = 0;
    return 0;
}

static void *
tcp_repl_client_worker(void *data)
{
    int fd = *(int *)data;
    char *line;
    char *output;
    unsigned char input[1024];
    size_t line_length = 0;
    ssize_t received;
    size_t i;
    int pending_cr = 0;

    free(data);
    line = malloc(TCP_REPL_LINE_CAPACITY + 1);
    output = malloc(8192);
    if (line == NULL || output == NULL)
        goto done;
    if (tcp_repl_send(fd, "CPython 3.14.7 TCP REPL\r\n>>> ") != 0)
        goto done;
    for (;;) {
        received = recv(fd, input, sizeof input, 0);
        if (received <= 0 || server_stop)
            break;
        for (i = 0; i < (size_t)received; i++) {
            if (pending_cr) {
                pending_cr = 0;
                if (input[i] == '\n')
                    continue;
            }
            if (input[i] == '\r') {
                if (tcp_repl_execute_line(fd, line, &line_length, output) != 0)
                    goto done;
                pending_cr = 1;
                continue;
            }
            if (input[i] != '\n') {
                if (line_length + 1 >= TCP_REPL_LINE_CAPACITY) {
                    if (tcp_repl_send(fd, "input line too long\r\n>>> ") != 0)
                        goto done;
                    line_length = 0;
                } else {
                    line[line_length++] = (char)input[i];
                }
                continue;
            }
            if (tcp_repl_execute_line(fd, line, &line_length, output) != 0)
                goto done;
        }
    }
done:
    free(output);
    free(line);
    shutdown(fd, SHUT_RDWR);
    close(fd);
    return NULL;
}

static void *
tcp_repl_server_worker(void *unused)
{
    struct sockaddr_in address;
    socklen_t address_length;
    int client_fd;
    int *client_data;
    pthread_t thread;

    (void)unused;
    while (!server_stop) {
        address_length = sizeof address;
        client_fd = accept(tcp_repl_fd, (struct sockaddr *)&address,
                           &address_length);
        if (client_fd < 0) {
            if (server_stop)
                break;
            continue;
        }
        client_data = malloc(sizeof *client_data);
        if (client_data == NULL) {
            close(client_fd);
            continue;
        }
        *client_data = client_fd;
        if (pthread_create(&thread, NULL, tcp_repl_client_worker,
                           client_data) != 0) {
            close(client_fd);
            free(client_data);
            continue;
        }
        pthread_detach(thread);
    }
    return NULL;
}

static int
tcp_repl_start(unsigned short port)
{
    struct sockaddr_in address;
    int option = 1;

    tcp_repl_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (tcp_repl_fd < 0)
        return -1;
    (void)setsockopt(tcp_repl_fd, SOL_SOCKET, SO_REUSEADDR,
                     &option, sizeof option);
    memset(&address, 0, sizeof address);
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = htonl(INADDR_ANY);
    address.sin_port = htons(port);
    if (bind(tcp_repl_fd, (struct sockaddr *)&address, sizeof address) != 0 ||
        listen(tcp_repl_fd, 4) != 0 ||
        pthread_create(&tcp_repl_thread, NULL, tcp_repl_server_worker, NULL) != 0) {
        close(tcp_repl_fd);
        tcp_repl_fd = -1;
        return -1;
    }
    tcp_repl_started = 1;
    return 0;
}

static void
tcp_repl_stop(void)
{
    if (!tcp_repl_started)
        return;
    shutdown(tcp_repl_fd, SHUT_RDWR);
    close(tcp_repl_fd);
    tcp_repl_fd = -1;
    pthread_join(tcp_repl_thread, NULL);
    tcp_repl_started = 0;
}

static void
log_append(const char *data, size_t length)
{
    size_t drop;

    if (length == 0)
        return;
    if (length > LOG_CAPACITY) {
        data += length - LOG_CAPACITY;
        length = LOG_CAPACITY;
    }
    pthread_mutex_lock(&log_mutex);
    if (log_length + length > LOG_CAPACITY) {
        drop = log_length + length - LOG_CAPACITY;
        memmove(log_buffer, log_buffer + drop, log_length - drop);
        log_length -= drop;
        log_base += drop;
    }
    memcpy(log_buffer + log_length, data, length);
    log_length += length;
    log_next += length;
    pthread_mutex_unlock(&log_mutex);
    ws_broadcast_log(data, length);
}

static void *
log_reader(void *unused)
{
    char buffer[1024];
    ssize_t length;

    (void)unused;
    for (;;) {
        length = read(log_pipe[0], buffer, sizeof buffer);
        if (length <= 0)
            break;
        log_append(buffer, (size_t)length);
    }
    return NULL;
}

static int
start_log_capture(void)
{
    pthread_t thread;

    if (pipe(log_pipe) != 0)
        return -1;
    if (dup2(log_pipe[1], STDOUT_FILENO) < 0 ||
        dup2(log_pipe[1], STDERR_FILENO) < 0) {
        close(log_pipe[0]);
        close(log_pipe[1]);
        return -1;
    }
    close(log_pipe[1]);
    log_pipe[1] = -1;
    return pthread_create(&thread, NULL, log_reader, NULL);
}

static void
log_reset(void)
{
    pthread_mutex_lock(&log_mutex);
    log_length = 0;
    log_base = 0;
    log_next = 0;
    pthread_mutex_unlock(&log_mutex);
}

static void
log_clear_broadcast(void)
{
    log_reset();
    ws_broadcast("{\"type\":\"clear\"}");
}

static int
manifest_value(const char *manifest, const char *key,
               char *value, size_t value_size)
{
    FILE *file;
    char buffer[4096];
    char needle[80];
    char *cursor;
    char *end;
    size_t length;
    size_t key_length;

    file = fopen(manifest, "rb");
    if (file == NULL)
        return -1;
    length = fread(buffer, 1, sizeof buffer - 1, file);
    fclose(file);
    buffer[length] = '\0';
    snprintf(needle, sizeof needle, "\"%s\"", key);
    cursor = strstr(buffer, needle);
    if (cursor == NULL)
        return -1;
    cursor = strchr(cursor + strlen(needle), ':');
    if (cursor == NULL)
        return -1;
    cursor++;
    while (*cursor && isspace((unsigned char)*cursor))
        cursor++;
    if (*cursor != '"')
        return -1;
    cursor++;
    end = strchr(cursor, '"');
    if (end == NULL)
        return -1;
    key_length = (size_t)(end - cursor);
    if (key_length + 1 > value_size)
        return -1;
    memcpy(value, cursor, key_length);
    value[key_length] = '\0';
    return 0;
}

static int
valid_app_id(const char *id)
{
    const unsigned char *cursor = (const unsigned char *)id;

    if (*cursor == '\0')
        return 0;
    while (*cursor) {
        if (!(isalnum(*cursor) || *cursor == '_' || *cursor == '-'))
            return 0;
        cursor++;
    }
    return 1;
}

static const char *
query_value(const char *query, const char *key, char *out, size_t out_size)
{
    size_t key_length = strlen(key);
    const char *cursor = query;
    const char *end;
    size_t length;

    while (cursor != NULL && *cursor) {
        if (!strncmp(cursor, key, key_length) && cursor[key_length] == '=') {
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

static enum MHD_Result
queue_response(struct MHD_Connection *connection, unsigned status,
               const char *type, const char *extra, const void *body,
               size_t body_length, enum MHD_ResponseMemoryMode mode)
{
    struct MHD_Response *response;
    enum MHD_Result result;

    response = MHD_create_response_from_buffer(body_length, (void *)body, mode);
    if (response == NULL) {
        if (mode == MHD_RESPMEM_MUST_FREE)
            free((void *)body);
        return MHD_NO;
    }
    MHD_add_response_header(response, "Content-Type", type);
    MHD_add_response_header(response, "Access-Control-Allow-Origin", "*");
    if (extra != NULL)
        MHD_add_response_header(response, "X-Log-Next", extra);
    result = MHD_queue_response(connection, status, response);
    MHD_destroy_response(response);
    return result;
}

static enum MHD_Result
queue_text(struct MHD_Connection *connection, unsigned status,
           const char *type, const char *body)
{
    return queue_response(connection, status, type, NULL, body, strlen(body),
                          MHD_RESPMEM_MUST_COPY);
}

static enum MHD_Result
static_file_response(struct MHD_Connection *connection, const char *path,
                     const char *type)
{
    FILE *file;
    char *body;
    long file_size;
    size_t body_length;

    file = fopen(path, "rb");
    if (file == NULL)
        return queue_text(connection, MHD_HTTP_NOT_FOUND, "text/plain",
                          "file not found\n");
    if (fseek(file, 0, SEEK_END) != 0 ||
        (file_size = ftell(file)) < 0 ||
        fseek(file, 0, SEEK_SET) != 0 || file_size > 1024 * 1024) {
        fclose(file);
        return queue_text(connection, MHD_HTTP_INTERNAL_SERVER_ERROR,
                          "text/plain", "file read failed\n");
    }
    body_length = (size_t)file_size;
    body = malloc(body_length ? body_length : 1);
    if (body == NULL || fread(body, 1, body_length, file) != body_length) {
        free(body);
        fclose(file);
        return queue_text(connection, MHD_HTTP_INTERNAL_SERVER_ERROR,
                          "text/plain", "file read failed\n");
    }
    fclose(file);
    return queue_response(connection, MHD_HTTP_OK, type, NULL, body,
                          body_length, MHD_RESPMEM_MUST_FREE);
}

static enum MHD_Result
apps_response(struct MHD_Connection *connection)
{
    DIR *directory;
    struct dirent *entry;
    char body[8192];
    char manifest[PATH_CAPACITY];
    char name[128];
    char id[128];
    size_t at = 0;
    int first = 1;

    at += (size_t)snprintf(body + at, sizeof body - at, "[");
    directory = opendir("/data/python/apps");
    if (directory != NULL) {
        while ((entry = readdir(directory)) != NULL) {
            if (entry->d_name[0] == '.' || !valid_app_id(entry->d_name))
                continue;
            snprintf(manifest, sizeof manifest, "/data/python/apps/%s/app.json",
                     entry->d_name);
            if (manifest_value(manifest, "name", name, sizeof name) != 0)
                snprintf(name, sizeof name, "%s", entry->d_name);
            snprintf(id, sizeof id, "%s", entry->d_name);
            if (!first)
                at += (size_t)snprintf(body + at, sizeof body - at, ",");
            at += (size_t)snprintf(body + at, sizeof body - at,
                                   "{\"id\":\"");
            at = json_append(body, at, sizeof body, id);
            at += (size_t)snprintf(body + at, sizeof body - at,
                                   "\",\"name\":\"");
            at = json_append(body, at, sizeof body, name);
            at += (size_t)snprintf(body + at, sizeof body - at, "\"}");
            first = 0;
            if (at + 128 >= sizeof body)
                break;
        }
        closedir(directory);
    }
    at += (size_t)snprintf(body + at, sizeof body - at, "]");
    return queue_response(connection, MHD_HTTP_OK, "application/json", NULL,
                          body, at, MHD_RESPMEM_MUST_COPY);
}

static enum MHD_Result
status_response(struct MHD_Connection *connection)
{
    char body[224];
    int started;
    int running;
    int finished;
    int exit_code;
    long process_id;

    pthread_mutex_lock(&state_mutex);
    started = launch_started;
    running = app_running;
    finished = app_finished;
    exit_code = app_exit_code;
    pthread_mutex_unlock(&state_mutex);
    process_id = (long)getpid();
    snprintf(body, sizeof body,
             "{\"pid\":%ld,\"started\":%s,\"running\":%s,"
             "\"finished\":%s,\"exit_code\":%d,\"repl_port\":%u}",
             process_id, started ? "true" : "false",
             running ? "true" : "false", finished ? "true" : "false",
             exit_code, (unsigned)tcp_repl_port);
    return queue_response(connection, MHD_HTTP_OK, "application/json", NULL,
                          body, strlen(body), MHD_RESPMEM_MUST_COPY);
}

static enum MHD_Result
repl_reset_response(struct MHD_Connection *connection)
{
    int running;

    pthread_mutex_lock(&state_mutex);
    running = app_running;
    pthread_mutex_unlock(&state_mutex);
    if (running)
        return queue_text(connection, MHD_HTTP_CONFLICT, "text/plain",
                          "stop the running app before resetting the interpreter");
    if (cpython_ps5_runtime_reset(NULL) != 0)
        return queue_text(connection, MHD_HTTP_INTERNAL_SERVER_ERROR,
                          "text/plain", "interpreter reset failed");
    ws_broadcast("{\"type\":\"repl_reset\"}");
    return queue_text(connection, MHD_HTTP_OK, "application/json",
                      "{\"reset\":true}");
}

static enum MHD_Result
logs_response(struct MHD_Connection *connection, const char *query)
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
        return queue_text(connection, MHD_HTTP_INTERNAL_SERVER_ERROR,
                          "text/plain", "out of memory");
    pthread_mutex_lock(&log_mutex);
    start = since < log_base ? 0 : since - log_base;
    if (start > log_length)
        start = log_length;
    length = log_length - (size_t)start;
    memcpy(body, log_buffer + start, length);
    snprintf(next_text, sizeof next_text, "%llu",
             (unsigned long long)log_next);
    pthread_mutex_unlock(&log_mutex);
    return queue_response(connection, MHD_HTTP_OK, "text/plain; charset=utf-8",
                          next_text, body, length, MHD_RESPMEM_MUST_FREE);
}

static void *
app_worker(void *data)
{
    app_launch_t *launch = data;
    cpython_run_options_t options;
    int result;
    char line[96];
    int length;

    options.runtime_path = "/data/python/runtime/cpython-lib";
    options.app_root_path = launch->app_root;
    options.app_lib_path = launch->app_lib;
    result = cpython_ps5_run_file(launch->script_path, &options);
    pthread_mutex_lock(&state_mutex);
    app_exit_code = result;
    app_running = 0;
    app_finished = 1;
    launch_started = 0;
    pthread_mutex_unlock(&state_mutex);
    ws_broadcast_status();
    length = snprintf(line, sizeof line,
                      "\n[launcher] app exited with code %d\n", result);
    if (length > 0)
        log_append(line, (size_t)length);
    /* Preserve failed-start diagnostics so the web console can explain why
       an app exited. Successful short-lived apps keep the old behavior. */
    if (result == 0)
        log_reset();
    free(launch);
    return NULL;
}

static enum MHD_Result
launch_response(struct MHD_Connection *connection, const char *query)
{
    char app_id[128];
    char manifest[PATH_CAPACITY];
    char entry[128];
    app_launch_t *launch;
    pthread_t thread;
    char body[160];

    if (!query_value(query, "app", app_id, sizeof app_id) ||
        !valid_app_id(app_id))
        return queue_text(connection, MHD_HTTP_BAD_REQUEST, "text/plain",
                          "invalid app");
    pthread_mutex_lock(&state_mutex);
    if (launch_started) {
        pthread_mutex_unlock(&state_mutex);
        return queue_text(connection, MHD_HTTP_CONFLICT, "text/plain",
                          "app already launched");
    }
    pthread_mutex_unlock(&state_mutex);
    snprintf(manifest, sizeof manifest, "/data/python/apps/%s/app.json", app_id);
    if (manifest_value(manifest, "entry", entry, sizeof entry) != 0 ||
        entry[0] == '/' || strstr(entry, "..") != NULL)
        return queue_text(connection, MHD_HTTP_BAD_REQUEST, "text/plain",
                          "invalid manifest");
    launch = calloc(1, sizeof *launch);
    if (launch == NULL)
        return queue_text(connection, MHD_HTTP_INTERNAL_SERVER_ERROR,
                          "text/plain", "out of memory");
    snprintf(launch->app_root, sizeof launch->app_root,
             "/data/python/apps/%s", app_id);
    snprintf(launch->app_lib, sizeof launch->app_lib, "%s/lib",
             launch->app_root);
    snprintf(launch->script_path, sizeof launch->script_path, "%s/%s",
             launch->app_root, entry);
    if (access(launch->script_path, R_OK) != 0) {
        free(launch);
        return queue_text(connection, MHD_HTTP_NOT_FOUND, "text/plain",
                          "entry not found");
    }
    log_clear_broadcast();
    pthread_mutex_lock(&state_mutex);
    launch_started = 1;
    app_running = 1;
    app_finished = 0;
    app_exit_code = -1;
    pthread_mutex_unlock(&state_mutex);
    ws_broadcast_status();
    if (pthread_create(&thread, NULL, app_worker, launch) != 0) {
        pthread_mutex_lock(&state_mutex);
        launch_started = 0;
        app_running = 0;
        pthread_mutex_unlock(&state_mutex);
        free(launch);
        ws_broadcast_status();
        return queue_text(connection, MHD_HTTP_INTERNAL_SERVER_ERROR,
                          "text/plain", "thread failed");
    }
    pthread_detach(thread);
    snprintf(body, sizeof body, "{\"started\":true,\"app\":\"%s\"}",
             app_id);
    return queue_response(connection, MHD_HTTP_OK, "application/json", NULL,
                          body, strlen(body), MHD_RESPMEM_MUST_COPY);
}

static int
header_has_token(const char *header, const char *token)
{
    size_t token_length = strlen(token);
    const char *cursor = header;

    while (cursor != NULL && *cursor) {
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

static enum MHD_Result
websocket_response(struct MHD_Connection *connection)
{
    const char *key;
    const char *upgrade;
    const char *connection_header;
    char accept[64];
    struct MHD_Response *response;
    enum MHD_Result result;

    upgrade = MHD_lookup_connection_value(connection, MHD_HEADER_KIND,
                                          "Upgrade");
    connection_header = MHD_lookup_connection_value(connection, MHD_HEADER_KIND,
                                                    "Connection");
    key = MHD_lookup_connection_value(connection, MHD_HEADER_KIND,
                                      "Sec-WebSocket-Key");
    if (upgrade == NULL || strcasecmp(upgrade, "websocket") != 0 ||
        connection_header == NULL || !header_has_token(connection_header, "Upgrade") ||
        key == NULL || websocket_accept_key(key, accept) != 0)
        return queue_text(connection, MHD_HTTP_BAD_REQUEST, "text/plain",
                          "WebSocket upgrade required");
    response = MHD_create_response_for_upgrade(ws_upgrade_handler, NULL);
    if (response == NULL)
        return MHD_NO;
    MHD_add_response_header(response, "Upgrade", "websocket");
    MHD_add_response_header(response, "Connection", "Upgrade");
    MHD_add_response_header(response, "Sec-WebSocket-Accept", accept);
    result = MHD_queue_response(connection, MHD_HTTP_SWITCHING_PROTOCOLS,
                                response);
    MHD_destroy_response(response);
    return result;
}

static enum MHD_Result
access_handler(void *cls, struct MHD_Connection *connection,
               const char *url, const char *method, const char *version,
               const char *upload_data, size_t *upload_data_size,
               void **con_cls)
{
    char target[1024];
    const char *argument;
    char query[160];

    (void)cls;
    (void)version;
    (void)upload_data;
    if (strcmp(method, "GET") != 0)
        return queue_text(connection, MHD_HTTP_BAD_REQUEST, "text/plain",
                          "GET required");
    if (*con_cls == NULL) {
        *con_cls = (void *)1;
        return MHD_YES;
    }
    if (*upload_data_size != 0) {
        *upload_data_size = 0;
        return MHD_YES;
    }
    if (strlen(url) >= sizeof target)
        return queue_text(connection, MHD_HTTP_URI_TOO_LONG, "text/plain",
                          "URI too long");
    strcpy(target, url);
    if (!strcmp(target, "/ws"))
        return websocket_response(connection);
    if (!strcmp(target, "/"))
        return static_file_response(connection, "/data/python/web/index.html",
                                    "text/html; charset=utf-8");
    if (!strcmp(target, "/app.css"))
        return static_file_response(connection, "/data/python/web/app.css",
                                    "text/css");
    if (!strcmp(target, "/app.js"))
        return static_file_response(connection, "/data/python/web/app.js",
                                    "application/javascript");
    if (!strcmp(target, "/api/apps"))
        return apps_response(connection);
    if (!strcmp(target, "/api/status"))
        return status_response(connection);
    if (!strcmp(target, "/api/repl/reset"))
        return repl_reset_response(connection);
    if (!strcmp(target, "/api/logs/clear")) {
        log_clear_broadcast();
        return queue_text(connection, MHD_HTTP_OK, "application/json",
                          "{\"cleared\":true}");
    }
    if (!strcmp(target, "/api/logs")) {
        argument = MHD_lookup_connection_value(connection, MHD_GET_ARGUMENT_KIND,
                                                "since");
        if (argument != NULL)
            snprintf(query, sizeof query, "since=%s", argument);
        else
            query[0] = '\0';
        return logs_response(connection, query);
    }
    if (!strcmp(target, "/api/launch")) {
        argument = MHD_lookup_connection_value(connection, MHD_GET_ARGUMENT_KIND,
                                                "app");
        if (argument != NULL)
            snprintf(query, sizeof query, "app=%s", argument);
        else
            query[0] = '\0';
        return launch_response(connection, query);
    }
    if (!strcmp(target, "/api/shutdown")) {
        server_stop = 1;
        return queue_text(connection, MHD_HTTP_OK, "text/plain", "bye\n");
    }
    return queue_text(connection, MHD_HTTP_NOT_FOUND, "text/plain",
                      "not found\n");
}

int
main(int argc, char **argv)
{
    unsigned long port = DEFAULT_PORT;
    unsigned long repl_port = 0;
    struct MHD_Daemon *daemon;
    cpython_run_options_t runtime_options;

    if (argc > 1)
        port = strtoul(argv[1], NULL, 10);
    if (argc > 2)
        repl_port = strtoul(argv[2], NULL, 10);
    else if (port < 65535)
        repl_port = port + 1;
    if (port == 0 || port > 65535 || repl_port == 0 || repl_port > 65535 ||
        repl_port == port)
        return 2;
    tcp_repl_port = (unsigned short)repl_port;
    runtime_options.runtime_path = "/data/python/runtime/cpython-lib";
    runtime_options.app_root_path = NULL;
    runtime_options.app_lib_path = NULL;
    if (cpython_ps5_runtime_start(&runtime_options) != 0)
        return 1;
    signal(SIGPIPE, SIG_IGN);
    if (start_log_capture() != 0) {
        cpython_ps5_runtime_stop();
        return 1;
    }
    if (tcp_repl_start(tcp_repl_port) != 0) {
        cpython_ps5_runtime_stop();
        return 1;
    }
    daemon = MHD_start_daemon(MHD_USE_INTERNAL_POLLING_THREAD |
                                  MHD_USE_THREAD_PER_CONNECTION | MHD_USE_ITC |
                                  MHD_ALLOW_UPGRADE,
                              (uint16_t)port, NULL, NULL, access_handler, NULL,
                              MHD_OPTION_END);
    if (daemon == NULL) {
        tcp_repl_stop();
        cpython_ps5_runtime_stop();
        return 1;
    }
    while (!server_stop)
        sleep(1);
    MHD_stop_daemon(daemon);
    tcp_repl_stop();
    cpython_ps5_runtime_stop();
    return 0;
}
