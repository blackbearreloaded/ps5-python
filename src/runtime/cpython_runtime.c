#include <Python.h>

#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef CPYTHON_PS5
#include <sys/resource.h>
#endif

#include "cpython_ps5_host.h"
#include "cpython_runtime.h"

PyMODINIT_FUNC PyInit__codecs(void);
PyMODINIT_FUNC PyInit__cpython_ps5_control(void);

static pthread_mutex_t runtime_mutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_mutex_t runtime_job_mutex = PTHREAD_MUTEX_INITIALIZER;
static int runtime_active;
static PyInterpreterState *runtime_interpreter;
static PyThreadState *runtime_main_state;
static int runtime_job_active;
static int runtime_job_stop_requested;
static int codecs_registered;
static int control_module_registered;

static PyObject *cpython_ps5_exit(PyObject *self, PyObject *args)
{
    PyObject *value = Py_None;

    (void)self;
    if (PyTuple_GET_SIZE(args) > 0)
        value = PyTuple_GET_ITEM(args, 0);
    PyErr_SetObject(PyExc_SystemExit, value);
    return NULL;
}

static PyMethodDef cpython_ps5_exit_method = {
    "exit", cpython_ps5_exit, METH_VARARGS, "Restart the embedded interpreter."
};

static long cpython_ps5_peak_rss(void)
{
#ifdef CPYTHON_PS5
    struct rusage usage;

    if (getrusage(RUSAGE_SELF, &usage) == 0)
        return usage.ru_maxrss;
#endif
    return -1;
}

#ifdef CPYTHON_PS5
static void cpython_ps5_configure_tempdir(void)
{
    /* /user/temp is a PS5-managed writable directory cleaned on restart. */
    const char *tmpdir = getenv("TMPDIR");

    if (tmpdir == NULL || tmpdir[0] == '\0')
        (void)setenv("TMPDIR", "/user/temp", 0);
}
#endif

static int append_module_path(PyConfig *config, const char *path)
{
    PyStatus status;
    wchar_t *path_wide;

    if (path == NULL || path[0] == '\0')
        return 0;

    path_wide = Py_DecodeLocale(path, NULL);
    if (path_wide == NULL)
        return -1;
    status = PyWideStringList_Append(&config->module_search_paths, path_wide);
    PyMem_RawFree(path_wide);
    return PyStatus_Exception(status) ? -1 : 0;
}

static int append_runtime_path(const char *path)
{
    PyObject *sys_path;
    PyObject *path_object;
    int result;

    if (path == NULL || path[0] == '\0')
        return 0;
    sys_path = PySys_GetObject("path");
    if (sys_path == NULL)
        return -1;
    path_object = PyUnicode_DecodeFSDefault(path);
    if (path_object == NULL)
        return -1;
    result = PyList_Append(sys_path, path_object);
    Py_DECREF(path_object);
    return result;
}

static int set_sys_argv(const char *script_path, const cpython_run_options_t *options)
{
    size_t argument_count = options == NULL ? 0 : options->argc;
    PyObject *argv = PyList_New(argument_count + 1);
    if (argv == NULL)
        return -1;
    for (size_t index = 0; index <= argument_count; index++)
    {
        const char *value = index == 0 ? script_path : options->argv[index - 1];
        PyObject *item = PyUnicode_DecodeFSDefault(value == NULL ? "" : value);
        if (item == NULL || PyList_SetItem(argv, (Py_ssize_t)index, item) != 0)
        {
            Py_XDECREF(item);
            Py_DECREF(argv);
            return -1;
        }
    }
    if (PySys_SetObject("argv", argv) != 0)
    {
        Py_DECREF(argv);
        return -1;
    }
    Py_DECREF(argv);
    return 0;
}

static int install_exit_helpers(void)
{
    PyObject *builtins = PyEval_GetBuiltins();
    PyObject *exit_function;

    if (builtins == NULL)
        return -1;
    exit_function = PyCFunction_NewEx(&cpython_ps5_exit_method, NULL, NULL);
    if (exit_function == NULL)
        return -1;
    if (PyDict_SetItemString(builtins, "exit", exit_function) != 0 ||
        PyDict_SetItemString(builtins, "quit", exit_function) != 0)
    {
        Py_DECREF(exit_function);
        return -1;
    }
    Py_DECREF(exit_function);
    return 0;
}

static int runtime_initialize(const cpython_run_options_t *options)
{
    PyConfig config;
    PyStatus status;
    const char *runtime_path = "/data/python/runtime/cpython-lib";

    if (options != NULL && options->runtime_path != NULL && options->runtime_path[0] != '\0')
        runtime_path = options->runtime_path;

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
    if (!codecs_registered)
    {
        if (PyImport_AppendInittab("_codecs", PyInit__codecs) != 0)
            return -1;
        codecs_registered = 1;
    }
    if (!control_module_registered)
    {
        if (PyImport_AppendInittab("_cpython_ps5_control", PyInit__cpython_ps5_control) != 0)
            return -1;
        control_module_registered = 1;
    }
    if (append_module_path(&config, runtime_path) != 0)
    {
        PyConfig_Clear(&config);
        return -1;
    }
    config.module_search_paths_set = 1;
    status = Py_InitializeFromConfig(&config);
    PyConfig_Clear(&config);
    if (PyStatus_Exception(status))
        return -1;
    runtime_interpreter = PyInterpreterState_Get();
    if (runtime_interpreter == NULL)
    {
        Py_Finalize();
        return -1;
    }
    if (install_exit_helpers() != 0)
    {
        PyErr_PrintEx(0);
        runtime_interpreter = NULL;
        Py_Finalize();
        return -1;
    }
    runtime_main_state = PyEval_SaveThread();
    runtime_active = 1;
    return 0;
}

int cpython_ps5_runtime_start(const cpython_run_options_t *options)
{
    int result = 0;

    pthread_mutex_lock(&runtime_mutex);
    if (!runtime_active && runtime_initialize(options) != 0)
        result = -1;
    pthread_mutex_unlock(&runtime_mutex);
    return result;
}

static void runtime_finalize_locked(void)
{
    if (runtime_active)
    {
        PyEval_RestoreThread(runtime_main_state);
        runtime_main_state = NULL;
        runtime_interpreter = NULL;
        runtime_active = 0;
        Py_Finalize();
    }
}

int cpython_ps5_runtime_reset(const cpython_run_options_t *options)
{
    int result;

    pthread_mutex_lock(&runtime_mutex);
    runtime_finalize_locked();
    result = runtime_initialize(options);
    pthread_mutex_unlock(&runtime_mutex);
    return result;
}

void cpython_ps5_runtime_stop(void)
{
    pthread_mutex_lock(&runtime_mutex);
    runtime_finalize_locked();
    pthread_mutex_unlock(&runtime_mutex);
}

static PyThreadState *runtime_attach_thread(void)
{
    PyThreadState *thread_state;

    thread_state = PyThreadState_New(runtime_interpreter);
    if (thread_state == NULL)
        return NULL;
    PyThreadState_Swap(thread_state);
    return thread_state;
}

static void runtime_detach_thread(PyThreadState *thread_state)
{
    PyThreadState_Clear(thread_state);
    PyThreadState_DeleteCurrent();
}

static void runtime_job_start(PyThreadState *thread_state)
{
    (void)thread_state;
    pthread_mutex_lock(&runtime_job_mutex);
    runtime_job_active = 1;
    pthread_mutex_unlock(&runtime_job_mutex);
}

static int runtime_job_should_stop(void)
{
    int requested;

    pthread_mutex_lock(&runtime_job_mutex);
    requested = runtime_job_stop_requested;
    pthread_mutex_unlock(&runtime_job_mutex);
    return requested;
}

static void runtime_job_end(void)
{
    pthread_mutex_lock(&runtime_job_mutex);
    runtime_job_active = 0;
    runtime_job_stop_requested = 0;
    pthread_mutex_unlock(&runtime_job_mutex);
}

static PyObject *cpython_ps5_stop_requested(PyObject *self, PyObject *args)
{
    (void)self;
    (void)args;
    if (runtime_job_should_stop())
        Py_RETURN_TRUE;
    Py_RETURN_FALSE;
}

static PyMethodDef cpython_ps5_control_methods[] = {
    {"stop_requested", cpython_ps5_stop_requested, METH_NOARGS,
     "Return whether the current app has received a stop request."},
    {NULL, NULL, 0, NULL},
};

static struct PyModuleDef cpython_ps5_control_module = {
    PyModuleDef_HEAD_INIT,
    "_cpython_ps5_control",
    NULL,
    -1,
    cpython_ps5_control_methods,
};

PyMODINIT_FUNC PyInit__cpython_ps5_control(void)
{
    return PyModule_Create(&cpython_ps5_control_module);
}

int cpython_ps5_runtime_request_stop(void)
{
    int active;

    pthread_mutex_lock(&runtime_job_mutex);
    runtime_job_stop_requested = 1;
    active = runtime_job_active;
    pthread_mutex_unlock(&runtime_job_mutex);
    if (!active)
        return 1;
    return 0;
}

static int runtime_is_active(void)
{
    int active;

    pthread_mutex_lock(&runtime_mutex);
    active = runtime_active;
    pthread_mutex_unlock(&runtime_mutex);
    return active;
}

static int copy_unicode_output(PyObject *value, char *output, size_t output_size)
{
    const char *text;
    Py_ssize_t length;

    if (output_size == 0 || value == NULL)
        return -1;
    text = PyUnicode_AsUTF8AndSize(value, &length);
    if (text == NULL)
        return -1;
    if ((size_t)length >= output_size)
        length = (Py_ssize_t)output_size - 1;
    memcpy(output, text, (size_t)length);
    output[length] = '\0';
    return 0;
}

static int evaluate_source_locked(const char *source, char *output, size_t output_size)
{
    PyThreadState *thread_state;
    PyObject *io = NULL;
    PyObject *capture = NULL;
    PyObject *previous_stdout = NULL;
    PyObject *previous_stderr = NULL;
    PyObject *main_module;
    PyObject *globals;
    PyObject *result = NULL;
    PyObject *text = NULL;
    PyObject *repr = NULL;
    PyObject *write_result = NULL;
    PyObject *newline_result = NULL;
    char *source_text = NULL;
    size_t source_length;
    int single_line;
    int failed = 0;
    int restart_requested = 0;

    if (!runtime_active || source == NULL || output == NULL || output_size == 0)
        return -1;
    thread_state = runtime_attach_thread();
    if (thread_state == NULL)
        return -1;
    output[0] = '\0';
    source_length = strlen(source);
    if (strspn(source, " \t\r\n") == source_length)
    {
        runtime_detach_thread(thread_state);
        return 0;
    }
    source_text = malloc(source_length + 2);
    if (source_text == NULL)
    {
        runtime_detach_thread(thread_state);
        return -1;
    }
    memcpy(source_text, source, source_length);
    if (source_length == 0 || source_text[source_length - 1] != '\n')
        source_text[source_length++] = '\n';
    source_text[source_length] = '\0';
    single_line = strchr(source, '\n') == NULL || strchr(source, '\n')[1] == '\0';
    main_module = PyImport_AddModule("__main__");
    globals = main_module ? PyModule_GetDict(main_module) : NULL;
    io = PyImport_ImportModule("io");
    capture = io ? PyObject_CallMethod(io, "StringIO", NULL) : NULL;
    previous_stdout = PySys_GetObject("stdout");
    previous_stderr = PySys_GetObject("stderr");
    Py_XINCREF(previous_stdout);
    Py_XINCREF(previous_stderr);
    if (main_module == NULL || globals == NULL || capture == NULL || previous_stdout == NULL ||
        previous_stderr == NULL)
    {
        PyErr_PrintEx(0);
        failed = 1;
        goto restore;
    }
    PySys_SetObject("stdout", capture);
    PySys_SetObject("stderr", capture);
    if (single_line)
    {
        result = PyRun_StringFlags(source_text, Py_eval_input, globals, globals, NULL);
        if (result == NULL && PyErr_ExceptionMatches(PyExc_SyntaxError))
        {
            PyErr_Clear();
            result = PyRun_StringFlags(source_text, Py_single_input, globals, globals, NULL);
        }
    }
    else
    {
        result = PyRun_StringFlags(source_text, Py_file_input, globals, globals, NULL);
    }
    if (result == NULL)
    {
        if (PyErr_ExceptionMatches(PyExc_SystemExit))
        {
            PyErr_Clear();
            restart_requested = 1;
            write_result = PyObject_CallMethod(capture, "write", "s",
                                               "Interpreter restarted\n");
            if (write_result == NULL)
                PyErr_Clear();
        }
        else
        {
            PyErr_PrintEx(0);
        }
        failed = 1;
    }
    else if (single_line && result != Py_None)
    {
        repr = PyObject_Repr(result);
        if (repr == NULL || capture == NULL)
        {
            PyErr_PrintEx(0);
            failed = 1;
        }
        else
        {
            write_result = PyObject_CallMethod(capture, "write", "O", repr);
            if (write_result == NULL)
                failed = 1;
            else
                newline_result = PyObject_CallMethod(capture, "write", "s", "\n");
            if (newline_result == NULL)
            {
                PyErr_PrintEx(0);
                failed = 1;
            }
        }
    }
    Py_XDECREF(newline_result);
    Py_XDECREF(write_result);
    Py_XDECREF(repr);
    Py_XDECREF(result);

restore:
    if (previous_stdout != NULL)
        PySys_SetObject("stdout", previous_stdout);
    if (previous_stderr != NULL)
        PySys_SetObject("stderr", previous_stderr);
    if (capture != NULL)
        text = PyObject_CallMethod(capture, "getvalue", NULL);
    if (text != NULL)
        copy_unicode_output(text, output, output_size);
    else if (!failed)
        failed = 1;
    Py_XDECREF(text);
    Py_XDECREF(previous_stdout);
    Py_XDECREF(previous_stderr);
    Py_XDECREF(capture);
    Py_XDECREF(io);
    free(source_text);
    runtime_detach_thread(thread_state);
    return restart_requested ? CPYTHON_PS5_RUNTIME_RESTARTED : failed;
}

int cpython_ps5_runtime_eval(const char *source, char *output, size_t output_size)
{
    int result;

    pthread_mutex_lock(&runtime_mutex);
    result = evaluate_source_locked(source, output, output_size);
    if (result == CPYTHON_PS5_RUNTIME_RESTARTED)
    {
        runtime_finalize_locked();
        if (runtime_initialize(NULL) != 0)
        {
            result = -1;
            if (output_size > 0)
                (void)snprintf(output, output_size, "Interpreter restart failed\n");
        }
    }
    pthread_mutex_unlock(&runtime_mutex);
    return result;
}

int cpython_ps5_run_file(const char *script_path, const cpython_run_options_t *options)
{
    FILE *script;
    long script_size;
    char *script_source;
    char message[320];
    PyThreadState *thread_state;
    PyObject *main_module;
    PyObject *main_globals = NULL;
    PyObject *file_name;
    PyObject *run_result;
    long peak_rss;
    int persistent;
    int job_started = 0;
    int result = 0;

    if (script_path == NULL || script_path[0] == '\0')
    {
        cpython_ps5_notify("CPYTHON PATH FAIL");
        return 2;
    }
    script = fopen(script_path, "rb");
    if (script == NULL)
    {
        snprintf(message, sizeof message, "CPYTHON OPEN FAIL: %s", script_path);
        cpython_ps5_notify(message);
        return 2;
    }
    if (fseek(script, 0, SEEK_END) != 0 || (script_size = ftell(script)) < 0 ||
        fseek(script, 0, SEEK_SET) != 0 || script_size > 1024 * 1024)
    {
        fclose(script);
        cpython_ps5_notify("CPYTHON SCRIPT READ FAIL");
        return 1;
    }
    script_source = (char *)malloc((size_t)script_size + 1);
    if (script_source == NULL ||
        fread(script_source, 1, (size_t)script_size, script) != (size_t)script_size)
    {
        free(script_source);
        fclose(script);
        cpython_ps5_notify("CPYTHON SCRIPT READ FAIL");
        return 1;
    }
    script_source[script_size] = '\0';
    fclose(script);

    persistent = runtime_is_active();
    if (!persistent && cpython_ps5_runtime_start(options) != 0)
    {
        free(script_source);
        cpython_ps5_notify("CPYTHON INIT FAIL");
        return 1;
    }
    pthread_mutex_lock(&runtime_mutex);
    thread_state = runtime_attach_thread();
    if (thread_state == NULL)
    {
        pthread_mutex_unlock(&runtime_mutex);
        free(script_source);
        if (!persistent)
            cpython_ps5_runtime_stop();
        cpython_ps5_notify("CPYTHON THREAD STATE FAIL");
        return 1;
    }
    runtime_job_start(thread_state);
    job_started = 1;
    if (options != NULL && (append_runtime_path(options->app_root_path) != 0 ||
                            append_runtime_path(options->app_lib_path) != 0))
    {
        PyErr_PrintEx(0);
        result = 1;
        goto done;
    }
    main_module = PyImport_AddModule("__main__");
    main_globals = main_module ? PyModule_GetDict(main_module) : NULL;
    file_name = PyUnicode_DecodeFSDefault(script_path);
    if (main_globals == NULL || file_name == NULL ||
        PyDict_SetItemString(main_globals, "__file__", file_name) < 0)
    {
        PyErr_PrintEx(0);
        Py_XDECREF(file_name);
        result = 1;
        goto done;
    }
    if (options != NULL && options->argc > 0 && options->argv == NULL)
    {
        PyErr_SetString(PyExc_ValueError, "invalid application arguments");
        PyErr_PrintEx(0);
        result = 1;
        goto done;
    }
    if (set_sys_argv(script_path, options) != 0)
    {
        PyErr_PrintEx(0);
        result = 1;
        goto done;
    }
    if (runtime_job_should_stop())
    {
        PyErr_SetNone(PyExc_KeyboardInterrupt);
        run_result = NULL;
    }
    else
        run_result = PyRun_StringFlags(script_source, Py_file_input, main_globals, main_globals, NULL);
    Py_DECREF(file_name);
    if (run_result == NULL || PyErr_Occurred())
    {
        if (runtime_job_should_stop() && PyErr_ExceptionMatches(PyExc_KeyboardInterrupt))
        {
            PyErr_Clear();
            result = CPYTHON_PS5_RUNTIME_STOPPED;
        }
        else
        {
            PyErr_PrintEx(0);
            result = 1;
        }
        Py_XDECREF(run_result);
        goto done;
    }
    Py_DECREF(run_result);

done:
    if (job_started)
    {
        runtime_job_end();
    }
    runtime_detach_thread(thread_state);
    pthread_mutex_unlock(&runtime_mutex);
    free(script_source);
    if (!persistent)
        cpython_ps5_runtime_stop();
    if (result == CPYTHON_PS5_RUNTIME_STOPPED)
    {
        cpython_ps5_notify("CPYTHON STOPPED");
        return result;
    }
    if (result != 0)
    {
        cpython_ps5_notify("CPYTHON SCRIPT FAIL");
        return result;
    }
    peak_rss = cpython_ps5_peak_rss();
    if (peak_rss >= 0)
        snprintf(message, sizeof message, "CPYTHON OK: %s RSS=%ld", script_path, peak_rss);
    else
        snprintf(message, sizeof message, "CPYTHON OK: %s", script_path);
    cpython_ps5_notify(message);
    return 0;
}
