#include <Python.h>

#include <stdio.h>

#include "cpython_ps5_host.h"

static const char core_script[] = "result = 10 + 32\n"
                                  "assert result == 42\n";

int main(int argc, char **argv)
{
    PyConfig config;
    PyStatus status;
    PyObject *main_module;
    PyObject *globals;
    PyObject *execution;
    PyObject *result;
    char script_path[256];

    if (cpython_ps5_select_script(argc, argv, script_path, sizeof script_path) != 0)
    {
        fprintf(stderr, "CPYTHON_PS5: invalid script path\n");
        return 2;
    }

    PyConfig_InitIsolatedConfig(&config);
    config.install_signal_handlers = 0;
    config.parse_argv = 0;
    config.site_import = 0;
    config.user_site_directory = 0;
    config.use_environment = 0;

    status = Py_InitializeFromConfig(&config);
    PyConfig_Clear(&config);
    if (PyStatus_Exception(status))
    {
        PyStatus rc = status;
        Py_ExitStatusException(rc);
    }

    main_module = PyImport_AddModule("__main__");
    globals = PyModule_GetDict(main_module);
    execution = PyRun_StringFlags(core_script, Py_file_input, globals, globals, NULL);
    Py_XDECREF(execution);

    if (PyErr_Occurred())
    {
        PyErr_Print();
        Py_Finalize();
        return 1;
    }

    result = PyDict_GetItemString(globals, "result");
    if (!PyLong_Check(result) || PyLong_AsLong(result) != 42)
    {
        fprintf(stderr, "CPYTHON_PS5: core result failed\n");
        Py_Finalize();
        return 1;
    }

    printf("CPYTHON_PS5: core result=42 script=%s\n", script_path);
    Py_Finalize();
    return 0;
}
