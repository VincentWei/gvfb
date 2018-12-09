/*
** $Id: gvfb_view.h 261 2010-12-30 07:37:54Z xbwang $
** 
** gvfbview.h: for gvfb display event.
**
** Copyright (C) 2009 Feynman Software.
**
** All rights reserved by Feynman Software.
**
** Create data: 2009-12-17
 */

#ifndef _GVFB_VIEW_H_
#define _GVFB_VIEW_H_

#include <gtk/gtk.h>
#include <gdk/gdk.h>

#include "gvfbhdr.h"

/*
 * CreateGVFBWindow : Create a GVFB window.
 *
 * Params           : width             Window width.
 *                  : height            Window height.
 *                  : depth             Color depth.
 *                  : caption           Caption for gvfb.
 *
 * Return           : Return the window.
 */
GtkWidget *CreateGVFBWindow (gint width, gint height, gint depth, const char *caption);

/*
 * InitMenu : init menu
 *
 * Params   : (none)
 *
 * Return   : (none)
 */
void InitMenu();

/*
 * CreateMainWnd : Create a main window.
 *
 * Params        : width             Window width.
 *               : height            Window height.
 *
 * Return        : Return the window.
 */
GtkWidget *CreateMainWnd (gint width, gint height);

/*
 * CreateScrollWnd : Create a Scrolled window.
 *
 * Params           : width             Window width.
 *                  : height            Window height.
 *
 * Return           : Return the window.
 */
GtkWidget *CreateScrollWnd (gint width, gint height);

/*
 * CreateDrawArea : Create a Draw Area.
 *
 * Params         : width             Window width.
 *                : height            Window height.
 *
 * Return         : Return the window.
 */
GtkWidget* CreateDrawArea (gint width, gint height);

/*
 * CreateIMContext : Create IM Context.
 *
 * Params          : window.
 *
 * Return          : Return the window.
 */
GtkIMContext *CreateIMContext (GtkWidget *window);

/*
 * DrawImage : Draw image and show it.
 *
 * Params    : x        X coordinate for image to draw.
 *             y        Y coordinate for image to draw.
 *             width    The width of image.
 *             height   The height of image.
 *
 * Return    : (none)
 */
void DrawImage (int x, int y, int width, int height);

/*
 * SetCaption : Set window caption.
 *
 * Params     : caption   Caption.
 *
 * Return     : (none)
 */
void SetCaption (const char *caption);

/*
 * ShowHide : minimize or recover window.
 *
 * Params   : bshow   0 to hide window, others to show window.
 *
 * Return   : (none)
 */
void ShowHide (int bshow);

/*
 * GetDrawRect : get draw area rect
 *
 * Params      : drawrect.
 *
 * Return      : (none)
 */
gboolean GetDrawRect (GVFBRECT *drawrect);

/*
 * FullScreen : fullscreen or unfullscreen show window.
 *
 * Params     : (none)
 *
 * Return     : (none)
 */
void FullScreen (void);

/*
 * ZoomScale  : zoom_percent Scale.
 *
 * Params     : zoom
 *
 * Return     : (none)
 */
void ZoomScale (int zoom);

/*
 * FitScnRect : FitScnRect.
 *
 * Params     : width
 *            : height
 *            : fit_flag
 *
 * Return     : (none)
 */
gboolean FitScnRect (int width, int height, int fit_flag);

/*
 * SetMouseXY : Set the mouse position.
 *
 * Params     : x   The x coordinate of mouse.
 *            : y   The y coordinate of mouse.
 *
 * Return     : (none)
 */
void SetMouseXY (int x, int y);

#endif /* end of _GVFB_VIEW_H_ */

