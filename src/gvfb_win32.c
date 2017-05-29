#include <stdio.h>
#include <stdlib.h>

#include <windows.h>
#include <wtypes.h>

#include <winuser.h>

#pragma comment(lib, "ws2_32.lib")

#include "gvfbhdr.h"
#include "gvfb_main.h"
#include "gvfb_win32.h"
#include "gvfb_log.h"

#pragma warning(disable:4996)

#pragma comment(lib,"ws2_32.lib")

/* static */
const static char *gMapFilePre = "WVFBScreenMap-";      /* mapfile must match with MiniGUI's */
const static char *lockname = "WVFBScreenObject";       /* lockname must match with MiniGUI's */

static char gMapFile[PATH_MAX] = { 0 };

static HANDLE hMutex = NULL;
static HANDLE hFile = NULL;
static HANDLE hMap = NULL;
static LPVOID lpScreen = NULL;

int InitLock (int lockkey)
{
    hMutex = CreateMutex (NULL, FALSE, lockname);

    if (hMutex == NULL) {
        msg_out (LEVEL_0, "cannot create mutex.");

        return -1;
    }

    return 0;
}

void UnInitLock (int shmid)
{
    CloseHandle (hMutex);

    hMutex = NULL;
}

void Lock (void)
{
    WaitForSingleObject (hMutex, INFINITE);
}

void UnLock (void)
{
    ReleaseMutex (hMutex);
}

gulong GVFBGetCurrentTime (void)
{
    GTimeVal gtv;

    g_get_current_time (&gtv);

    return (gtv.tv_sec * 1000 + gtv.tv_usec / 1000);
}

gboolean IsCapslockOn (void)
{
    unsigned char kbuf[256];

    GetKeyboardState (kbuf);

    if (kbuf[VK_CAPITAL] & 1) {
        return TRUE;
    }

    return FALSE;
}

unsigned char *CreateShareMemory (int key, int data_size)
{
    {
        char tmp[PATH_MAX];
        GetTempPath (sizeof (tmp) / sizeof (tmp[0]), tmp);
        sprintf (gMapFile, "%s/%s-%d", tmp, gMapFilePre, key);
    }

    hFile = CreateFile (gMapFile,
                        GENERIC_READ | GENERIC_WRITE,
                        FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                        NULL,
                        CREATE_ALWAYS,
                        FILE_ATTRIBUTE_NORMAL | FILE_FLAG_DELETE_ON_CLOSE,
                        NULL);

    if (hFile == INVALID_HANDLE_VALUE) {
        msg_out (LEVEL_0, "CreateFile error.(%s)", gMapFile);

        return (unsigned char *) -1;
    }

    hMap = CreateFileMapping (hFile, NULL, PAGE_READWRITE, 0, data_size, NULL);

    if (hMap == NULL) {
        msg_out (LEVEL_0, "CreateFileMapping error.");

        return (unsigned char *) -1;
    }

    gvfbruninfo.shmid = (int) hMap;

    lpScreen = MapViewOfFile (hMap, FILE_MAP_ALL_ACCESS, 0, 0, 0);

    return (unsigned char *) lpScreen;
}

void DestroyShareMemory (void)
{
    if (lpScreen) {
        UnmapViewOfFile (lpScreen);
        lpScreen = NULL;
    }

    if (hMap) {
        CloseHandle (hMap);
        hMap = NULL;
    }

    if (hFile) {
        CloseHandle (hFile);
        hFile = NULL;
    }

    DeleteFile (gMapFile);
}

int Recv (int s, unsigned char *buf, int len, unsigned int flags)
{
    int nrecv = 0;
    int ntotal = 0;

    do {
        nrecv = recv (s, &buf[ntotal], (len - ntotal), flags);
        /* check */
        if (nrecv == SOCKET_ERROR) {
            return -1;
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
        if (nsend == SOCKET_ERROR) {
            return -1;
        }

        ntotal += nsend;
    }
    while (ntotal < len);

    return ntotal;
}

int ConnectToMiniGUI (int ppid)
{
    int sockfd;
    struct sockaddr_in serv_ad;

    WORD wVersionRequested;
    WSADATA wsaData;

    wVersionRequested = MAKEWORD (2, 2);

    sockfd = INVALID_SOCKET;

    if (0 != WSAStartup (wVersionRequested, &wsaData)) {
        msg_out (LEVEL_0, "cannot create socket.(WSAStartup)");

        return -1;
    }

    sockfd = socket (AF_INET, SOCK_STREAM, IPPROTO_TCP);

    if (sockfd == INVALID_SOCKET) {
        msg_out (LEVEL_0, "cannot create socket.(socket)");

        return -1;
    }

    serv_ad.sin_family = AF_INET;
    serv_ad.sin_port = htons (ppid);
    serv_ad.sin_addr.S_un.S_addr = inet_addr ("127.0.0.1");

    if (0 != connect (sockfd, (struct sockaddr *) &serv_ad, sizeof (serv_ad))) {
        msg_out (LEVEL_0, "cannot connect to MiniGUI.(%d)", ppid);

        return -1;
    }

    return sockfd;
}
