#pragma once

#include <stddef.h>

typedef struct cpython_run_options {
    const char *runtime_path;
    const char *app_root_path;
    const char *app_lib_path;
    const char *const *argv;
    size_t argc;
} cpython_run_options_t;

#define CPYTHON_PS5_RUNTIME_RESTARTED 1
#define CPYTHON_PS5_RUNTIME_STOPPED 2

int cpython_ps5_run_file(const char *script_path,
                         const cpython_run_options_t *options);
/* Request a cooperative KeyboardInterrupt in the currently running app. */
int cpython_ps5_runtime_request_stop(void);

/* Persistent interpreter services used by the web launcher. */
int cpython_ps5_runtime_start(const cpython_run_options_t *options);
int cpython_ps5_runtime_reset(const cpython_run_options_t *options);
int cpython_ps5_runtime_eval(const char *source, char *output,
                             size_t output_size);
void cpython_ps5_runtime_stop(void);
