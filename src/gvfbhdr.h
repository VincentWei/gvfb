/*
** $Id: gvfbhdr.h 274 2011-01-27 02:39:25Z xbwang $
**
** gvfbhdr.h : for minigui pc_xvfb.
**
** Copyright (C) 2009 Feynman Software.
**
** All rights reserved by Feynman Software.
**
** Create data: 2009-12-17
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
    unsigned int info_size;    /* info size */
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

typedef struct _MotionJPEGInfo {
    guint16     nr_frames;
    guint16     frame_rate;
    guint32     frame_offset [0];
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
     * 0x01xx for camera, the lower byte is the zoom level
     * 0x02xx for video playback;
     *       the lower byte is the current status,
     *       1 for playing and 0 for stopped.
     */
    unsigned int video_layer_mode;

    /* the alpha channel (0...255) if the video layer enabled */
    int graph_alpha_channel;

    /* the motion jpeg stream */
    GFileInputStream *motion_jpeg_stream;

    /* the motion jpeg information */
    MotionJPEGInfo *motion_jpeg_info;

    /* the index of the current frame of simulation video */
    int video_frame_idx;

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
    GdkPixbuf *pixbuf_s;

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

