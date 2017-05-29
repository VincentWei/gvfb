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

#ifndef PATH_MAX
#   define PATH_MAX MAX_PATH
#endif

#define msgcat(_msg,...)                 \
    do {                                 \
        char _msg_ [PATH_MAX];           \
        sprintf (_msg_, __VA_ARGS__);    \
        strcat (_msg, _msg_);            \
    }                                    \
    while (0)

extern GtkWidget *msg_dialog;
extern int  has_err;
extern char err_msg [PATH_MAX];

#endif /* _GVFB_ERRMSG_H_ */

