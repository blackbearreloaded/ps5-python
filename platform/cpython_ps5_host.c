#include "cpython_ps5_host.h"

#include <stdio.h>
#include <string.h>

static const char default_script[] = "/data/python/main.py";

int
cpython_ps5_select_script(int argc, char **argv, char *out, size_t out_size)
{
    const char *script = default_script;
    size_t length;

    if (argc > 1 && argv != NULL && argv[1] != NULL && argv[1][0] != '\0')
        script = argv[1];

    length = strlen(script);
    if (out == NULL || out_size == 0 || length + 1 > out_size)
        return -1;

    memcpy(out, script, length + 1);
    return 0;
}

#ifdef CPYTHON_PS5
typedef struct notify_request {
    char useless1[45];
    char message[3075];
} notify_request_t;

int sceKernelSendNotificationRequest(int, notify_request_t *, size_t, int);
#endif

void
cpython_ps5_notify(const char *message)
{
#ifdef CPYTHON_PS5
    notify_request_t request;

    memset(&request, 0, sizeof request);
    strncpy(request.message, message, sizeof request.message - 1);
    sceKernelSendNotificationRequest(0, &request, sizeof request, 0);
#else
    fprintf(stderr, "%s\n", message);
#endif
}
