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

#ifndef _GVFB_MAIN_H_
#define _GVFB_MAIN_H_

#include <gtk/gtk.h>
#include <gdk/gdk.h>

#include "gvfbhdr.h"

extern GVFBRUNINFO gvfbruninfo;

/*
 * InitRunInfo : Init Run information.
 *
 * Params      : (none)
 *
 * Return      : (none)
 */
void InitRunInfo (void);

/*
 * UnInitRunInfo : UnInit Run information.
 *
 * Params        : (none)
 *
 * Return        : (none)
 */
void UnInitRunInfo (void);

/*
 * EventProc : Handling the incident according to different events.
 *
 * Params    : window        The window to catch events.
 *             event         Gtk event.
 *
 * Return    : TRUE to stop other handlers from being invoked for the event.
 *             FALSE to propagate the event further.
 */
gboolean EventProc (GtkWidget *window, GdkEvent *event);

/*
 * CheckEventThread : check the event from minigui.
 *
 * Params           : void *   params
 *
 * Return           : void *
 */
void *CheckEventThread (void *args);

/*
 * DrawDirtyThread  : draw the dirty rect.
 *
 * Params           : void *   params
 *
 * Return           : void *
 */
void *DrawDirtyThread (void *args);

/*
 * SaveImage : Save image from pixel data.
 * 
 * Params    : filename  Save the image to this filename.
 */
void SaveImage (const char *filename);

/*
 * ScaleImage : Save image from pixel data.
 * 
 * Params     : (x, y)  position.
 *            : (width, height)  width and height.
 */
void ScaleImage (int x, int y, int width, int height);

/*
 * MarkDrawAll : Mark the whole window for dirty region.
 */
void MarkDrawAll (void);

/*
 * Init         : init share memory and init the virtual framebuffer header
 *
 * Params       : ppid
 *              : width
 *              : height
 *              : depth
 *              : color_format
 *
 * Return       :  0  success
 *              : -1  fail
 */
int Init (int ppid, int width, int height, int depth,
        const char *color_format);

/*
 * UnInit      : uninit share memory
 *
 * Params      : none
 *
 * Return      : none
 */
void UnInit();

/*
 * FixDepth   : fix depth
 *
 * Params     : int
 *
 * Return     : int 
 */
int FixDepth (int depth);

/* GetColorFormatIndex : get color format index
 *
 * Params              : depth
 *                     : color_format
 *
 * Return              : int
 */
int GetColorFormatIndex (int depth, const char *color_format);

#endif /* end of _GVFB_MAIN_H_ */

