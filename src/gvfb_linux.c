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
#include "gvfb_linux.h"
#include "gvfb_log.h"

/* static function */
static void signal_handler (int v);

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
