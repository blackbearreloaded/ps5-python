#include <arpa/inet.h>
#include <ctype.h>
#include <errno.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>

#include "cpython_runtime.h"

#define PATH_CAPACITY 512
#define CONTROL_CAPACITY 4096
#define LOG_CHUNK 1024
#define STOP_TIMEOUT_SECONDS 3
#define MAX_SESSIONS 16
#define MAX_APP_ARGUMENTS 32
#define MAX_APP_ARGUMENT_LENGTH 512

static volatile sig_atomic_t session_stop_requested;

static void session_signal_handler(int signal_number)
{
    (void)signal_number;
    session_stop_requested = 1;
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

static int send_line(int fd, const char *line)
{
    return write_all(fd, line, strlen(line));
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

static int send_logs(int fd, int output_fd)
{
    char buffer[LOG_CHUNK];
    ssize_t length = read(output_fd, buffer, sizeof buffer);
    if (length <= 0)
        return length == 0 ? 1 : -1;
    char header[64];
    int header_length = snprintf(header, sizeof header, "LOG %ld\n", (long)length);
    if (header_length <= 0 || write_all(fd, header, (size_t)header_length) != 0 ||
        write_all(fd, buffer, (size_t)length) != 0)
        return -1;
    return 0;
}

static void free_app_arguments(char **arguments, size_t count)
{
    for (size_t index = 0; index < count; index++)
        free(arguments[index]);
    free(arguments);
}

static int parse_app_arguments(const char *text, char ***arguments_out, size_t *count_out)
{
    char **arguments = calloc(MAX_APP_ARGUMENTS, sizeof *arguments);
    size_t count = 0;
    const char *cursor = text == NULL ? "" : text;
    if (arguments == NULL)
        return -1;
    while (*cursor != '\0')
    {
        char argument[MAX_APP_ARGUMENT_LENGTH];
        size_t length = 0;
        int quote = 0;
        int escaped = 0;
        while (isspace((unsigned char)*cursor))
            cursor++;
        if (*cursor == '\0')
            break;
        while (*cursor != '\0')
        {
            unsigned char current = (unsigned char)*cursor++;
            if (escaped)
            {
                if (length + 1 >= sizeof argument)
                    goto invalid;
                argument[length++] = (char)current;
                escaped = 0;
            }
            else if (current == '\\' && !quote)
                escaped = 1;
            else if (quote != 0)
            {
                if (current == (unsigned char)quote)
                    quote = 0;
                else
                {
                    if (length + 1 >= sizeof argument)
                        goto invalid;
                    argument[length++] = (char)current;
                }
            }
            else if (current == '\'' || current == '"')
                quote = current;
            else if (isspace(current))
                break;
            else
            {
                if (length + 1 >= sizeof argument)
                    goto invalid;
                argument[length++] = (char)current;
            }
        }
        if (quote != 0 || escaped || count >= MAX_APP_ARGUMENTS)
            goto invalid;
        argument[length] = '\0';
        arguments[count] = malloc(length + 1);
        if (arguments[count] == NULL)
            goto invalid;
        memcpy(arguments[count], argument, length + 1);
        count++;
    }
    *arguments_out = arguments;
    *count_out = count;
    return 0;

invalid:
    free_app_arguments(arguments, count);
    return -1;
}

static int run_child_session(int client_fd, const char *script_path, const char *app_root,
                             const char *app_lib, const char *argument_text)
{
    int output_pipe[2];
    pid_t child_pid;
    int child_status = 0;
    int output_open = 1;
    int stopping = 0;
    int shutdown_requested = 0;
    time_t stop_started = 0;
    char pending[CONTROL_CAPACITY];
    size_t pending_length = 0;
    struct sigaction action;

    memset(&action, 0, sizeof action);
    action.sa_handler = session_signal_handler;
    sigemptyset(&action.sa_mask);
    sigaction(SIGTERM, &action, NULL);
    sigaction(SIGINT, &action, NULL);
    session_stop_requested = 0;
    if (pipe(output_pipe) != 0)
        return -1;

    child_pid = fork();
    if (child_pid < 0)
    {
        close(output_pipe[0]);
        close(output_pipe[1]);
        return -1;
    }
    if (child_pid == 0)
    {
        char **arguments = NULL;
        size_t argument_count = 0;
        cpython_run_options_t options = {
            .runtime_path = "/data/python/runtime/cpython-lib",
            .app_root_path = app_root,
            .app_lib_path = app_lib,
        };
        int result;
        if (parse_app_arguments(argument_text, &arguments, &argument_count) != 0)
            _exit(2);
        options.argv = (const char *const *)arguments;
        options.argc = argument_count;
        close(output_pipe[0]);
        close(client_fd);
        if (dup2(output_pipe[1], STDOUT_FILENO) < 0 ||
            dup2(output_pipe[1], STDERR_FILENO) < 0)
            _exit(127);
        close(output_pipe[1]);
        result = cpython_ps5_run_file(script_path, &options);
        free_app_arguments(arguments, argument_count);
        _exit(result < 0 ? 1 : result & 0xff);
    }

    close(output_pipe[1]);
    {
        char line[64];
        snprintf(line, sizeof line, "STARTED %ld\n", (long)child_pid);
        if (send_line(client_fd, line) != 0)
            stopping = 1;
    }

    while (output_open || child_pid > 0)
    {
        fd_set read_set;
        struct timeval timeout = {.tv_sec = 0, .tv_usec = 100000};
        int max_fd = -1;
        int ready;
        char command[CONTROL_CAPACITY];

        FD_ZERO(&read_set);
        if (client_fd >= 0)
        {
            FD_SET(client_fd, &read_set);
            max_fd = client_fd;
        }
        if (output_open)
        {
            FD_SET(output_pipe[0], &read_set);
            if (output_pipe[0] > max_fd)
                max_fd = output_pipe[0];
        }
        ready = select(max_fd + 1, &read_set, NULL, NULL, &timeout);
        if (ready < 0 && errno != EINTR)
            break;
        if (session_stop_requested && !stopping && child_pid > 0)
        {
            (void)kill(child_pid, SIGTERM);
            stopping = 1;
            stop_started = time(NULL);
        }
        if (client_fd >= 0 && FD_ISSET(client_fd, &read_set))
        {
            ssize_t length = recv(client_fd, command, sizeof command, 0);
            if (length <= 0)
            {
                close(client_fd);
                client_fd = -1;
                if (!stopping)
                {
                    (void)kill(child_pid, SIGTERM);
                    stopping = 1;
                    stop_started = time(NULL);
                }
            }
            else if (pending_length + (size_t)length <= sizeof pending)
            {
                memcpy(pending + pending_length, command, (size_t)length);
                pending_length += (size_t)length;
                char *newline;
                while ((newline = memchr(pending, '\n', pending_length)) != NULL)
                {
                    size_t command_length = (size_t)(newline - pending);
                    memmove(command, pending, command_length);
                    command[command_length] = '\0';
                    memmove(pending, newline + 1,
                            pending_length - command_length - 1);
                    pending_length -= command_length + 1;
                    if (!strcmp(command, "STOP") || !strcmp(command, "SHUTDOWN"))
                    {
                        if (!strcmp(command, "SHUTDOWN"))
                            shutdown_requested = 1;
                        if (!stopping)
                        {
                            (void)send_line(client_fd, "STOPPING\n");
                            (void)kill(child_pid, SIGTERM);
                            stopping = 1;
                            stop_started = time(NULL);
                        }
                    }
                }
            }
        }
        if (output_open && FD_ISSET(output_pipe[0], &read_set))
        {
            int result = send_logs(client_fd, output_pipe[0]);
            if (result == 1)
            {
                close(output_pipe[0]);
                output_open = 0;
            }
            else if (result < 0)
            {
                close(output_pipe[0]);
                output_open = 0;
                if (client_fd >= 0)
                {
                    close(client_fd);
                    client_fd = -1;
                }
            }
        }
        if (child_pid > 0)
        {
            pid_t waited = waitpid(child_pid, &child_status, WNOHANG);
            if (waited == child_pid)
                child_pid = -1;
            if (stopping && stop_started != 0 && time(NULL) - stop_started >= STOP_TIMEOUT_SECONDS)
            {
                if (child_pid > 0)
                    (void)kill(child_pid, SIGKILL);
                stop_started = 0;
            }
        }
    }
    if (output_open)
        close(output_pipe[0]);
    if (child_pid > 0)
    {
        (void)kill(child_pid, SIGKILL);
        (void)waitpid(child_pid, &child_status, 0);
    }
    if (client_fd >= 0)
    {
        int result = WIFEXITED(child_status) ? WEXITSTATUS(child_status) : 130;
        char line[64];
        snprintf(line, sizeof line, "EXIT %d\n", result);
        (void)send_line(client_fd, line);
        close(client_fd);
    }
    return shutdown_requested ? 1 : 0;
}

static int handle_connection(int client_fd, int server_fd, pid_t *session_pid)
{
    char line[CONTROL_CAPACITY];
    if (read_line(client_fd, line, sizeof line) != 0)
        return 0;
    if (!strcmp(line, "PING"))
    {
        (void)send_line(client_fd, "PONG\n");
        return 0;
    }
    if (!strcmp(line, "SHUTDOWN"))
    {
        (void)send_line(client_fd, "BYE\n");
        return 1;
    }
    if (strncmp(line, "START\t", 6) != 0)
        return 0;

    char *fields[5] = {0};
    char *cursor = line;
    for (unsigned i = 0; i < 5; i++)
    {
        fields[i] = cursor;
        cursor = strchr(cursor, '\t');
        if (cursor == NULL)
            break;
        *cursor++ = '\0';
    }
    if (fields[1] == NULL || fields[2] == NULL || fields[3] == NULL || fields[4] == NULL ||
        fields[0][0] == '\0' || fields[1][0] == '\0' || fields[2][0] == '\0' ||
        fields[3][0] == '\0')
        return 0;
    *session_pid = fork();
    if (*session_pid < 0)
        return 0;
    if (*session_pid == 0)
    {
        int result;
        close(server_fd);
        result = run_child_session(client_fd, fields[1], fields[2], fields[3], fields[4]);
        close(client_fd);
        _exit(result == 0 ? 0 : 1);
    }
    return 0;
}

int main(int argc, char **argv)
{
    unsigned long port = argc > 1 ? strtoul(argv[1], NULL, 10) : 8092;
    struct sockaddr_in address;
    int server_fd;
    pid_t session_pids[MAX_SESSIONS] = {0};
    unsigned session_count = 0;

    if (port == 0 || port > 65535 || cpython_ps5_runtime_start(NULL) != 0)
        return 2;
    signal(SIGPIPE, SIG_IGN);
    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd < 0)
        return 1;
    (void)setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &(int){1}, sizeof(int));
    memset(&address, 0, sizeof address);
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    address.sin_port = htons((unsigned short)port);
    if (bind(server_fd, (struct sockaddr *)&address, sizeof address) != 0 ||
        listen(server_fd, 4) != 0)
    {
        close(server_fd);
        cpython_ps5_runtime_stop();
        return 1;
    }
    printf("[app-supervisor] ready on 127.0.0.1:%lu\n", port);
    fflush(stdout);
    for (;;)
    {
        for (unsigned i = 0; i < MAX_SESSIONS; i++)
        {
            if (session_pids[i] > 0 && waitpid(session_pids[i], NULL, WNOHANG) == session_pids[i])
            {
                session_pids[i] = 0;
                if (session_count > 0)
                    session_count--;
            }
        }
        int client_fd = accept(server_fd, NULL, NULL);
        if (client_fd < 0)
        {
            if (errno == EINTR)
                continue;
            break;
        }
        char command[CONTROL_CAPACITY];
        ssize_t peek_length = recv(client_fd, command, sizeof command, MSG_PEEK);
        if (peek_length > 0 && !strncmp(command, "START\t", 6) && session_count >= MAX_SESSIONS)
        {
            (void)send_line(client_fd, "BUSY\n");
            close(client_fd);
            continue;
        }
        pid_t session_pid = 0;
        if (handle_connection(client_fd, server_fd, &session_pid))
            break;
        if (session_pid > 0)
        {
            for (unsigned i = 0; i < MAX_SESSIONS; i++)
            {
                if (session_pids[i] == 0)
                {
                    session_pids[i] = session_pid;
                    session_count++;
                    break;
                }
            }
            close(client_fd);
        }
        else
            close(client_fd);
    }
    for (unsigned i = 0; i < MAX_SESSIONS; i++)
    {
        if (session_pids[i] > 0)
            (void)kill(session_pids[i], SIGTERM);
    }
    for (unsigned i = 0; i < MAX_SESSIONS; i++)
    {
        if (session_pids[i] > 0)
            (void)waitpid(session_pids[i], NULL, 0);
    }
    close(server_fd);
    cpython_ps5_runtime_stop();
    return 0;
}
