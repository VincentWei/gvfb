/*
** $Id: gvfb_view.c 277 2011-02-17 03:33:07Z xbwang $
**
** gvfbview.c: Handles all the graphics operations, including create window,
**             draw image, scale image, show image and so on.
**
** Copyright (C) 2009 Feynman Software.
**
** All rights reserved by Feynman Software.
**
** Create data: 2009-12-17
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#ifdef WIN32
#   pragma warning(disable:4996)
#else
#   include <unistd.h>
#endif

#include "skin.h"

#include "gvfbhdr.h"
#include "gvfb_main.h"
#include "gvfb_callbacks.h"
#include "gvfb_interface.h"
#include "gvfb_input.h"
#include "gvfb_view.h"
#include "gvfb_log.h"

/* local function */
static void on_im_commit_cb (GtkIMContext * context, const gchar * str);
static gboolean on_im_focus_event_cb (GtkWidget * widget,
                                      GdkEventFocus * event,
                                      gpointer user_data);
static gboolean on_im_preedit_end_cb (GtkIMContext * context,
                                      gpointer user_data);

static gboolean init_cursor (void);
static void set_skin_win_center (int width, int height);

/* XPM */
static char *bg_xpm[] = {
    "32 32 3 1",
    " 	c None",
    ".	c #999999",
    "+	c #666666",
    "........++++++++........++++++++",
    "........++++++++........++++++++",
    "........++++++++........++++++++",
    "........++++++++........++++++++",
    "........++++++++........++++++++",
    "........++++++++........++++++++",
    "........++++++++........++++++++",
    "........++++++++........++++++++",
    "++++++++........++++++++........",
    "++++++++........++++++++........",
    "++++++++........++++++++........",
    "++++++++........++++++++........",
    "++++++++........++++++++........",
    "++++++++........++++++++........",
    "++++++++........++++++++........",
    "++++++++........++++++++........",
    "........++++++++........++++++++",
    "........++++++++........++++++++",
    "........++++++++........++++++++",
    "........++++++++........++++++++",
    "........++++++++........++++++++",
    "........++++++++........++++++++",
    "........++++++++........++++++++",
    "........++++++++........++++++++",
    "++++++++........++++++++........",
    "++++++++........++++++++........",
    "++++++++........++++++++........",
    "++++++++........++++++++........",
    "++++++++........++++++++........",
    "++++++++........++++++++........",
    "++++++++........++++++++........",
    "++++++++........++++++++........"
};

gboolean FitScnRect (int width, int height, int fit_flag)
{
    /* zoom of width and height */
    int zoom_w, zoom_h, zoom;

    assert (gvfbruninfo.has_skin != TRUE);

    if (fit_flag) {
        /* fit screen */
        zoom_w =
            (width - 2 * gvfbruninfo.fix_border) * 100 / gvfbruninfo.actual_w;
        zoom_h =
            (height - 2 * gvfbruninfo.fix_border) * 100 / gvfbruninfo.actual_h;

        zoom = min (zoom_w, zoom_h);

        if (zoom > 0) {
            if (zoom != gvfbruninfo.Zoom) {
                gvfbruninfo.Zoom = zoom;

                gvfbruninfo.drawarea_w =
                    gvfbruninfo.actual_w * gvfbruninfo.Zoom / 100;
                gvfbruninfo.drawarea_h =
                    gvfbruninfo.actual_h * gvfbruninfo.Zoom / 100;

                gtk_widget_set_size_request (GTK_WIDGET (gvfbruninfo.draw_area),
                                             gvfbruninfo.drawarea_w,
                                             gvfbruninfo.drawarea_h);

                /* reset pixbuf_s */
                g_object_unref (gvfbruninfo.pixbuf_s);
                gvfbruninfo.pixbuf_s =
                    gdk_pixbuf_new (GDK_COLORSPACE_RGB, TRUE, 8,
                                    gvfbruninfo.drawarea_w,
                                    gvfbruninfo.drawarea_h);

                MarkDrawAll ();
            }

            gvfbruninfo.sclwnd_w =
                gvfbruninfo.drawarea_w + 2 * gvfbruninfo.fix_border;
            gvfbruninfo.sclwnd_h =
                gvfbruninfo.drawarea_h + 2 * gvfbruninfo.fix_border;

            gvfbruninfo.sclwnd_x = (width - gvfbruninfo.sclwnd_w) / 2;
            gvfbruninfo.sclwnd_y = 0;

            gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW(gvfbruninfo.scroll_win),
                                            GTK_POLICY_NEVER, GTK_POLICY_NEVER);

            /* move scroll window */
            gtk_layout_move (GTK_LAYOUT(gvfbruninfo.layout),
                             gvfbruninfo.scroll_win, gvfbruninfo.sclwnd_x,
                             gvfbruninfo.sclwnd_y);

            /* set scroll window size */
            gtk_widget_set_size_request (GTK_WIDGET(gvfbruninfo.scroll_win),
                                         gvfbruninfo.sclwnd_w,
                                         gvfbruninfo.sclwnd_h);
        }       /* end if */
    }
    else {
        gvfbruninfo.sclwnd_x = 0;
        gvfbruninfo.sclwnd_y = 0;

        gtk_layout_move (GTK_LAYOUT (gvfbruninfo.layout),
                         gvfbruninfo.scroll_win, gvfbruninfo.sclwnd_x,
                         gvfbruninfo.sclwnd_y);

        if (width > 0) {
            gvfbruninfo.sclwnd_w = width;
        }

        if (height > 0) {
            gvfbruninfo.sclwnd_h = height;
        }

        gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW
                                        (gvfbruninfo.scroll_win),
                                        GTK_POLICY_AUTOMATIC,
                                        GTK_POLICY_AUTOMATIC);

        gtk_widget_set_size_request (GTK_WIDGET (gvfbruninfo.scroll_win),
                                     gvfbruninfo.sclwnd_w,
                                     gvfbruninfo.sclwnd_h);
    }

    return FALSE;
}

static gboolean on_main_configure_event (GtkWidget * window,
                                         GdkEventConfigure * event)
{
#ifdef DEBUG
    printf ("main_vbox:: %d %d %d %d\n",
            GTK_WIDGET (gvfbruninfo.main_vbox)->allocation.x,
            GTK_WIDGET (gvfbruninfo.main_vbox)->allocation.y,
            GTK_WIDGET (gvfbruninfo.main_vbox)->allocation.width,
            GTK_WIDGET (gvfbruninfo.main_vbox)->allocation.height);

    printf ("layout:: %d %d %d %d\n",
            GTK_WIDGET (gvfbruninfo.layout)->allocation.x,
            GTK_WIDGET (gvfbruninfo.layout)->allocation.y,
            GTK_WIDGET (gvfbruninfo.layout)->allocation.width,
            GTK_WIDGET (gvfbruninfo.layout)->allocation.height);
#endif

    /* if has skin then return */
    if (gvfbruninfo.has_skin) {
        set_skin_win_center (event->width,
                             event->height - gvfbruninfo.menu_height);
    }
    else {
        FitScnRect (event->width,
                    event->height - gvfbruninfo.menu_height,
                    gvfbruninfo.fit_screen);
    }

    return FALSE;
}

void FullScreen (void)
{
    /* full screen menu item */
    GtkAction *fullscreen_item = NULL;

    /* if disable fullscreen then return */
    if (!gvfbruninfo.o_fullscreen) {
        return;
    }

    /* get full screen menu item */
    fullscreen_item = gtk_ui_manager_get_action (gvfbruninfo.ui_manager,
            "/MainMenu/ViewMenuAction/ViewFullScreenMenuAction");

    assert (fullscreen_item != NULL);

    if (!gvfbruninfo.full_screen) {
        gtk_toggle_action_set_active (GTK_TOGGLE_ACTION (fullscreen_item),
                                      TRUE);
    }
    else {
        gtk_toggle_action_set_active (GTK_TOGGLE_ACTION (fullscreen_item),
                                      FALSE);
    }
}

void ZoomScale (int zoom)
{
    /* scroll windows size */
    int sclwnd_w, sclwnd_h;
    int layout_w, layout_h;

    assert (zoom > 0);
    assert (gvfbruninfo.has_skin != TRUE);

    /* get drawarea size */
    gvfbruninfo.drawarea_w = gvfbruninfo.actual_w * zoom / 100;
    gvfbruninfo.drawarea_h = gvfbruninfo.actual_h * zoom / 100;

    gtk_widget_set_size_request (GTK_WIDGET (gvfbruninfo.draw_area),
                                 gvfbruninfo.drawarea_w,
                                 gvfbruninfo.drawarea_h);

    /* setup pixbuf of drawarea */
    g_object_unref (gvfbruninfo.pixbuf_s);
    gvfbruninfo.pixbuf_s = gdk_pixbuf_new (GDK_COLORSPACE_RGB, TRUE, 8,
                                           gvfbruninfo.drawarea_w,
                                           gvfbruninfo.drawarea_h);

    /* set scroll windows */
    sclwnd_w = gvfbruninfo.drawarea_w + 2 * gvfbruninfo.fix_border;
    sclwnd_h = gvfbruninfo.drawarea_h + 2 * gvfbruninfo.fix_border;

    layout_w = GTK_WIDGET (gvfbruninfo.layout)->allocation.width;
    layout_h = GTK_WIDGET (gvfbruninfo.layout)->allocation.height;

    /* setup scroll window size */
    gvfbruninfo.sclwnd_w = min (sclwnd_w, layout_w);
    gvfbruninfo.sclwnd_h = min (sclwnd_h, layout_h);

    gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW
                                    (gvfbruninfo.scroll_win),
                                    GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);

    gtk_widget_set_size_request (GTK_WIDGET (gvfbruninfo.scroll_win),
                                 gvfbruninfo.sclwnd_w, gvfbruninfo.sclwnd_h);

    /* redraw all */
    MarkDrawAll ();
}

/* get draw rect */
gboolean GetDrawRect (GVFBRECT *draw_rect)
{
    gint fix_x, fix_y, fix_w, fix_h;

    GtkAdjustment *hadjustment;
    GtkAdjustment *vadjustment;

    if (gvfbruninfo.has_skin) {
        draw_rect->x = 0;
        draw_rect->y = 0;
        draw_rect->w = gvfbruninfo.drawarea_w;
        draw_rect->h = gvfbruninfo.drawarea_h;
    }
    else {
        /* fix dirty rect */
        hadjustment =
            gtk_scrolled_window_get_hadjustment (GTK_SCROLLED_WINDOW
                                                 (gvfbruninfo.scroll_win));
        vadjustment =
            gtk_scrolled_window_get_vadjustment (GTK_SCROLLED_WINDOW
                                                 (gvfbruninfo.scroll_win));

        fix_x = GPOINTER_TO_INT (hadjustment->value);
        fix_y = GPOINTER_TO_INT (vadjustment->value);

        fix_w = GPOINTER_TO_INT (hadjustment->page_size);
        fix_h = GPOINTER_TO_INT (vadjustment->page_size);

        if ((fix_x < 0) || (fix_y < 0) || (fix_w < 0) || (fix_h < 0)) {
            return FALSE;
        }

        if ((fix_x > gvfbruninfo.drawarea_w)
            || (fix_y > gvfbruninfo.drawarea_h)) {
            return FALSE;
        }

        fix_w = min (fix_w, (gvfbruninfo.drawarea_w - fix_x));
        fix_h = min (fix_h, (gvfbruninfo.drawarea_h - fix_y));

        draw_rect->x = fix_x;
        draw_rect->y = fix_y;
        draw_rect->w = fix_w;
        draw_rect->h = fix_h;
    }

    return TRUE;
}

static gboolean expose_event (GtkWidget * window, GdkEventExpose * event)
{
    gboolean ret;
    GVFBRECT dirty;
    GVFBRECT draw_rect;
    gint fix_l, fix_t, fix_r, fix_b;

    GVFBHeader *hdr;

    hdr = gvfbruninfo.hdr;

    fix_l = max (event->area.x, 0);
    fix_t = max (event->area.y, 0);

    fix_r = min ((event->area.x + event->area.width), gvfbruninfo.drawarea_w);
    fix_b = min ((event->area.y + event->area.height), gvfbruninfo.drawarea_h);

    if (gvfbruninfo.Zoom != 100) {
        fix_r = min (fix_r, (hdr->width * gvfbruninfo.Zoom / 100));
        fix_b = min (fix_b, (hdr->height * gvfbruninfo.Zoom / 100));
    }
    else {
        fix_r = min (fix_r, hdr->width);
        fix_b = min (fix_b, hdr->height);
    }

    /* draw rect */
    /* fix dirty rect */
    ret = GetDrawRect (&draw_rect);

    if (ret != TRUE) {
        return FALSE;
    }

    dirty.x = max (fix_l, draw_rect.x);
    dirty.y = max (fix_t, draw_rect.y);
    dirty.w = min (fix_r, (draw_rect.x + draw_rect.w)) - dirty.x;
    dirty.h = min (fix_b, (draw_rect.y + draw_rect.h)) - dirty.y;

    if ((dirty.x < 0) || (dirty.y < 0) || (dirty.w <= 0) || (dirty.h <= 0)) {
        return FALSE;
    }

    if (gvfbruninfo.Zoom != 100) {
        ScaleImage (dirty.x, dirty.y, dirty.w, dirty.h);
    }

    DrawImage (dirty.x, dirty.y, dirty.w, dirty.h);

    return FALSE;
}

/* create scroll wnd */
GtkWidget *CreateScrollWnd (gint width, gint height)
{
    GtkWidget *scroll_win = NULL;

    /* Create scroll window */
    scroll_win = gtk_scrolled_window_new (NULL, NULL);

    if (scroll_win == NULL) {
        return NULL;
    }

    gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW(scroll_win),
            GTK_POLICY_NEVER, GTK_POLICY_NEVER);

//    gtk_widget_set_size_request (GTK_WIDGET (scroll_win), width, height);

    return scroll_win;
}

GtkWidget *CreateDrawArea (gint width, gint height)
{
    GtkWidget *draw_area = NULL;

    /* create draw area */
    draw_area = gtk_drawing_area_new ();

    /* check */
    if (draw_area == NULL) {
        return NULL;
    }

    gtk_widget_set_size_request (GTK_WIDGET (draw_area), width, height);
    gtk_widget_set_events (GTK_WIDGET (draw_area), GDK_ALL_EVENTS_MASK);

    g_signal_connect (G_OBJECT (draw_area), "expose-event",
                      GTK_SIGNAL_FUNC (expose_event), NULL);

    g_signal_connect (G_OBJECT (draw_area), "button-press-event",
                      GTK_SIGNAL_FUNC (EventProc), NULL);

    g_signal_connect (G_OBJECT (draw_area), "button-release-event",
                      GTK_SIGNAL_FUNC (EventProc), NULL);

    g_signal_connect (G_OBJECT (draw_area), "motion-notify-event",
                      GTK_SIGNAL_FUNC (EventProc), NULL);

    return draw_area;
}

GtkIMContext *CreateIMContext (GtkWidget * window)
{
    GtkIMContext *IMContext = NULL;

    /* create imcontext */
    IMContext = gtk_im_multicontext_new ();

    if (IMContext == NULL) {
        msg_out (LEVEL_0, "CreateIMContext Error.(gtk_im_multicontext_new)");

        return NULL;
    }

    /* setup signal */
    /* commit */
    g_signal_connect (IMContext, "commit",
                      GTK_SIGNAL_FUNC (on_im_commit_cb), window);

    g_signal_connect (IMContext, "preedit-end",
                      GTK_SIGNAL_FUNC (on_im_preedit_end_cb), window);

    g_signal_connect (G_OBJECT (window), "focus_in_event",
                      GTK_SIGNAL_FUNC (on_im_focus_event_cb), (gpointer) 1);

    g_signal_connect (G_OBJECT (window), "focus_out_event",
                      GTK_SIGNAL_FUNC (on_im_focus_event_cb), (gpointer) 0);

    return IMContext;
}

/* create main wnd */
GtkWidget *CreateMainWnd (gint width, gint height)
{
    GtkWidget *window = NULL;

    /* setup window */
    window = gtk_window_new (GTK_WINDOW_TOPLEVEL);

    /* check */
    if (window == NULL) {
        msg_out (LEVEL_0, "CreateMainWnd Error.(gtk_window_new)");

        return NULL;
    }

    gtk_window_set_default_size (GTK_WINDOW (window), width, height);
//    gtk_container_set_border_width (GTK_CONTAINER(window), 0);
    gtk_widget_set_events (GTK_WIDGET (window), GDK_ALL_EVENTS_MASK);

    /* setup signal */
    g_signal_connect (G_OBJECT (window), "destroy",
                      ACTION_CB (on_m_quit_cb), NULL);

    g_signal_connect (G_OBJECT (window), "key-press-event",
                      GTK_SIGNAL_FUNC (EventProc), NULL);

    g_signal_connect (G_OBJECT (window), "key-release-event",
                      GTK_SIGNAL_FUNC (EventProc), NULL);

    g_signal_connect (G_OBJECT (window), "configure-event",
                      GTK_SIGNAL_FUNC (on_main_configure_event), NULL);

    /* end setup signal */
    /* end setup window */

    return window;
}

GtkWidget *CreateGVFBWindow (gint width, gint height, gint depth,
                             const char *caption)
{
    GtkWidget *mainwnd = NULL;
    GtkWidget *menu_bar = NULL;
    GdkScreen *screen = NULL;

    gint skin_width;
    gint skin_height;

    /* get screen rect */
    screen = gdk_screen_get_default ();

    /* check */
    if (screen == NULL) {
        msg_out (LEVEL_0, "CreateGVFBWindow error.(gdk_screen_get_default)");

        return NULL;
    }

    /* setup layout */
    gvfbruninfo.layout = gtk_layout_new (NULL, NULL);

    /* check */
    if (gvfbruninfo.layout == NULL) {
        msg_out (LEVEL_0, "CreateGVFBWindow error.(gtk_layout_new)");

        return NULL;
    }

    gvfbruninfo.screen_w = gdk_screen_get_width (screen);
    gvfbruninfo.screen_h = gdk_screen_get_height (screen);

    /* The actual width and height */
    if (gvfbruninfo.has_skin) {
        /* has skin */
        assert (g_gvfbskin != NULL);
        assert (g_gvfbskin->pskin != NULL);

        GetSkinSize (&skin_width, &skin_height);

        /* actual size */
        gvfbruninfo.actual_w =
            g_gvfbskin->pskin->screenrect.right -
            g_gvfbskin->pskin->screenrect.left;
        gvfbruninfo.actual_h =
            g_gvfbskin->pskin->screenrect.bottom -
            g_gvfbskin->pskin->screenrect.top;

        /* The width and height show on the screen */
        gvfbruninfo.main_w = skin_width;
        gvfbruninfo.main_h = skin_height + gvfbruninfo.menu_height;
    }
    else {
        /* has not skin */
        /* actual size */
        gvfbruninfo.actual_w = width;
        gvfbruninfo.actual_h = height;

        /* The width and height show on the screen */
        gvfbruninfo.main_w =
            min ((gvfbruninfo.actual_w + 2 * gvfbruninfo.fix_border),
                 gvfbruninfo.screen_w);

        gvfbruninfo.main_h =
            min ((gvfbruninfo.actual_h + gvfbruninfo.menu_height +
                  2 * gvfbruninfo.fix_border),
                 (gvfbruninfo.screen_h - FIX_HEIGHT));
    }

    /* setup window */
    mainwnd = CreateMainWnd (gvfbruninfo.main_w, gvfbruninfo.main_h);

    /* check */
    if (mainwnd == NULL) {
        msg_out (LEVEL_0, "CreateGVFBWindow error.(CreateMainWnd)");

        return NULL;
    }

    /* set caption */
    gtk_window_set_title (GTK_WINDOW (mainwnd), caption);

    gvfbruninfo.main_vbox = gtk_vbox_new (FALSE, 0);

    gtk_container_add (GTK_CONTAINER (mainwnd), gvfbruninfo.main_vbox);

    /* create menu bar */
    gvfbruninfo.ui_manager = CreateMainMenu (mainwnd);

    /* check */
    if (gvfbruninfo.ui_manager == NULL) {
        msg_out (LEVEL_0, "CreateGVFBWindow error.(CreateMainMenu)");

        return NULL;
    }

    menu_bar = gtk_ui_manager_get_widget (gvfbruninfo.ui_manager, "/MainMenu");

    assert (menu_bar != NULL);

    /* put menu */
    gtk_box_pack_start (GTK_BOX (gvfbruninfo.main_vbox), menu_bar, FALSE, TRUE,
                        0);

    /* add to main box */
    gtk_box_pack_start (GTK_BOX (gvfbruninfo.main_vbox), gvfbruninfo.layout,
                        TRUE, TRUE, 0);

    if (gvfbruninfo.has_skin) {
        /* with device skin, has not scroll windows */
        /* create skin windows */
        GetSkinSize (&skin_width, &skin_height);

        gvfbruninfo.skin_x = 0;
        gvfbruninfo.skin_y = 0;
        gvfbruninfo.skin_w = skin_width;
        gvfbruninfo.skin_h = skin_height;

        /* call CreateSkinWnd */
        gvfbruninfo.skin_win =
            CreateSkinWnd (gvfbruninfo.skin_w, gvfbruninfo.skin_h);

        /* check */
        if (gvfbruninfo.skin_win == NULL) {
            msg_out (LEVEL_0, "CreateGVFBWindow error.(CreateSkinWnd)");

            return NULL;
        }

        gtk_layout_put (GTK_LAYOUT (gvfbruninfo.layout), gvfbruninfo.skin_win,
                        gvfbruninfo.skin_x, gvfbruninfo.skin_y);

        /* create drawarea */
        gvfbruninfo.drawarea_x = g_gvfbskin->pskin->screenrect.left;
        gvfbruninfo.drawarea_y = g_gvfbskin->pskin->screenrect.top;
        gvfbruninfo.drawarea_w = gvfbruninfo.actual_w;
        gvfbruninfo.drawarea_h = gvfbruninfo.actual_h;

        gvfbruninfo.draw_area =
            CreateDrawArea (gvfbruninfo.drawarea_w, gvfbruninfo.drawarea_h);

        if (gvfbruninfo.draw_area == NULL) {
            msg_out (LEVEL_0, "CreateGVFBWindow error.(CreateDrawArea)");

            return NULL;
        }

        /* put draw area to layout */
        gtk_layout_put (GTK_LAYOUT (gvfbruninfo.layout), gvfbruninfo.draw_area,
                        gvfbruninfo.drawarea_x, gvfbruninfo.drawarea_y);
    }
    else {
        /* without device skin */
        /* create drawarea */
        gvfbruninfo.drawarea_x = 0;
        gvfbruninfo.drawarea_y = 0;
        gvfbruninfo.drawarea_w = gvfbruninfo.actual_w;
        gvfbruninfo.drawarea_h = gvfbruninfo.actual_h;

        gvfbruninfo.draw_area =
            CreateDrawArea (gvfbruninfo.drawarea_w, gvfbruninfo.drawarea_h);

        /* check */
        if (gvfbruninfo.draw_area == NULL) {
            msg_out (LEVEL_0, "CreateGVFBWindow error.(CreateDrawArea)");

            return NULL;
        }

        /* Create scroll window */
        gvfbruninfo.sclwnd_x = 0;
        gvfbruninfo.sclwnd_y = 0;

        gvfbruninfo.sclwnd_w =
            gvfbruninfo.drawarea_w + 2 * gvfbruninfo.fix_border;
        gvfbruninfo.sclwnd_h =
            gvfbruninfo.drawarea_h + 2 * gvfbruninfo.fix_border;

        gvfbruninfo.scroll_win =
            CreateScrollWnd (gvfbruninfo.sclwnd_w, gvfbruninfo.sclwnd_h);

        /* check */
        if (gvfbruninfo.scroll_win == NULL) {
            msg_out (LEVEL_0, "CreateGVFBWindow error.(CreateScrollWnd)");

            return NULL;
        }
        /* end Create Scroll Window */

        /* add view port */
        gtk_scrolled_window_add_with_viewport (GTK_SCROLLED_WINDOW(gvfbruninfo.scroll_win),
                                               gvfbruninfo.draw_area);

        /* put to layout */
        gtk_layout_put (GTK_LAYOUT (gvfbruninfo.layout), gvfbruninfo.scroll_win,
                        gvfbruninfo.sclwnd_x, gvfbruninfo.sclwnd_y);
    }

    assert (gvfbruninfo.PixelData != NULL);

    gvfbruninfo.pixbuf_r = gdk_pixbuf_new_from_data (gvfbruninfo.PixelData,
                                                     GDK_COLORSPACE_RGB, TRUE,
                                                     8, width, height,
                                                     width * 4, NULL, NULL);

    gvfbruninfo.pixbuf_s = gdk_pixbuf_new (GDK_COLORSPACE_RGB, TRUE, 8,
                                           gvfbruninfo.drawarea_w,
                                           gvfbruninfo.drawarea_h);

    /* im context setup */
    gvfbruninfo.IMContext = CreateIMContext (mainwnd);

    if (gvfbruninfo.IMContext == NULL) {
        msg_out (LEVEL_0, "CreateGVFBWindow error.(CreateIMContext)");

        return NULL;
    }

    gtk_im_context_set_client_window (gvfbruninfo.IMContext, mainwnd->window);

    if (init_cursor () != TRUE) {
        return NULL;
    }

    gtk_widget_show_all (mainwnd);

    return mainwnd;
}

void InitMenu (void)
{
    GtkWidget *menu_bar = NULL;

    gint skin_width;
    gint skin_height;

    assert (gvfbruninfo.ui_manager != NULL);

    menu_bar = gtk_ui_manager_get_widget (gvfbruninfo.ui_manager, "/MainMenu");

    assert (menu_bar != NULL);

    if (gvfbruninfo.has_menu) {
        /* get menu_height */
        gtk_widget_show (menu_bar);
        gvfbruninfo.fix_menu_height = GTK_WIDGET (menu_bar)->allocation.height;
    }
    else {
        /* get menu_height */
        gtk_widget_hide (menu_bar);
        gvfbruninfo.fix_menu_height = 0;
    }

    if (gvfbruninfo.has_skin) {
        GtkAction *zoomscale_item = NULL;
        GtkAction *fitscreen_item = NULL;

        fitscreen_item = gtk_ui_manager_get_action (gvfbruninfo.ui_manager,
                "/MainMenu/ViewMenuAction/ViewAutoFitScreenMenuAction");

        assert (fitscreen_item != NULL);

        gtk_action_set_sensitive (fitscreen_item, FALSE);

        zoomscale_item = gtk_ui_manager_get_action (gvfbruninfo.ui_manager,
                "/MainMenu/ViewMenuAction/ViewZoomScale400MenuAction");

        assert (zoomscale_item != NULL);

        gtk_action_set_sensitive (zoomscale_item, FALSE);

        zoomscale_item = gtk_ui_manager_get_action (gvfbruninfo.ui_manager,
                "/MainMenu/ViewMenuAction/ViewZoomScale200MenuAction");

        assert (zoomscale_item != NULL);

        gtk_action_set_sensitive (zoomscale_item, FALSE);

        zoomscale_item = gtk_ui_manager_get_action (gvfbruninfo.ui_manager,
                "/MainMenu/ViewMenuAction/ViewZoomScale100MenuAction");

        assert (zoomscale_item != NULL);

        gtk_action_set_sensitive (zoomscale_item, FALSE);

        zoomscale_item = gtk_ui_manager_get_action (gvfbruninfo.ui_manager,
                "/MainMenu/ViewMenuAction/ViewZoomScale050MenuAction");

        assert (zoomscale_item != NULL);

        gtk_action_set_sensitive (zoomscale_item, FALSE);

        zoomscale_item = gtk_ui_manager_get_action (gvfbruninfo.ui_manager,
                "/MainMenu/ViewMenuAction/ViewZoomScale025MenuAction");

        assert (zoomscale_item != NULL);

        gtk_action_set_sensitive (zoomscale_item, FALSE);
    }
    else {
        GtkAction *zoomscale_item = NULL;
        GtkAction *fitscreen_item = NULL;

        fitscreen_item = gtk_ui_manager_get_action (gvfbruninfo.ui_manager,
                "/MainMenu/ViewMenuAction/ViewAutoFitScreenMenuAction");

        assert (fitscreen_item != NULL);

        gtk_toggle_action_set_active (GTK_TOGGLE_ACTION (fitscreen_item),
                                      FALSE);

        zoomscale_item = gtk_ui_manager_get_action (gvfbruninfo.ui_manager,
                "/MainMenu/ViewMenuAction/ViewZoomScale100MenuAction");

        assert (zoomscale_item != NULL);

        gtk_toggle_action_set_active (GTK_TOGGLE_ACTION (zoomscale_item), TRUE);
    }

    {
        GtkAction *fitscreen_item = NULL;
        gboolean b_fit_screen;

        fitscreen_item = gtk_ui_manager_get_action (gvfbruninfo.ui_manager,
                "/MainMenu/ViewMenuAction/ViewAutoFitScreenMenuAction");

        assert (fitscreen_item != NULL);

        b_fit_screen =
            gtk_toggle_action_get_active (GTK_TOGGLE_ACTION (fitscreen_item));

        if (gvfbruninfo.has_skin != TRUE) {
            if (b_fit_screen) {
                gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW
                                                (gvfbruninfo.scroll_win),
                                                GTK_POLICY_NEVER,
                                                GTK_POLICY_NEVER);
            }
            else {
                gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW
                                                (gvfbruninfo.scroll_win),
                                                GTK_POLICY_AUTOMATIC,
                                                GTK_POLICY_AUTOMATIC);
            }
        }
    }

    if (!gvfbruninfo.o_fullscreen) {
        GtkAction *fullscreen_item = NULL;

        fullscreen_item = gtk_ui_manager_get_action (gvfbruninfo.ui_manager,
                "/MainMenu/ViewMenuAction/ViewFullScreenMenuAction");

        assert (fullscreen_item != NULL);

        gtk_action_set_sensitive (fullscreen_item, FALSE);
    }

    /* setup cursor */
    {
        GtkAction *showgtkcursor_item = NULL;

        showgtkcursor_item = gtk_ui_manager_get_action (gvfbruninfo.ui_manager,
                "/MainMenu/ViewMenuAction/ViewShowGtkCursorMenuAction");

        if (gtk_toggle_action_get_active
            (GTK_TOGGLE_ACTION (showgtkcursor_item))) {
            gdk_window_set_cursor (gvfbruninfo.draw_area->window,
                                   gvfbruninfo.gtkdef_cursor);
        }
        else {
            gdk_window_set_cursor (gvfbruninfo.draw_area->window,
                                   gvfbruninfo.userdef_cursor);
        }
    }

    if (gvfbruninfo.show_gtk_cursor) {
        GtkAction *showgtkcursor_item = NULL;

        showgtkcursor_item = gtk_ui_manager_get_action (gvfbruninfo.ui_manager,
                "/MainMenu/ViewMenuAction/ViewShowGtkCursorMenuAction");

        assert (showgtkcursor_item != NULL);

        gtk_toggle_action_set_active (GTK_TOGGLE_ACTION (showgtkcursor_item),
                                      TRUE);
    }

    if (gvfbruninfo.full_screen) {
        GtkAction *fullscreen_item = NULL;

        fullscreen_item = gtk_ui_manager_get_action (gvfbruninfo.ui_manager,
                "/MainMenu/ViewMenuAction/ViewFullScreenMenuAction");

        assert (fullscreen_item != NULL);

        gtk_toggle_action_set_active (GTK_TOGGLE_ACTION (fullscreen_item),
                                      TRUE);
    }

    gvfbruninfo.menu_height = gvfbruninfo.fix_menu_height;

    /* fix main window size */
    if (gvfbruninfo.has_skin) {
        GetSkinSize (&skin_width, &skin_height);

        /* The width and height show on the screen */
        gvfbruninfo.main_w = skin_width;
        gvfbruninfo.main_h = skin_height + gvfbruninfo.menu_height;
    }
    else {
        /* The width and height show on the screen */
        gvfbruninfo.main_w =
            min ((gvfbruninfo.actual_w + 2 * gvfbruninfo.fix_border),
                 gvfbruninfo.screen_w);

        gvfbruninfo.main_h =
            min ((gvfbruninfo.actual_h + gvfbruninfo.menu_height +
                  2 * gvfbruninfo.fix_border),
                 (gvfbruninfo.screen_h - FIX_HEIGHT));
    }

    gtk_window_resize (GTK_WINDOW (gvfbruninfo.window),
                       gvfbruninfo.main_w, gvfbruninfo.main_h);
}

void DrawImage (int x, int y, int width, int height)
{
    if (gvfbruninfo.graph_with_alpha) {
        /* with alpha */
        GdkPixmap *pixmap;
        GdkGC *gc;

        int offset_x, offset_y;

        offset_x = x & 0xf;
        offset_y = y & 0xf;

        pixmap =
            gdk_pixmap_new (gvfbruninfo.draw_area->window, width + offset_x,
                            height + offset_y,
                            gdk_drawable_get_depth (gvfbruninfo.
                                                    draw_area->window));

        gc = gdk_gc_new (pixmap);

        if (gvfbruninfo.tile_pixmap == NULL) {
            gvfbruninfo.tile_pixmap =
                gdk_pixmap_create_from_xpm_d (gvfbruninfo.draw_area->window,
                                              NULL, NULL, bg_xpm);
        }

        gdk_gc_set_fill (gc, GDK_TILED);
        gdk_gc_set_tile (gc, gvfbruninfo.tile_pixmap);

        gdk_draw_rectangle (pixmap, gc, TRUE, 0, 0, width + offset_x,
                            height + offset_y);

        /* draw dirty */
        if (gvfbruninfo.Zoom == 100) {
            gdk_draw_pixbuf (pixmap, gc, gvfbruninfo.pixbuf_r, x, y, offset_x,
                             offset_y, width, height, GDK_RGB_DITHER_NONE, 0,
                             0);
        }
        else {
            gdk_draw_pixbuf (pixmap, gc, gvfbruninfo.pixbuf_s, x, y, offset_x,
                             offset_y, width, height, GDK_RGB_DITHER_NONE, 0,
                             0);
        }

        gdk_draw_drawable (gvfbruninfo.draw_area->window, gc, pixmap,
                           offset_x, offset_y, x, y, width, height);

        g_object_unref (gc);
        g_object_unref (pixmap);
    }
    else {
        /* without alpha */
        /* draw dirty */
        if (gvfbruninfo.Zoom == 100) {
            gdk_draw_pixbuf (gvfbruninfo.draw_area->window,
                             gvfbruninfo.draw_area->
                             style->fg_gc[GTK_WIDGET_STATE
                                          (gvfbruninfo.draw_area)],
                             gvfbruninfo.pixbuf_r, x, y, x, y, width, height,
                             GDK_RGB_DITHER_NONE, 0, 0);
        }
        else {
            gdk_draw_pixbuf (gvfbruninfo.draw_area->window,
                             gvfbruninfo.draw_area->
                             style->fg_gc[GTK_WIDGET_STATE
                                          (gvfbruninfo.draw_area)],
                             gvfbruninfo.pixbuf_s, x, y, x, y, width, height,
                             GDK_RGB_DITHER_NONE, 0, 0);
        }
    }
}

void ShowHide (int bshow)
{
    if (bshow) {
        gtk_window_maximize (GTK_WINDOW (gvfbruninfo.window));
        gtk_window_present (GTK_WINDOW (gvfbruninfo.window));

        gtk_widget_set_state (gvfbruninfo.window, GTK_STATE_ACTIVE);

        gtk_window_unmaximize (GTK_WINDOW (gvfbruninfo.window));
        gtk_window_present (GTK_WINDOW (gvfbruninfo.window));
    }
    else {
        gtk_window_iconify (GTK_WINDOW (gvfbruninfo.window));
    }
}

void SetMouseXY (int x, int y)
{
    int real_x = 0, real_y = 0;

    real_x = max (x, 0);
    real_y = max (y, 0);

    real_x = min (real_x, gvfbruninfo.actual_w);
    real_y = min (real_y, gvfbruninfo.actual_h);

    SendMouseData (real_x, real_y, 0);
}

static void on_im_commit_cb (GtkIMContext * context, const gchar * str)
{
#ifdef DEBUG
    printf ("on_im_commit_cb\n");
#endif

    if (str) {
        SendIMData ((char *) str);
    }
}

static gboolean on_im_focus_event_cb (GtkWidget * widget, GdkEventFocus * event,
                                      gpointer user_data)
{
#ifdef DEBUG
    printf ("on_im_focus_event_cb\n");
#endif

    gtk_im_context_reset (gvfbruninfo.IMContext);

    SendUnpressedKeys ();

    gtk_im_context_focus_in (gvfbruninfo.IMContext);

    return TRUE;
}

static gboolean on_im_preedit_end_cb (GtkIMContext * context,
                                      gpointer user_data)
{
#ifdef DEBUG
    printf ("on_im_preedit_end_cb\n");
#endif

    SendUnpressedKeys ();

    return TRUE;
}

/* init cursour */
static gboolean init_cursor (void)
{
    GdkPixmap *cursor_pixmap = NULL;

    gchar blank_cursor[1] = { 0x00 };

    GdkColor fg = { 0, 0, 0, 0 };       /* black. */
    GdkColor bg = { 0, 0, 0, 0 };       /* black. */

    cursor_pixmap = gdk_bitmap_create_from_data (NULL, blank_cursor, 1, 1);

    if (cursor_pixmap == NULL) {
        return FALSE;
    }

    gvfbruninfo.userdef_cursor =
        gdk_cursor_new_from_pixmap (cursor_pixmap, cursor_pixmap, &fg, &bg, 0,
                                    0);

    g_object_unref (cursor_pixmap);
    cursor_pixmap = NULL;

    if (gvfbruninfo.userdef_cursor == NULL) {
        return FALSE;
    }

    gvfbruninfo.gtkdef_cursor = gdk_cursor_new (GDK_LEFT_PTR);

    if (gvfbruninfo.gtkdef_cursor == NULL) {
        return FALSE;
    }

    return TRUE;
}

static void set_skin_win_center (int width, int height)
{
    assert (gvfbruninfo.has_skin);

    if (gvfbruninfo.skin_w < width) {
        gvfbruninfo.skin_x = (width - gvfbruninfo.skin_w) / 2;

        gtk_layout_move (GTK_LAYOUT (gvfbruninfo.layout), gvfbruninfo.skin_win,
                         gvfbruninfo.skin_x, gvfbruninfo.skin_y);

        gvfbruninfo.drawarea_x =
            gvfbruninfo.skin_x + g_gvfbskin->pskin->screenrect.left;

        gtk_layout_move (GTK_LAYOUT (gvfbruninfo.layout), gvfbruninfo.draw_area,
                         gvfbruninfo.drawarea_x, gvfbruninfo.drawarea_y);
    }
    else {
        gvfbruninfo.skin_x = 0;

        gtk_layout_move (GTK_LAYOUT (gvfbruninfo.layout), gvfbruninfo.skin_win,
                         gvfbruninfo.skin_x, gvfbruninfo.skin_y);

        gvfbruninfo.drawarea_x = g_gvfbskin->pskin->screenrect.left;

        gtk_layout_move (GTK_LAYOUT (gvfbruninfo.layout), gvfbruninfo.draw_area,
                         gvfbruninfo.drawarea_x, gvfbruninfo.drawarea_y);
    }
}
