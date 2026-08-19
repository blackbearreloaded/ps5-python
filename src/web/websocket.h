#ifndef CPYTHON_PS5_WEBSOCKET_H
#define CPYTHON_PS5_WEBSOCKET_H

#include <microhttpd.h>
#include <pthread.h>
#include <stddef.h>

/* Shared client storage is kept here so the HTTP upgrade code and broadcast
   implementation use the same connection pool. */
#define WS_MAX_CLIENTS 4
#define WS_INITIAL_CAPACITY 2048
#define WS_FRAME_CAPACITY 65536

typedef struct ws_client
{
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

extern pthread_mutex_t ws_mutex;
extern ws_client_t ws_clients[WS_MAX_CLIENTS];

int websocket_send_frame(ws_client_t *client, unsigned opcode, const void *payload, size_t length);
int websocket_send_text(ws_client_t *client, const char *text);
void websocket_broadcast(const char *message);
void websocket_broadcast_log(const char *data, size_t length);
void websocket_broadcast_status(void);

#endif
