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

#ifndef _GVFB_LOG_H_
#define _GVFB_LOG_H_

#define LEVEL_0 0
#define LEVEL_1 1
#define LEVEL_2 2

#ifndef msg_out

#ifdef SHOW_DBG_MSG
#ifdef SHOW_FULL_DBG_MSG
#define msg_out(_level,...)                                 \
    do {                                                    \
        fprintf (stderr, "[%s]%d: ", __FILE__, __LINE__);   \
        fprintf (stderr, __VA_ARGS__);                      \
        fprintf (stderr, "\n");                             \
    }                                                       \
    while (0)
#else /* SHOW_FULL_DBG_MSG */
#define msg_out(_level,...)             \
    do {                                \
        fprintf (stderr, __VA_ARGS__);  \
        fprintf (stderr, "\n");         \
    }                                   \
    while (0)
#endif /* SHOW_FULL_DBG_MSG */
#else  /* SHOW_DBG_MSG */
#define msg_out(_level,...)
#endif /* SHOW_DBG_MSG */

#endif

#ifdef DEBUG
void msg_log(const char* fmt, ...);
#else
#define msg_log(fmt, ...)   \
    do {                    \
    } while(0)
#endif

#endif /* _GVFB_LOG_H_ */

