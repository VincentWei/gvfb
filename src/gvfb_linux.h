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

#ifndef _GVFB_LINUX_H_
#define _GVFB_LINUX_H_

#include <glib.h>

/*
 * SetupSignal : set signal function
 *
 * Params      : (none)
 *
 * Return      : (none)
 */
void SetupSignal (void);

/*
 * InitLock   : init lock
 *
 * Params     : lockkey
 *
 * Return     : 
 */
int InitLock (int lockkey);

/*
 * UnInitLock : uninit lock
 *
 * Params     : shmid
 *
 * Return     :
 */
void UnInitLock (int shmid);

void Lock (void);
void UnLock (void);

/*
 * IsCapslockOn : check is capslock status on
 *
 * Params       : (none)
 *
 * Return       : TRUE    capslock on
 *              : FALSE   capslock off
 */
gboolean IsCapslockOn (void);

/*
 * GVFBGetCurrentTime : get current time
 *
 * Params             : (none)
 *
 * Return             : gulong     ms
 */
gulong GVFBGetCurrentTime (void);

/*
 * CreateShareMemory : create share memory
 *
 * Params            : key
 *                   : data_size
 *
 * Return            :
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

/* socket function */
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
 * Params   : s
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
 * Params   : s
 *          : buf
 *          : len
 *          : flags
 *
 * Return   :
 */
int Recv (int s, unsigned char *buf, int len, unsigned int flags);

#define VRT_RESPONSE            1
    #define VRS_OK                  0
    #define VRS_INV_REQUEST         1
    #define VRS_BAD_OPERATION       2
    #define VRS_OPERATION_FAILED    3
    #define VRS_OPERATION_FINISHED  4

#define VRT_SET_GRAPH_ALPHA     11

#define VRT_OPEN_CAMERA         21
#define VRT_CLOSE_CAMERA        22
#define VRT_SET_ZOOM_LEVEL      23

#define VRT_PLAY_VIDEO          31
#define VRT_SEEK_VIDEO          32
#define VRT_PAUSE_PLAYBACK      33
#define VRT_RESUME_PLAYBACK     34
#define VRT_STOP_PLAYBACK       35

#define VRT_CAPTURE_PHOTO       41
#define VRT_START_RECORD        42
#define VRT_STOP_RECORD         43

struct _vvlc_data_header {
    unsigned int    type;
    unsigned int    param1;
    unsigned int    param2;
    size_t          payload_len;
    char            payload[0];
};

gboolean CheckAsyncOperation (int vvlc_sockfd);

/*
 * HandleVvlcRequest
 *          : Handle a request from Virtual Video Layer client.
 */
gboolean HandleVvlcRequest (int vvlc_sockfd);

#endif /* end of _GVFB_LINUX_H_ */

