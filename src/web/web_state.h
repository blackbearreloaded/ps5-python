#ifndef CPYTHON_PS5_WEB_STATE_H
#define CPYTHON_PS5_WEB_STATE_H

#include <pthread.h>
#include <signal.h>
#include <stddef.h>
#include <stdint.h>

#define WEB_LOG_CAPACITY 65536

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
extern volatile sig_atomic_t server_stop;
extern unsigned short tcp_repl_port;

#endif
