#ifndef _SKIN_H_
#define _SKIN_H_

#include <gtk/gtk.h>

#include "deviceskin.h"

/* struct of gvfb skin */
typedef struct _GVFBSKIN{
    SKIN *pskin;

    /* sizeof skin */
    int skin_width;
    int skin_height;

    GdkPixbuf *pixbufs[SKIN_FACE_NUM];
    GtkWidget *skin_widget;

    SKINKEYITEM *pressed_item;
} GVFBSKIN;

extern GVFBSKIN *g_gvfbskin;

/*
 * CreateSkinWnd   :
 *
 * Params          : width
 *                 : height
 *
 * Return          : widget of skin window
 */
GtkWidget *CreateSkinWnd (int width, int height);

/*
 * InitSkin   :
 *
 * Params     : skinfile
 *
 * Return     :  0 success
 *            : -1 fail
 */
int InitSkin (const char *skinfile);

/*
 * UnInitSkin   :
 *
 * Params       : (none)
 *
 * Return       : (none)
 */
void UnInitSkin ();

/*
 * GetSkinSize   :
 *
 * Params        : *width
 *               : *height
 *
 * Return        : (none)
 */
void GetSkinSize (int *width, int *height);

/*
 * GetSkinSize   :
 *
 * Params        : point to screenrect
 *
 * Return        :
 */
void GetSkinScreenRect (SKINRECT *screenrect);

#endif /* end of _SKIN_H_ */

