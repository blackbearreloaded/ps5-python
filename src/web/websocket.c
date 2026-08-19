#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

#include "web_state.h"
#include "web_utils.h"
#include "websocket.h"

pthread_mutex_t ws_mutex = PTHREAD_MUTEX_INITIALIZER;
ws_client_t ws_clients[WS_MAX_CLIENTS];

int websocket_send_frame(ws_client_t *client, unsigned opcode, const void *payload, size_t length)
{
    unsigned char header[10];
    size_t header_length;
    if (length > WS_FRAME_CAPACITY)
        return -1;
    header[0] = (unsigned char)(0x80 | (opcode & 0x0f));
    if (length < 126)
    {
        header[1] = (unsigned char)length;
        header_length = 2;
    }
    else if (length <= 0xffff)
    {
        header[1] = 126;
        header[2] = (unsigned char)(length >> 8);
        header[3] = (unsigned char)length;
        header_length = 4;
    }
    else
    {
        header[1] = 127;
        for (unsigned i = 0; i < 8; i++)
            header[2 + i] = (unsigned char)(length >> (56 - i * 8));
        header_length = 10;
    }
    pthread_mutex_lock(&client->send_mutex);
    int result = web_send_all(client->fd, header, header_length) == 0 &&
                         web_send_all(client->fd, payload, length) == 0
                     ? 0
                     : -1;
    pthread_mutex_unlock(&client->send_mutex);
    return result;
}

int websocket_send_text(ws_client_t *client, const char *text)
{
    return websocket_send_frame(client, 1, text, strlen(text));
}

void websocket_broadcast(const char *message)
{
    pthread_mutex_lock(&ws_mutex);
    for (unsigned i = 0; i < WS_MAX_CLIENTS; i++)
    {
        if (ws_clients[i].active && websocket_send_text(&ws_clients[i], message) != 0)
            ws_clients[i].active = 0;
    }
    pthread_mutex_unlock(&ws_mutex);
}

void websocket_broadcast_log(const char *data, size_t length)
{
    static const char prefix[] = "{\"type\":\"log\",\"data\":\"";
    char *message = malloc(length * 2 + 32);
    if (message == NULL)
        return;
    memcpy(message, prefix, sizeof prefix - 1);
    size_t at = web_json_append_bytes(message, sizeof prefix - 1, length * 2 + 32,
                                      (const unsigned char *)data, length);
    (void)snprintf(message + at, length * 2 + 32 - at, "\"}");
    websocket_broadcast(message);
    free(message);
}

void websocket_broadcast_status(void)
{
    char message[224];
    pthread_mutex_lock(&web_state_mutex);
    snprintf(message, sizeof message,
             "{\"type\":\"status\",\"pid\":%ld,\"running\":%s,"
             "\"finished\":%s,\"exit_code\":%d,\"repl_port\":%u}",
             (long)getpid(), web_app_running ? "true" : "false",
             web_app_finished ? "true" : "false", web_app_exit_code, (unsigned)tcp_repl_port);
    pthread_mutex_unlock(&web_state_mutex);
    websocket_broadcast(message);
}
