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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

/* for gtk */
#include <gtk/gtk.h>
#include <gdk/gdkkeysyms.h>
#include <gio/gunixinputstream.h>

#include <assert.h>

#ifdef WIN32
#   pragma warning(disable:4996)
#   include <winsock2.h>

#   include "gvfb_win32.h"
#else
#   include <sys/socket.h>
#   include <sys/ipc.h>
#   include <sys/sem.h>
#   include <sys/shm.h>
#   include <sys/un.h>
#   include <unistd.h>
#   include <signal.h>
#   include <errno.h>

#   include "gvfb_linux.h"
#endif

#include "gvfb_main.h"
#include "gvfb_view.h"
#include "gvfb_input.h"
#include "gvfb_errmsg.h"
#include "gvfb_log.h"

#define R 0
#define G 1
#define B 2
#define A 3

#define ISUTF8(c)  (((c) & 0xc0) == 0xc0)

/* global variable */
GVFBRUNINFO gvfbruninfo;

/* static variable */
static int s_mask[4];
static int s_shift[4];
static int s_color_map_table[4][256];
static int is_default_color = 0;

/* set capslock state */
static void set_capslock (void);
static void set_palette (void);
/* Read data from buffer and save them to pixel_data */
static void get_pixbuf_data (int x, int y, int width, int height);
static void set_dirty (int flag, int l, int t, int r, int b);
static void draw_dirty_rect (void);
static gboolean is_keypad_data (unsigned int keycode);

static struct color_format_table
{
    const char *name;
    int depth;
    unsigned int mask[4];       // RGBA
} s_color_format_table[] = {
    {
        "rgba32", 32, {
        0x000000FF, 0x0000FF00, 0x00FF0000, 0xFF000000}
    }, {
        "argb32", 32, {
        0x00FF0000, 0x0000FF00, 0x000000FF, 0xFF000000}
    }, {
        "rgb32", 32, {
        0x00FF0000, 0x0000FF00, 0x000000FF, 0x00000000}
    }, {
        "rgb24", 24, {
        0x00FF0000, 0x0000FF00, 0x000000FF, 0x00000000}
    }, {
        "rgb666", 24, {
        0x0003F000, 0x00000FC0, 0x0000003F, 0x00000000}
    }, {
        "rgb565", 16, {
        0x0000F800, 0x000007E0, 0x0000001F, 0x00000000}
    }, {
        "argb1555", 16, {
        0x00007C00, 0x000003E0, 0x0000001F, 0x00008000}
    }, {
        "rgb444", 16, {
        0x00000F00, 0x000000F0, 0x0000000F, 0x00000000}
    }, {
        "argb4444", 16, {
        0x00000F00, 0x000000F0, 0x0000000F, 0x0000F000}
    }
};

/* init running information struct */
void InitRunInfo ()
{
    /* set running flags */
    gvfbruninfo.running = TRUE;
    gvfbruninfo.fastmode = OPT_FASTMODE;
    gvfbruninfo.hdr = (void *) -1;

    gvfbruninfo.pixbuf_r = NULL;
    //gvfbruninfo.pixbuf_s = NULL;

    /* init gvfbruninfo */
    gvfbruninfo.has_menu = TRUE;
    gvfbruninfo.menu_height = FIX_MENU_HEIGHT;
    gvfbruninfo.fix_menu_height = gvfbruninfo.menu_height;
    gvfbruninfo.show_gtk_cursor = OPT_SHOWGTKCURSOR;

    gvfbruninfo.fix_border = FIX_BORDER;
    /* default has no skin */
    gvfbruninfo.has_skin = FALSE;

    /* enable full screen */
    gvfbruninfo.o_fullscreen = TRUE;

    /* default is normal screen */
    gvfbruninfo.full_screen = FALSE;

    /* default is fit screen */
    gvfbruninfo.fit_screen = TRUE;

    /* default is not zoom */
    gvfbruninfo.zoom_percent = 100;

    /* default is no rotation */
    gvfbruninfo.rotation = 0;

    /* set screen size */
    gvfbruninfo.screen_w = 0;
    gvfbruninfo.screen_h = 0;

    gvfbruninfo.shmid = -1;
    gvfbruninfo.pixel_data = NULL;

    gvfbruninfo.drawarea_x = 0;
    gvfbruninfo.drawarea_y = 0;
    gvfbruninfo.drawarea_w = 0;
    gvfbruninfo.drawarea_h = 0;

    gvfbruninfo.actual_w = 0;
    gvfbruninfo.actual_h = 0;

    gvfbruninfo.window = NULL;
    gvfbruninfo.scroll_win = NULL;

    /* refresh rate */
    gvfbruninfo.window = NULL;
    gvfbruninfo.refresh_rate = OPT_REFRESHRATE;
    gvfbruninfo.bkgnd_pixmap = NULL;
}

void UnInitRunInfo ()
{
#if 0
    if (gvfbruninfo.pixbuf_s != NULL) {
        g_object_unref (gvfbruninfo.pixbuf_s);
    }
#endif
}

int FixDepth (int depth)
{
    int fixed_depth = depth;

    if (depth == 12) {
        fixed_depth = 16;
    }
    else if (depth == 18) {
        fixed_depth = 24;
    }

    return fixed_depth;
}

int GetColorFormatIndex (int depth, const char *color_format)
{
    int index;
    int len;
    int i;

    len = sizeof (s_color_format_table) / sizeof (s_color_format_table[0]);

    index = -1;

    /* Find color format */
    for (i = 0; i < len; i++) {
        if (s_color_format_table[i].depth != depth) {
            continue;
        }

        if (!color_format ||
            strncasecmp (color_format,
                         s_color_format_table[i].name,
                         strlen (s_color_format_table[i].name)) == 0) {
            index = i;
            break;
        }
    }

    return index;
}

/* Init */
int Init (int ppid, int width, int height, int depth, const char *color_format)
{
    GVFBHeader *hdr;

    int pitch = 0;
    int len;
    int format_index;
    int s_bit[4];
    int i, k;
    unsigned int data_size;
    unsigned char t;

    int nr_entry = 0;

    /* malloc pixel buffer */
    gvfbruninfo.pixel_data = (unsigned char *) malloc (width * height * 4);

    if (gvfbruninfo.pixel_data == NULL) {
        msg_out (LEVEL_0, "cannot malloc memory.(pixel_data)");

        return -1;
    }

    /* set nr_entry */
    if (depth <= 8) {
        nr_entry = 1 << depth;
    }

    /* Calculate pitch */
    if (depth == 1) {
        pitch = (width * depth + 7) / 8;
    }
    else {
        pitch = ((width * depth + 31) / 32) * 4;
    }

    /* Calculate the size of pixel data.
     *
     * Since 1.2.4, we create double buffers for the clients.
     * The client (e.g., MiniGUI) can use the double buffers to eliminate
     * the mess screen due to the fast asynchronous update, and support
     * the hardware cursor.
     *
     * When double buffers used, the value of the first field `data_size`
     * in the header will be the size of the shared memory in bytes.
     * This field is previously called `info_size`, which was initialized
     * as 0 before 1.2.4, and ignored by the clients. Therefore, a new client
     * which want to support double buffering can depend on the value of this
     * field to check the availability of double buffers.
     *
     * GVFB always reads the pixels in the first buffer to show the contents
     * in the window.
     */
    data_size =
        (pitch * height) * 2 + sizeof (GVFBHeader) + nr_entry * sizeof (GVFBPalEntry);

    /* init header */
    hdr = (GVFBHeader *) CreateShareMemory (ppid, data_size);
    if ((intptr_t) hdr == -1) {
        msg_out (LEVEL_0, "CreateShareMemory error.");

        /* free pixel_data */
        free (gvfbruninfo.pixel_data);
        gvfbruninfo.pixel_data = NULL;

        return -1;
    }

    /* save gvfb header */
    gvfbruninfo.hdr = hdr;

    hdr->data_size = data_size;
    hdr->width = width;
    hdr->height = height;
    hdr->depth = depth;
    hdr->pitch = pitch;
    hdr->palette_changed = 0;
    hdr->palette_offset = sizeof (GVFBHeader);
    hdr->fb_offset = sizeof (GVFBHeader) + nr_entry * sizeof (GVFBPalEntry);
    hdr->dirty = 0;
    hdr->dirty_rc_l = 0;
    hdr->dirty_rc_t = 0;
    hdr->dirty_rc_r = 0;
    hdr->dirty_rc_b = 0;
    hdr->MSBLeft = 0;

    if (depth > 8) {
        format_index = GetColorFormatIndex (depth, color_format);

        if (format_index == -1) {
            /* Not exist current color format */
            msg_out (LEVEL_0, "Unknown color_format.(%d:%s)",
                     depth, color_format);

            /* Destroy Share memorry */
            DestroyShareMemory ();

            /* free pixel_data */
            free (gvfbruninfo.pixel_data);
            gvfbruninfo.pixel_data = NULL;

            return -1;
        }

        if (format_index == 0) {
            is_default_color = 1;
        }

        /* Set mask */
        s_mask[R] = hdr->Rmask = s_color_format_table[format_index].mask[R];
        s_mask[G] = hdr->Gmask = s_color_format_table[format_index].mask[G];
        s_mask[B] = hdr->Bmask = s_color_format_table[format_index].mask[B];
        s_mask[A] = hdr->Amask = s_color_format_table[format_index].mask[A];

        if (s_mask[A]) {
            gvfbruninfo.graph_with_alpha = TRUE;
        }

        /* Calculate rgba shift and bit */
        for (i = 0; i < 4; i++) {
            unsigned int tmp = s_mask[i];
            s_shift[i] = 0;
            s_bit[i] = 0;

            if (s_mask[i]) {
                while (!(tmp & 0x1)) {
                    s_shift[i]++;
                    tmp >>= 1;
                }

                while ((tmp & 0x1)) {
                    s_bit[i]++;
                    tmp >>= 1;
                }
            }   /* end if */
        }       /* end for */

        /* Create color map table */
        memset (s_color_map_table, -1, sizeof (s_color_map_table));

        for (k = 0; k < 4; k++) {
            if (s_bit[k] == 0) {
                continue;
            }

            len = (1 << s_bit[k]);

            for (i = 0; i < len; i++) {
                t = i * 255 / (len - 1);
                s_color_map_table[k][i] = t;
            }   /* end for */
        }       /* end for */
    }
#if 0
    else {
        if (d == 2) {
            hdr->MSBLeft = 1;
        }
    }   /* end if */
#endif

    gvfbruninfo.sockfd = ConnectToMiniGUI (ppid);

    if (gvfbruninfo.sockfd == -1) {
        msg_out (LEVEL_0, "cannot connect to MiniGUI.");

        /* Destroy Share memorry */
        DestroyShareMemory ();

        /* free pixel_data */
        free (gvfbruninfo.pixel_data);
        gvfbruninfo.pixel_data = NULL;

        return -1;
    }

    /* send share memory id to MiniGUI */
#ifdef WIN32
    /* none */
#else
    if (Send
        (gvfbruninfo.sockfd, (const unsigned char *) &gvfbruninfo.shmid,
         sizeof (int), 0) != sizeof (int)) {
        msg_out (LEVEL_0, "cannot send shmid to MiniGUI.");

        close (gvfbruninfo.sockfd);
        gvfbruninfo.sockfd = -1;

        /* Destroy Share memorry */
        DestroyShareMemory ();

        /* free pixel_data */
        free (gvfbruninfo.pixel_data);
        gvfbruninfo.pixel_data = NULL;

        return -1;
    }
#endif /* !WIN32 */

    /* set capslock state */
    set_capslock ();

    /* read the palette from minigui */
    if (depth <= 8) {
        len = 1 << depth;

        gvfbruninfo.palette = (int *) malloc (len * sizeof (int));

        if (gvfbruninfo.palette == NULL) {
            msg_out (LEVEL_0, "cannot malloc memory.");

#ifdef WIN32
            closesocket (gvfbruninfo.sockfd);
#else
            close (gvfbruninfo.sockfd);
#endif
            gvfbruninfo.sockfd = -1;

            /* Destroy Share memorry */
            DestroyShareMemory ();

            /* free pixel_data */
            free (gvfbruninfo.pixel_data);
            gvfbruninfo.pixel_data = NULL;

            return -1;
        }       /* end if */

        set_palette ();
    }   /* end if */

    /* Init Lock */
    if (InitLock (ppid) == -1) {
        msg_out (LEVEL_0, "cannot init lock.");

#ifdef WIN32
        closesocket (gvfbruninfo.sockfd);
#else
        close (gvfbruninfo.sockfd);
#endif
        gvfbruninfo.sockfd = -1;

        /* Destroy Share memorry */
        DestroyShareMemory ();

        /* free pixel_data */
        free (gvfbruninfo.pixel_data);
        gvfbruninfo.pixel_data = NULL;

        free (gvfbruninfo.palette);
        gvfbruninfo.palette = NULL;

        return -1;
    }

#ifdef WIN32
    gvfbruninfo.video_layer_mode = 0x0000;
    gvfbruninfo.graph_alpha_channel = 127;
#else
    gvfbruninfo.video_layer_mode = 0x0000;
    gvfbruninfo.graph_alpha_channel = 255;
    gvfbruninfo.camera_zoom_level = 0x30;

    gvfbruninfo.vvls_sockfd = -1;
    gvfbruninfo.vvlc_sockfd = -1;

    msg_log ("%s: Initializing VVLSd: %s\n", __FUNCTION__, SOCKET_VVLS);

    {
        struct sockaddr_un server_address;
        gvfbruninfo.vvls_sockfd = socket (AF_UNIX, SOCK_STREAM, 0);
        if (gvfbruninfo.vvls_sockfd >= 0) {
            server_address.sun_family = AF_UNIX;
            unlink (SOCKET_VVLS);
            strcpy (server_address.sun_path, SOCKET_VVLS);

            if (bind (gvfbruninfo.vvls_sockfd,
                    (struct sockaddr *)&server_address,
                    sizeof (server_address)) >= 0) {

                chmod (SOCKET_VVLS, 0666);

                if (listen (gvfbruninfo.vvls_sockfd, 1) < 0) {
                    msg_log ("%s: Failed to listen to VVLS socket\n", __FUNCTION__);
                    close (gvfbruninfo.vvls_sockfd);
                    gvfbruninfo.vvls_sockfd = -1;
                }

                msg_log ("%s: VVLS Listen at %d\n",
                    __FUNCTION__, gvfbruninfo.vvls_sockfd);
            }
            else {
                msg_log ("%s: Failed to bind to VVLS socket\n", __FUNCTION__);
                close (gvfbruninfo.vvls_sockfd);
                gvfbruninfo.vvls_sockfd = -1;
            }
        }
        else {
            msg_log ("%s: Failed to create VVLS socket\n", __FUNCTION__);
        }
    }
#endif /* !WIN32 */

    return 0;
}

/* close sockfd, free memory etc. */
void UnInit ()
{
    UnInitLock (0);

    if (gvfbruninfo.motion_jpeg_info) {
        free (gvfbruninfo.motion_jpeg_info);
        gvfbruninfo.motion_jpeg_info = NULL;
    }

    if (gvfbruninfo.motion_jpeg_stream) {
        g_object_unref (gvfbruninfo.motion_jpeg_stream);
        gvfbruninfo.motion_jpeg_stream = NULL;
    }

#ifdef WIN32
    if (gvfbruninfo.vvls_sockfd >= 0) {
        close (gvfbruninfo.vvls_sockfd);
        gvfbruninfo.vvls_sockfd = -1;
    }

    if (gvfbruninfo.vvlc_sockfd >= 0) {
        close (gvfbruninfo.vvlc_sockfd);
        gvfbruninfo.vvlc_sockfd = -1;
    }
#endif

    if (gvfbruninfo.sockfd != -1) {
#ifdef WIN32
        closesocket (gvfbruninfo.sockfd);

        WSACleanup ();
#else
        close (gvfbruninfo.sockfd);
#endif
        gvfbruninfo.sockfd = -1;
    }

    if (gvfbruninfo.pixel_data != NULL) {
        /* free pixel_data */
        free (gvfbruninfo.pixel_data);
        gvfbruninfo.pixel_data = NULL;
    }

    /* Destroy Share memorry */
    DestroyShareMemory ();

    if (gvfbruninfo.palette != NULL) {
        free (gvfbruninfo.palette);
        gvfbruninfo.palette = NULL;
    }
}

/* main event */
#define LBUTTON_DOWN        1
#define RBUTTON_DOWN        2
#define MBUTTON_DOWN        4

gboolean EventProc (GtkWidget * window, GdkEvent * event)
{
    int x, y, button;
    unsigned int keycode;
    static int pressed_buttons = 0;

    switch (event->type) {
    case GDK_BUTTON_PRESS:
        /* mouse button press */
        msg_log ("catch GDK_BUTTON_PRESS event\n");

        x = (int) (event->button.x * 100 / gvfbruninfo.zoom_percent);
        y = (int) (event->button.y * 100 / gvfbruninfo.zoom_percent);

        button = event->button.button;

        if (button == 1) {
            /* left */
            pressed_buttons |= LBUTTON_DOWN;
        }
        else if (button == 2) {
            /* middle */
            pressed_buttons |= MBUTTON_DOWN;
        }
        else if (button == 3) {
            /* right */
            pressed_buttons |= RBUTTON_DOWN;
        }

        SendMouseData (x, y, pressed_buttons);
        break;

    case GDK_BUTTON_RELEASE:
        /* mouse button release */
        msg_log ("catch GDK_BUTTON_RELEASE event\n");

        x = (int) (event->button.x * 100 / gvfbruninfo.zoom_percent);
        y = (int) (event->button.y * 100 / gvfbruninfo.zoom_percent);

        button = event->button.button;

        if (button == 1) {
            /* the left mouse release */
            pressed_buttons &= ~LBUTTON_DOWN;
        }
        else if (button == 2) {
            /* middle */
            pressed_buttons &= ~MBUTTON_DOWN;
        }
        else if (button == 3) {
            /* right */
            pressed_buttons &= ~RBUTTON_DOWN;
        }
        SendMouseData (x, y, pressed_buttons);
        break;

    case GDK_2BUTTON_PRESS:
#if 0   /* deprecated code: the client will treat the double clicks */
#ifdef DEBUG
        printf ("catch GDK_2BUTTON_PRESS event\n");
#endif

        x = (int) (event->button.x * 100 / gvfbruninfo.zoom_percent);
        y = (int) (event->button.y * 100 / gvfbruninfo.zoom_percent);

        button = event->button.button;

        if (button == 2) {
            /* middle */
            button = 4;
        }
        else if (button == 3) {
            /* right */
            button = 2;
        }
        SendMouseData (x, y, button);
#endif  /* deprecated code */
        break;

    case GDK_MOTION_NOTIFY:
        x = (int) (event->button.x * 100 / gvfbruninfo.zoom_percent);
        y = (int) (event->button.y * 100 / gvfbruninfo.zoom_percent);

        SendMouseData (x, y, pressed_buttons);
        break;

    case GDK_KEY_PRESS:
        keycode = event->key.keyval;
        if (is_keypad_data (keycode)) {
            IsKeypadData++;
        }

        /* F11 for full screen */
        if (gvfbruninfo.o_fullscreen
                && keycode == GDK_F11
                && event->key.state & GDK_SHIFT_MASK) {
            FullScreen ();
        }

        msg_log ("got GDK_KEY_PRESS %x\n", keycode);
#if 0
        if (gtk_im_context_filter_keypress (gvfbruninfo.im_context, &event->key)) {
            return TRUE;
        }
#endif
        if (keycode & 0x1000000) {
            char code[2 + 1];
            char utf8[3 + 1];

            code[0] = (keycode >> 8) & 0xFF;
            code[1] = keycode & 0xFF;
            code[2] = '\0';

            /* convert unicode to utf-8 */
            if (!ISUTF8 (code[0])) {
                /* unicode */
                memset (utf8, 0x00, sizeof (utf8));

                /* unicode to utf-8 */
                keycode = keycode & 0xFFFF;

                utf8[0] = 0xE0 | (keycode >> 12);
                utf8[1] = 0x80 | ((keycode >> 6) & 0x3F);
                utf8[2] = 0x80 | (keycode & 0x3F);
                utf8[3] = '\0';

                SendIMData (utf8);
            }
            else {
                SendIMData (code);
            }

            return TRUE;
        }

        /* control key press */
        SetCtrlKey (keycode);
        SendKeyboardData (keycode, 1, 0);
        return TRUE;
        break;

    case GDK_KEY_RELEASE:
        msg_log ("catch GDK_KEY_RELEASE event\n");
        SendKeyboardData (event->key.keyval, 0, 0);
        return TRUE;
        break;

    default:
        /* do nothing */
        break;
    }

    return FALSE;
}

void ScaleImage (int x, int y, int width, int height)
{
#if 0
    if (gvfbruninfo.fastmode == TRUE) {
        gdk_pixbuf_scale (gvfbruninfo.pixbuf_r, gvfbruninfo.pixbuf_s, x, y,
                          width, height, 0.0, 0.0,
                          (double) gvfbruninfo.zoom_percent / 100,
                          (double) gvfbruninfo.zoom_percent / 100, GDK_INTERP_NEAREST);
    }
    else {
        gdk_pixbuf_scale (gvfbruninfo.pixbuf_r, gvfbruninfo.pixbuf_s, x, y,
                          width, height, 0.0, 0.0,
                          (double) gvfbruninfo.zoom_percent / 100,
                          (double) gvfbruninfo.zoom_percent / 100, GDK_INTERP_TILES);
    }
#endif
}

void *CheckEventThread (void *args)
{
    int ret;
    int err_flag = 0;

    GThread *drawthread = NULL;

    fd_set rfds, efds;

    int type, size, temp;
    /* mouse position */
    int x, y;
    /* buffer */
    char buffer[1024];

    struct timeval tv;

    GVFBRUNINFO *runinfo;
    GVFBHeader *hdr;

    runinfo = (GVFBRUNINFO *) args;

    assert (runinfo != NULL);

    /* get gvfb header */
    hdr = runinfo->hdr;

    assert (hdr != NULL);

    /* show the message */
    gdk_threads_enter ();
    if (has_err) {
        /* show error message */
        msg_dialog = gtk_message_dialog_new (GTK_WINDOW (gvfbruninfo.window),
                                             0,
                                             GTK_MESSAGE_ERROR, GTK_BUTTONS_OK,
                                             err_msg, NULL);

        gtk_dialog_run (GTK_DIALOG (msg_dialog));
        gtk_widget_destroy (msg_dialog);
    }
    gdk_threads_leave ();

    /* create draw dirty thread */
    drawthread = g_thread_new ("draw-dirty", DrawDirtyThread, args);
#if 0
    g_thread_set_priority (drawthread, G_THREAD_PRIORITY_LOW);
#endif

    /* running */
    while (runinfo->running) {
        int maxfd = runinfo->sockfd;

        FD_ZERO (&rfds);
        FD_SET (runinfo->sockfd, &rfds);
        FD_ZERO (&efds);
        FD_SET (runinfo->sockfd, &efds);

#ifndef WIN32
        if (runinfo->vvlc_sockfd >= 0) {
            if (!CheckAsyncOperation (runinfo->vvlc_sockfd)) {
                close (runinfo->vvlc_sockfd);
                runinfo->vvlc_sockfd = -1;
            }
        }

        if (runinfo->vvls_sockfd >= 0) {
            FD_SET (runinfo->vvls_sockfd, &rfds);
            if (runinfo->vvls_sockfd > maxfd)
                maxfd = runinfo->vvls_sockfd;
        }

        if (runinfo->vvlc_sockfd >= 0) {
            FD_SET (runinfo->vvlc_sockfd, &rfds);
            if (runinfo->vvlc_sockfd > maxfd)
                maxfd = runinfo->vvlc_sockfd;
        }
#endif /* !WIN32 */

        /* 0.1s */
        tv.tv_sec = 0;
        tv.tv_usec = 10 * 1000;

        ret = select (maxfd + 1, &rfds, NULL, &efds, &tv);
        msg_out (LEVEL_0, "retval of select: %d\n", ret);

        if (ret < 0) {
#ifndef WIN32
            if (errno == EINTR) {
                continue;
            }
#endif /* !WIN32 */

            msg_out (LEVEL_0, "Failed to select.");
            err_flag = 1;
            runinfo->running = FALSE;
            break;
        }
        else if (ret == 0) {
            continue;
        }

#ifndef WIN32
        if (runinfo->vvls_sockfd >= 0 &&
                FD_ISSET (runinfo->vvls_sockfd, &rfds)) {
            struct sockaddr_un client_address;
            socklen_t client_len;
            int fd;

            fd = accept (runinfo->vvls_sockfd,
                (struct sockaddr *)&client_address, &client_len);
            if (fd < 0) {
                msg_out (LEVEL_0, "Failed to accept connection request");
            }
            else if (runinfo->vvlc_sockfd < 0) {
                runinfo->vvlc_sockfd = fd;
                msg_out (LEVEL_0, "Accepted VVLC connection request: %d", fd);
            }
            else {
                close (fd);
            }

            continue;
        }
        else if (runinfo->vvlc_sockfd >= 0 &&
                FD_ISSET (runinfo->vvlc_sockfd, &rfds)) {
            if (!HandleVvlcRequest (runinfo->vvlc_sockfd)) {
                close (runinfo->vvlc_sockfd);
                runinfo->vvlc_sockfd = -1;
            }

            continue;
        }
        else if (!FD_ISSET (runinfo->sockfd, &rfds)) {
            continue;
        }
        else if (FD_ISSET (runinfo->sockfd, &efds)) {
            msg_out (LEVEL_0, "exception on fd: %d", runinfo->sockfd);
            runinfo->running = FALSE;
            break;
        }
#endif /* !WIN32 */

        ret =
            Recv (runinfo->sockfd, (unsigned char *) &type, sizeof (type),
                  MSG_TRUNC);

        if (ret != sizeof (type)) {
            if (ret < 0) {
                msg_out (LEVEL_0, "recv type error.");
                err_flag = 1;
            }
            runinfo->running = FALSE;
            break;
        }

        switch (type) {
        case CAPTION_TYPE:
            ret =
                Recv (runinfo->sockfd, (unsigned char *) &size, sizeof (size),
                      MSG_TRUNC);

            if (ret != sizeof (size)) {
                if (ret < 0) {
                    msg_out (LEVEL_0, "recv caption size error.");

                    err_flag = 1;
                }

                runinfo->running = FALSE;
                break;
            }

            if (size <= 0) {
                msg_out (LEVEL_0, "caption size error.");

                err_flag = 1;

                runinfo->running = FALSE;
                break;
            }

            temp =
                Recv (runinfo->sockfd, (unsigned char *) &buffer[0],
                      (size >
                       (sizeof (buffer) - 1) ? (sizeof (buffer) - 1) : size),
                      MSG_TRUNC);

            if (temp <= 0) {
                msg_out (LEVEL_0, "recv caption error.");
                err_flag = 1;
                runinfo->running = FALSE;
                break;
            }

            buffer[temp] = '\0';

            gdk_threads_enter ();
            gtk_window_set_title (GTK_WINDOW (runinfo->window), buffer);
            gdk_threads_leave ();
            break;

        case SHOW_HIDE_TYPE:
            temp = 0;

            ret =
                Recv (runinfo->sockfd, (unsigned char *) &temp, sizeof (int),
                      MSG_TRUNC);
            if (ret != sizeof (int)) {
                if (ret < 0) {
                    msg_out (LEVEL_0, "recv show hide error.");
                    err_flag = 1;
                }

                runinfo->running = FALSE;
                break;
            }

            gdk_threads_enter ();
            ShowHide (temp);
            gdk_threads_leave ();
            break;

        case MOUSE_TYPE:
            ret =
                Recv (runinfo->sockfd, (unsigned char *) &x, sizeof (int),
                      MSG_TRUNC);

            if (ret != sizeof (int)) {
                if (ret < 0) {
                    msg_out (LEVEL_0, "recv mouse postion error.");
                    err_flag = 1;
                }

                runinfo->running = FALSE;
                break;
            }

            ret =
                Recv (runinfo->sockfd, (unsigned char *) &y, sizeof (int),
                      MSG_TRUNC);

            if (ret != sizeof (int)) {
                if (ret < 0) {
                    msg_out (LEVEL_0, "recv mouse postion error.");
                    err_flag = 1;
                }

                runinfo->running = FALSE;
                break;
            }

            /* fix position */
            x = max (x, 0);
            y = max (y, 0);

            x = min (x, hdr->width);
            y = min (y, hdr->height);

            gdk_threads_enter ();

            SetMouseXY (x, y);

            gdk_threads_leave ();
            break;

        case IME_TYPE:
            msg_log ("got IME_TYPE\n");
            ret =
                Recv (runinfo->sockfd, (unsigned char *) &temp, sizeof (int),
                      MSG_TRUNC);

            if (ret != sizeof (int)) {
                if (ret < 0) {
                    msg_out (LEVEL_0, "recv ime type error.");

                    err_flag = 1;
                }

                runinfo->running = FALSE;
                break;
            }
            break;

        case CMD_TYPE:
            ret =
                Recv (runinfo->sockfd, (unsigned char *) &temp, sizeof (int),
                      MSG_TRUNC);

            msg_log ("got CMD_TYPE: retval (%d), cmd (%d)\n", ret, temp);

            if (ret != sizeof (int)) {
                if (ret < 0) {
                    msg_out (LEVEL_0, "recv ime type error.");
                    err_flag = 1;
                }

                runinfo->running = FALSE;
                break;
            }
            else if (temp == XVFB_CMD_QUIT) {
                runinfo->running = FALSE;
                break;
            }
            break;

        default:
            /* do nothing */
            break;
        } /* end switch */

    } /* end while */

    msg_out (LEVEL_0, "waiting for the quit of draw thread");

    g_thread_join (drawthread);

    gdk_threads_enter ();

    if (gtk_main_level () > 0) {
        gtk_main_quit ();
    }

    gdk_threads_leave ();

    return NULL;
}

void *DrawDirtyThread (void *args)
{
    gulong msec;
    gulong spf; /* fps */

    gulong start, end;

#ifdef SHOW_FPS
    /* begin debug */
    gulong fps_start, fps_end;
    gulong old_cnt;
    gulong frame;
    /* end debug */
#endif

    GVFBRUNINFO *runinfo;
    GVFBHeader *hdr;

    runinfo = (GVFBRUNINFO *) args;

    assert (runinfo != NULL);

    /* get gvfb header */
    hdr = runinfo->hdr;

    assert (hdr != NULL);

    spf = 1000 / runinfo->refresh_rate;

#ifdef SHOW_FPS
    /* begin debug */
    old_cnt = 0;
    frame = 0;
    fps_start = GVFBGetCurrentTime ();
    /* end debug */
#endif

    while (runinfo->running) {
        assert (runinfo->refresh_rate > 0);

#ifdef SHOW_FPS
        /* begin debug */
        fps_end = GVFBGetCurrentTime ();
        if (((fps_end - fps_start) / 10000) > old_cnt) {
            old_cnt = (fps_end - fps_start) / 10000;
            printf ("current time : %ld, current frame : %ld, fps : %ld\n",
                    fps_end - fps_start, frame,
                    frame * 1000 / (fps_end - fps_start));

            old_cnt = 0;
            frame = 0;
            fps_start = fps_end;
        }
        frame++;
        /* end debug */
#endif

        spf = 1000 / runinfo->refresh_rate;

        if (hdr->dirty != 0 || (gvfbruninfo.graph_with_alpha && gvfbruninfo.video_layer_mode)) {
            start = GVFBGetCurrentTime ();

            gdk_threads_enter ();

            draw_dirty_rect ();

            gdk_threads_leave ();

            end = GVFBGetCurrentTime ();

            msec = ((end - start) < (spf - 2)) ? (spf - (end - start)) : 2;

            msec *= 1000;

            g_usleep (msec);

            continue;
        }

        msec = spf * 1000;

        g_usleep (msec);
    }

    return NULL;
}

/* mark all screen dirty */
void MarkDrawAll (void)
{
    GVFBHeader *hdr;

    /* get gvfb header */
    hdr = gvfbruninfo.hdr;

    Lock ();

    set_dirty (1, 0, 0, hdr->width, hdr->height);

    UnLock ();
}

/* save screen into a png image */
/* input filename */
void SaveImage (const char *filename)
{
    GdkPixbuf *pixbuf = NULL;
    GVFBHeader *hdr;

    /* get gvfb header */
    hdr = gvfbruninfo.hdr;

    assert (filename != NULL);

    pixbuf = gdk_pixbuf_new_from_data (gvfbruninfo.pixel_data,
                                       GDK_COLORSPACE_RGB, TRUE, 8,
                                       hdr->width, hdr->height, hdr->width * 4,
                                       NULL, NULL);

    if (pixbuf == NULL) {
        return;
    }

    gdk_pixbuf_save (pixbuf, filename, "png", NULL,
                     "tEXt::Software", "testpixbuf-save", NULL);

    g_object_unref (pixbuf);
}

static void set_palette (void)
{
    GVFBHeader *hdr;
    GVFBPalEntry *pal;

    int i, len;

    /* get gvfb header */
    hdr = gvfbruninfo.hdr;

    pal = (GVFBPalEntry *) ((unsigned char *) hdr + hdr->palette_offset);

    len = 1 << hdr->depth;

    for (i = 0; i < len; i++) {
        gvfbruninfo.palette[i] =
            (pal->a << 24) | (pal->r << 16) | (pal->g << 8) | pal->b;

        pal++;
    }
}

/* check is keypad data */
static gboolean is_keypad_data (unsigned int keycode)
{
    if ((keycode >= 0xffad) && (keycode <= 0xffb9)) {
        return TRUE;
    }

    return FALSE;
}

/* set dirty rect */
static void set_dirty (int flag, int l, int t, int r, int b)
{
    GVFBHeader *hdr;

    /* get gvfb header */
    hdr = gvfbruninfo.hdr;

    hdr->dirty = flag;

    hdr->dirty_rc_l = l;
    hdr->dirty_rc_t = t;
    hdr->dirty_rc_r = r;
    hdr->dirty_rc_b = b;
}

/* set capslock state */
static void set_capslock (void)
{
    if (IsCapslockOn ()) {
        SetCtrlKey (GDK_Caps_Lock);

        SendKeyboardData (GDK_Caps_Lock, 1, 0);
        SendKeyboardData (GDK_Caps_Lock, 0, 0);
    }
}

/* get buffer */
static void get_pixbuf_data (int x, int y, int width, int height)
{
    int i, j, k, bpp, index = 0;
    unsigned char *start_at;
    unsigned char temp;
    unsigned int pixel_data = 0;

    GVFBHeader *hdr;

    /* get gvfb header */
    hdr = gvfbruninfo.hdr;

    bpp = (hdr->depth + 7) / 8;

    if (hdr->palette_changed) {
        set_palette ();
        hdr->palette_changed = 0;
    }

    switch (hdr->depth) {
    case 1:
        /* depth = 1 */
        start_at =
            (unsigned char *) hdr + hdr->fb_offset + hdr->pitch * y + x / 8;

        if ((x % 8) != 0) {
            width = width + x % 8;
            x = x - x % 8;
        }

        if ((width % 8) != 0) {
            width = width + width % 8;
        }

        if ((width + x) > hdr->width) {
            width = hdr->width - x;
        }

        index = (hdr->width * y + x) * 4;

        if (gvfbruninfo.hdr->MSBLeft) {
            for (j = 0; j < height; j++) {
                for (i = 0; i < width * 4; i += 32) {
                    temp = *(start_at + i / 32);

                    for (k = 0; k < 32; k += 4) {
                        pixel_data = gvfbruninfo.palette[temp >> 7];

                        gvfbruninfo.pixel_data[index + i + k] = pixel_data;
                        gvfbruninfo.pixel_data[index + i + k + 1] = pixel_data;
                        gvfbruninfo.pixel_data[index + i + k + 2] = pixel_data;
                        gvfbruninfo.pixel_data[index + i + k + 3] = 0xff;

                        temp <<= 1;
                    }   /* end for */
                }       /* end for */

                start_at += hdr->pitch;
                index += hdr->width * 4;
            }   /* end for */
        }
        else {
            for (j = 0; j < height; j++) {
                for (i = 0; i < width * 4; i += 32) {
                    temp = *(start_at + i / 32);

                    for (k = 0; k < 32; k += 4) {
                        pixel_data = gvfbruninfo.palette[temp & 0x1];

                        gvfbruninfo.pixel_data[index + i + k] = pixel_data;
                        gvfbruninfo.pixel_data[index + i + k + 1] = pixel_data;
                        gvfbruninfo.pixel_data[index + i + k + 2] = pixel_data;
                        gvfbruninfo.pixel_data[index + i + k + 3] = 0xff;

                        temp >>= 1;
                    }   /* end for */
                }       /* end for */

                start_at += hdr->pitch;
                index += hdr->width * 4;
            }   /* end for */
        }       /* end if */
        break;
    case 2:
        /* depth = 2 */
        start_at =
            (unsigned char *) hdr + hdr->fb_offset + hdr->pitch * y + x / 4;

        if ((x % 4) != 0) {
            width = width + x % 4;
            x = x - x % 4;
        }

        if ((width % 4) != 0) {
            width = width + width % 4;
        }

        if ((width + x) > hdr->width) {
            width = hdr->width - x;
        }

        index = (hdr->width * y + x) * 4;

        if (gvfbruninfo.hdr->MSBLeft) {
            for (j = 0; j < height; j++) {
                for (i = 0; i < width * 4; i += 16) {
                    temp = *(start_at + i / 16);

                    for (k = 0; k < 16; k += 4) {
                        pixel_data = gvfbruninfo.palette[temp >> 6];

                        gvfbruninfo.pixel_data[index + i + k] =
                            (pixel_data >> 16) & 0xff;
                        gvfbruninfo.pixel_data[index + i + k + 1] =
                            (pixel_data >> 8) & 0xff;
                        gvfbruninfo.pixel_data[index + i + k + 2] =
                            pixel_data & 0xff;
                        gvfbruninfo.pixel_data[index + i + k + 3] = 0xff;

                        temp <<= 2;
                    }
                }

                start_at += hdr->pitch;
                index += hdr->width * 4;
            }
        }
        else {
            for (j = 0; j < height; j++) {
                for (i = 0; i < width * 4; i += 16) {
                    temp = *(start_at + i / 16);

                    for (k = 0; k < 16; k += 4) {
                        pixel_data = gvfbruninfo.palette[temp & 0x03];

                        gvfbruninfo.pixel_data[index + i + k] =
                            (pixel_data >> 16) & 0xff;
                        gvfbruninfo.pixel_data[index + i + k + 1] =
                            (pixel_data >> 8) & 0xff;
                        gvfbruninfo.pixel_data[index + i + k + 2] =
                            pixel_data & 0xff;
                        gvfbruninfo.pixel_data[index + i + k + 3] = 0xff;

                        temp >>= 2;
                    }   /* end for */
                }       /* end for */

                start_at += hdr->pitch;
                index += hdr->width * 4;
            }   /* end for */
        }       /* end if */
        break;
    case 4:
        /* depth = 4 */
        start_at =
            (unsigned char *) hdr + hdr->fb_offset + hdr->pitch * y + x / 2;

        if ((x % 2) != 0) {
            x--;
            width++;
        }

        if ((width % 2) != 0) {
            width++;
        }

        if ((width + x) > hdr->width) {
            width = hdr->width - x;
        }

        index = (hdr->width * y + x) * 4;

        if (gvfbruninfo.hdr->MSBLeft) {
            for (j = 0; j < height; j++) {
                for (i = 0; i < width * 4; i += 8) {
                    temp = *(start_at + i / 8);

                    for (k = 0; k < 8; k += 4) {
                        pixel_data = gvfbruninfo.palette[temp >> 4];

                        gvfbruninfo.pixel_data[index + i + k] =
                            (pixel_data >> 16) & 0xff;
                        gvfbruninfo.pixel_data[index + i + k + 1] =
                            (pixel_data >> 8) & 0xff;
                        gvfbruninfo.pixel_data[index + i + k + 2] =
                            pixel_data & 0xff;
                        gvfbruninfo.pixel_data[index + i + k + 3] = 0xff;

                        temp <<= 4;
                    }
                }

                start_at += hdr->pitch;
                index += hdr->width * 4;
            }
        }
        else {
            for (j = 0; j < height; j++) {
                for (i = 0; i < width * 4; i += 8) {
                    temp = *(start_at + i / 8);

                    for (k = 0; k < 8; k += 4) {
                        pixel_data = gvfbruninfo.palette[temp & 0x0f];

                        gvfbruninfo.pixel_data[index + i + k] =
                            (pixel_data >> 16) & 0xff;
                        gvfbruninfo.pixel_data[index + i + k + 1] =
                            (pixel_data >> 8) & 0xff;
                        gvfbruninfo.pixel_data[index + i + k + 2] =
                            pixel_data & 0xff;
                        gvfbruninfo.pixel_data[index + i + k + 3] = 0xff;

                        temp >>= 4;
                    }
                }

                start_at += hdr->pitch;
                index += hdr->width * 4;
            }
        }
        break;
    case 8:
        /* depth = 8 */
        index = (hdr->width * y + x) * 4;
        start_at =
            (unsigned char *) hdr + hdr->fb_offset + hdr->pitch * y + bpp * x;

        for (j = 0; j < height; j++) {
            for (i = 0; i < width * 4; i += 4) {
                temp = *(start_at + i / 4);

                pixel_data = gvfbruninfo.palette[temp];

                gvfbruninfo.pixel_data[index + i] = (pixel_data >> 16) & 0xff;
                gvfbruninfo.pixel_data[index + i + 1] = (pixel_data >> 8) & 0xff;
                gvfbruninfo.pixel_data[index + i + 2] = pixel_data & 0xff;
                gvfbruninfo.pixel_data[index + i + 3] = 0xff;
            }

            start_at += hdr->pitch;
            index += hdr->width * 4;
        }
        break;
    case 16:
        /* depth = 16 */
        index = (hdr->width * y + x) * 4;
        start_at =
            (unsigned char *) hdr + hdr->fb_offset + hdr->pitch * y + bpp * x;

        for (j = 0; j < height; j++) {
            for (i = 0; i < width * 4; i += 4) {
                pixel_data = *((unsigned short *) start_at + i / 4);

                for (k = 0; k < 4; k++) {
                    temp = (pixel_data & s_mask[k]) >> s_shift[k];
                    gvfbruninfo.pixel_data[index + i + k] =
                        (unsigned char) (s_color_map_table[k][temp]);
                }
            }

            start_at += hdr->pitch;
            index += hdr->width * 4;
        }
        break;
    case 32:
        /* depth = 32 */
        if (is_default_color) {
            start_at = (unsigned char *) hdr + hdr->fb_offset;
            memcpy (gvfbruninfo.pixel_data, start_at, hdr->pitch * hdr->height);
            break;
        }

        index = (hdr->width * y + x) * 4;
        start_at =
            (unsigned char *) hdr + hdr->fb_offset + hdr->pitch * y + bpp * x;

        for (j = 0; j < height; j++) {
            for (i = 0; i < width * 4; i += 4) {
                pixel_data = *((unsigned int *) start_at + i / 4);

                for (k = 0; k < 4; k++) {
                    temp = (pixel_data & s_mask[k]) >> s_shift[k];
                    gvfbruninfo.pixel_data[index + i + k] =
                        (unsigned char) (s_color_map_table[k][temp]);
                }
            }

            start_at += hdr->pitch;
            index += hdr->width * 4;
        }
        break;
    default:
        break;
    }   /* end switch */
}

static void draw_dirty_rect (void)
{
    GVFBRECT dirty;
    GVFBRECT draw_rect;
    gint fix_l, fix_t, fix_r, fix_b;
    GVFBHeader *hdr;

    /* get gvfb header */
    hdr = gvfbruninfo.hdr;

    Lock ();

    fix_l = max (hdr->dirty_rc_l, 0);
    fix_t = max (hdr->dirty_rc_t, 0);
    fix_r = min (hdr->dirty_rc_r, hdr->width);
    fix_b = min (hdr->dirty_rc_b, hdr->height);

    set_dirty (0, 0, 0, 0, 0);

    get_pixbuf_data (fix_l, fix_t, (fix_r - fix_l), (fix_b - fix_t));

    if (gvfbruninfo.zoom_percent != 100) {
        fix_l = fix_l * gvfbruninfo.zoom_percent / 100;
        fix_t = fix_t * gvfbruninfo.zoom_percent / 100;

        fix_r = (((fix_r * gvfbruninfo.zoom_percent) % 100) == 0) ?
            (fix_r * gvfbruninfo.zoom_percent / 100) :
            ((fix_r + 1) * gvfbruninfo.zoom_percent / 100);
        fix_b = (((fix_b * gvfbruninfo.zoom_percent) % 100) == 0) ?
            (fix_b * gvfbruninfo.zoom_percent / 100) :
            ((fix_b + 1) * gvfbruninfo.zoom_percent / 100);
    }

    /* draw rect */
    /* fix dirty rect */
    if (!GetDrawRect (&draw_rect)) {
        goto ret;
    }

    if (gvfbruninfo.graph_with_alpha && gvfbruninfo.video_layer_mode) {
        dirty.x = draw_rect.x;
        dirty.y = draw_rect.y;
        dirty.w = draw_rect.w;
        dirty.h = draw_rect.h;
    }
    else {
        dirty.x = max (fix_l, draw_rect.x);
        dirty.y = max (fix_t, draw_rect.y);
        dirty.w = min (fix_r, (draw_rect.x + draw_rect.w)) - dirty.x;
        dirty.h = min (fix_b, (draw_rect.y + draw_rect.h)) - dirty.y;

        /* check dirty rect */
        if ((dirty.x < 0) || (dirty.y < 0) || (dirty.w <= 0) || (dirty.h <= 0)) {
            goto ret;
        }
    }

    if (gvfbruninfo.zoom_percent != 100) {
        ScaleImage (dirty.x, dirty.y, dirty.w, dirty.h);
    }

    DrawImage (dirty.x, dirty.y, dirty.w, dirty.h);

ret:
    UnLock ();
}

