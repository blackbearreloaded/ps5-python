#ifndef CPYTHON_PS5_WEB_STATE_H
#define CPYTHON_PS5_WEB_STATE_H

#include <pthread.h>
#include <signal.h>
#include <stddef.h>
#include <stdint.h>

#define WEB_LOG_CAPACITY 65536
#define WEB_APP_ID_CAPACITY 128

enum web_app_state
{
    WEB_APP_IDLE = 0,
    WEB_APP_STARTING,
    WEB_APP_RUNNING,
    WEB_APP_STOPPING,
    WEB_APP_FINISHED,
    WEB_APP_FAILED,
    WEB_APP_STOPPED
};

extern pthread_mutex_t web_log_mutex;
extern char web_log_buffer[WEB_LOG_CAPACITY];
extern size_t web_log_length;
extern uint64_t web_log_base;
extern uint64_t web_log_next;
extern int web_log_pipe[2];

extern pthread_mutex_t web_state_mutex;
extern int web_launch_started;
extern int web_app_running;
extern int web_app_finished;
extern int web_app_exit_code;
extern unsigned long web_app_job_id;
extern long web_app_pid;
extern char web_app_id[WEB_APP_ID_CAPACITY];
extern int web_app_state;
const char *web_app_state_name(int state);
extern volatile sig_atomic_t server_stop;
extern unsigned short tcp_repl_port;

#endif
