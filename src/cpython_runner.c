#include <stddef.h>

#include "cpython_ps5_host.h"
#include "cpython_runtime.h"

int
main(int argc, char **argv)
{
    char script_path[256];
    cpython_run_options_t options;

    if (cpython_ps5_select_script(argc, argv, script_path,
                                  sizeof script_path) != 0) {
        cpython_ps5_notify("CPYTHON PATH FAIL");
        return 2;
    }

    options.runtime_path = (argc > 2 && argv[2] != NULL) ? argv[2] : NULL;
    options.app_root_path = (argc > 3 && argv[3] != NULL) ? argv[3] : NULL;
    options.app_lib_path = (argc > 4 && argv[4] != NULL) ? argv[4] : NULL;
    return cpython_ps5_run_file(script_path, &options);
}
