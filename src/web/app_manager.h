#ifndef CPYTHON_PS5_APP_MANAGER_H
#define CPYTHON_PS5_APP_MANAGER_H

#include <microhttpd.h>
#include <stddef.h>

enum MHD_Result app_list_response(struct MHD_Connection *connection);
enum MHD_Result app_launch_response(struct MHD_Connection *connection, const char *query);
enum MHD_Result app_stop_response(struct MHD_Connection *connection);
int app_manager_status_json(char *body, size_t capacity);
int app_manager_start(unsigned short supervisor_port);
void app_manager_stop(void);

#endif
