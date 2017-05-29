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

#endif /* end of _GVFB_LINUX_H_ */

