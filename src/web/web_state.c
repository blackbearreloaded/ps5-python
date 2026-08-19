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
volatile sig_atomic_t server_stop;
unsigned short tcp_repl_port;
