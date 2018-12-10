#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <signal.h>
#include <unistd.h>
#include <errno.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <sys/ipc.h>
#include <sys/sem.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/un.h>
#include <sys/time.h>

/* for gtk */
#include <gdk/gdkx.h>
#include <gtk/gtk.h>
#include <X11/XKBlib.h>

#include <assert.h>

#include "gvfbhdr.h"
#include "gvfb_main.h"
#include "gvfb_view.h"
#include "gvfb_linux.h"
#include "gvfb_log.h"

/* static function */
static void signal_handler (int v)
{
#ifdef DEBUG
    switch (v) {
    case SIGHUP:
        printf ("SIGHUP\n");
        break;
    case SIGPIPE:
        printf ("SIGPIPE\n");
        break;
    case SIGTERM:
        printf ("SIGTERM\n");
        break;
    case SIGINT:
        printf ("SIGINT\n");
        break;
    default:
        printf ("unknown\n");
        break;
    }
#endif

    if (v == SIGSEGV) {
        msg_out (LEVEL_0, "SIGSEGV.");
    }

    msg_out (LEVEL_0, "signal.(%d)", v);

    gvfbruninfo.running = FALSE;
}

/* locale value */
static int lockid = -1;

/* function */
void SetupSignal (void)
{
    struct sigaction siga;

    siga.sa_flags = 0;
    siga.sa_handler = signal_handler;

    memset (&siga.sa_mask, 0, sizeof (sigset_t));

    sigaction (SIGHUP, &siga, NULL);
    sigaction (SIGPIPE, &siga, NULL);
//    sigaction (SIGSEGV, &siga, NULL);
    sigaction (SIGTERM, &siga, NULL);
    sigaction (SIGINT, &siga, NULL);
    sigaction (SIGQUIT, &siga, NULL);
}

int InitLock (int lockkey)
{
    lockid = semget (lockkey, 0, 0);
#ifdef DEBUG
    printf ("InitLock semget id : %d\n", lockid);
#endif

    return lockid;
}

void UnInitLock (int shmid)
{
#ifdef DEBUG
    printf ("UnInitLock semctl id : %d\n", lockid);
#endif

    semctl (lockid, 0, IPC_RMID, NULL);
}

void Lock (void)
{
    if (lockid >= 0) {
        struct sembuf sops;

        sops.sem_num = 0;
        sops.sem_op = -1;
        sops.sem_flg = SEM_UNDO;

        semop (lockid, &sops, 1);
    }
}

void UnLock (void)
{
    if (lockid >= 0) {
        struct sembuf sops;

        sops.sem_num = 0;
        sops.sem_op = +1;
        sops.sem_flg = SEM_UNDO;

        semop (lockid, &sops, 1);
    }
}

gboolean IsCapslockOn (void)
{
    Display *dsp;

    unsigned int states;

    dsp = GDK_DISPLAY ();

    if (XkbGetIndicatorState (dsp, XkbUseCoreKbd, &states) != Success) {
        return FALSE;
    }

    if ((states & ShiftMask) != 0) {
        return TRUE;
    }
    else {
        return FALSE;
    }
}

/* Get current time (ms) */
gulong GVFBGetCurrentTime (void)
{
    GTimeVal gtv;

    g_get_current_time (&gtv);

    return (gtv.tv_sec * 1000 + gtv.tv_usec / 1000);
}

unsigned char *CreateShareMemory (int key, int data_size)
{
    unsigned char *shm_data;
    struct shmid_ds shm;

    gvfbruninfo.shmid = shmget (key, data_size, IPC_CREAT | 0666);

    if (gvfbruninfo.shmid != -1) {
        shm_data = (unsigned char *) shmat (gvfbruninfo.shmid, NULL, 0);
    }
    else {
        shmctl (gvfbruninfo.shmid, IPC_RMID, &shm);

        gvfbruninfo.shmid = shmget (key, data_size, IPC_CREAT | 0666);

        shm_data = (unsigned char *) shmat (gvfbruninfo.shmid, NULL, 0);
    }
#ifdef DEBUG
    printf ("CreateShareMemory : key : %d shmget : %d size : %d\n",
            key, gvfbruninfo.shmid, data_size);
#endif

    return shm_data;
}

void DestroyShareMemory (void)
{
    GVFBHeader *hdr;

    /* get gvfb header */
    hdr = gvfbruninfo.hdr;
#ifdef DEBUG
    printf ("DestroyShareMemory : shmctl : %d\n", gvfbruninfo.shmid);
#endif

    if (gvfbruninfo.shmid != -1) {
        assert (hdr != NULL);
        shmdt (hdr);
        hdr = NULL;
        shmctl (gvfbruninfo.shmid, IPC_RMID, NULL);
        gvfbruninfo.shmid = -1;
    }
}

int Recv (int s, unsigned char *buf, int len, unsigned int flags)
{
    int nrecv = 0;
    int ntotal = 0;

    do {
        nrecv = recv (s, &buf[ntotal], (len - ntotal), flags);
        /* check */
        if ((nrecv < 0) && (errno == EINTR)) {
            continue;
        }
        else if (nrecv <= 0) {
            return nrecv;
        }

        ntotal += nrecv;
    }
    while (ntotal < len);

    return ntotal;
}

int Send (int s, const unsigned char *buf, int len, unsigned int flags)
{
    int nsend = 0;
    int ntotal = 0;

    do {
        nsend = send (s, &buf[ntotal], (len - ntotal), flags);
        /* check */
        if ((nsend < 0) && (errno == EINTR)) {
            continue;
        }
        else if (nsend <= 0) {
            return nsend;
        }

        ntotal += nsend;
    }
    while (ntotal < len);

    return ntotal;
}

int ConnectToMiniGUI (int ppid)
{
    int sockfd;

    struct sockaddr_un address;
    char socket_file[PATH_MAX];

    snprintf (socket_file, sizeof (socket_file), "/tmp/pcxvfb_socket%d", ppid);

    /* unix socket */
    sockfd = socket (AF_UNIX, SOCK_STREAM, 0);

    if (sockfd < 0) {
        msg_out (LEVEL_0, "create socket error.(socket)");

        return -1;
    }

    address.sun_family = AF_UNIX;

    strcpy (address.sun_path, socket_file);

    if (connect (sockfd, (struct sockaddr *) &address, sizeof (address)) < 0) {
        msg_out (LEVEL_0, "cannot connect to MiniGUI.(%s)", socket_file);

        return -1;
    }

    return sockfd;
}

gboolean HandleVvlcRequest (void)
{
    ssize_t n = 0;
    struct _vvlc_data_header header;
    int status = VRS_OK;
    char path [PATH_MAX];

    n = read (gvfbruninfo.vvlc_sockfd, &header, sizeof (header));
    if (n == 0) {
        return FALSE;
    }

    if (n < sizeof (header)) {
        return FALSE;
    }

    Lock ();

    switch (header.type) {
    case VRT_SET_GRAPH_ALPHA:
        gvfbruninfo.graph_alpha_channel = (int)header.param1;
        break;

    case VRT_OPEN_CAMERA:
        if (gvfbruninfo.video_layer_mode) {
            status = VRS_BAD_OPERATION;
        }
        else if (header.payload_len <= 0) {
            status = VRS_INV_REQUEST;
        }
        else {
            n = read (gvfbruninfo.vvlc_sockfd, path, header.payload_len);
            if (n < header.payload_len) {
                status = VRS_INV_REQUEST;
            }
            else if (!VvlOpenCamera (path, header.param1)) {
                status = VRS_OPERATION_FAILED;
            }
        }
        break;

    case VRT_CLOSE_CAMERA:
        if ((gvfbruninfo.video_layer_mode) & 0xFF00 != 0x0100) {
            status = VRS_BAD_OPERATION;
        }
        else if (!VvlCloseCamera ()) {
            status = VRS_OPERATION_FAILED;
        }
        break;

    case VRT_SET_ZOOM_LEVEL:
        if ((gvfbruninfo.video_layer_mode) & 0xFF00 != 0x0100) {
            status = VRS_BAD_OPERATION;
        }
        else if (!VvlSetZoomLevel (header.param1)) {
            status = VRS_OPERATION_FAILED;
        }
        break;

    case VRT_PLAY_VIDEO:
        if (gvfbruninfo.video_layer_mode) {
            status = VRS_BAD_OPERATION;
        }
        else if (header.payload_len <= 0) {
            status = VRS_INV_REQUEST;
        }
        else {
            n = read (gvfbruninfo.vvlc_sockfd, path, header.payload_len);
            if (n < header.payload_len) {
                status = VRS_INV_REQUEST;
            }
            else if (VvlPlayVideo (path, header.param1) == 0) {
                status = VRS_OPERATION_FAILED;
            }
        }
        break;

    case VRT_SEEK_VIDEO:
        if ((gvfbruninfo.video_layer_mode) & 0xFF00 != 0x0200) {
            status = VRS_BAD_OPERATION;
        }
        else if (!VvlSeekVideo (header.param1)) {
            status = VRS_OPERATION_FAILED;
        }
        break;

    case VRT_PAUSE_PLAYBACK:
        if ((gvfbruninfo.video_layer_mode) & 0xFFFF != 0x0201) {
            status = VRS_BAD_OPERATION;
        }
        else if (!VvlPausPlayback ()) {
            status = VRS_OPERATION_FAILED;
        }
        break;

    case VRT_RESUME_PLAYBACK:
        if ((gvfbruninfo.video_layer_mode) & 0xFFFF != 0x0200) {
            status = VRS_BAD_OPERATION;
        }
        else if (!VvlResumePlayback ()) {
            status = VRS_OPERATION_FAILED;
        }
        break;

    case VRT_STOP_PLAYBACK:
        if ((gvfbruninfo.video_layer_mode) & 0xFF00 != 0x0200) {
            status = VRS_BAD_OPERATION;
        }
        else if (!VvlStopPlayback ()) {
            status = VRS_OPERATION_FAILED;
        }
        break;

    case VRT_CAPTURE_PHOTO:
        if ((gvfbruninfo.video_layer_mode) & 0xFFFF != 0x0100) {
            status = VRS_BAD_OPERATION;
        }
        else if (header.payload_len <= 0) {
            status = VRS_INV_REQUEST;
        }
        else {
            n = read (gvfbruninfo.vvlc_sockfd, path, header.payload_len);
            if (n < header.payload_len) {
                status = VRS_INV_REQUEST;
            }
            else if (!VvlCapturePhoto (path)) {
                status = VRS_OPERATION_FAILED;
            }
        }
        break;

    case VRT_START_RECORD:
        if ((gvfbruninfo.video_layer_mode) & 0xFFFF != 0x0100) {
            status = VRS_BAD_OPERATION;
        }
        else if (header.payload_len <= 0) {
            status = VRS_INV_REQUEST;
        }
        else {
            n = read (gvfbruninfo.vvlc_sockfd, path, header.payload_len);
            if (n < header.payload_len) {
                status = VRS_INV_REQUEST;
            }
            else if (!VvlStartRecord (path)) {
                status = VRS_OPERATION_FAILED;
            }
        }
        break;

    case VRT_STOP_RECORD:
        if ((gvfbruninfo.video_layer_mode) & 0xFFFF != 0x0101) {
            status = VRS_BAD_OPERATION;
        }
        else if (!VvlStopRecord ()) {
            status = VRS_OPERATION_FAILED;
        }
        break;

    default:
        status = VRS_INV_REQUEST;
        break;
    }

    header.type = VRT_RESPONSE;
    header.param1 = status;
    header.payload_len = 0;
    n = write (gvfbruninfo.vvlc_sockfd, &header, sizeof (header));
    if (n != sizeof (header)) {
        msg_out (LEVEL_0, "Error when writting UNIX socket.");
    }

    UnLock ();
    return TRUE;
}

