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
** Copyright (C) 2010~2018 Beijing FMSoft Technologies Co., Ltd.
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

#include <sys/types.h>

#ifdef WIN32
#   include <gtk/gtk.h>
#   include <gtk/gtkaction.h>
#   include <winsock2.h>

#   include "gvfb_win32.h"
#else
#   include <sys/socket.h>

#   include "gvfb_linux.h"
#endif

#include "gvfbhdr.h"
#include "gvfb_main.h"
#include "gvfb_view.h"
#include "gvfb_callbacks.h"

#define SNAPSHOT "minigui.png"

/* show save image dialog */
ACTION (on_m_save_image_cb)
{
    GtkWidget *file_dialog;
    gchar *filename;

    /* create dialog */
    file_dialog = gtk_file_chooser_dialog_new ("Save File", NULL,
                                               GTK_FILE_CHOOSER_ACTION_SAVE,
                                               GTK_STOCK_CANCEL,
                                               GTK_RESPONSE_CANCEL,
                                               GTK_STOCK_SAVE,
                                               GTK_RESPONSE_ACCEPT, NULL);

    gtk_file_chooser_set_do_overwrite_confirmation (GTK_FILE_CHOOSER
                                                    (file_dialog), TRUE);
    gtk_file_chooser_set_current_name (GTK_FILE_CHOOSER (file_dialog),
                                       SNAPSHOT);

    if (gtk_dialog_run (GTK_DIALOG (file_dialog)) == GTK_RESPONSE_ACCEPT) {
        filename =
            gtk_file_chooser_get_filename (GTK_FILE_CHOOSER (file_dialog));

        SaveImage (filename);

        g_free (filename);
    }

    /* destroy dialog */
    gtk_widget_destroy (file_dialog);
}

/* quit */
ACTION (on_m_quit_cb)
{
    int event_type = GVFB_CLOSE_TYPE;

    Send (gvfbruninfo.sockfd,
            (const void *) &event_type, sizeof (event_type), 0);

    gvfbruninfo.running = FALSE;

    /* end running */
}

/* show hide cursor */
TOGGLEACTION (on_m_show_gtk_cursor_cb)
{
    gboolean b_show_gtk_cursor;

    b_show_gtk_cursor = gtk_toggle_action_get_active (toggleaction);

    if (b_show_gtk_cursor) {
        gdk_window_set_cursor (gvfbruninfo.draw_area->window,
                               gvfbruninfo.gtkdef_cursor);
    }
    else {
        gdk_window_set_cursor (gvfbruninfo.draw_area->window,
                               gvfbruninfo.userdef_cursor);
    }
}

/* show fps set dialog */
ACTION (on_m_refresh_rate_cb)
{
    GtkWidget *fpsset_dialog;
    GtkWidget *refresh_fixed;

    GtkWidget *slider;
    GtkWidget *label;

    /* create dialog */
    fpsset_dialog = gtk_dialog_new_with_buttons ("Refresh Setting", NULL,
                                                 GTK_DIALOG_MODAL |
                                                 GTK_DIALOG_DESTROY_WITH_PARENT
                                                 | GTK_DIALOG_NO_SEPARATOR,
                                                 GTK_STOCK_CANCEL,
                                                 GTK_RESPONSE_CANCEL,
                                                 GTK_STOCK_OK,
                                                 GTK_RESPONSE_ACCEPT, NULL);

    gtk_widget_set_size_request (GTK_WIDGET (fpsset_dialog), 240, 100);

    gtk_window_set_resizable (GTK_WINDOW (fpsset_dialog), FALSE);

    /* setup fixed */
    refresh_fixed = gtk_fixed_new ();

    gtk_box_pack_start (GTK_BOX (GTK_DIALOG (fpsset_dialog)->vbox),
                        refresh_fixed, TRUE, TRUE, 0);

    gtk_widget_show (refresh_fixed);

    /* setup slider */
    slider = gtk_hscale_new_with_range (1, 100, 1);

    gtk_widget_set_size_request (slider, 200, 60);
    gtk_scale_set_draw_value (GTK_SCALE (slider), TRUE);
    gtk_scale_set_value_pos (GTK_SCALE (slider), GTK_POS_RIGHT);
    gtk_range_set_value (GTK_RANGE (slider), gvfbruninfo.refresh_rate);

    gtk_fixed_put (GTK_FIXED (refresh_fixed), slider, 10, 0);

    gtk_widget_show (slider);

    /* show label */
    label = gtk_label_new ("fps");

    gtk_label_set_use_markup (GTK_LABEL (label), TRUE);
    gtk_misc_set_alignment (GTK_MISC (label), 0, 0);
    gtk_label_set_justify (GTK_LABEL (label), GTK_JUSTIFY_RIGHT);

    gtk_fixed_put (GTK_FIXED (refresh_fixed), label, 215, 22);

    gtk_widget_show (label);

    /* running */
    if (gtk_dialog_run (GTK_DIALOG (fpsset_dialog)) == GTK_RESPONSE_ACCEPT) {
        gvfbruninfo.refresh_rate =
            (int) gtk_range_get_value (GTK_RANGE (slider));

        assert (gvfbruninfo.refresh_rate > 0);
    }

    /* destory */
    gtk_widget_destroy (fpsset_dialog);
}

/* fit screen setting */
TOGGLEACTION (on_m_auto_fit_screen_cb)
{
    int mainwnd_w, mainwnd_h;

    assert (!gvfbruninfo.has_skin);

    /* get fit flag */
    gvfbruninfo.fit_screen = gtk_toggle_action_get_active (toggleaction);

    gtk_window_get_size (GTK_WINDOW (gvfbruninfo.window),
                         &mainwnd_w, &mainwnd_h);

    FitScnRect (mainwnd_w,
                mainwnd_h - gvfbruninfo.menu_height, gvfbruninfo.fit_screen);
}

/* fullscreen setting */
TOGGLEACTION (on_m_full_screen_cb)
{
    static int myzoom;
    static gboolean old_fit_flag = TRUE;

    /* menu bar widget */
    GtkWidget *menu_bar = NULL;
    GtkAction *fitscreen_item;
    GtkAction *fullscreen_item;

    GdkColor color;

    fullscreen_item = gtk_ui_manager_get_action (gvfbruninfo.ui_manager,
            "/MainMenu/ViewMenuAction/ViewFullScreenMenuAction");

    if (!gvfbruninfo.o_fullscreen) {
        //gtk_toggle_action_set_active (GTK_TOGGLE_ACTION(fullscreen_item), FALSE);

        return;
    }

    fitscreen_item = gtk_ui_manager_get_action (gvfbruninfo.ui_manager,
            "/MainMenu/ViewMenuAction/ViewAutoFitScreenMenuAction");

    gvfbruninfo.full_screen =
        gtk_toggle_action_get_active (GTK_TOGGLE_ACTION (fullscreen_item));

    /* move menu bar */
    menu_bar = gtk_ui_manager_get_widget (gvfbruninfo.ui_manager, "/MainMenu");

    assert (menu_bar != NULL);

    if (gvfbruninfo.o_fullscreen && gvfbruninfo.full_screen) {
        gtk_widget_hide (menu_bar);
        gvfbruninfo.menu_height = 0;
    }
    else {
        if (gvfbruninfo.has_menu) {
            gtk_widget_show (menu_bar);
            gvfbruninfo.menu_height = gvfbruninfo.fix_menu_height;
        }
    }

    if (gvfbruninfo.full_screen) {
        /* save old zoom */
        myzoom = gvfbruninfo.zoom_percent;

        gtk_window_fullscreen (GTK_WINDOW (gvfbruninfo.window));

        /* Set black background */
        gdk_color_parse ("black", &color);

        gtk_widget_modify_bg (gvfbruninfo.layout, GTK_STATE_NORMAL, &color);

        old_fit_flag =
            gtk_toggle_action_get_active (GTK_TOGGLE_ACTION (fitscreen_item));

        gtk_toggle_action_set_active (GTK_TOGGLE_ACTION (fitscreen_item), TRUE);
    }
    else {
        /* release old zoom */
        gvfbruninfo.zoom_percent = myzoom;

        gtk_window_unfullscreen (GTK_WINDOW (gvfbruninfo.window));

        /* Set the background to normal color */
        color.red = 0xdcdc;
        color.green = 0xdada;
        color.blue = 0xd5d5;

        gtk_widget_modify_bg (gvfbruninfo.layout, GTK_STATE_NORMAL, &color);

        if (gvfbruninfo.has_skin) {
            assert (gvfbruninfo.zoom_percent == 100);
            return;
        }

        ZoomScale (gvfbruninfo.zoom_percent);

        gtk_toggle_action_set_active (GTK_TOGGLE_ACTION (fitscreen_item),
                                      old_fit_flag);
    }
}

/* zoom scale */
RADIOACTION (on_m_zoom_scale_cb)
{
    GtkAction *fitscreen_item;
    int value;

    if (gvfbruninfo.has_skin) {
        return;
    }

    value = gtk_radio_action_get_current_value (radioaction);

    assert (value > 0);

    /* set default */
    gvfbruninfo.zoom_percent = value;

    /* set fit screen false */
    fitscreen_item = gtk_ui_manager_get_action (gvfbruninfo.ui_manager,
            "/MainMenu/ViewMenuAction/ViewAutoFitScreenMenuAction");

    gtk_toggle_action_set_active (GTK_TOGGLE_ACTION (fitscreen_item), FALSE);

    ZoomScale (gvfbruninfo.zoom_percent);

    /* setup main window size */
    gvfbruninfo.main_w = gvfbruninfo.sclwnd_w;
    gvfbruninfo.main_h = gvfbruninfo.sclwnd_h + gvfbruninfo.menu_height;

    gtk_window_resize (GTK_WINDOW (gvfbruninfo.window),
                       gvfbruninfo.main_w, gvfbruninfo.main_h);
}

/* show about dialog */
ACTION (on_m_about_cb)
{
    GtkWidget *about_dialog;

    GtkWidget *title;
    GtkWidget *doc;

    gchar *strtitle =
        "<span size=\"16000\" foreground=\"black\">The Virtual "
        "Framebuffer Ver 1.2</span>\n\n";

    gchar *strdoc =
        "This application runs under gtk, emulating a "
        "framebuffer, which MiniGUI V3.2 can attach to just as if it "
        "was a hardware Linux framebuffer.\n\n"
        "With the aid of this development tool, you can develop "
        "MiniGUI V3.2 applications under X11 without having to "
        "switch to a virtual console. This means you can "
        "comfortably use your other development tools such as GUI "
        "profilers and debuggers.\n";

    /* create about_dialog */
    about_dialog = gtk_dialog_new_with_buttons ("About GVFB", NULL,
                                                GTK_DIALOG_MODAL |
                                                GTK_DIALOG_DESTROY_WITH_PARENT |
                                                GTK_DIALOG_NO_SEPARATOR,
                                                GTK_STOCK_OK, GTK_RESPONSE_OK,
                                                NULL);

    gtk_window_set_resizable (GTK_WINDOW (about_dialog), FALSE);

    /* start create title */
    title = gtk_label_new (strtitle);

    gtk_label_set_use_markup (GTK_LABEL (title), TRUE);
    gtk_misc_set_alignment (GTK_MISC (title), 0, 0);
    gtk_label_set_justify (GTK_LABEL (title), GTK_JUSTIFY_CENTER);
    gtk_box_pack_start (GTK_BOX (GTK_DIALOG (about_dialog)->vbox),
                        title, TRUE, TRUE, 0);

    gtk_widget_show (title);
    /* end create title */

    /* start create doc */
    doc = gtk_label_new (strdoc);

    gtk_label_set_use_markup (GTK_LABEL (doc), TRUE);
    gtk_misc_set_alignment (GTK_MISC (doc), 0, 0);
    gtk_label_set_justify (GTK_LABEL (doc), GTK_JUSTIFY_LEFT);
    gtk_label_set_line_wrap (GTK_LABEL (doc), TRUE);
    gtk_box_pack_start (GTK_BOX (GTK_DIALOG (about_dialog)->vbox),
                        doc, TRUE, TRUE, 0);

    gtk_widget_show (doc);
    /* end create doc */

    /* running */
    gtk_dialog_run (GTK_DIALOG (about_dialog));

    /* destory */
    gtk_widget_destroy (about_dialog);
}
