/*
 * PS5-Python - CPython for the PlayStation 5.
 * Copyright (C) 2026 BlackBearReloaded
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef CPYTHON_PS5_LOG_CAPTURE_H
#define CPYTHON_PS5_LOG_CAPTURE_H

int start_log_capture(void);
void log_append(const char *data, size_t length);
void log_reset(void);
void log_clear_broadcast(void);

#endif
