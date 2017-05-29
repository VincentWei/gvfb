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

