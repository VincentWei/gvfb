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

#ifndef _DEVICESKIN_H_
#define _DEVICESKIN_H_

#include <limits.h>

#ifdef WIN32
#pragma warning(disable:4996)
#   include <windows.h>
#endif

#include <gtk/gtk.h>

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#ifndef PATH_MAX
#   define PATH_MAX MAX_PATH
#endif

#define POLYGON_MAX_POINTS 64

#define SKIN_BUTTON_UP    0
#define SKIN_BUTTON_DOWN  1

/* points */
typedef struct _SKINPOINT {
    int x;
    int y;
} SKINPOINT;

/* skin rect */
typedef struct _SKINRECT {
    int left;
    int top;
    int right;
    int bottom;
} SKINRECT;

/* SKINSCREENFACE */
enum SKINSCREENFACE {
    SKIN_UP_FACE     = 0,
    SKIN_DOWN_FACE   = 1,
    SKIN_CLOSE_FACE  = 2,
    SKIN_FACE_NUM
};

/* SKINKEYITEM */
typedef struct _SKINKEYITEM {
    char name [PATH_MAX];
    unsigned int keycode;

    struct {
        SKINPOINT *points;
        int npoint;
    } polygon;

    int state;
    void *region;
} SKINKEYITEM;

/* skin */
typedef struct _SKIN {
    char images [SKIN_FACE_NUM][PATH_MAX];  /* skin images file name */

    SKINRECT screenrect;                    /* screen rect */

    int skinitem_num;                       /* key item number */

    SKINKEYITEM *skinkeyitem; 
} SKIN;

/*
 * SkinLoad    : load skin from file
 *
 * Params      : skinfile  skin file name with full path
 *
 * Return      : struct of SKIN
 */
SKIN *SkinLoad (const char *filename);

/* 
 * SkinUnLoad  : unload skin
 *
 * Params      : skin   point of SKIN struct
 *
 * Return      : void
 */
void  SkinUnload (SKIN *skin);

/* 
 * SkinPointToSkinItem  : unload skin
 *
 * Params               : skin   point of SKIN struct
 *                      : pt     point
 *
 * Return               : SKINKEYITEM 
 */
SKINKEYITEM *SkinPointToSkinItem (SKIN *skin, const SKINPOINT *pt);

/* 
 * SkinKeyCodeToSkinItem  : unload skin
 *
 * Params                 : skin     point of SKIN struct
 *                        : keycode
 *
 * Return                 : SKINKEYITEM 
 */
SKINKEYITEM *SkinKeyCodeToSkinItem (SKIN *skin, int keycode);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* end of _DEVICESKIN_H_ */
