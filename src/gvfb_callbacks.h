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

#ifndef _GVFB_CALLBACKS_H_
#define _GVFB_CALLBACKS_H_

#include <glib.h>
#include <gtk/gtk.h>

#include <assert.h>

/* define */
#define ACTION(Action) void gvfb_##Action(GtkAction *action, gpointer data)
#define ACTION_CB(Action) G_CALLBACK(gvfb_##Action)
#define TOGGLEACTION(Action) void gvfb_##Action(GtkToggleAction *toggleaction, gpointer data)
#define RADIOACTION(Action) void gvfb_##Action(GtkRadioAction *radioaction, GtkRadioAction *currentradioaction, gpointer data)

/* Menu CallBack */
/* File Menu */
/* File chooser when save image */
ACTION(on_m_save_image_cb);

/* quit */
ACTION(on_m_quit_cb);

/* View Menu */
/* show gtk cursor */
TOGGLEACTION(on_m_show_gtk_cursor_cb);

/* show Refresh rate setting window */
ACTION(on_m_refresh_rate_cb);

/* fitscreen setting */
TOGGLEACTION(on_m_auto_fit_screen_cb);

/* fullscreen setting */
TOGGLEACTION(on_m_full_screen_cb);

/* Setting the zoom  */
RADIOACTION(on_m_zoom_scale_cb);

/* Help Menu */
/* gvfb about window */
ACTION(on_m_about_cb);

#endif /* end of _GVFB_CALLBACKS_H_ */

