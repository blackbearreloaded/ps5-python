#ifndef CPYTHON_PS5_HTTP_RESPONSE_H
#define CPYTHON_PS5_HTTP_RESPONSE_H

#include <microhttpd.h>

enum MHD_Result http_queue_response(struct MHD_Connection *connection, unsigned status,
                                    const char *type, const char *extra, const void *body,
                                    size_t body_length, enum MHD_ResponseMemoryMode mode);
enum MHD_Result http_queue_text(struct MHD_Connection *connection, unsigned status,
                                const char *type, const char *body);
enum MHD_Result http_static_file_response(struct MHD_Connection *connection, const char *path,
                                          const char *type);

#endif
