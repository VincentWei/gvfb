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

#ifndef _GVFBHDR_H_
#define _GVFBHDR_H_

#include <linux/limits.h>
#include <gtk/gtk.h>
#include <gdk/gdk.h>

#include "gvfb_option.h"

/* define */
#define MOUSE_TYPE       0
#define KB_TYPE          1
#define CAPTION_TYPE     2
#define IME_TYPE         3
#define IME_MESSAGE_TYPE 4
#define SHOW_HIDE_TYPE   5
#define GVFB_CLOSE_TYPE  6

#define FIX_HEIGHT       80
#define FIX_MENU_HEIGHT  29
/* border fix */
#define FIX_BORDER       2

#ifndef max
#define max(a,b) ((a) > (b) ? (a) : (b))
#define min(a,b) ((a) < (b) ? (a) : (b))
#endif

typedef struct _GVFBRect{
    int x;
    int y;
    int w;
    int h;
} GVFBRECT;

typedef struct _GVFBHeader {
    unsigned int data_size;    /* the size of the shared memory in bytes */
    int width;                 /* window width */
    int height;                /* window height */
    int depth;                 /* color depth */
    int pitch;                 /* bytes per line */
    int dirty;                 /* dirty flag */

    int dirty_rc_l;            /* left of dirty region */
    int dirty_rc_t;            /* top of dirty region */
    int dirty_rc_r;            /* right of dirty region */
    int dirty_rc_b;            /* buttom of dirty region */

    int palette_changed;       /* palette flag */
    int palette_offset;        /* palette offset */
    int fb_offset;             /* framebuffer offset */
    int MSBLeft;               /* most significant bits */

    int Rmask;                 /* red color mask */
    int Gmask;                 /* green color mask */
    int Bmask;                 /* blue color mask */
    int Amask;                 /* alpha color mask */
} GVFBHeader;

/* gvfb key data */
typedef struct _GVFBKeyData {
    unsigned short key_code;    /* minigui scancode */
    unsigned short key_state;   /* key state. 1 is press, 0 is release */
} GVFBKeyData;

/* gvfb mouse data */
typedef struct _GVFBMouseData {
    unsigned short x;           /* x coordinate of mouse */
    unsigned short y;           /* y coordinate of mouse */
    unsigned int button;        /* mouse button. 1 is left mouse,
                                   2 is right mouse, 4 is middle mouse */
} GVFBMouseData;

/* gvfb event data */
typedef struct _GVFBEventData {
    int event_type;             /* event type. KB_TYPE or MOUSE_TYPE */
    union
    {
        GVFBKeyData key;        /* keyboard data */
        GVFBMouseData mouse;    /* mouse data */
    }data;
} GVFBEventData;

/* gvfb im event data */
typedef struct _GVFBIMEventData {
    int event_type;              /* IM event type. IME_MESSAGE_TYPE */
    int size;                    /* size of input string */
    char buff[0];                /* input string buffer */
} GVFBIMEventData;

/* gvfb pal entry */
typedef struct _GVFBPalEntry {
    unsigned char r;
    unsigned char g;
    unsigned char b;
    unsigned char a;
} GVFBPalEntry;

typedef struct __attribute__ ((packed)) _MotionJPEGInfo {
    guint32     nr_frames;          /* number of frames */
    guint16     frame_rate;         /* frames per second */
    guint16     reserved;
    guint32     offset_first_frame; /* offset of the first frame */
    guint32     frame_offset[0];
} MotionJPEGInfo;

/* runinfo of gvfb */
typedef struct _GVFBRUNINFO {
    /* running flag */
    gboolean  running;
    gboolean  fastmode;
    GVFBHeader *hdr;

    /* main window */
    GtkWidget *window;
    GtkWidget *main_vbox;
    /* layout of window */
    GtkWidget *layout;

    /* ui menu */
    GtkUIManager *ui_manager;

    /* skin widget */
    GtkWidget *skin_win;
    GtkWidget *scroll_win;
    GtkWidget *draw_area;

    GdkPixmap *bkgnd_pixmap;

    GdkCursor *gtkdef_cursor;
    GdkCursor *userdef_cursor;

    GtkIMContext *im_context;

    int *palette;

    /* flag of show gtk cursor */
    gboolean show_gtk_cursor;

    /* enable disable full screen */
    gboolean o_fullscreen;

    /* full screen flag */
    gboolean full_screen;

    /* fit screen flag */
    gboolean fit_screen;

    /* has menu */
    gboolean has_menu;
    gboolean has_skin;

    /* menu height */
    int menu_height;
    int fix_menu_height;

    int fix_border;

    /* graphics layer has alpha component or not */
    int graph_with_alpha;

#ifndef WIN32
#   define SOCKET_VVLS     "/tmp/pcxvvl_socket"
    /* file descriptor of the unix socket for virtual video layer server */
    int vvls_sockfd;

    /* file descriptor of the unix socket for virtual video layer client */
    int vvlc_sockfd;
#endif

    /*
     * video layer mode:
     * 0x0000 for off (grid background)
     * 0x01xx for camera,
     *      the lower byte is the current status,
     *      0 for idle, 1 for recording,  2 for frozen, 3 for recording paused.
     * 0x02xx for video playback;
     *      the lower byte is the current status,
     *      0 for stopped, 1 for playing, 2 for paused, 3 for end.
     */
    unsigned int video_layer_mode;

    /* the alpha channel (0...255) if the video layer enabled */
    int graph_alpha_channel;

    /* the motion jpeg stream */
    GFileInputStream *motion_jpeg_stream;

    /* the motion jpeg information */
    MotionJPEGInfo *motion_jpeg_info;

    /* the current zoom level of camera */
    int camera_zoom_level;

    /* the index of the current frame of simulation video */
    unsigned int video_frame_idx;

    /*
     * the motion jpeg stream
     * if video_layer_mode is 0x0101 but video_record_stream is null,
     * it was failed to record the video.
     */
    GFileOutputStream *video_record_stream;
    guint32 nr_frames_recorded;

    /* the rotation angel (degree) of graphics layer */
    int rotation;

    int refresh_rate;

    /* screen size */
    int screen_w;
    int screen_h;

    /* main wnd size */
    int main_w;
    int main_h;

    /* scroll window size */
    int sclwnd_x;
    int sclwnd_y;
    int sclwnd_w;
    int sclwnd_h;

    /* skin window size */
    int skin_x;
    int skin_y;
    int skin_w;
    int skin_h;

    /* drawarea size */
    int drawarea_x;
    int drawarea_y;
    int drawarea_w;
    int drawarea_h;

    /* user screen size (fixed) */
    int actual_w;
    int actual_h;

    /* the zoom percentage */
    int zoom_percent;

    unsigned char *pixel_data;
    GdkPixbuf *pixbuf_r;
    //GdkPixbuf *pixbuf_s;

#ifdef WIN32
    int lockid;

    /* share memory */
    unsigned char *shm_data;
    int shmid;
#else
    int lockid;

    /* share memory */
    unsigned char *shm_data;
    int shmid;
#endif

    int timer;
    int sockfd;
} GVFBRUNINFO;

#endif /* end of _GVFBHDR_H_ */

