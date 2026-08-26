/*
 * PS5-Python - CPython for the PlayStation 5.
 * Copyright (C) 2026 BlackBearReloaded
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef CPYTHON_PS5_TCP_REPL_H
#define CPYTHON_PS5_TCP_REPL_H

int tcp_repl_start(unsigned short port);
void tcp_repl_stop(void);

#endif
