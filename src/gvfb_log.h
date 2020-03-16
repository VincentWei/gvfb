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

#ifndef _GVFB_LOG_H_
#define _GVFB_LOG_H_

#define LEVEL_0 0
#define LEVEL_1 1
#define LEVEL_2 2

#ifdef DEBUG
void msg_log(const char* fmt, ...);
#else   /* defined DEBUG */
#define msg_log(fmt, ...)   \
    do {                    \
    } while(0)
#endif  /* not defined DEBUG */

#ifdef SHOW_DBG_MSG
#ifdef SHOW_FULL_DBG_MSG
#define msg_out(_level,...)                                 \
    do {                                                    \
        msg_log ("[%s]%d: ", __FILE__, __LINE__);           \
        msg_log (__VA_ARGS__);                              \
        msg_log ("\n");                             \
    }                                                       \
    while (0)
#else /* SHOW_FULL_DBG_MSG */
#define msg_out(_level,...)             \
    do {                                \
        msg_log (__VA_ARGS__);          \
        msg_log ("\n");                 \
    }                                   \
    while (0)
#endif /* not defined SHOW_FULL_DBG_MSG */

#else  /* SHOW_DBG_MSG */

#define msg_out(_level,...)

#endif /* not define SHOW_DBG_MSG */

#endif /* _GVFB_LOG_H_ */

