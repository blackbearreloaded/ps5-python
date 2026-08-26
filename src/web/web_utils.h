/*
 * PS5-Python - CPython for the PlayStation 5.
 * Copyright (C) 2026 BlackBearReloaded
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef CPYTHON_PS5_WEB_UTILS_H
#define CPYTHON_PS5_WEB_UTILS_H

#include <stddef.h>

int web_send_all(int fd, const void *data, size_t length);
size_t web_json_append(char *out, size_t at, size_t capacity, const char *text);
size_t web_json_append_bytes(char *out, size_t at, size_t capacity, const unsigned char *data,
                             size_t length);

#endif
