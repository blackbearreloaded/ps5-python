#pragma once

typedef struct cpython_run_options {
    const char *runtime_path;
    const char *app_root_path;
    const char *app_lib_path;
} cpython_run_options_t;

int cpython_ps5_run_file(const char *script_path,
                         const cpython_run_options_t *options);
