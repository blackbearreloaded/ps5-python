#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "http_response.h"

enum MHD_Result http_queue_response(struct MHD_Connection *connection, unsigned status,
                                    const char *type, const char *extra, const void *body,
                                    size_t body_length, enum MHD_ResponseMemoryMode mode)
{
    struct MHD_Response *response =
        MHD_create_response_from_buffer(body_length, (void *)body, mode);
    if (response == NULL)
    {
        if (mode == MHD_RESPMEM_MUST_FREE)
            free((void *)body);
        return MHD_NO;
    }
    MHD_add_response_header(response, "Content-Type", type);
    MHD_add_response_header(response, "Access-Control-Allow-Origin", "*");
    if (extra != NULL)
        MHD_add_response_header(response, "X-Log-Next", extra);
    enum MHD_Result result = MHD_queue_response(connection, status, response);
    MHD_destroy_response(response);
    return result;
}

enum MHD_Result http_queue_text(struct MHD_Connection *connection, unsigned status,
                                const char *type, const char *body)
{
    return http_queue_response(connection, status, type, NULL, body, strlen(body),
                               MHD_RESPMEM_MUST_COPY);
}

enum MHD_Result http_static_file_response(struct MHD_Connection *connection, const char *path,
                                          const char *type)
{
    FILE *file = fopen(path, "rb");
    if (file == NULL)
        return http_queue_text(connection, MHD_HTTP_NOT_FOUND, "text/plain", "file not found\n");
    if (fseek(file, 0, SEEK_END) != 0)
    {
        fclose(file);
        return http_queue_text(connection, MHD_HTTP_INTERNAL_SERVER_ERROR, "text/plain",
                               "file read failed\n");
    }
    long file_size = ftell(file);
    if (file_size < 0 || file_size > 1024 * 1024 || fseek(file, 0, SEEK_SET) != 0)
    {
        fclose(file);
        return http_queue_text(connection, MHD_HTTP_INTERNAL_SERVER_ERROR, "text/plain",
                               "file read failed\n");
    }
    size_t body_length = (size_t)file_size;
    char *body = malloc(body_length ? body_length : 1);
    if (body == NULL || fread(body, 1, body_length, file) != body_length)
    {
        free(body);
        fclose(file);
        return http_queue_text(connection, MHD_HTTP_INTERNAL_SERVER_ERROR, "text/plain",
                               "file read failed\n");
    }
    fclose(file);
    return http_queue_response(connection, MHD_HTTP_OK, type, NULL, body, body_length,
                               MHD_RESPMEM_MUST_FREE);
}
