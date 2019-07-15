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

/*
** Created by VincentWei on 2018-12-11
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <time.h>

#include <linux/limits.h>
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

#define VRT_RESPONSE            1
    #define VRS_OK                  0
    #define VRS_INV_REQUEST         1
    #define VRS_BAD_OPERATION       2
    #define VRS_OPERATION_FAILED    3
    #define VRS_OPERATION_FINISHED  4
    #define VRS_MAX                 VRS_OPERATION_FINISHED

/*
 * video layer status (returned through param2 of VRT_RESPONSE):
 * 0x0000 for off (grid background)
 * 0x01xx for camera status,
 *      0 (0x0100) for idle,
 *      1 (0x0101) for recording video,
 *      2 (0x0102) for camera frozen,
 *      3 (0x0103) for recording paused.
 * 0x02xx for video playback status,
 *      0 (0x0200) for stopped,
 *      1 (0x0201) for playing,
 *      2 (0x0202) for paused,
 *      3 (0x0203) for playback end.
 */
#define VRT_GET_STATUS          2

#define VRT_SET_GRAPH_ALPHA     11
#define VRT_SET_GRAPH_ROTATION  12

#define VRT_OPEN_CAMERA         21
#define VRT_CLOSE_CAMERA        22
#define VRT_SET_ZOOM_LEVEL      23
#define VRT_FREEZE_CAMERA       24
#define VRT_UNFREEZE_CAMERA     25

#define VRT_PLAY_VIDEO          31
#define VRT_SEEK_VIDEO          32
#define VRT_PAUSE_PLAYBACK      33
#define VRT_RESUME_PLAYBACK     34
#define VRT_STOP_PLAYBACK       35

#define VRT_CAPTURE_PHOTO       41
#define VRT_START_RECORD        42
#define VRT_STOP_RECORD         43
#define VRT_PAUSE_RECORD        44
#define VRT_RESUME_RECORD       45

#define VRT_MAX                 VRT_RESUME_RECORD

struct _vvlc_data_header {
    unsigned int    type;
    unsigned int    param1;
    unsigned int    param2;
    unsigned int    payload_len;
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

static const char* operation_name [] = {
    "VRT_UNKNOWN(0)",        //      0
    "VRT_RESPONSE",          //      1
    "VRT_GET_STATUS",        //      2
    "VRT_UNKNOWN(3)",        //      3
    "VRT_UNKNOWN(4)",        //      4
    "VRT_UNKNOWN(5)",        //      5
    "VRT_UNKNOWN(6)",        //      6
    "VRT_UNKNOWN(7)",        //      7
    "VRT_UNKNOWN(8)",        //      8
    "VRT_UNKNOWN(9)",        //      9
    "VRT_UNKNOWN(10)",       //      10
    "VRT_SET_GRAPH_ALPHA",   //      11
    "VRT_SET_GRAPH_ROTATION",//      12
    "VRT_UNKNOWN(13)",       //      13
    "VRT_UNKNOWN(14)",       //      14
    "VRT_UNKNOWN(15)",       //      15
    "VRT_UNKNOWN(16)",       //      16
    "VRT_UNKNOWN(17)",       //      17
    "VRT_UNKNOWN(18)",       //      18
    "VRT_UNKNOWN(19)",       //      19
    "VRT_UNKNOWN(20)",       //      20
    "VRT_OPEN_CAMERA",       //      21
    "VRT_CLOSE_CAMERA",      //      22
    "VRT_SET_ZOOM_LEVEL",    //      23
    "VRT_FREEZE_CAMERA",     //      24
    "VRT_UNFREEZE_CAMERA",   //      25
    "VRT_UNKNOWN(26)",       //      26
    "VRT_UNKNOWN(27)",       //      27
    "VRT_UNKNOWN(28)",       //      28
    "VRT_UNKNOWN(29)",       //      29
    "VRT_UNKNOWN(30)",       //      30
    "VRT_PLAY_VIDEO",        //      31
    "VRT_SEEK_VIDEO",        //      32
    "VRT_PAUSE_PLAYBACK",    //      33
    "VRT_RESUME_PLAYBACK",   //      34
    "VRT_STOP_PLAYBACK",     //      35
    "VRT_UNKNOWN(36)",       //      36
    "VRT_UNKNOWN(37)",       //      37
    "VRT_UNKNOWN(38)",       //      38
    "VRT_UNKNOWN(39)",       //      39
    "VRT_UNKNOWN(40)",       //      40
    "VRT_CAPTURE_PHOTO",     //      41
    "VRT_START_RECORD",      //      42
    "VRT_STOP_RECORD",       //      43
    "VRT_PAUSE_RECORD",      //      44
    "VRT_RESUME_RECORD",     //      45
};

static const char* status_name [] = {
    "VRS_OK",
    "VRS_INV_REQUEST",
    "VRS_BAD_OPERATION",
    "VRS_OPERATION_FAILED",
    "VRS_OPERATION_FINISHED",
};

static int handle_response (int fd)
{
    ssize_t n;
    struct _vvlc_data_header response;

    n = read (fd, &response, sizeof (struct _vvlc_data_header));
    if (n < sizeof (struct _vvlc_data_header)) {
        printf ("Error when reading UNIX socket (%lu to read, %ld got).\n",
                sizeof (struct _vvlc_data_header), n);
        exit_on_error(fd);
        return 0;
    }

    if (response.type != VRT_RESPONSE
            || response.param1 > VRT_MAX) {
        printf ("Wrong response packet (type: %d, operation: %d, status: %d).\n",
                response.type, response.param1, response.param2);
    }
    else if (response.param1 == VRT_PLAY_VIDEO) {
        printf ("Response for playing video request: video time: %ds.\n",
                response.param2);
    }
    else if (response.param1 == VRT_GET_STATUS) {
        printf ("Response for getting status: current status: 0x%04x.\n",
                response.param2);
    }
    else {
        printf ("Response from VVLS (Operation: %s; Status: %s).\n",
                operation_name [response.param1], status_name [response.param2]);
    }

    return 0;
}

static void write_request (int fd, const struct _vvlc_data_header* request,
        int request_len)
{
    ssize_t n;

    printf ("request type (%d), request len: %d, payload length (%u), payload (%s).\n",
        request->type, request_len, request->payload_len,
        (request->payload_len > 0) ? request->payload: "NULL");

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
    scanf("%s", path);
}

static struct _vvlc_data_header* prepare_packet_with_path (const char* path)
{
    struct _vvlc_data_header* packet;

    packet = (struct _vvlc_data_header*)
            calloc (1, sizeof (struct _vvlc_data_header) + strlen (path) + 1);

    if (packet == NULL) {
        return NULL;
    }

    packet->type = VRT_OPEN_CAMERA;
    packet->payload_len = strlen (path) + 1;
    memcpy (packet->payload, path, packet->payload_len);

    return packet;
}

static void open_camera (int fd, const char* path, int zoom_level)
{
    struct _vvlc_data_header* packet;

    packet = prepare_packet_with_path (path);
    if (packet == NULL) {
        printf ("%s: failed to prepare packet\n", __FUNCTION__);
        return;
    }
    packet->type = VRT_OPEN_CAMERA;
    packet->param1 = zoom_level;

    write_request (fd, packet,
            sizeof (struct _vvlc_data_header) + packet->payload_len);

    free (packet);
}

static void set_graph_alpha (int fd, int alpha)
{
    struct _vvlc_data_header packet;

    packet.type = VRT_SET_GRAPH_ALPHA;
    packet.param1 = alpha;
    packet.param2 = 0;
    packet.payload_len = 0;
    write_request (fd, &packet, sizeof (struct _vvlc_data_header));
}

static void set_graph_rotation (int fd, int rotation)
{
    struct _vvlc_data_header packet;

    packet.type = VRT_SET_GRAPH_ROTATION;
    packet.param1 = rotation;
    packet.param2 = 0;
    packet.payload_len = 0;
    write_request (fd, &packet, sizeof (struct _vvlc_data_header));
}

static void capture_photo (int fd, const char* path)
{
    struct _vvlc_data_header* packet;

    packet = prepare_packet_with_path (path);
    if (packet == NULL) {
        printf ("%s: failed to prepare packet\n", __FUNCTION__);
        return;
    }
    packet->type = VRT_CAPTURE_PHOTO;

    write_request (fd, packet,
            sizeof (struct _vvlc_data_header) + packet->payload_len);

    free (packet);
}

static void start_video_record (int fd, const char* path)
{
    struct _vvlc_data_header* packet;

    packet = prepare_packet_with_path (path);
    if (packet == NULL) {
        printf ("%s: failed to prepare packet\n", __FUNCTION__);
        return;
    }
    packet->type = VRT_START_RECORD;

    write_request (fd, packet,
            sizeof (struct _vvlc_data_header) + packet->payload_len);

    free (packet);
}

static void stop_video_record (int fd)
{
    struct _vvlc_data_header packet;

    packet.type = VRT_STOP_RECORD;
    packet.param1 = 0;
    packet.param2 = 0;
    packet.payload_len = 0;
    write_request (fd, &packet, sizeof (struct _vvlc_data_header));
}

static void close_camera (int fd)
{
    struct _vvlc_data_header packet;

    packet.type = VRT_CLOSE_CAMERA;
    packet.param1 = 0;
    packet.param2 = 0;
    packet.payload_len = 0;
    write_request (fd, &packet, sizeof (struct _vvlc_data_header));
}

static void play_video (int fd, const char* path)
{
    struct _vvlc_data_header* packet;

    packet = prepare_packet_with_path (path);
    if (packet == NULL) {
        printf ("%s: failed to prepare packet\n", __FUNCTION__);
        return;
    }
    packet->type = VRT_PLAY_VIDEO;

    write_request (fd, packet,
            sizeof (struct _vvlc_data_header) + packet->payload_len);

    free (packet);
}

static void pause_video_playback (int fd)
{
    struct _vvlc_data_header packet;

    packet.type = VRT_PAUSE_PLAYBACK;
    packet.param1 = 0;
    packet.param2 = 0;
    packet.payload_len = 0;
    write_request (fd, &packet, sizeof (struct _vvlc_data_header));
}

static void resume_video_playback (int fd)
{
    struct _vvlc_data_header packet;

    packet.type = VRT_RESUME_PLAYBACK;
    packet.param1 = 0;
    packet.param2 = 0;
    packet.payload_len = 0;
    write_request (fd, &packet, sizeof (struct _vvlc_data_header));
}

static void pause_video_record (int fd)
{
    struct _vvlc_data_header packet;

    packet.type = VRT_PAUSE_RECORD;
    packet.param1 = 0;
    packet.param2 = 0;
    packet.payload_len = 0;
    write_request (fd, &packet, sizeof (struct _vvlc_data_header));
}

static void resume_video_record (int fd)
{
    struct _vvlc_data_header packet;

    packet.type = VRT_RESUME_RECORD;
    packet.param1 = 0;
    packet.param2 = 0;
    packet.payload_len = 0;
    write_request (fd, &packet, sizeof (struct _vvlc_data_header));
}

static void stop_video_playback (int fd)
{
    struct _vvlc_data_header packet;

    packet.type = VRT_STOP_PLAYBACK;
    packet.param1 = 0;
    packet.param2 = 0;
    packet.payload_len = 0;
    write_request (fd, &packet, sizeof (struct _vvlc_data_header));
}

static void zoom_camera (int fd, int zoom_level)
{
    struct _vvlc_data_header packet;

    packet.type = VRT_SET_ZOOM_LEVEL;
    packet.param1 = zoom_level;
    packet.param2 = 0;
    packet.payload_len = 0;
    write_request (fd, &packet, sizeof (struct _vvlc_data_header));
}

static void get_status (int fd)
{
    struct _vvlc_data_header packet;

    packet.type = VRT_GET_STATUS;
    packet.param1 = 0;
    packet.param2 = 0;
    packet.payload_len = 0;
    write_request (fd, &packet, sizeof (struct _vvlc_data_header));
}

int main (int argc, const char* argv[])
{
    int cmd;
    int fd = connect_to_vvls (SOCKET_VVLS);
    char path[PATH_MAX];
    int zoom_level = 0;


    if (fd < 0)
        return 1;

    while (1) {
        fd_set fds;
        struct timeval tv;
        int ret;

        FD_ZERO (&fds);
        FD_SET (fd, &fds);

        /* 0.5s */
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
            continue;
        }

        printf ("Please choose a command:\n");
        printf ("\t0) Quit.\n");
        printf ("\t1) Open camera.\n");
        printf ("\t2) Capture a photo.\n");
        printf ("\t3) Start video record.\n");
        printf ("\t4) Stop video record.\n");
        printf ("\t5) Close camera.\n");
        printf ("\t6) Play video.\n");
        printf ("\t7) Pause video playback.\n");
        printf ("\t8) Resume video playback.\n");
        printf ("\t9) Stop video playback.\n");
        printf ("\t10) Zoom in camera.\n");
        printf ("\t11) Zoom out camera.\n");
        printf ("\t12) Pause video record.\n");
        printf ("\t13) Resume video record.\n");
        printf ("\t14) Rotates graphics 90° clockwise.\n");
        printf ("\t15) Reset graphics (rotation and alpha).\n");
        printf ("\t20) Get status.\n");

        scanf ("%d", &cmd);

        switch (cmd) {
        case 0:
            close (fd);
            exit (0);
            break;

        case 1:
            got_file_path (path);
            open_camera (fd, path, zoom_level);
            set_graph_alpha (fd, 200);
            break;

        case 2:
            got_file_path (path);
            capture_photo (fd, path);
            break;

        case 3:
            got_file_path (path);
            start_video_record (fd, path);
            break;

        case 4:
            stop_video_record (fd);
            break;

        case 5:
            close_camera (fd);
            set_graph_alpha (fd, 255);
            break;

        case 6:
            got_file_path (path);
            play_video (fd, path);
            set_graph_alpha (fd, 128);
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

        case 10:
            zoom_level += 0x10;
            zoom_camera (fd, zoom_level);
            break;

        case 11:
            zoom_level -= 0x10;
            zoom_camera (fd, zoom_level);
            break;

        case 12:
            pause_video_record (fd);
            break;

        case 13:
            resume_video_record (fd);
            break;

        case 14:
            set_graph_rotation (fd, 1);
            break;

        case 15:
            set_graph_rotation (fd, 0);
            set_graph_alpha (fd, 255);
            break;

        case 20:
            get_status (fd);
            break;
        }

        handle_response (fd);
    }

    return 0;
}

