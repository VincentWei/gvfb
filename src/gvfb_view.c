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
            if (zoom != gvfbruninfo.zoom_percent) {
                gvfbruninfo.zoom_percent = zoom;

                gvfbruninfo.drawarea_w =
                    gvfbruninfo.actual_w * gvfbruninfo.zoom_percent / 100;
                gvfbruninfo.drawarea_h =
                    gvfbruninfo.actual_h * gvfbruninfo.zoom_percent / 100;

                gtk_widget_set_size_request (GTK_WIDGET (gvfbruninfo.draw_area),
                                             gvfbruninfo.drawarea_w,
                                             gvfbruninfo.drawarea_h);

#if 0
                /* reset pixbuf_s */
                g_object_unref (gvfbruninfo.pixbuf_s);
                gvfbruninfo.pixbuf_s =
                    gdk_pixbuf_new (GDK_COLORSPACE_RGB, TRUE, 8,
                                    gvfbruninfo.drawarea_w,
                                    gvfbruninfo.drawarea_h);
#endif

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

#if 0
    /* setup pixbuf of drawarea */
    g_object_unref (gvfbruninfo.pixbuf_s);
    gvfbruninfo.pixbuf_s = gdk_pixbuf_new (GDK_COLORSPACE_RGB, TRUE, 8,
                                           gvfbruninfo.drawarea_w,
                                           gvfbruninfo.drawarea_h);
#endif

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

        if (gvfbruninfo.graph_with_alpha && gvfbruninfo.video_layer_mode) {
            draw_rect->x = fix_x;
            draw_rect->y = fix_y;
            draw_rect->w = fix_w;
            draw_rect->h = fix_h;
            return TRUE;
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
    GVFBRECT dirty;
    GVFBRECT draw_rect;
    gint fix_l, fix_t, fix_r, fix_b;

    GVFBHeader *hdr;

    hdr = gvfbruninfo.hdr;

    fix_l = max (event->area.x, 0);
    fix_t = max (event->area.y, 0);

    fix_r = min ((event->area.x + event->area.width), gvfbruninfo.drawarea_w);
    fix_b = min ((event->area.y + event->area.height), gvfbruninfo.drawarea_h);

    if (gvfbruninfo.zoom_percent != 100) {
        fix_r = min (fix_r, (hdr->width * gvfbruninfo.zoom_percent / 100));
        fix_b = min (fix_b, (hdr->height * gvfbruninfo.zoom_percent / 100));
    }
    else {
        fix_r = min (fix_r, hdr->width);
        fix_b = min (fix_b, hdr->height);
    }

    /* draw rect */
    /* fix dirty rect */
    if (!GetDrawRect (&draw_rect)) {
        return FALSE;
    }

    dirty.x = max (fix_l, draw_rect.x);
    dirty.y = max (fix_t, draw_rect.y);
    dirty.w = min (fix_r, (draw_rect.x + draw_rect.w)) - dirty.x;
    dirty.h = min (fix_b, (draw_rect.y + draw_rect.h)) - dirty.y;

    if ((dirty.x < 0) || (dirty.y < 0) || (dirty.w <= 0) || (dirty.h <= 0)) {
        return FALSE;
    }

    if (gvfbruninfo.zoom_percent != 100) {
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
    GtkIMContext *im_context = NULL;

    /* create imcontext */
    im_context = gtk_im_multicontext_new ();

    if (im_context == NULL) {
        msg_out (LEVEL_0, "CreateIMContext Error.(gtk_im_multicontext_new)");
        return NULL;
    }

    /* setup signal */
    /* commit */
    g_signal_connect (im_context, "commit",
                      GTK_SIGNAL_FUNC (on_im_commit_cb), window);

    g_signal_connect (im_context, "preedit-end",
                      GTK_SIGNAL_FUNC (on_im_preedit_end_cb), window);

    g_signal_connect (G_OBJECT (window), "focus_in_event",
                      GTK_SIGNAL_FUNC (on_im_focus_event_cb), (gpointer) 1);

    g_signal_connect (G_OBJECT (window), "focus_out_event",
                      GTK_SIGNAL_FUNC (on_im_focus_event_cb), (gpointer) 0);

    return im_context;
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

    assert (gvfbruninfo.pixel_data != NULL);

    gvfbruninfo.pixbuf_r = gdk_pixbuf_new_from_data (gvfbruninfo.pixel_data,
                                                     GDK_COLORSPACE_RGB, TRUE,
                                                     8, width, height,
                                                     width * 4, NULL, NULL);

#if 0
    gvfbruninfo.pixbuf_s = gdk_pixbuf_new (GDK_COLORSPACE_RGB, TRUE, 8,
                                           gvfbruninfo.drawarea_w,
                                           gvfbruninfo.drawarea_h);
#endif

    /* im context setup */
    gvfbruninfo.im_context = CreateIMContext (mainwnd);

    if (gvfbruninfo.im_context == NULL) {
        msg_out (LEVEL_0, "CreateGVFBWindow error.(CreateIMContext)");

        return NULL;
    }

    gtk_im_context_set_client_window (gvfbruninfo.im_context, mainwnd->window);

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

static void cleanup_motion_jpeg (void)
{
    gvfbruninfo.video_layer_mode = 0;

    if (gvfbruninfo.video_record_stream) {
        g_output_stream_close (
                G_OUTPUT_STREAM (gvfbruninfo.video_record_stream),
                NULL, NULL);
        g_object_unref (gvfbruninfo.video_record_stream);
        gvfbruninfo.video_record_stream = NULL;
        gvfbruninfo.nr_frames_recorded = 0;
    }

    if (gvfbruninfo.motion_jpeg_info) {
        free (gvfbruninfo.motion_jpeg_info);
        gvfbruninfo.motion_jpeg_info = NULL;
    }

    if (gvfbruninfo.motion_jpeg_stream) {
        g_input_stream_close (
                G_INPUT_STREAM (gvfbruninfo.motion_jpeg_stream),
                NULL, NULL);
        g_object_unref (gvfbruninfo.motion_jpeg_stream);
        gvfbruninfo.motion_jpeg_stream = NULL;
    }

}

static gboolean calc_frame_offset (GVFBRUNINFO* runinfo)
{
    guint32 idx_frame = 0;
    guint32 left_frames = runinfo->motion_jpeg_info->nr_frames;

    g_seekable_seek (G_SEEKABLE(runinfo->motion_jpeg_stream),
            runinfo->motion_jpeg_info->offset_first_frame,
            G_SEEK_SET, NULL, NULL);

    while (left_frames > 0) {
        guint32 frame_size;
        gssize bytes_read;

        runinfo->motion_jpeg_info->frame_offset[idx_frame] =
            (guint32)g_seekable_tell (G_SEEKABLE(runinfo->motion_jpeg_stream));

        bytes_read = g_input_stream_read (
            G_INPUT_STREAM (runinfo->motion_jpeg_stream),
            &frame_size, sizeof (guint32), NULL, NULL);

        if (bytes_read != sizeof (guint32)) {
            msg_out (LEVEL_0, "g_input_stream_read");
            return FALSE;
        }

        g_seekable_seek (G_SEEKABLE(runinfo->motion_jpeg_stream),
            frame_size, G_SEEK_CUR, NULL, NULL);

        left_frames--;
        idx_frame++;
    }

    return TRUE;
}

gboolean VvlOpenMotionJPEG (const char* file_name)
{
    cleanup_motion_jpeg ();

    GFile* file = g_file_new_for_path (file_name);
    gvfbruninfo.motion_jpeg_stream = g_file_read (file, NULL, NULL);

    if (gvfbruninfo.motion_jpeg_stream) {
        gsize my_size;
        gssize bytes_read;

        my_size = sizeof (MotionJPEGInfo);
        gvfbruninfo.motion_jpeg_info = (MotionJPEGInfo*)malloc (my_size);

        if (gvfbruninfo.motion_jpeg_stream && gvfbruninfo.motion_jpeg_info) {

            bytes_read = g_input_stream_read (
                G_INPUT_STREAM (gvfbruninfo.motion_jpeg_stream),
                gvfbruninfo.motion_jpeg_info, my_size, NULL, NULL);

            if ((gsize)bytes_read == my_size &&
                    gvfbruninfo.motion_jpeg_info->nr_frames > 0) {
                void* tmp;

                my_size = sizeof (MotionJPEGInfo) +
                    sizeof (guint32) * gvfbruninfo.motion_jpeg_info->nr_frames;
                tmp = realloc (gvfbruninfo.motion_jpeg_info, my_size);
                if (tmp == NULL)
                    goto error;
                gvfbruninfo.motion_jpeg_info = (MotionJPEGInfo*)tmp;

                if (!calc_frame_offset (&gvfbruninfo))
                    goto error;
            }
            else {
                goto error;
            }
        }
        else {
            goto error;
        }

        g_object_unref (file);
        gvfbruninfo.video_frame_idx = 0;
        return TRUE;
    }

error:
    msg_out (LEVEL_0, "VvlOpenMotionJPEG failed");
    g_object_unref (file);
    cleanup_motion_jpeg ();
    return FALSE;
}

gboolean VvlOpenCamera (const char* path, int zoom_level)
{
    if (VvlOpenMotionJPEG (path)) {
        if (zoom_level > 255)
            gvfbruninfo.camera_zoom_level = 0x30;
        else if (zoom_level < 0)
            gvfbruninfo.camera_zoom_level = 0x30;
        else
            gvfbruninfo.camera_zoom_level = zoom_level;

        gvfbruninfo.video_layer_mode = 0x0100;

        return TRUE;
    }

    return FALSE;
}

gboolean VvlCloseCamera (void)
{
    cleanup_motion_jpeg ();
    gvfbruninfo.video_layer_mode = 0x0000;
    return TRUE;
}

gboolean VvlSetZoomLevel (int zoom_level)
{
    if (zoom_level > 255)
        gvfbruninfo.camera_zoom_level = 0x30;
    else if (zoom_level < 0)
        gvfbruninfo.camera_zoom_level = 0x30;
    else
        gvfbruninfo.camera_zoom_level = zoom_level;

    return TRUE;
}

/* Return video length in seconds */
unsigned int VvlPlayVideo (const char* path, int idx_frame)
{
    if (VvlOpenMotionJPEG (path)) {
        if (idx_frame >= gvfbruninfo.motion_jpeg_info->nr_frames)
            gvfbruninfo.video_frame_idx
                = gvfbruninfo.motion_jpeg_info->nr_frames - 1;
        else if (idx_frame < 0)
            gvfbruninfo.video_frame_idx = 0;
        else
            gvfbruninfo.video_frame_idx = idx_frame;

        return gvfbruninfo.motion_jpeg_info->nr_frames /
                    gvfbruninfo.motion_jpeg_info->frame_rate;
    }

    return 0;
}

gboolean VvlSeekVideo (int idx_frame)
{
    if (gvfbruninfo.motion_jpeg_info) {
        if (idx_frame >= gvfbruninfo.motion_jpeg_info->nr_frames)
            gvfbruninfo.video_frame_idx = gvfbruninfo.motion_jpeg_info->nr_frames - 1;
        else if (idx_frame < 0)
            gvfbruninfo.video_frame_idx = 0;
        else
            gvfbruninfo.video_frame_idx = idx_frame;

        return TRUE;
    }

    return FALSE;
}

gboolean VvlPausePlayback (void)
{
    gvfbruninfo.video_layer_mode = 0x0200;
    return TRUE;
}

gboolean VvlResumePlayback (void)
{
    gvfbruninfo.video_layer_mode = 0x0201;
    return TRUE;
}

gboolean VvlStopPlayback (void)
{
    cleanup_motion_jpeg ();
    gvfbruninfo.video_layer_mode = 0x0000;
    return TRUE;
}

#define LEN_RW_BUFF  8192

static gboolean record_current_frame (GFileOutputStream* output_stream)
{
    guint32 left_size;
    gssize bytes_read, bytes_wrotten;

    g_seekable_seek (G_SEEKABLE(gvfbruninfo.motion_jpeg_stream),
            gvfbruninfo.motion_jpeg_info
                    ->frame_offset[gvfbruninfo.video_frame_idx],
            G_SEEK_SET, NULL, NULL);

    bytes_read = g_input_stream_read (
            G_INPUT_STREAM (gvfbruninfo.motion_jpeg_stream),
            &left_size, sizeof (guint32), NULL, NULL);

    if (bytes_read != sizeof (guint32)) {
        msg_out (LEVEL_0, "g_input_stream_read");
        return FALSE;
    }

    while (left_size > 0) {
        char buff[LEN_RW_BUFF];
        gsize read_size = (left_size > LEN_RW_BUFF) ? LEN_RW_BUFF : left_size;

        bytes_read = g_input_stream_read (
                G_INPUT_STREAM (gvfbruninfo.motion_jpeg_stream),
                buff, read_size, NULL, NULL);

        bytes_wrotten = g_output_stream_write (
                G_OUTPUT_STREAM (output_stream),
                buff, bytes_read, NULL, NULL);

        if (bytes_wrotten < bytes_read) // disk full
            return FALSE;

        if (bytes_read < read_size) // end of file
            break;

        left_size -= read_size;
    }

    return TRUE;
}

gboolean VvlCapturePhoto (const char* path)
{
    GFile* file = NULL;
    GFileOutputStream* photo_stream = NULL;
    guint32 left_size;
    gssize bytes_read, bytes_wrotten;

    if (gvfbruninfo.motion_jpeg_stream == NULL
            || gvfbruninfo.motion_jpeg_info == NULL) {
        return FALSE;
    }

    file = g_file_new_for_path (path);
    photo_stream = g_file_create (file, G_FILE_CREATE_NONE, NULL, NULL);
    if (photo_stream == NULL) {
        goto error;
    }

    record_current_frame (photo_stream);

    g_output_stream_close (
            G_OUTPUT_STREAM (photo_stream), NULL, NULL);
    g_object_unref (photo_stream);
    g_object_unref (file);
    return TRUE;

error:
    if (photo_stream) {
        g_output_stream_close (
                G_OUTPUT_STREAM (photo_stream), NULL, NULL);
        g_object_unref (photo_stream);
    }
    g_object_unref (file);
    return FALSE;
}

gboolean VvlStartRecord (const char* path)
{
    GFile* file = NULL;
    GFileOutputStream* video_stream = NULL;
    MotionJPEGInfo header;
    gssize bytes_wrotten;

    if (gvfbruninfo.motion_jpeg_stream == NULL
            || gvfbruninfo.motion_jpeg_info == NULL
            || gvfbruninfo.video_record_stream != NULL
            || gvfbruninfo.video_layer_mode != 0x0100) {
        return FALSE;
    }

    file = g_file_new_for_path (path);
    video_stream = g_file_create (file, G_FILE_CREATE_NONE, NULL, NULL);
    if (video_stream == NULL) {
        goto error;
    }

    memcpy (&header, gvfbruninfo.motion_jpeg_info, sizeof (MotionJPEGInfo));
    header.nr_frames = 0;

    // write header
    bytes_wrotten = g_output_stream_write (
            G_OUTPUT_STREAM (video_stream),
            &header, sizeof (MotionJPEGInfo), NULL, NULL);

    if (bytes_wrotten < sizeof (MotionJPEGInfo)) {
        goto error;
    }

    g_object_unref (file);

    gvfbruninfo.video_record_stream = video_stream;
    gvfbruninfo.nr_frames_recorded = 0;
    gvfbruninfo.video_layer_mode = 0x0101;
    return TRUE;

error:
    if (video_stream)
        g_object_unref (video_stream);

    g_file_delete (file, NULL, NULL);
    g_object_unref (file);
    return FALSE;
}

static void update_record_header (void)
{
    gssize bytes_wrotten;

    gvfbruninfo.nr_frames_recorded++;

    g_seekable_seek (G_SEEKABLE(gvfbruninfo.video_record_stream),
            0, G_SEEK_SET, NULL, NULL);

    bytes_wrotten = g_output_stream_write (
            G_OUTPUT_STREAM (gvfbruninfo.video_record_stream),
            &gvfbruninfo.nr_frames_recorded, sizeof (guint32), NULL, NULL);

    if (bytes_wrotten < sizeof (guint32)) {
        msg_out (LEVEL_0, "update_record_header failed (%u)",
                gvfbruninfo.nr_frames_recorded);
    }

    /* alwasy seek to end of the stream */
    g_seekable_seek (G_SEEKABLE(gvfbruninfo.video_record_stream),
            0, G_SEEK_END, NULL, NULL);
}

static void cleanup_video_record_stream (void)
{
    g_output_stream_close (
            G_OUTPUT_STREAM (gvfbruninfo.video_record_stream),
            NULL, NULL);
    g_object_unref (gvfbruninfo.video_record_stream);
    gvfbruninfo.video_record_stream = NULL;
    gvfbruninfo.nr_frames_recorded = 0;
}

gboolean VvlStopRecord (void)
{
    if (gvfbruninfo.motion_jpeg_stream == NULL
            || gvfbruninfo.motion_jpeg_info == NULL
            || gvfbruninfo.video_record_stream == NULL
            || gvfbruninfo.video_layer_mode != 0x0101) {
        return FALSE;
    }

    gvfbruninfo.video_layer_mode = 0x0100;
    cleanup_video_record_stream ();
    return TRUE;
}

void DrawImage (int x, int y, int width, int height)
{
    if (gvfbruninfo.graph_with_alpha) {
        /* Cairo implementaion */

        cairo_t *cr;
        cairo_pattern_t *pattern;

        cr = gdk_cairo_create (gvfbruninfo.draw_area->window);

        if (gvfbruninfo.video_layer_mode == 0) {
            /* draw default background */
            if (gvfbruninfo.bkgnd_pixmap == NULL) {
                gvfbruninfo.bkgnd_pixmap =
                    gdk_pixmap_create_from_xpm_d (
                            gvfbruninfo.draw_area->window,
                            NULL, NULL, bg_xpm);
            }

            gdk_cairo_set_source_pixmap (cr, gvfbruninfo.bkgnd_pixmap, 0, 0);
            pattern = cairo_get_source (cr);
            cairo_pattern_set_extend (pattern, CAIRO_EXTEND_REPEAT);
            cairo_rectangle (cr, 0, 0,
                    gvfbruninfo.actual_w, gvfbruninfo.actual_h);
            cairo_fill (cr);
        }
        else if (gvfbruninfo.motion_jpeg_info
                    && gvfbruninfo.motion_jpeg_stream) {
            /* draw video frame */
            GdkPixbuf *frame_pixbuf;

            if (gvfbruninfo.video_layer_mode == 0x0101
                    && gvfbruninfo.video_record_stream) {
                if (record_current_frame (gvfbruninfo.video_record_stream)) {
                    update_record_header ();
                }
                else {
                    cleanup_video_record_stream ();
                }
            }

            g_seekable_seek (G_SEEKABLE(gvfbruninfo.motion_jpeg_stream),
                    (goffset)(gvfbruninfo.motion_jpeg_info
                        ->frame_offset[gvfbruninfo.video_frame_idx]
                        + sizeof (guint32)),
                    G_SEEK_SET, NULL, NULL);

            frame_pixbuf = gdk_pixbuf_new_from_stream (
                    G_INPUT_STREAM(gvfbruninfo.motion_jpeg_stream), NULL, NULL);
            if (frame_pixbuf) {
                if ((gvfbruninfo.video_layer_mode & 0xFF00) == 0x0100) {
                    int zoom_level = gvfbruninfo.camera_zoom_level;
                    int frame_width = gdk_pixbuf_get_width (frame_pixbuf);
                    int frame_height = gdk_pixbuf_get_height (frame_pixbuf);

                    zoom_level = zoom_level / 16 + 1;

                    frame_width = (int)(frame_width * zoom_level *
                            gvfbruninfo.zoom_percent / 400.0f);
                    frame_height = (int)(frame_height * zoom_level *
                            gvfbruninfo.zoom_percent / 400.0f);
                    cairo_translate (cr,
                            (gvfbruninfo.actual_w - frame_width)/2.0f,
                            (gvfbruninfo.actual_h - frame_height)/2.0f);

                    cairo_scale (cr,
                            zoom_level * gvfbruninfo.zoom_percent / 400.0f,
                            zoom_level * gvfbruninfo.zoom_percent / 400.0f);
                }

                gdk_cairo_set_source_pixbuf (cr, frame_pixbuf, 0, 0);
                pattern = cairo_get_source (cr);
                cairo_pattern_set_extend (pattern, CAIRO_EXTEND_NONE);
                cairo_paint (cr);

                g_object_unref (frame_pixbuf);
            }

            if ((gvfbruninfo.video_layer_mode & 0xFF00) == 0x0100) {
                gvfbruninfo.video_frame_idx++;
                if (gvfbruninfo.video_frame_idx
                        > gvfbruninfo.motion_jpeg_info->nr_frames) {
                    gvfbruninfo.video_frame_idx = 0;
                }
            }
            else if ((gvfbruninfo.video_layer_mode & 0xFFFF) == 0x0201) {
                gvfbruninfo.video_frame_idx++;
                if (gvfbruninfo.video_frame_idx
                        > gvfbruninfo.motion_jpeg_info->nr_frames) {
                    gvfbruninfo.video_frame_idx
                        = gvfbruninfo.motion_jpeg_info->nr_frames - 1;
                    gvfbruninfo.video_layer_mode = 0x0200;
                }
            }
        }

        cairo_identity_matrix (cr);
        if (gvfbruninfo.zoom_percent != 100)
            cairo_scale (cr,
                    gvfbruninfo.zoom_percent / 100.0f,
                    gvfbruninfo.zoom_percent / 100.0f);

        /* draw graphics */
        gdk_cairo_set_source_pixbuf (cr, gvfbruninfo.pixbuf_r, 0, 0);
        cairo_paint_with_alpha (cr, gvfbruninfo.graph_alpha_channel/255.0f);

        cairo_destroy (cr);
    }
    else {
        cairo_t *cr;

        cr = gdk_cairo_create (gvfbruninfo.draw_area->window);

        /* draw graphics */
        if (gvfbruninfo.zoom_percent != 100)
            cairo_scale (cr,
                    gvfbruninfo.zoom_percent / 100.0f,
                    gvfbruninfo.zoom_percent / 100.0f);
        gdk_cairo_set_source_pixbuf (cr, gvfbruninfo.pixbuf_r, 0, 0);
        cairo_paint (cr);

        cairo_destroy (cr);
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

    gtk_im_context_reset (gvfbruninfo.im_context);

    SendUnpressedKeys ();

    gtk_im_context_focus_in (gvfbruninfo.im_context);

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

