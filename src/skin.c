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

#include <assert.h>

#include <gtk/gtk.h>

#ifdef WIN32
#   pragma warning(disable:4996)
#endif

#include "deviceskin.h"
#include "skin.h"

#include "gvfb_input.h"
#include "gvfb_log.h"

GVFBSKIN *g_gvfbskin = NULL;

/* static function */
static int draw_skin (GtkWidget * widget, SKIN *pskin);
static gboolean mouse_button_down (GtkWidget * widget,
                                   GdkEventButton * event, gpointer data);
static gboolean mouse_button_up (GtkWidget * widget,
                                 GdkEventButton * event, gpointer data);
static gboolean mouse_move (GtkWidget * widget, GdkEventButton * event,
                            gpointer data);
static gboolean expose_event (GtkWidget * widget,
                              GdkEventExpose * event, gpointer data);

#ifdef SKINDEBUG
void SendKeyboardData (int keycode, int press, int repeat)
{
    printf ("keycode: 0x%08x, statu: %d\n", keycode, press);
}
#endif

GtkWidget *CreateSkinWnd (int width, int height)
{
    GtkWidget *skin_widget = gtk_drawing_area_new ();

    g_gvfbskin->skin_widget = skin_widget;

    gtk_widget_set_size_request (GTK_WIDGET (skin_widget), width, height);

    gtk_widget_set_events (skin_widget,
                           GDK_EXPOSURE_MASK | GDK_LEAVE_NOTIFY_MASK |
                           GDK_POINTER_MOTION_HINT_MASK);

    gtk_widget_add_events (skin_widget,
                           GDK_BUTTON_PRESS_MASK | GDK_BUTTON_RELEASE_MASK |
                           GDK_POINTER_MOTION_MASK);

    /* setup signal */
    gtk_signal_connect (GTK_OBJECT (skin_widget), "button_release_event",
                        GTK_SIGNAL_FUNC (mouse_button_up), g_gvfbskin);

    gtk_signal_connect (GTK_OBJECT (skin_widget), "button_press_event",
                        GTK_SIGNAL_FUNC (mouse_button_down), g_gvfbskin);

    gtk_signal_connect (GTK_OBJECT (skin_widget), "motion_notify_event",
                        GTK_SIGNAL_FUNC (mouse_move), g_gvfbskin);

    gtk_signal_connect (GTK_OBJECT (skin_widget), "expose-event",
                        GTK_SIGNAL_FUNC (expose_event), g_gvfbskin);

    return skin_widget;
}

static char *normal_path (char *path)
{
    char *p;
    for (p = path; *p; ++p) {
        if (*p == '\\') {
            *p = '/';
        }
    }
    return path;
}

int InitSkin (const char *_skinfile)
{
    SKIN *pskin;
    SKINKEYITEM *item;

    char basedir[PATH_MAX];
    char imagename[PATH_MAX];
    char skinfile[PATH_MAX];

    char *pos;

    int i;

    int err_flag = 0;


    memset (basedir, '\0', sizeof (basedir));

    strcpy (skinfile, _skinfile);
    normal_path (skinfile);

    pos = strrchr (skinfile, '/');

    if (pos != NULL) {
        strncpy (basedir, skinfile, (int) (pos - skinfile));
    }

    g_gvfbskin = (GVFBSKIN *) malloc (sizeof (GVFBSKIN));

    if (g_gvfbskin == NULL) {
        msg_out (LEVEL_0, "malloc memory error.(InitSkin)");

        return -1;
    }

    memset (g_gvfbskin, 0x00, sizeof (GVFBSKIN));

    do {
        /* load configure file */
        g_gvfbskin->pskin = SkinLoad (skinfile);

        if (g_gvfbskin->pskin == NULL) {
            msg_out (LEVEL_0, "SkinLoad error.(%s)", skinfile);

            err_flag = 1;

            break;
        }

        /* load pixbufs */
        for (i = 0; i < SKIN_FACE_NUM; i++) {
            if (g_gvfbskin->pskin->images[i][0] != 0) {
                normal_path (g_gvfbskin->pskin->images[i]);
                pos = strchr (g_gvfbskin->pskin->images[i], '/');
                if (pos != NULL) {
                    strcpy (imagename, g_gvfbskin->pskin->images[i]);
                }
                else {
                    sprintf (imagename, "%s/%s", basedir,
                             g_gvfbskin->pskin->images[i]);
                }

                g_gvfbskin->pixbufs[i] =
                    gdk_pixbuf_new_from_file (imagename, NULL);

                if (g_gvfbskin->pixbufs[i] == NULL) {
                    msg_out (LEVEL_0, "error pasting.(%s)", imagename);

                    err_flag = 1;

                    break;
                }
            }
            else {
                msg_out (LEVEL_0,
                         "lost skin images. please check skin file format.(%s)",
                         skinfile);

                err_flag = 1;

                break;
            }   /* end if */
        }       /* end for */

        if (err_flag != 0) {
            break;
        }

        pskin = g_gvfbskin->pskin;

        /* create region */
        for (i = 0; i < pskin->skinitem_num; ++i) {
            item = &(pskin->skinkeyitem[i]);

            if (!item->region) {
                item->region =
                    gdk_region_polygon ((GdkPoint *) item->polygon.points,
                                        item->polygon.npoint, GDK_WINDING_RULE);
            }
        }

        assert (g_gvfbskin->pixbufs[SKIN_UP_FACE] != NULL);

        g_gvfbskin->skin_width =
            gdk_pixbuf_get_width (g_gvfbskin->pixbufs[SKIN_UP_FACE]);
        g_gvfbskin->skin_height =
            gdk_pixbuf_get_height (g_gvfbskin->pixbufs[SKIN_UP_FACE]);
    }
    while (0);

    if (err_flag != 0) {
        /* error */
        for (i = 0; i < SKIN_FACE_NUM; i++) {
            if (g_gvfbskin->pixbufs[i] != NULL) {
                g_object_unref (g_gvfbskin->pixbufs[i]);
                g_gvfbskin->pixbufs[i] = NULL;
            }
        }

        if (g_gvfbskin->pskin != NULL) {
            SkinUnload (g_gvfbskin->pskin);
            g_gvfbskin->pskin = NULL;
        }

        if (g_gvfbskin != NULL) {
            free (g_gvfbskin);
            g_gvfbskin = NULL;
        }

        return -1;
    }

    return 0;
}

void UnInitSkin ()
{
    SKIN *pskin;
    SKINKEYITEM *item;

    int i;

    assert (g_gvfbskin != NULL);

    pskin = g_gvfbskin->pskin;

    /* destroy region */
    for (i = 0; i < pskin->skinitem_num; ++i) {
        item = &(pskin->skinkeyitem[i]);

        if (item->region) {
            gdk_region_destroy ((GdkRegion *) item->region);
            item->region = NULL;
        }
    }

    SkinUnload (g_gvfbskin->pskin);
    g_gvfbskin->pskin = NULL;

    for (i = 0; i < SKIN_FACE_NUM; ++i) {
        if (g_gvfbskin->pixbufs[i] == NULL) {
            continue;
        }

        g_object_unref (g_gvfbskin->pixbufs[i]);
        g_gvfbskin->pixbufs[i] = NULL;
    }

    g_gvfbskin->skin_widget = NULL;
    g_gvfbskin->pressed_item = NULL;

    free (g_gvfbskin);
    g_gvfbskin = NULL;
}

void GetSkinScreenRect (SKINRECT *screenrect)
{
    assert (g_gvfbskin != NULL);
    assert (g_gvfbskin->pskin != NULL);

    assert (screenrect != NULL);

    screenrect->left = g_gvfbskin->pskin->screenrect.left;
    screenrect->top = g_gvfbskin->pskin->screenrect.top;
    screenrect->right = g_gvfbskin->pskin->screenrect.right;
    screenrect->bottom = g_gvfbskin->pskin->screenrect.bottom;
}

void GetSkinSize (int *width, int *height)
{
    assert (g_gvfbskin != NULL);

    *width = g_gvfbskin->skin_width;
    *height = g_gvfbskin->skin_height;
}


/* static function */
static int draw_skin (GtkWidget * widget, SKIN *pskin)
{
    GdkGC *gc = NULL;
    GdkPixmap *pixmap = NULL;

    SKINKEYITEM *item = NULL;

    int i;

    int width = g_gvfbskin->skin_width;
    int height = g_gvfbskin->skin_height;

    pixmap = gdk_pixmap_new (widget->window, width, height,
                             gdk_drawable_get_depth (widget->window));

    if (pixmap == NULL) {
        msg_out (LEVEL_0, "draw_skin error.(gdk_pixmap_new)");

        return -1;
    }

    /* draw background */
    gc = gdk_gc_new (widget->window);

    gdk_gc_set_clip_region (gc, gdk_drawable_get_clip_region (widget->window));

    gdk_draw_rectangle (pixmap, gc, TRUE, 0, 0, width, height);

    gdk_draw_pixbuf (pixmap, gc, g_gvfbskin->pixbufs[SKIN_UP_FACE], 0, 0,
                     0, 0, width, height, GDK_RGB_DITHER_NONE, 0, 0);

    gdk_draw_drawable (widget->window, gc, pixmap, 0, 0, 0, 0, width, height);

    /* set region for foreground if button down */
    for (i = 0; i < pskin->skinitem_num; ++i) {
        item = &pskin->skinkeyitem[i];

        if (item->state == SKIN_BUTTON_DOWN) {

            /* draw foreground */
            gdk_draw_pixbuf (pixmap, gc, g_gvfbskin->pixbufs[SKIN_DOWN_FACE], 0,
                             0, 0, 0, width, height, GDK_RGB_DITHER_NONE, 0, 0);

            gdk_draw_drawable (widget->window, gc, pixmap, 0, 0, 0, 0, width,
                               height);

            break;
        }
    }

    g_object_unref (gc);
    g_object_unref (pixmap);

    return 0;
}

static gboolean mouse_button_down (GtkWidget * widget, GdkEventButton * event,
                                   gpointer data)
{
    GVFBSKIN *gvfbskin = (GVFBSKIN *) data;

    if (event->button == 1) {   /* left button */
        SKINPOINT point = { (int) event->x, (int) event->y };
        SKINKEYITEM *item = SkinPointToSkinItem (gvfbskin->pskin, &point);

        if (item && item != gvfbskin->pressed_item) {     /* at item */

            msg_log("%s: sending keyboard data: %d, pressed\n", __FUNCTION__, item->keycode);
            SendKeyboardData (item->keycode, 1, 0);

            item->state = SKIN_BUTTON_DOWN;
            gvfbskin->pressed_item = item;

            gdk_window_invalidate_region (widget->window,
                                          (GdkRegion *) item->region, 0);
        }
    }

    return TRUE;
}

static gboolean mouse_button_up (GtkWidget * widget, GdkEventButton * event,
                                 gpointer data)
{
    GVFBSKIN *gvfbskin = (GVFBSKIN *) data;

    if (event->button == 1) {
        /* left button */
        SKINKEYITEM *item = gvfbskin->pressed_item;

        if (!item) {
            return TRUE;
        }

        assert (item->region != NULL);

        if (item) {     /* at item */
            msg_log("%s: sending keyboard data: %d, released\n", __FUNCTION__, item->keycode);
            SendKeyboardData (item->keycode, 0, 0);

            item->state = SKIN_BUTTON_UP;
            gvfbskin->pressed_item = NULL;

            gdk_window_invalidate_region (widget->window,
                                          (GdkRegion *) item->region, 0);
        }
    }

    return TRUE;
}

static gboolean mouse_move (GtkWidget * widget, GdkEventButton * event,
                            gpointer data)
{
    GVFBSKIN *gvfbskin = (GVFBSKIN *) data;
    SKINPOINT point = { (int) event->x, (int) event->y };
    SKINKEYITEM *lastitem = gvfbskin->pressed_item;
    SKINKEYITEM *item = SkinPointToSkinItem (gvfbskin->pskin, &point);

    /* if mouse move to anther item reset statu */
    if (!lastitem || (item && item == lastitem)) {
        return TRUE;
    }

    msg_log("%s: sending keyboard data: %d, released\n", __FUNCTION__, lastitem->keycode);
    SendKeyboardData (lastitem->keycode, 0, 0);

    lastitem->state = SKIN_BUTTON_UP;
    gvfbskin->pressed_item = NULL;

    /* send expose event and wait */
    gdk_window_invalidate_region (widget->window,
                                  (GdkRegion *) lastitem->region, 0);

    return TRUE;
}

static gboolean expose_event (GtkWidget * widget, GdkEventExpose * event,
                              gpointer data)
{
    GVFBSKIN *gvfbskin = (GVFBSKIN *) data;

    draw_skin (widget, gvfbskin->pskin);

    return TRUE;
}

#ifdef SKINDEBUG
static void destroy (GtkWidget * widget, gpointer data)
{
    printf ("destory\n");
    gtk_main_quit ();
}

int main (int argc, char *argv[])
{
    GtkWidget *skin_widget;
    int skin_w, skin_h;

    gtk_init (&argc, &argv);
    GtkWidget *window = gtk_window_new (GTK_WINDOW_TOPLEVEL);

    g_signal_connect (window, "destroy", G_CALLBACK (destroy), NULL);

    if (argc > 1) {
        InitSkin (argv[1]);
    }
    else {
        InitSkin ("dial.skin");
    }

    GetSkinSize (&skin_w, &skin_h);
    skin_widget = CreateSkinWnd (skin_w, skin_h);

    gtk_container_add (GTK_CONTAINER (window), skin_widget);

    gtk_widget_show_all (window);

    gtk_main ();

    UnInitSkin ();

    return TRUE;
}
#endif
