/*
** $Id: gvfb_main.h 251 2010-12-17 01:37:00Z xbwang $
**
** gvfb.h: for gvfb.c.
**
** Copyright (C) 2009 Feynman Software.
**
** All rights reserved by Feynman Software.
**
** Create data: 2009-12-18
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

