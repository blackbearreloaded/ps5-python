#ifndef CPYTHON_PS5_HOST_H
#define CPYTHON_PS5_HOST_H

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/*
 * Resolve the script passed to the runtime without relying on PATH or the
 * current working directory. The initial payload default is
 * /data/python/main.py; applications can later switch this to /app0/main.py.
 */
int cpython_ps5_select_script(int argc, char **argv,
                              char *out, size_t out_size);

void cpython_ps5_notify(const char *message);

#ifdef __cplusplus
}
#endif

#endif
