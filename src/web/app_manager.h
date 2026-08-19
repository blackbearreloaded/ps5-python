#ifndef CPYTHON_PS5_APP_MANAGER_H
#define CPYTHON_PS5_APP_MANAGER_H

#include <microhttpd.h>

enum MHD_Result app_list_response(struct MHD_Connection *connection);
enum MHD_Result app_launch_response(struct MHD_Connection *connection, const char *query);

#endif
