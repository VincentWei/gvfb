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
    int             type;
    unsigned int    param1;
    unsigned int    param2;
    size_t          payload_len;
    char            payload[0];
};

/*
 * HandleVvlcRequest
 *          : Handle a request from Virtual Video Layer client.
 */
gboolean HandleVvlcRequest (void);

#endif /* end of _GVFB_LINUX_H_ */

