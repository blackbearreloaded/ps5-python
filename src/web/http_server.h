#ifndef CPYTHON_PS5_HTTP_SERVER_H
#define CPYTHON_PS5_HTTP_SERVER_H

#include <microhttpd.h>

enum MHD_Result web_access_handler(void *cls, struct MHD_Connection *connection, const char *url,
                                   const char *method, const char *version, const char *upload_data,
                                   size_t *upload_data_size, void **con_cls);

#endif
