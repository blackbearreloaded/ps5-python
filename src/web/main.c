#include <microhttpd.h>
#include <signal.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "cpython_runtime.h"
#include "http_server.h"
#include "log_capture.h"
#include "tcp_repl.h"
#include "web_state.h"

#define DEFAULT_PORT 8090

int main(int argc, char **argv)
{
    unsigned long port = DEFAULT_PORT;
    unsigned long repl_port = 0;
    struct MHD_Daemon *daemon;
    cpython_run_options_t runtime_options;

    if (argc > 1)
        port = strtoul(argv[1], NULL, 10);
    if (argc > 2)
        repl_port = strtoul(argv[2], NULL, 10);
    else if (port < 65535)
        repl_port = port + 1;
    if (port == 0 || port > 65535 || repl_port == 0 || repl_port > 65535 || repl_port == port)
        return 2;

    tcp_repl_port = (unsigned short)repl_port;
    runtime_options.runtime_path = "/data/python/runtime/cpython-lib";
    runtime_options.app_root_path = NULL;
    runtime_options.app_lib_path = NULL;
    if (cpython_ps5_runtime_start(&runtime_options) != 0)
        return 1;
    signal(SIGPIPE, SIG_IGN);
    if (start_log_capture() != 0)
    {
        cpython_ps5_runtime_stop();
        return 1;
    }
    if (tcp_repl_start(tcp_repl_port) != 0)
    {
        cpython_ps5_runtime_stop();
        return 1;
    }
    daemon = MHD_start_daemon(MHD_USE_INTERNAL_POLLING_THREAD | MHD_USE_THREAD_PER_CONNECTION |
                                  MHD_USE_ITC | MHD_ALLOW_UPGRADE,
                              (uint16_t)port, NULL, NULL, web_access_handler, NULL, MHD_OPTION_END);
    if (daemon == NULL)
    {
        tcp_repl_stop();
        cpython_ps5_runtime_stop();
        return 1;
    }
    printf("[launcher] started\n");
    printf("[launcher] HTTP endpoint: http://<PS5-IP>:%lu/\n", port);
    printf("[launcher] TCP REPL endpoint: tcp://<PS5-IP>:%u\n", (unsigned)tcp_repl_port);
    fflush(stdout);
    while (!server_stop)
        sleep(1);
    MHD_stop_daemon(daemon);
    tcp_repl_stop();
    cpython_ps5_runtime_stop();
    return 0;
}
