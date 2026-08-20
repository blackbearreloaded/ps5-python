#include "web_state.h"

pthread_mutex_t web_log_mutex = PTHREAD_MUTEX_INITIALIZER;
char web_log_buffer[WEB_LOG_CAPACITY];
size_t web_log_length;
uint64_t web_log_base;
uint64_t web_log_next;
int web_log_pipe[2] = {-1, -1};

pthread_mutex_t web_state_mutex = PTHREAD_MUTEX_INITIALIZER;
int web_launch_started;
int web_app_running;
int web_app_finished;
int web_app_exit_code;
unsigned long web_app_job_id;
long web_app_pid;
char web_app_id[WEB_APP_ID_CAPACITY];
int web_app_state = WEB_APP_IDLE;
volatile sig_atomic_t server_stop;

const char *web_app_state_name(int state)
{
    switch (state)
    {
    case WEB_APP_STARTING:
        return "starting";
    case WEB_APP_RUNNING:
        return "running";
    case WEB_APP_STOPPING:
        return "stopping";
    case WEB_APP_FINISHED:
        return "finished";
    case WEB_APP_FAILED:
        return "failed";
    case WEB_APP_STOPPED:
        return "stopped";
    default:
        return "idle";
    }
}
unsigned short tcp_repl_port;
