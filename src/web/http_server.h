/*
 * PS5-Python - CPython for the PlayStation 5.
 * Copyright (C) 2026 BlackBearReloaded
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef CPYTHON_PS5_HTTP_SERVER_H
#define CPYTHON_PS5_HTTP_SERVER_H

#include <microhttpd.h>

enum MHD_Result web_access_handler(void *cls, struct MHD_Connection *connection, const char *url,
                                   const char *method, const char *version, const char *upload_data,
                                   size_t *upload_data_size, void **con_cls);
void web_request_completed(void *cls, struct MHD_Connection *connection, void **con_cls,
                           enum MHD_RequestTerminationCode termination_code);

#endif
