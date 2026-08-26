/*
 * PS5-Python - CPython for the PlayStation 5.
 * Copyright (C) 2026 BlackBearReloaded
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

fetch('/api/time').then(response=>response.json()).then(data=>{document.querySelector('#time').textContent=new Date(data.unix*1000).toLocaleString()}).catch(()=>{document.querySelector('#time').textContent='unavailable'});
