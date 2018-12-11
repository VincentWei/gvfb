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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <sys/stat.h>
#include <sys/types.h>
#include <sys/stat.h>

#include "deviceskin.h"
#include "gvfb_log.h"

#ifdef WIN32
#   pragma warning(disable:4996)
#   define strtok_r(x, y, z) strtok_s(x, y, z)
#endif /* WIN32 */

/* static function */
static int parse_rect (char *str, SKINRECT *rc);
static int parse_skin_key_item (char *data, SKINKEYITEM *item,
                                int parsed_count);
static int point_in_polygon (int tx, int ty, SKINPOINT *points, int npoint);

/* static */
static const char *FILE_HEADER = "[SkinFile]";
static const char *UP = "Up";
static const char *DOWN = "Down";
static const char *CLOSED = "Closed";
static const char *SCREEN = "Screen";
static const char *AREAS = "Areas";

SKIN *SkinLoad (const char *skinfile)
{
    FILE *fp = NULL;
    SKIN *pskin = NULL;

    char buf[PATH_MAX];

    int btn_parsed = 0;

    int err_flag = 0;

    //form full path name of the skin config file
    fp = fopen (skinfile, "r");

    if (NULL == fp) {
        msg_out (LEVEL_0, "cannot open skin file.(%s)", skinfile);

        return NULL;
    }

    pskin = (SKIN *) malloc (sizeof (SKIN));

    if (NULL == pskin) {
        msg_out (LEVEL_0, "cannot malloc memory.");

        /* close opened file */
        fclose (fp);
        fp = NULL;

        return NULL;
    }

    memset (pskin, 0x00, sizeof (SKIN));
    memset (buf, 0x00, sizeof (buf));

    do {
        if (NULL == fgets (buf, sizeof (buf), fp)) {
            msg_out (LEVEL_0, "cannot read skin file.(%s)", skinfile);

            err_flag = 1;

            break;
        }

        if (0 != strncmp (buf, FILE_HEADER, strlen (FILE_HEADER))) {
            msg_out (LEVEL_0, "wrong skin file format.(%s)", skinfile);

            err_flag = 1;

            break;
        }

        /* loop */
        while (TRUE) {
            int i = 0;
            //read a line from skin config file
            char *result = fgets (buf, sizeof (buf), fp);

            if (NULL == result) {
                /* end of file */
                break;
            }

            //skip whitespace
            while (result[i++] == ' ') {
                result++;
            }

            i = strlen (result);

            while (i--
                   && (result[i] == '\n' || result[i] == ' '
                       || result[i] == '\r')) {
                result[i] = 0;
            }

            if (strlen (result) < 2) {
                continue;
            }

            //skip comment
            if (result[0] == '#') {
                continue;
            }

            //parse the line
            if (0 == strncmp (result, UP, strlen (UP))) {
                result = strchr (result, '=') + 1;

                if (result) {
                    strcpy (pskin->images[SKIN_UP_FACE], result);
                }
            }
            else if (0 == strncmp (result, DOWN, strlen (DOWN))) {
                result = strchr (result, '=') + 1;

                if (result) {
                    strcpy (pskin->images[SKIN_DOWN_FACE], result);
                }
            }
            else if (0 == strncmp (result, CLOSED, strlen (CLOSED))) {
                result = strchr (result, '=') + 1;

                if (result) {
                    strcpy (pskin->images[SKIN_CLOSE_FACE], result);
                }
            }
            else if (0 == strncmp (result, SCREEN, strlen (SCREEN))) {
                result = strchr (result, '=') + 1;

                if (result) {
                    parse_rect (result, &pskin->screenrect);
                }
            }
            else if (0 == strncmp (result, AREAS, strlen (AREAS))) {
                result = strchr (result, '=') + 1;

                if (result) {
                    /* fix num in c */
                    pskin->skinitem_num = atoi (result);
                }

                pskin->skinkeyitem =
                    (SKINKEYITEM *) calloc (pskin->skinitem_num,
                                            sizeof (SKINKEYITEM));

                if (!pskin->skinkeyitem) {
                    msg_out (LEVEL_0, "calloc memory for item error.");

                    err_flag = 1;

                    break;
                }

                memset (pskin->skinkeyitem, 0x00,
                        pskin->skinitem_num * sizeof (SKINKEYITEM));
            }
            else if (result[0] == '"') {
                if (0 == pskin->skinkeyitem) {
                    msg_out (LEVEL_0, "botton number not specified.");

                    err_flag = 1;

                    break;
                }

                if (0 ==
                    parse_skin_key_item (result, pskin->skinkeyitem,
                                         btn_parsed)) {
                    btn_parsed++;
                }       /* end if */
            }
            else {
                msg_out (LEVEL_0, "unknown element of skin file.(%s)", result);
            }   /* end if */
        }       /* end while (TRUE) */

        if (err_flag != 0) {
            break;
        }

        if (btn_parsed != pskin->skinitem_num) {
            msg_out (LEVEL_0, "wrong skin file key number.");

            err_flag = 1;

            break;
        }
    }
    while (0);

    if (err_flag != 0) {
        /* has error */
        if (pskin != NULL) {
            if (pskin->skinkeyitem != NULL) {
                free (pskin->skinkeyitem);
                pskin->skinkeyitem = NULL;
            }

            free (pskin);
            pskin = NULL;
        }

        return NULL;
    }

    if (fp != NULL) {
        fclose (fp);
        fp = NULL;
    }

    return pskin;
}

void SkinUnload (SKIN *pskin)
{
    SKINKEYITEM *item;
    int i;

    /* free points */
    for (i = 0; i < pskin->skinitem_num - 1; ++i) {
        item = &pskin->skinkeyitem[i];
        free (item->polygon.points);
    }

    if (pskin->skinkeyitem) {
        free (pskin->skinkeyitem);
        pskin->skinkeyitem = NULL;
    }

    if (pskin) {
        free (pskin);
        pskin = NULL;
    }

    return;
}

SKINKEYITEM *SkinKeyCodeToSkinItem (SKIN *pskin, int keycode)
{
    SKINKEYITEM *item;
    int i;

    for (i = 0; i < pskin->skinitem_num; i++) {
        item = &pskin->skinkeyitem[i];

        if (item->keycode == keycode) {
            return item;
        }
    }

    return NULL;
}

SKINKEYITEM *SkinPointToSkinItem (SKIN *pskin, const SKINPOINT *pt)
{
    SKINKEYITEM *item;
    int i;

    for (i = 0; i < pskin->skinitem_num; i++) {
        item = &pskin->skinkeyitem[i];

        if (point_in_polygon
            (pt->x, pt->y, item->polygon.points, item->polygon.npoint)) {
            return item;
        }
    }

    return NULL;
}

static int parse_rect (char *str, SKINRECT *rect)
{
    char *token;
    char *delim = " ";
    char *saveptr;
    int v[4];
    int i;

    for (i = 0;; i++, str = NULL) {
        token = strtok_r (str, delim, &saveptr);
        if (token == NULL) {
            if (i <= 3) {
                msg_out (LEVEL_0, "less the four axis value.");

                return -1;
            }

            break;
        }

        v[i] = atoi (token);

        if (i > 3) {
            break;
        }
    }

    rect->left = v[0];
    rect->top = v[1];
    rect->right = v[2];
    rect->bottom = v[3];

    return 0;
}

static int parse_skin_key_item (char *data, SKINKEYITEM *item, int parsed_count)
{
    char *token;
    char *delim = " \t\n\"";
    char *saveptr = NULL;
    int v[POLYGON_MAX_POINTS * 2];
    int val_count = 0;
    int i;
    char *str = data;
    SKINPOINT *points = NULL;
    int npoint = 0;

    int err_flag = 0;

    do {
        for (i = 0;; i++, str = NULL) {
            token = strtok_r (str, delim, &saveptr);
            if (token == NULL) {
                if (i < 5) {
                    msg_out (LEVEL_0, "wrong button item format.");

                    err_flag = 1;

                    break;
                }

                break;
            }

            if (i == 0) {
                /* key name */
                strcpy (item[parsed_count].name, token);
            }
            else if (i == 1) {
                /* scancode */
                char *endptr;

                item[parsed_count].keycode =
                    (unsigned short) strtol (token, &endptr, 0);
            }
            else {      /* get value */
                v[i - 2] = atoi (token);
                val_count++;
            }   /* end if */
        }       /* end for */

        if (err_flag != 0) {
            break;
        }

        if (val_count % 2) {
            msg_out (LEVEL_0, "miss y position for last point, drop it.(%s)",
                     item[parsed_count].name);

            val_count--;
        }

        points = (SKINPOINT *) calloc (POLYGON_MAX_POINTS, sizeof (SKINPOINT));

        if (val_count < 4) {
            msg_out (LEVEL_0, "less position to create area.(%s)",
                     item[parsed_count].name);

            err_flag = 1;

            break;
        }
        else if (val_count == 4) {
            points[0].x = v[0];
            points[0].y = v[1];
            points[1].x = v[2];
            points[1].y = v[1];
            points[2].x = v[2];
            points[2].y = v[3];
            points[3].x = v[0];
            points[3].y = v[3];

            npoint = 4;
        }
        else {
            for (i = 0; i < val_count; i += 2) {
                if (npoint > POLYGON_MAX_POINTS) {
                    msg_out (LEVEL_0, "too many points.");

                    err_flag = 1;

                    break;
                }

                points[npoint].x = v[i];
                points[npoint].y = v[i + 1];

                npoint++;
            }   /* end for */

            if (err_flag != 0) {
                break;
            }
        }       /* end if */

        item[parsed_count].polygon.points = points;
        item[parsed_count].polygon.npoint = npoint;

#ifdef DEBUG
        {
            int i;

            printf ("######\n");

            for (i = 0; i < npoint; i++) {
                printf ("name: %s, scancode: %d, points[%d]: (%d, %d)\n",
                        item[parsed_count].name, item[parsed_count].keycode, i,
                        points[i].x, points[i].y);
            }
        }
#endif
    }
    while (0);

    if (err_flag != 0) {
        if (points != NULL) {
            free (points);
            points = NULL;
        }

        return -1;
    }

    return 0;
}

static int point_in_polygon (int tx, int ty, SKINPOINT *points, int npoint)
{
    int *arry = (int *) points;
    int i;
    int yflag0, yflag1, inside_flag;
    int vtx0, vty0, vtx1, vty1;
    int k;

    if (npoint < 3) {
        return -1;
    }

    vtx0 = arry[(npoint - 1) * 2];
    vty0 = arry[(npoint - 1) * 2 + 1];

    yflag0 = (vty0 >= ty) ? 1 : 0;

    vtx1 = arry[0];
    vty1 = arry[1];

    inside_flag = 0;

    for (i = 1; i <= npoint; i++) {
        yflag1 = (vty1 >= ty) ? 1 : 0;

        if (yflag0 != yflag1) {
            if (((vty1 - ty) * (vtx0 - vtx1) >= (vtx1 - tx) * (vty0 - vty1)) ==
                yflag1) {
                inside_flag ^= 1;
            }
        }

        yflag0 = yflag1;
        vtx0 = vtx1;
        vty0 = vty1;

        k = (i >= npoint) ? i - npoint : i;
        vtx1 = arry[k * 2];
        vty1 = arry[k * 2 + 1];
    }

    /* if in polygon return 1; else return 0 */
    return ((inside_flag != 0) ? 1 : 0);
}
