#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>

#include <assert.h>

#include <glib.h>

#include "gvfb_errmsg.h"

GtkWidget *msg_dialog;
int has_err = FALSE;
char err_msg[PATH_MAX] = { '\0' };
