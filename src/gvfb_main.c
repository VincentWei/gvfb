/*
** $Id: gvfb_main.c 276 2011-02-17 03:28:00Z xbwang $
**
** gvfb.c: Main modules.
**
** Copyright (C) 2009 Feynman Software.
**
** All rights reserved by Feynman Software.
**
** Create data: 2009-12-18
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include <sys/types.h>
#include <sys/stat.h>

/* for gtk */
#include <gtk/gtk.h>
#include <gdk/gdkkeysyms.h>

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
/* Read data from buffer and save them to PixelData */
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
    gvfbruninfo.pixbuf_s = NULL;

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
    gvfbruninfo.Zoom = 100;

    /* set screen size */
    gvfbruninfo.screen_w = 0;
    gvfbruninfo.screen_h = 0;

    gvfbruninfo.shmid = -1;
    gvfbruninfo.PixelData = NULL;

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
    gvfbruninfo.RefreshRate = OPT_REFRESHRATE;
    gvfbruninfo.tile_pixmap = NULL;
}

void UnInitRunInfo ()
{
    if (gvfbruninfo.pixbuf_s != NULL) {
        g_object_unref (gvfbruninfo.pixbuf_s);
    }
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
    int i, k, data_size;
    unsigned char t;

    int nr_entry = 0;

    /* malloc Pixel buffer */
    gvfbruninfo.PixelData = (unsigned char *) malloc (width * height * 4);

    if (gvfbruninfo.PixelData == NULL) {
        msg_out (LEVEL_0, "cannot malloc memory.(PixelData)");

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

    /* Calculate the size of pixel data */
    data_size =
        pitch * height + sizeof (GVFBHeader) + nr_entry * sizeof (GVFBPalEntry);

    /* init header */
    hdr = (GVFBHeader *) CreateShareMemory (ppid, data_size);

    if ((int) hdr == -1) {
        msg_out (LEVEL_0, "CreateShareMemory error.");

        /* free PixelData */
        free (gvfbruninfo.PixelData);
        gvfbruninfo.PixelData = NULL;

        return -1;
    }

    /* save gvfb header */
    gvfbruninfo.hdr = hdr;

    hdr->info_size = 0;
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

            /* free PixelData */
            free (gvfbruninfo.PixelData);
            gvfbruninfo.PixelData = NULL;

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
            gvfbruninfo.WithAlpha = TRUE;
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

        /* free PixelData */
        free (gvfbruninfo.PixelData);
        gvfbruninfo.PixelData = NULL;

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

        /* free PixelData */
        free (gvfbruninfo.PixelData);
        gvfbruninfo.PixelData = NULL;

        return -1;
    }
#endif

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

            /* free PixelData */
            free (gvfbruninfo.PixelData);
            gvfbruninfo.PixelData = NULL;

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

        /* free PixelData */
        free (gvfbruninfo.PixelData);
        gvfbruninfo.PixelData = NULL;

        free (gvfbruninfo.palette);
        gvfbruninfo.palette = NULL;

        return -1;
    }

    return 0;
}

/* close sockfd, free memory etc. */
void UnInit ()
{
    UnInitLock (0);

    if (gvfbruninfo.sockfd != -1) {
#ifdef WIN32
        closesocket (gvfbruninfo.sockfd);

        WSACleanup ();
#else
        close (gvfbruninfo.sockfd);
#endif
        gvfbruninfo.sockfd = -1;
    }

    if (gvfbruninfo.PixelData != NULL) {
        /* free PixelData */
        free (gvfbruninfo.PixelData);
        gvfbruninfo.PixelData = NULL;
    }

    /* Destroy Share memorry */
    DestroyShareMemory ();

    if (gvfbruninfo.palette != NULL) {
        free (gvfbruninfo.palette);
        gvfbruninfo.palette = NULL;
    }
}

/* main event */
gboolean EventProc (GtkWidget * window, GdkEvent * event)
{
    int x, y, button;
    unsigned int keycode;
    static int button_press = 0;

    switch (event->type) {
    case GDK_BUTTON_PRESS:
        /* mouse button press */
#ifdef DEBUG
        printf ("catch GDK_BUTTON_PRESS event\n");
#endif

        x = (int) (event->button.x * 100 / gvfbruninfo.Zoom);
        y = (int) (event->button.y * 100 / gvfbruninfo.Zoom);

        button = event->button.button;

        if (button == 1) {
            /* left */
            button_press = 1;
        }
        else if (button == 2) {
            /* middle */
            button = 4;
        }
        else if (button == 3) {
            /* right */
            button = 2;
        }

        SendMouseData (x, y, button);

        break;
    case GDK_BUTTON_RELEASE:
        /* mouse button release */
#ifdef DEBUG
        printf ("catch GDK_BUTTON_RELEASE event\n");
#endif

        x = (int) (event->button.x * 100 / gvfbruninfo.Zoom);
        y = (int) (event->button.y * 100 / gvfbruninfo.Zoom);

        button = event->button.button;

        if (button == 1) {
            /* the left mouse release */
            button_press = 0;
        }

        SendMouseData (x, y, 0);

        break;
    case GDK_2BUTTON_PRESS:
        /*  */
#ifdef DEBUG
        printf ("catch GDK_2BUTTON_PRESS event\n");
#endif

        x = (int) (event->button.x * 100 / gvfbruninfo.Zoom);
        y = (int) (event->button.y * 100 / gvfbruninfo.Zoom);

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

        break;
    case GDK_MOTION_NOTIFY:
        /* */
#ifdef DEBUG
        //printf ("catch GDK_MOTION_NOTIFY event\n");
#endif

        x = (int) (event->button.x * 100 / gvfbruninfo.Zoom);
        y = (int) (event->button.y * 100 / gvfbruninfo.Zoom);

        /* if press and hold down the left mouse */
        if (button_press) {
            /* left */
            button = 1;
        }
        else {
            /* none */
            button = 0;
        }

        SendMouseData (x, y, button);

        break;
    case GDK_KEY_PRESS:
#ifdef DEBUG
        printf ("catch GDK_KEY_PRESS event\n");
#endif

        keycode = event->key.keyval;

        if (is_keypad_data (keycode)) {
            IsKeypadData++;
        }

        /* F11 for full screen */
        if (gvfbruninfo.o_fullscreen && (keycode == GDK_F11)) {
            FullScreen ();
        }

#ifdef DEBUG
        printf ("got GDK_KEY_PRESS %x\n", keycode);
#endif
#if 0
        if (gtk_im_context_filter_keypress (gvfbruninfo.IMContext, &event->key)) {
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
#ifdef DEBUG
        printf ("catch GDK_KEY_RELEASE event\n");
#endif

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
    if (gvfbruninfo.fastmode == TRUE) {
        gdk_pixbuf_scale (gvfbruninfo.pixbuf_r, gvfbruninfo.pixbuf_s, x, y,
                          width, height, 0.0, 0.0,
                          (double) gvfbruninfo.Zoom / 100,
                          (double) gvfbruninfo.Zoom / 100, GDK_INTERP_NEAREST);
    }
    else {
        gdk_pixbuf_scale (gvfbruninfo.pixbuf_r, gvfbruninfo.pixbuf_s, x, y,
                          width, height, 0.0, 0.0,
                          (double) gvfbruninfo.Zoom / 100,
                          (double) gvfbruninfo.Zoom / 100, GDK_INTERP_TILES);
    }
}

void *CheckEventThread (void *args)
{
    int ret;
    int err_flag = 0;

    GThread *drawthread = NULL;

    fd_set fds;

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
    drawthread = g_thread_create (DrawDirtyThread, args, TRUE, NULL);
    g_thread_set_priority (drawthread, G_THREAD_PRIORITY_LOW);

    /* running */
    while (runinfo->running) {
        FD_ZERO (&fds);
        FD_SET (runinfo->sockfd, &fds);

        /* 1s */
        tv.tv_sec = 0;
        tv.tv_usec = 100 * 1000;

        ret = select (runinfo->sockfd + 1, &fds, NULL, NULL, &tv);

        if (ret < 0) {
#ifdef WIN32
#else
            if (errno == EINTR) {
                continue;
            }
#endif

            msg_out (LEVEL_0, "select.");

            err_flag = 1;

            break;
        }
        else if (ret == 0) {
            continue;
        }

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
#ifdef DEBUG
            printf ("IME_TYPE\n");
#endif

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
        default:
            /* do nothing */
            break;
        }       /* end switch */
    }   /* end while */

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

    spf = 1000 / runinfo->RefreshRate;

#ifdef SHOW_FPS
    /* begin debug */
    old_cnt = 0;
    frame = 0;
    fps_start = GVFBGetCurrentTime ();
    /* end debug */
#endif

    while (runinfo->running) {
        assert (runinfo->RefreshRate > 0);

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

        spf = 1000 / runinfo->RefreshRate;

        if (hdr->dirty != 0) {
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

    pixbuf = gdk_pixbuf_new_from_data (gvfbruninfo.PixelData,
                                       GDK_COLORSPACE_RGB, TRUE, 8,
                                       hdr->width, hdr->height, hdr->width * 4,
                                       NULL, NULL);

    if (pixbuf == NULL) {
        return;
    }

    gdk_pixbuf_save (pixbuf, filename, "png", NULL,
                     "tEXt::Software", "testpixbuf-save", NULL);

    gdk_pixbuf_unref (pixbuf);
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

#ifdef DEBUG
//    printf ("get_pixbuf_data : %d %d %d %d\n", x, y, width, height);
#endif

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

                        gvfbruninfo.PixelData[index + i + k] = pixel_data;
                        gvfbruninfo.PixelData[index + i + k + 1] = pixel_data;
                        gvfbruninfo.PixelData[index + i + k + 2] = pixel_data;
                        gvfbruninfo.PixelData[index + i + k + 3] = 0xff;

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

                        gvfbruninfo.PixelData[index + i + k] = pixel_data;
                        gvfbruninfo.PixelData[index + i + k + 1] = pixel_data;
                        gvfbruninfo.PixelData[index + i + k + 2] = pixel_data;
                        gvfbruninfo.PixelData[index + i + k + 3] = 0xff;

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

                        gvfbruninfo.PixelData[index + i + k] =
                            (pixel_data >> 16) & 0xff;
                        gvfbruninfo.PixelData[index + i + k + 1] =
                            (pixel_data >> 8) & 0xff;
                        gvfbruninfo.PixelData[index + i + k + 2] =
                            pixel_data & 0xff;
                        gvfbruninfo.PixelData[index + i + k + 3] = 0xff;

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

                        gvfbruninfo.PixelData[index + i + k] =
                            (pixel_data >> 16) & 0xff;
                        gvfbruninfo.PixelData[index + i + k + 1] =
                            (pixel_data >> 8) & 0xff;
                        gvfbruninfo.PixelData[index + i + k + 2] =
                            pixel_data & 0xff;
                        gvfbruninfo.PixelData[index + i + k + 3] = 0xff;

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

                        gvfbruninfo.PixelData[index + i + k] =
                            (pixel_data >> 16) & 0xff;
                        gvfbruninfo.PixelData[index + i + k + 1] =
                            (pixel_data >> 8) & 0xff;
                        gvfbruninfo.PixelData[index + i + k + 2] =
                            pixel_data & 0xff;
                        gvfbruninfo.PixelData[index + i + k + 3] = 0xff;

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

                        gvfbruninfo.PixelData[index + i + k] =
                            (pixel_data >> 16) & 0xff;
                        gvfbruninfo.PixelData[index + i + k + 1] =
                            (pixel_data >> 8) & 0xff;
                        gvfbruninfo.PixelData[index + i + k + 2] =
                            pixel_data & 0xff;
                        gvfbruninfo.PixelData[index + i + k + 3] = 0xff;

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

                gvfbruninfo.PixelData[index + i] = (pixel_data >> 16) & 0xff;
                gvfbruninfo.PixelData[index + i + 1] = (pixel_data >> 8) & 0xff;
                gvfbruninfo.PixelData[index + i + 2] = pixel_data & 0xff;
                gvfbruninfo.PixelData[index + i + 3] = 0xff;
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
                    gvfbruninfo.PixelData[index + i + k] =
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
            memcpy (gvfbruninfo.PixelData, start_at, hdr->pitch * hdr->height);
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
                    gvfbruninfo.PixelData[index + i + k] =
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
    gboolean ret;
    GVFBRECT dirty;
    GVFBRECT draw_rect;
    gint fix_l, fix_t, fix_r, fix_b;

    GVFBHeader *hdr;

    /* get gvfb header */
    hdr = gvfbruninfo.hdr;

    Lock ();
#ifdef DEBUG
//    printf ("draw dirty rect : %d %d %d %d\n",
//            hdr->dirty_rc_l, hdr->dirty_rc_t,
//            hdr->dirty_rc_r, hdr->dirty_rc_b);
#endif

    fix_l = max (hdr->dirty_rc_l, 0);
    fix_t = max (hdr->dirty_rc_t, 0);
    fix_r = min (hdr->dirty_rc_r, hdr->width);
    fix_b = min (hdr->dirty_rc_b, hdr->height);

    set_dirty (0, 0, 0, 0, 0);

    get_pixbuf_data (fix_l, fix_t, (fix_r - fix_l), (fix_b - fix_t));

    UnLock ();

    if (gvfbruninfo.Zoom != 100) {
        fix_l = fix_l * gvfbruninfo.Zoom / 100;
        fix_t = fix_t * gvfbruninfo.Zoom / 100;

        fix_r = (((fix_r * gvfbruninfo.Zoom) % 100) == 0) ?
            (fix_r * gvfbruninfo.Zoom / 100) :
            ((fix_r + 1) * gvfbruninfo.Zoom / 100);
        fix_b = (((fix_b * gvfbruninfo.Zoom) % 100) == 0) ?
            (fix_b * gvfbruninfo.Zoom / 100) :
            ((fix_b + 1) * gvfbruninfo.Zoom / 100);
    }

    /* draw rect */
    /* fix dirty rect */
    ret = GetDrawRect (&draw_rect);

    if (ret != TRUE) {
        return;
    }

    dirty.x = max (fix_l, draw_rect.x);
    dirty.y = max (fix_t, draw_rect.y);
    dirty.w = min (fix_r, (draw_rect.x + draw_rect.w)) - dirty.x;
    dirty.h = min (fix_b, (draw_rect.y + draw_rect.h)) - dirty.y;

    /* check dirty rect */
    if ((dirty.x < 0) || (dirty.y < 0) || (dirty.w <= 0) || (dirty.h <= 0)) {
        return;
    }

    if (gvfbruninfo.Zoom != 100) {
        ScaleImage (dirty.x, dirty.y, dirty.w, dirty.h);
    }

    DrawImage (dirty.x, dirty.y, dirty.w, dirty.h);
}
