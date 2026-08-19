#include <errno.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

static void
usage(const char *program)
{
    fprintf(stderr, "usage: %s PID [SIGNAL]\n", program);
}

int
main(int argc, char **argv)
{
    char *end;
    long pid_value;
    long signal_value = SIGTERM;
    pid_t pid;
    int signal_number;

    if (argc < 2 || argc > 3) {
        usage(argv[0]);
        return 2;
    }
    errno = 0;
    pid_value = strtol(argv[1], &end, 10);
    if (errno != 0 || end == argv[1] || *end != '\0' ||
        pid_value <= 1 || pid_value > 0x7fffffff) {
        fprintf(stderr, "invalid PID: %s\n", argv[1]);
        return 2;
    }
    if (argc == 3) {
        errno = 0;
        signal_value = strtol(argv[2], &end, 10);
        if (errno != 0 || end == argv[2] || *end != '\0' ||
            signal_value <= 0 || signal_value > 128) {
            fprintf(stderr, "invalid signal: %s\n", argv[2]);
            return 2;
        }
    }
    pid = (pid_t)pid_value;
    signal_number = (int)signal_value;
    if (kill(pid, signal_number) != 0) {
        fprintf(stderr, "kill(%ld, %d) failed: %s\n", pid_value,
                signal_number, strerror(errno));
        return 1;
    }
    printf("sent signal %d to PID %ld\n", signal_number, pid_value);
    return 0;
}
