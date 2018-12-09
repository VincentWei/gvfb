#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <glib.h>
#include <gtk/gtk.h>

#include <assert.h>

#ifdef WIN32
#else
#   include <unistd.h>
#   include <strings.h>
#endif

#include "gvfbhdr.h"
#include "gvfb_main.h"
#include "gvfb_callbacks.h"

static const gchar *ui_description =
    "<ui>"
    "  <menubar name='MainMenu'>"
    "    <menu action='FileMenuAction'>"
    "      <menuitem action='FileSaveImageMenuAction' />"
    "      <separator />"
    "      <menuitem action='FileQuitMenuAction' />"
    "    </menu>"
    "    <menu action='ViewMenuAction'>"
    "      <menuitem action='ViewShowGtkCursorMenuAction' />"
    "      <separator />"
    "      <menuitem action='ViewRefreshRateMenuAction' />"
    "      <separator />"
    "      <menuitem action='ViewAutoFitScreenMenuAction' />"
    "      <separator />"
    "      <menuitem action='ViewFullScreenMenuAction' />"
    "      <separator />"
    "      <menuitem action='ViewZoomScale400MenuAction' />"
    "      <menuitem action='ViewZoomScale200MenuAction' />"
    "      <menuitem action='ViewZoomScale100MenuAction' />"
    "      <menuitem action='ViewZoomScale050MenuAction' />"
    "      <menuitem action='ViewZoomScale025MenuAction' />"
    "    </menu>"
    "    <menu action='HelpMenuAction'>"
    "      <menuitem action='HelpAboutMenuAction' />"
    "    </menu>" "  </menubar>" "</ui>";

/* Normal items */
/*
typedef struct {
  const gchar   *name;
  const gchar   *stock_id;
  const gchar   *label;
  const gchar   *accelerator;
  const gchar   *tooltip;
  GCallback     callback;
} GtkActionEntry;
*/

/* main menu */
static GtkActionEntry main_entries[] = {
    {"FileMenuAction", NULL, "File", NULL, NULL, NULL},
    {"ViewMenuAction", NULL, "View", NULL, NULL, NULL},
    {"HelpMenuAction", NULL, "Help", NULL, NULL, NULL}
};

static guint n_main_entries = G_N_ELEMENTS (main_entries);

/* normal menu */
static GtkActionEntry normal_entries[] = {
    /* File */
    {"FileSaveImageMenuAction", NULL,
     "Save Image...", NULL, "Save Image...",
     ACTION_CB (on_m_save_image_cb)},

    {"FileQuitMenuAction", NULL,
     "Quit", NULL, "Quit",
     ACTION_CB (on_m_quit_cb)},

    /* View */
    {"ViewRefreshRateMenuAction", NULL,
     "Refresh Rate...", NULL, "Refresh Rate Setting...",
     ACTION_CB (on_m_refresh_rate_cb)},

    /* help */
    {"HelpAboutMenuAction", NULL,
     "About...", NULL, "About GVFB...",
     ACTION_CB (on_m_about_cb)}
};

static guint n_normal_entries = G_N_ELEMENTS (normal_entries);

/* Toggle items */
/*
typedef struct {
  const gchar  *name;
  const gchar  *stock_id;
  const gchar  *label;
  const gchar  *accelerator;
  const gchar  *tooltip;
  GCallback    callback;
  gboolean     is_active;
} GtkToggleActionEntry;
*/

static GtkToggleActionEntry toggle_entries[] = {
    {"ViewShowGtkCursorMenuAction", NULL,
     "Show GTK Cursor", NULL, "Show GTK Cursor",
     ACTION_CB (on_m_show_gtk_cursor_cb), FALSE},

    {"ViewAutoFitScreenMenuAction", NULL,
     "Auto Fit-Screen", NULL, "Auto Fit Screen",
     ACTION_CB (on_m_auto_fit_screen_cb), TRUE},

    {"ViewFullScreenMenuAction", NULL,
     "Full Screen", NULL, "Full Screen",
     ACTION_CB (on_m_full_screen_cb), FALSE}
};

static guint n_toggle_entries = G_N_ELEMENTS (toggle_entries);

/* Radio items */
/*
typedef struc {
  const gchar   *name;
  const gchar   *stock_id;
  const gchar   *label;
  const gchar   *accelerator;
  const gchar   *tooltip;
  gint          value;
} GtkRadioActionEntry;
*/

/* default zoom is 100 */
static GtkRadioActionEntry radio_entries[] = {
    {"ViewZoomScale400MenuAction", NULL,
     "zoom_percent Scale 4", NULL, "zoom_percent Scale", 400},

    {"ViewZoomScale200MenuAction", NULL,
     "zoom_percent Scale 2", NULL, "zoom_percent Scale", 200},

    {"ViewZoomScale100MenuAction", NULL,
     "zoom_percent Scale 1", NULL, "zoom_percent Scale", 100},

    {"ViewZoomScale050MenuAction", NULL,
     "zoom_percent Scale 0.5", NULL, "zoom_percent Scale", 50},

    {"ViewZoomScale025MenuAction", NULL,
     "zoom_percent Scale 0.25", NULL, "zoom_percent Scale", 25}
};

static guint n_radio_entries = G_N_ELEMENTS (radio_entries);

GtkUIManager *CreateMainMenu (GtkWidget * window)
{
    GtkActionGroup *action_group;
    GtkUIManager *ui_manager;
    GtkAccelGroup *accel_group;

    action_group = gtk_action_group_new ("MenuActions");

    /* Normal items */
    gtk_action_group_add_actions (action_group,
                                  main_entries, n_main_entries, window);

    gtk_action_group_add_actions (action_group,
                                  normal_entries, n_normal_entries, window);

    /* Toggle items */
    gtk_action_group_add_toggle_actions (action_group,
                                         toggle_entries, n_toggle_entries,
                                         window);

    /* Radio items */
    gtk_action_group_add_radio_actions (action_group,
                                        radio_entries, n_radio_entries, -1,
                                        ACTION_CB (on_m_zoom_scale_cb), window);

    /* ui manager */
    ui_manager = gtk_ui_manager_new ();

    gtk_ui_manager_insert_action_group (ui_manager, action_group, 0);

    accel_group = gtk_ui_manager_get_accel_group (ui_manager);
    gtk_window_add_accel_group (GTK_WINDOW (window), accel_group);

    if (!gtk_ui_manager_add_ui_from_string
        (ui_manager, ui_description, -1, NULL)) {
        return NULL;
    }

    return ui_manager;
}
