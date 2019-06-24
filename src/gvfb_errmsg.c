/*
** GVFB - Gtk-based virtual frame buffer
**
** Copyright (C) 2010~2018 Beijing FMSoft Technologies Co., Ltd.
**
** This file is part of GVFB.
**
** This program is free software: you can redistribute it and/or modify
** it under the terms of the GNU General Public License as published by
** the Free Software Foundation, either version 3 of the License, or
** (at your option) any later version.
**
** This program is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
** GNU General Public License for more details.
**
** You should have received a copy of the GNU General Public License
** along with this program.  If not, see <https://www.gnu.org/licenses/>.
*/

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>

#include <assert.h>

#include <glib.h>

#include "gvfb_errmsg.h"
#include "gvfb_log.h"

GtkWidget *msg_dialog;
int has_err = FALSE;
char err_msg[MAX_MSG] = { '\0' };

#ifdef DEBUG

#define LOGFILE "/var/tmp/gvfb.log"

static FILE* log_fp;

void msg_log(const char* fmt, ...)
{
    if (log_fp == NULL) {
        log_fp = fopen (LOGFILE, "a");
    }

    if (log_fp) {
        va_list ap;
        va_start(ap, fmt);
        vfprintf(log_fp, fmt, ap);
        va_end(ap);
        fflush(log_fp);
    }
}

#endif /* DEBUG */

