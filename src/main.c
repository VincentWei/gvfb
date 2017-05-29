/*
** $Id: main.c 275 2011-01-29 03:25:50Z xbwang $
**
** main.c: Analysis the parameters and call MainWndLoop function.
**
** Copyright (C) 2009 Feynman Software.
**
** All rights reserved by Feynman Software.
**
** Create data: 2009-12-17
*/
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>

#include <assert.h>

#include <glib.h>

#ifdef WIN32
#   pragma comment(linker, "/subsystem:\"windows\" /entry:\"mainCRTStartup\"")
#   pragma warning(disable:4996)

#   include "getopt_long.h"
#   include "gvfb_win32.h"
#else
#   include <getopt.h>
#   include "gvfb_linux.h"
#endif

#include "skin.h"
#include "gvfb_main.h"
#include "gvfb_input.h"
#include "gvfb_view.h"
#include "gvfb_errmsg.h"
#include "gvfb_log.h"

void usage (void)
{
    printf ("usage: gvfb\n");
    printf ("    --disable-menubar,     -m   \n");
    printf ("    --disable-fullscreen,  -u   \n");
    printf ("    --disable-fastmode,    -s   \n");
    printf ("    --auto-fullscreen,     -f   \n");
    printf ("    --show-gtk-cursor,     -c   \n");
}

int main (int argc, char *argv[])
{
    int ret;

    /* default screen size and depth */
    int width = 240;
    int height = 320;

    int depth = 32;

    /* process id */
    int ppid = 0;
    int index = 1;

    int format_index = -1;

    char caption[PATH_MAX];
    char skinfile[PATH_MAX] = { '\0' };

    char *color_format = NULL;

    SKINRECT screenrect;

    GThread *checkthread = NULL;

    static struct option long_options[] = {
        {"disable-menubar", 0, 0, 'm'},
        {"disable-fullscreen", 0, 0, 'u'},
        {"disable-fastmode", 0, 0, 's'},
        {"auto-fullscreen", 0, 0, 'f'},
        {"show-gtk-cursor", 0, 0, 'c'},
        {0, 0, 0, 0}
    };

#ifdef DEBUG
    /* print argv */
    {
        int i = 0;
        for (i = 0; i < argc; i++) {
            msg_out (LEVEL_0, "argv[%d]: [%s]", i, argv[i]);
        }
    }
#endif

    InitRunInfo ();

    if (argc == 1) {
        usage ();
        return -1;
    }

    /* gvfb ppid caption 800x600-32bpp.rgb32 skin */
    /* ppid */
    if (argc >= 2) {
        ppid = atoi (argv[index]);

        index++;
    }

    /* caption */
    if (argc >= 3) {
        strcpy (caption, argv[index]);

        index++;
    }
    else {
        strcpy (caption, "GVFB");

        index++;
    }

    /* 800x600-32bpp */
    if (argc >= 4) {
        width = atoi (argv[index]);
        height = atoi (strchr (argv[index], 'x') + 1);
        depth = atoi (strchr (argv[index], '-') + 1);

        if ((color_format = strchr (argv[index], '.')) != NULL) {
            color_format++;
        }

        index++;
    }

    /* Mapping the gtk keycode to minigui scancode */
    InitCodeMap ();

    depth = FixDepth (depth);

    if (depth > 8) {
        format_index = GetColorFormatIndex (depth, color_format);

        if (format_index == -1) {
            has_err = TRUE;
            msgcat (err_msg,
                    "warning, wrong color format, using default.(%dbpp.%s)\n",
                    depth, color_format);
        }
    }
    else {
        if ((color_format != NULL) ||
            ((depth != 1) && (depth != 2) && (depth != 4) && (depth != 8))) {
            has_err = TRUE;
            msgcat (err_msg, "warning, wrong color format.(%dbpp)\n", depth);
        }
    }

    /* skin file */
    if (argc >= 5) {
        /* fix execlp in minigui */
        if ((argv[index][0] != '\0') && (argv[index][0] != '-')) {
            strcpy (skinfile, argv[index]);

            gvfbruninfo.has_skin = TRUE;
        }

        index++;
    }

    for (;;) {
        int c;

        if ((c = getopt_long (argc, argv, "musfc", long_options, NULL)) == -1) {
            break;
        }

        switch (c) {
        case 'm':
            gvfbruninfo.has_menu = FALSE;
            break;
        case 'u':
            gvfbruninfo.o_fullscreen = FALSE;
            break;
        case 's':
            gvfbruninfo.fastmode = FALSE;
            break;
        case 'f':
            gvfbruninfo.full_screen = TRUE;
            break;
        case 'c':
            gvfbruninfo.show_gtk_cursor = TRUE;
            break;
        default:
            msg_out (LEVEL_0, "Unknown option.");
            msgcat (err_msg, "Unknown option.\n");
            has_err = TRUE;

            usage ();
            break;
        }
    }

    /* init thread support */
    if (!g_thread_supported ()) {
        g_thread_init (NULL);
    }

    gdk_threads_init ();

    gdk_threads_enter ();

    /* init gtk */
    ret = gtk_init_check (&argc, &argv);

    if (ret != TRUE) {
        msg_out (LEVEL_0, "gtk_init_check error.");

        return -1;
    }

    /* SetupSignal */
#ifdef WIN32
    /* none */
#else
    SetupSignal ();
#endif

    /* check skin and load skin */
    if (gvfbruninfo.has_skin) {
        ret = InitSkin (skinfile);

        if (ret != 0) {
            msg_out (LEVEL_0, "InitSkin error.(%s)", skinfile);

            msgcat (err_msg, "init skin error, running without skin support.\n"
                    "please check the skin file.(%s)", skinfile);
            has_err = TRUE;

            gvfbruninfo.has_skin = FALSE;
        }
        else {
            GetSkinScreenRect (&screenrect);

            if (((screenrect.right - screenrect.left) <= 0)
                || ((screenrect.bottom - screenrect.top) <= 0)) {
                msg_out (LEVEL_0, "Screen Rect error in Skin File.(%s)",
                         skinfile);

                msgcat (err_msg,
                        "screen rect error in skin file, please check it.(%s)\n",
                        skinfile);
                has_err = TRUE;

                UnInitSkin ();

                gvfbruninfo.has_skin = FALSE;
            }   /* end if */
        }       /* end if */
    }   /* end if */

    if (gvfbruninfo.has_skin) {
        SKIN *pskin;
        SKINKEYITEM *item;

        int i;

        assert (g_gvfbskin != NULL);

        pskin = g_gvfbskin->pskin;

        /* destroy region */
        for (i = 0; i < pskin->skinitem_num; ++i) {
            item = &(pskin->skinkeyitem[i]);

            if (!CheckKeycode (item->keycode)) {
                has_err = TRUE;
                msgcat (err_msg,
                        "warning, %s bad keycode 0x%x in skin file, ignore it.\n",
                        item->name, item->keycode);
            }   /* end if */
        }       /* end for */
    }   /* end if */

    /* Create share memory and init the header */
    if (Init (ppid, width, height, depth, color_format) != 0) {
        assert (gvfbruninfo.PixelData == NULL);

        msg_out (LEVEL_0, "init error.");

        if (gvfbruninfo.has_skin) {
            UnInitSkin ();
        }

        UnInitRunInfo ();

        return -1;
    }

    /* Create window and show it */
    gvfbruninfo.window = CreateGVFBWindow (width, height, depth, caption);

    if (gvfbruninfo.window == NULL) {
        /* create GVFB window error */
        msg_out (LEVEL_0, "CreateGVFBWindow error.");

        UnInit ();

        if (gvfbruninfo.has_skin) {
            UnInitSkin ();
        }

        UnInitRunInfo ();

        return -1;
    }

    /* set up menu */
    InitMenu ();

    /* default refresh rate */
    assert (gvfbruninfo.RefreshRate > 0);

    /* create check event thread */
    checkthread = g_thread_create (CheckEventThread, &gvfbruninfo, TRUE, NULL);

    /* set it to low priority */
    g_thread_set_priority (checkthread, G_THREAD_PRIORITY_LOW);

    /* setup key press thread */
    g_timeout_add (1000 / 100, KeyPressTimeout, NULL);

    /* enter the GTK main loop */
#ifdef DEBUG
    msg_out (LEVEL_0, "gvfb_main.");
#endif

    gtk_main ();

    /* end */
    /* close sockfd, free memory etc. */

    g_thread_join (checkthread);

    UnInit ();

    if (gvfbruninfo.has_skin) {
        UnInitSkin ();
    }

    gdk_threads_leave ();

    UnInitRunInfo ();

    msg_out (LEVEL_0, "gvfb success exit.");

    return 0;
}
