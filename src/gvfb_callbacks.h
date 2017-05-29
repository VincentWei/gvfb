#ifndef _GVFB_CALLBACKS_H_
#define _GVFB_CALLBACKS_H_

#include <glib.h>
#include <gtk/gtk.h>

#include <assert.h>

/* define */
#define ACTION(Action) void gvfb_##Action(GtkAction *action, gpointer data)
#define ACTION_CB(Action) G_CALLBACK(gvfb_##Action)
#define TOGGLEACTION(Action) void gvfb_##Action(GtkToggleAction *toggleaction, gpointer data)
#define RADIOACTION(Action) void gvfb_##Action(GtkRadioAction *radioaction, GtkRadioAction *currentradioaction, gpointer data)

/* Menu CallBack */
/* File Menu */
/* File chooser when save image */
ACTION(on_m_save_image_cb);

/* quit */
ACTION(on_m_quit_cb);

/* View Menu */
/* show gtk cursor */
TOGGLEACTION(on_m_show_gtk_cursor_cb);

/* show Refresh rate setting window */
ACTION(on_m_refresh_rate_cb);

/* fitscreen setting */
TOGGLEACTION(on_m_auto_fit_screen_cb);

/* fullscreen setting */
TOGGLEACTION(on_m_full_screen_cb);

/* Setting the zoom  */
RADIOACTION(on_m_zoom_scale_cb);

/* Help Menu */
/* gvfb about window */
ACTION(on_m_about_cb);

#endif /* end of _GVFB_CALLBACKS_H_ */

