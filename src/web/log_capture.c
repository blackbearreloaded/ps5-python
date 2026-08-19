#include <pthread.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include "log_capture.h"
#include "web_state.h"
#include "websocket.h"

void log_append(const char *data, size_t length)
{
    if (length == 0)
        return;
    if (length > WEB_LOG_CAPACITY)
    {
        data += length - WEB_LOG_CAPACITY;
        length = WEB_LOG_CAPACITY;
    }
    pthread_mutex_lock(&web_log_mutex);
    if (web_log_length + length > WEB_LOG_CAPACITY)
    {
        size_t drop = web_log_length + length - WEB_LOG_CAPACITY;
        memmove(web_log_buffer, web_log_buffer + drop, web_log_length - drop);
        web_log_length -= drop;
        web_log_base += drop;
    }
    memcpy(web_log_buffer + web_log_length, data, length);
    web_log_length += length;
    web_log_next += length;
    pthread_mutex_unlock(&web_log_mutex);
    websocket_broadcast_log(data, length);
}

static void *log_reader(void *unused)
{
    char buffer[1024];
    (void)unused;
    for (;;)
    {
        ssize_t length = read(web_log_pipe[0], buffer, sizeof buffer);
        if (length <= 0)
            break;
        log_append(buffer, (size_t)length);
    }
    return NULL;
}

int start_log_capture(void)
{
    pthread_t thread;
    if (pipe(web_log_pipe) != 0)
        return -1;
    if (dup2(web_log_pipe[1], STDOUT_FILENO) < 0 || dup2(web_log_pipe[1], STDERR_FILENO) < 0)
    {
        close(web_log_pipe[0]);
        close(web_log_pipe[1]);
        return -1;
    }
    close(web_log_pipe[1]);
    web_log_pipe[1] = -1;
    return pthread_create(&thread, NULL, log_reader, NULL);
}

void log_reset(void)
{
    pthread_mutex_lock(&web_log_mutex);
    web_log_length = 0;
    web_log_base = 0;
    web_log_next = 0;
    pthread_mutex_unlock(&web_log_mutex);
}

void log_clear_broadcast(void)
{
    log_reset();
    websocket_broadcast("{\"type\":\"clear\"}");
}
