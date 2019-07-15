///////////////////////////////////////////////////////////////////////////////
//
//                          IMPORTANT NOTICE
//
// The following open source license statement does not apply to any
// entity in the Exception List published by FMSoft.
//
// For more information, please visit:
//
// https://www.fmsoft.cn/exception-list
//
//////////////////////////////////////////////////////////////////////////////
/*
** GVFB - Gtk-based virtual frame buffer
**
** Copyright (C) 2009~2018 Beijing FMSoft Technologies Co., Ltd.
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

#ifndef _GVFB_ERRMSG_H_
#define _GVFB_ERRMSG_H_

#include <stdio.h>
#include <stdlib.h>

#include <gtk/gtk.h>
#include <gdk/gdk.h>

#ifdef WIN32
#pragma warning(disable:4996)
#   include <windows.h>
#endif

#define MAX_MSG     5120

#define msgcat(_msg,...)                 \
    do {                                 \
        char _msg_ [MAX_MSG];           \
        sprintf (_msg_, __VA_ARGS__);    \
        strcat (_msg, _msg_);            \
    }                                    \
    while (0)

extern GtkWidget *msg_dialog;
extern int  has_err;
extern char err_msg [MAX_MSG];

#endif /* _GVFB_ERRMSG_H_ */

