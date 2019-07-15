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

#ifndef _SKIN_H_
#define _SKIN_H_

#include <gtk/gtk.h>

#include "deviceskin.h"

/* struct of gvfb skin */
typedef struct _GVFBSKIN{
    SKIN *pskin;

    /* sizeof skin */
    int skin_width;
    int skin_height;

    GdkPixbuf *pixbufs[SKIN_FACE_NUM];
    GtkWidget *skin_widget;

    SKINKEYITEM *pressed_item;
} GVFBSKIN;

extern GVFBSKIN *g_gvfbskin;

/*
 * CreateSkinWnd   :
 *
 * Params          : width
 *                 : height
 *
 * Return          : widget of skin window
 */
GtkWidget *CreateSkinWnd (int width, int height);

/*
 * InitSkin   :
 *
 * Params     : skinfile
 *
 * Return     :  0 success
 *            : -1 fail
 */
int InitSkin (const char *skinfile);

/*
 * UnInitSkin   :
 *
 * Params       : (none)
 *
 * Return       : (none)
 */
void UnInitSkin ();

/*
 * GetSkinSize   :
 *
 * Params        : *width
 *               : *height
 *
 * Return        : (none)
 */
void GetSkinSize (int *width, int *height);

/*
 * GetSkinSize   :
 *
 * Params        : point to screenrect
 *
 * Return        :
 */
void GetSkinScreenRect (SKINRECT *screenrect);

#endif /* end of _SKIN_H_ */

