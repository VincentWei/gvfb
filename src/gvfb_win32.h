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

#ifndef _GVFB_WIN32_H_ 
#define _GVFB_WIN32_H_

#include <stdio.h>
#include <stdlib.h>

#include <windows.h>

#include <gtk/gtk.h>

#define strncasecmp strnicmp

#ifndef MSG_TRUNC
#   define MSG_TRUNC 0
#endif

#ifndef PATH_MAX
#   define PATH_MAX MAX_PATH
#endif

/*
 * InitLock  : init lock
 *
 * Params    : lockkey
 *
 * Return    : 
 */
int InitLock (int lockkey);

/*
 * UnInitLock  : init lock
 *
 * Params      : shmid
 *
 * Return      : 
 */
void UnInitLock (int shmid);

/*
 * Lock      :
 *
 * Params    : (none)
 *
 * Return    : (none)
 */
void Lock();

/*
 * UnLock    :
 *
 * Params    :
 *
 * Return    :
 */
void UnLock();

/*
 * IsCapslockOn : check is capslock status on
 *
 * Params       :
 *
 * Return       :
 */
gboolean IsCapslockOn (void);

/*
 * GVFBGetCurrentTime : get current time
 *
 * Params             : (none)
 *
 * Return             : gulong
 */
gulong GVFBGetCurrentTime (void);

/*
 * CreateShareMemory : create share memory by key
 *
 * Params            : key
 *                   : data_size
 *
 * Return            : address of memory
 */
unsigned char *CreateShareMemory (int key, int data_size);

/*
 * DestroyShareMemory : destroy share memory
 *
 * Params             : (none)
 *
 * Return             : (none)
 */
void DestroyShareMemory (void);

/*
 * ConnectToMiniGUI : connect to minigui
 *
 * Params           : ppid
 *
 * Return           :
 */
int ConnectToMiniGUI (int ppid);

/*
 * Send     : send data
 *
 * Params   : s       sockfd
 *          : buf
 *          : len
 *          : flags
 *
 * Return   :
 */
int Send (int s, const unsigned char *buf, int len, unsigned int flags);

/*
 * Recv     : recv data
 *
 * Params   : s       sockfd
 *          : buf
 *          : len
 *          : flags
 *
 * Return   :
 */
int Recv (int s, unsigned char *buf, int len, unsigned int flags);

#endif /* end of _GVFB_WIN32_H_ */

