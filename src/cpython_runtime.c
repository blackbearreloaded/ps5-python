#include <Python.h>

#include <stdio.h>
#include <stdlib.h>

#ifdef CPYTHON_PS5
#include <sys/resource.h>
#endif

#include "cpython_ps5_host.h"
#include "cpython_runtime.h"

PyMODINIT_FUNC PyInit__codecs(void);

static long
cpython_ps5_peak_rss(void)
{
#ifdef CPYTHON_PS5
    struct rusage usage;

    if (getrusage(RUSAGE_SELF, &usage) == 0)
        return usage.ru_maxrss;
#endif
    return -1;
}

#ifdef CPYTHON_PS5
static void
cpython_ps5_configure_tempdir(void)
{
    /* /user/temp is a PS5-managed writable directory cleaned on restart. */
    const char *tmpdir = getenv("TMPDIR");

    if (tmpdir == NULL || tmpdir[0] == '\0')
        (void)setenv("TMPDIR", "/user/temp", 0);
}
#endif

static int
append_module_path(PyConfig *config, const char *path)
{
    PyStatus status;
    wchar_t *path_wide;

    if (path == NULL || path[0] == '\0')
        return 0;

    path_wide = Py_DecodeLocale(path, NULL);
    if (path_wide == NULL)
        return -1;

    status = PyWideStringList_Append(&config->module_search_paths,
                                     path_wide);
    PyMem_RawFree(path_wide);
    return PyStatus_Exception(status) ? -1 : 0;
}

int
cpython_ps5_run_file(const char *script_path,
                     const cpython_run_options_t *options)
{
    PyConfig config;
    PyStatus status;
    FILE *script;
    long script_size;
    char *script_source;
    char message[320];
    const char *runtime_path = "/data/python/runtime/cpython-lib";
    PyObject *main_module;
    PyObject *main_globals;
    PyObject *file_name;
    PyObject *run_result;
    long peak_rss;

    if (script_path == NULL || script_path[0] == '\0') {
        cpython_ps5_notify("CPYTHON PATH FAIL");
        return 2;
    }

    if (options != NULL && options->runtime_path != NULL &&
        options->runtime_path[0] != '\0') {
        runtime_path = options->runtime_path;
    }

    script = fopen(script_path, "rb");
    if (script == NULL) {
        snprintf(message, sizeof message, "CPYTHON OPEN FAIL: %s", script_path);
        cpython_ps5_notify(message);
        return 2;
    }

    PyConfig_InitIsolatedConfig(&config);
    config.install_signal_handlers = 0;
    config.parse_argv = 0;
    config.site_import = 0;
    config.user_site_directory = 0;
    config.use_environment = 0;
    config.buffered_stdio = 0;

#ifdef CPYTHON_PS5
    cpython_ps5_configure_tempdir();
#endif

    PyImport_AppendInittab("_codecs", PyInit__codecs);

    if (append_module_path(&config, runtime_path) != 0 ||
        (options != NULL &&
         append_module_path(&config, options->app_root_path) != 0) ||
        (options != NULL &&
         append_module_path(&config, options->app_lib_path) != 0)) {
        PyConfig_Clear(&config);
        fclose(script);
        cpython_ps5_notify("CPYTHON PATH CONFIG FAIL");
        return 1;
    }

    config.module_search_paths_set = 1;
    status = Py_InitializeFromConfig(&config);
    PyConfig_Clear(&config);
    if (PyStatus_Exception(status)) {
        snprintf(message, sizeof message, "CPYTHON INIT FAIL: %s",
                 status.err_msg ? status.err_msg : "unknown");
        fclose(script);
        cpython_ps5_notify(message);
        return 1;
    }

    if (fseek(script, 0, SEEK_END) != 0 ||
        (script_size = ftell(script)) < 0 ||
        fseek(script, 0, SEEK_SET) != 0 ||
        script_size > 1024 * 1024) {
        fclose(script);
        cpython_ps5_notify("CPYTHON SCRIPT READ FAIL");
        Py_Finalize();
        return 1;
    }

    script_source = (char *)malloc((size_t)script_size + 1);
    if (script_source == NULL ||
        fread(script_source, 1, (size_t)script_size, script) !=
            (size_t)script_size) {
        free(script_source);
        fclose(script);
        cpython_ps5_notify("CPYTHON SCRIPT READ FAIL");
        Py_Finalize();
        return 1;
    }
    script_source[script_size] = '\0';
    fclose(script);

    main_module = PyImport_AddModule("__main__");
    if (main_module == NULL) {
        free(script_source);
        PyErr_Print();
        Py_Finalize();
        cpython_ps5_notify("CPYTHON MAIN MODULE FAIL");
        return 1;
    }
    main_globals = PyModule_GetDict(main_module);
    file_name = PyUnicode_DecodeFSDefault(script_path);
    if (file_name == NULL ||
        PyDict_SetItemString(main_globals, "__file__", file_name) < 0) {
        Py_XDECREF(file_name);
        free(script_source);
        PyErr_Print();
        Py_Finalize();
        cpython_ps5_notify("CPYTHON SCRIPT CONTEXT FAIL");
        return 1;
    }

    run_result = PyRun_StringFlags(script_source, Py_file_input,
                                   main_globals, main_globals, NULL);
    Py_DECREF(file_name);
    free(script_source);
    if (run_result == NULL || PyErr_Occurred()) {
        PyErr_Print();
        Py_XDECREF(run_result);
        Py_Finalize();
        cpython_ps5_notify("CPYTHON SCRIPT FAIL");
        return 1;
    }
    Py_DECREF(run_result);

    Py_Finalize();
    peak_rss = cpython_ps5_peak_rss();
    if (peak_rss >= 0) {
        snprintf(message, sizeof message, "CPYTHON OK: %s RSS=%ld",
                 script_path, peak_rss);
    } else {
        snprintf(message, sizeof message, "CPYTHON OK: %s", script_path);
    }
    cpython_ps5_notify(message);
    return 0;
}
