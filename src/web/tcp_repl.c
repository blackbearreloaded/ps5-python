#include <netinet/in.h>
#include <pthread.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

#include "cpython_runtime.h"
#include "tcp_repl.h"
#include "web_state.h"

#define TCP_REPL_LINE_CAPACITY 65536

static int tcp_repl_fd = -1;
static pthread_t tcp_repl_thread;
static int tcp_repl_started;

static int send_all(int fd, const void *data, size_t length)
{
    const char *cursor = data;
    while (length > 0)
    {
        ssize_t sent = send(fd, cursor, length, 0);
        if (sent <= 0)
            return -1;
        cursor += sent;
        length -= (size_t)sent;
    }
    return 0;
}

static int send_text(int fd, const char *text)
{
    return send_all(fd, text, strlen(text));
}

static int execute_line(int fd, char *line, size_t *line_length, char *output)
{
    size_t output_length;
    line[*line_length] = '\0';
    (void)cpython_ps5_runtime_eval(line, output, 8192);
    output_length = strlen(output);
    if (output_length > 0 && send_all(fd, output, output_length) != 0)
        return -1;
    if (output_length == 0 || output[output_length - 1] != '\n')
    {
        if (send_text(fd, "\r\n") != 0)
            return -1;
    }
    if (send_text(fd, ">>> ") != 0)
        return -1;
    *line_length = 0;
    return 0;
}

static void *client_worker(void *data)
{
    int fd = *(int *)data;
    char *line = NULL;
    char *output = NULL;
    unsigned char input[1024];
    size_t line_length = 0;
    int pending_cr = 0;

    free(data);
    line = malloc(TCP_REPL_LINE_CAPACITY + 1);
    output = malloc(8192);
    if (line == NULL || output == NULL)
        goto done;
    if (send_text(fd, "CPython 3.14.7 TCP REPL\r\n>>> ") != 0)
        goto done;
    for (;;)
    {
        ssize_t received = recv(fd, input, sizeof input, 0);
        if (received <= 0 || server_stop)
            break;
        for (size_t i = 0; i < (size_t)received; i++)
        {
            if (pending_cr)
            {
                pending_cr = 0;
                if (input[i] == '\n')
                    continue;
            }
            if (input[i] == '\r')
            {
                if (execute_line(fd, line, &line_length, output) != 0)
                    goto done;
                pending_cr = 1;
            }
            else if (input[i] != '\n')
            {
                if (line_length + 1 >= TCP_REPL_LINE_CAPACITY)
                {
                    if (send_text(fd, "input line too long\r\n>>> ") != 0)
                        goto done;
                    line_length = 0;
                }
                else
                {
                    line[line_length++] = (char)input[i];
                }
            }
            else if (execute_line(fd, line, &line_length, output) != 0)
            {
                goto done;
            }
        }
    }
done:
    free(output);
    free(line);
    shutdown(fd, SHUT_RDWR);
    close(fd);
    return NULL;
}

static void *server_worker(void *unused)
{
    (void)unused;
    while (!server_stop)
    {
        struct sockaddr_in address;
        socklen_t address_length = sizeof address;
        int client_fd = accept(tcp_repl_fd, (struct sockaddr *)&address, &address_length);
        if (client_fd < 0)
        {
            if (server_stop)
                break;
            continue;
        }
        int *client_data = malloc(sizeof *client_data);
        pthread_t thread;
        if (client_data == NULL)
        {
            close(client_fd);
            continue;
        }
        *client_data = client_fd;
        if (pthread_create(&thread, NULL, client_worker, client_data) != 0)
        {
            close(client_fd);
            free(client_data);
            continue;
        }
        pthread_detach(thread);
    }
    return NULL;
}

int tcp_repl_start(unsigned short port)
{
    struct sockaddr_in address;
    int option = 1;

    tcp_repl_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (tcp_repl_fd < 0)
        return -1;
    (void)setsockopt(tcp_repl_fd, SOL_SOCKET, SO_REUSEADDR, &option, sizeof option);
    memset(&address, 0, sizeof address);
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = htonl(INADDR_ANY);
    address.sin_port = htons(port);
    if (bind(tcp_repl_fd, (struct sockaddr *)&address, sizeof address) != 0 ||
        listen(tcp_repl_fd, 4) != 0 ||
        pthread_create(&tcp_repl_thread, NULL, server_worker, NULL) != 0)
    {
        close(tcp_repl_fd);
        tcp_repl_fd = -1;
        return -1;
    }
    tcp_repl_started = 1;
    return 0;
}

void tcp_repl_stop(void)
{
    if (!tcp_repl_started)
        return;
    shutdown(tcp_repl_fd, SHUT_RDWR);
    close(tcp_repl_fd);
    tcp_repl_fd = -1;
    pthread_join(tcp_repl_thread, NULL);
    tcp_repl_started = 0;
}
