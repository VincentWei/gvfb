/*
** Copyright (C) 2018 FMSoft
**
** Create data: 2018-12-11
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <time.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <sys/ipc.h>
#include <sys/sem.h>
#include <sys/shm.h>
#include <sys/un.h>
#include <unistd.h>
#include <signal.h>
#include <errno.h>

#define SOCKET_VVLS     "/tmp/pcxvvl_socket"
#define CAMERA_MJPEG_FILE   "camera.mjpeg"

#define VRT_RESPONSE            1
    #define VRS_OK                  0
    #define VRS_INV_REQUEST         1
    #define VRS_BAD_OPERATION       2
    #define VRS_OPERATION_FAILED    3

    #define VRS_MAX                 VRS_OPERATION_FAILED

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

#define VRT_MAX                 VRT_STOP_RECORD

struct _vvlc_data_header {
    unsigned int    type;
    unsigned int    param1;
    unsigned int    param2;
    size_t          payload_len;
    char            payload[0];
};

static int connect_to_vvls (const char* socket_file)
{
    int sockfd;
    struct sockaddr_un address;

    /* unix socket */
    sockfd = socket (AF_UNIX, SOCK_STREAM, 0);
    if (sockfd < 0) {
        perror ("Failed to create socket");
        return -1;
    }

    address.sun_family = AF_UNIX;
    strcpy (address.sun_path, socket_file);

    if (connect (sockfd, (struct sockaddr *) &address, sizeof (address)) < 0) {
        perror ("Failed to connect to VVLS");
        return -1;
    }

    return sockfd;
}

static void exit_on_error (int fd)
{
    close (fd);
    exit (2);
}

static void write_request (int fd, const struct _vvlc_data_header* request,
        int request_len)
{
    ssize_t n;

    n = write (fd, request, request_len);
    if (n != request_len) {
        printf ("Error when writting UNIX socket (%d to write, %ld wrotten).\n",
            request_len, n);
        exit_on_error(fd);
    }
}

static void got_file_path (char* path)
{
    printf ("\nPlease input a file path:\n");
    scanf("%ms", &path);
}

static void open_camera (int fd, const char* path)
{
    struct _vvlc_data_header* packet;

    packet = (struct _vvlc_data_header*)
            calloc (1, sizeof (struct _vvlc_data_header) + strlen (path) + 1);

    if (packet == NULL) {
        printf ("Failed to prepare packet\n");
        return;
    }

    packet->type = VRT_OPEN_CAMERA;
    packet->payload_len = strlen (path) + 1;
    memcpy (packet->payload, path, packet->payload_len);

    write_request (fd, packet,
            sizeof (struct _vvlc_data_header) + packet->payload_len);
}

static void capture_photo (int fd, const char* path)
{
}

static void start_video_record (int fd, const char* path)
{
}

static void stop_video_record (int fd)
{
}

static void close_camera (int fd)
{
    struct _vvlc_data_header packet;

    packet.type = VRT_CLOSE_CAMERA;
    packet.payload_len = 0;
    write_request (fd, &packet, sizeof (struct _vvlc_data_header));
}

static void play_video (int fd, const char* path)
{
}

static void pause_video_playback (int fd)
{
    struct _vvlc_data_header packet;

    packet.type = VRT_PAUSE_PLAYBACK;
    packet.payload_len = 0;
    write_request (fd, &packet, sizeof (struct _vvlc_data_header));
}

static void resume_video_playback (int fd)
{
    struct _vvlc_data_header packet;

    packet.type = VRT_RESUME_PLAYBACK;
    packet.payload_len = 0;
    write_request (fd, &packet, sizeof (struct _vvlc_data_header));
}

static void stop_video_playback (int fd)
{
    struct _vvlc_data_header packet;

    packet.type = VRT_STOP_PLAYBACK;
    packet.payload_len = 0;
    write_request (fd, &packet, sizeof (struct _vvlc_data_header));
}

static const char* operation_name [] = {
    "VRT_UNKNOWN(0)"        //      0
    "VRT_RESPONSE"          //      1
    "VRT_UNKNOWN(2)"        //      2
    "VRT_UNKNOWN(3)"        //      3
    "VRT_UNKNOWN(4)"        //      4
    "VRT_UNKNOWN(5)"        //      5
    "VRT_UNKNOWN(6)"        //      6
    "VRT_UNKNOWN(7)"        //      7
    "VRT_UNKNOWN(8)"        //      8
    "VRT_UNKNOWN(9)"        //      9
    "VRT_UNKNOWN(10)"       //      10
    "VRT_SET_GRAPH_ALPHA"   //      11
    "VRT_UNKNOWN(12)"       //      12
    "VRT_UNKNOWN(13)"       //      13
    "VRT_UNKNOWN(14)"       //      14
    "VRT_UNKNOWN(15)"       //      15
    "VRT_UNKNOWN(16)"       //      16
    "VRT_UNKNOWN(17)"       //      17
    "VRT_UNKNOWN(18)"       //      18
    "VRT_UNKNOWN(19)"       //      19
    "VRT_UNKNOWN(20)"       //      20
    "VRT_OPEN_CAMERA"       //      21
    "VRT_CLOSE_CAMERA"      //      22
    "VRT_SET_ZOOM_LEVEL"    //      23
    "VRT_UNKNOWN(24)"       //      24
    "VRT_UNKNOWN(25)"       //      25
    "VRT_UNKNOWN(26)"       //      26
    "VRT_UNKNOWN(27)"       //      27
    "VRT_UNKNOWN(28)"       //      28
    "VRT_UNKNOWN(29)"       //      29
    "VRT_UNKNOWN(30)"       //      30
    "VRT_PLAY_VIDEO"        //      31
    "VRT_SEEK_VIDEO"        //      32
    "VRT_PAUSE_PLAYBACK"    //      33
    "VRT_RESUME_PLAYBACK"   //      34
    "VRT_STOP_PLAYBACK"     //      35
    "VRT_UNKNOWN(36)"       //      36
    "VRT_UNKNOWN(37)"       //      37
    "VRT_UNKNOWN(38)"       //      30
    "VRT_UNKNOWN(39)"       //      39
    "VRT_UNKNOWN(40)"       //      40
    "VRT_CAPTURE_PHOTO"     //      41
    "VRT_START_RECORD"      //      42
    "VRT_STOP_RECORD"       //      43
};

static const char* status_name [] = {
    "VRS_OK",
    "VRS_INV_REQUEST",
    "VRS_BAD_OPERATION",
    "VRS_OPERATION_FAILED",
};

static void handle_response (int fd)
{
    ssize_t n;
    struct _vvlc_data_header response;

    n = read (fd, &response, sizeof (struct _vvlc_data_header));
    if (n != sizeof (struct _vvlc_data_header)) {
        printf ("Error when reading UNIX socket (%lu to read, %ld got).\n",
                sizeof (struct _vvlc_data_header), n);
        exit_on_error(fd);
    }

    if (response.type != VRT_RESPONSE
            || response.param1 > VRT_MAX
            || response.param1 > VRS_MAX) {
        printf ("Wrong response packet (type: %d, operation: %d, status: %d).\n",
                response.type, response.param1, response.param2);
    }
    else {
        printf ("Response from VVLS (Operation: %s; Status: %s).\n",
                operation_name [response.param1], status_name [response.param2]);
    }
}

int main (int argc, const char* argv[])
{
    int cmd;
    int fd = connect_to_vvls (SOCKET_VVLS);
    char* path;

    if (fd < 0)
        return 1;

    while (1) {
        fd_set fds;
        struct timeval tv;
        int ret;

        FD_ZERO (&fds);
        FD_SET (fd, &fds);

        /* 5s */
        tv.tv_sec = 0;
        tv.tv_usec = 500 * 1000;
        ret = select (fd + 1, &fds, NULL, NULL, &tv);

        if (ret < 0) {
            if (errno == EINTR) {
                continue;
            }
            exit_on_error (fd);
        }
        else if (ret > 0 && FD_ISSET (fd, &fds)) {
            handle_response (fd);
        }

        printf ("Please choose a command:\n");
        printf ("\t1) Open camera.\n");
        printf ("\t2) Capture a photo.\n");
        printf ("\t3) Start video record.\n");
        printf ("\t4) Stop video record.\n");
        printf ("\t5) Close camera.\n");
        printf ("\t6) Play video.\n");
        printf ("\t7) Pause video playback.\n");
        printf ("\t8) Resume video playback.\n");
        printf ("\t9) Stop video playback.\n");
        printf ("\t0) Quit.\n");

        scanf ("%d", &cmd);

        switch (cmd) {
        case 0:
            close (fd);
            exit (0);
            break;

        case 1:
            got_file_path (path);
            open_camera (fd, path);
            free (path);
            break;

        case 2:
            got_file_path (path);
            capture_photo (fd, path);
            free (path);
            break;

        case 3:
            got_file_path (path);
            start_video_record (fd, path);
            free (path);
            break;

        case 4:
            stop_video_record (fd);
            break;

        case 5:
            close_camera (fd);
            break;

        case 6:
            got_file_path (path);
            play_video (fd, path);
            free (path);
            break;

        case 7:
            pause_video_playback (fd);
            break;

        case 8:
            resume_video_playback (fd);
            break;

        case 9:
            stop_video_playback (fd);
            break;
        }
    }

    return 0;
}

