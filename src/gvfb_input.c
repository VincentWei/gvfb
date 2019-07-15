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

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include <assert.h>

#ifdef WIN32
#   pragma warning(disable:4996)
#   include "gvfb_win32.h"
#else
#   include <sys/types.h>
#   include <sys/time.h>
#   include <sys/socket.h>
#   include <unistd.h>
#   include "gvfb_linux.h"
#endif

#include "gvfbhdr.h"
#include "gvfb_main.h"
#include "gvfb_input.h"
#include "gvfb_log.h"

#define MAX_KEY_LEN  50

/* global variable */
int IsKeypadData = 0;

/* locale function */
static unsigned int char_to_keycode (unsigned char c);
static int keycode_to_scancode (unsigned int keycode, unsigned char *scancode);
static void send_key (unsigned char scancode, unsigned int state);

/* locale value */
static int last_key = -1;
static gulong last_time = 0;
static int s_wait = 0;
static int s_key_press_flag[128];
static unsigned char keycode_scancode[256];
static unsigned int unpressedKey[MAX_KEY_LEN];
const unsigned int scancode_not_exist = 0x00;


/* init code map */
void InitCodeMap (void)
{
    memset (s_key_press_flag, 0x00, sizeof (s_key_press_flag));
    memset (unpressedKey, scancode_not_exist, sizeof (unpressedKey));

    keycode_scancode[0x20] = SCANCODE_SPACE;
    keycode_scancode[0x21] = SCANCODE_1;        /* ! */
    keycode_scancode[0x22] = SCANCODE_APOSTROPHE;       /* " */
    keycode_scancode[0x23] = SCANCODE_3;        /* # */
    keycode_scancode[0x24] = SCANCODE_4;        /* $ */
    keycode_scancode[0x25] = SCANCODE_5;        /* % */
    keycode_scancode[0x26] = SCANCODE_7;        /* & */
    keycode_scancode[0x27] = SCANCODE_APOSTROPHE;       /* ' */
    keycode_scancode[0x28] = SCANCODE_9;        /* ( */
    keycode_scancode[0x29] = SCANCODE_0;        /* ) */
    keycode_scancode[0x2A] = SCANCODE_KEYPADMULTIPLY;   /* * */
    keycode_scancode[0x2B] = SCANCODE_KEYPADPLUS;       /* + */
    keycode_scancode[0x2C] = SCANCODE_COMMA;    /* , */
    keycode_scancode[0x2D] = SCANCODE_MINUS;    /* - */
    keycode_scancode[0x2E] = SCANCODE_PERIOD;   /* . */
    keycode_scancode[0x2F] = SCANCODE_SLASH;    /* / */

    keycode_scancode[0x30] = SCANCODE_0;
    keycode_scancode[0x31] = SCANCODE_1;
    keycode_scancode[0x32] = SCANCODE_2;
    keycode_scancode[0x33] = SCANCODE_3;
    keycode_scancode[0x34] = SCANCODE_4;
    keycode_scancode[0x35] = SCANCODE_5;
    keycode_scancode[0x36] = SCANCODE_6;
    keycode_scancode[0x37] = SCANCODE_7;
    keycode_scancode[0x38] = SCANCODE_8;
    keycode_scancode[0x39] = SCANCODE_9;
    keycode_scancode[0x3A] = SCANCODE_SEMICOLON;        /* : */
    keycode_scancode[0x3B] = SCANCODE_SEMICOLON;        /* ; */
    keycode_scancode[0x3C] = SCANCODE_COMMA;    /* < */
    keycode_scancode[0x3D] = SCANCODE_EQUAL;    /* = */
    keycode_scancode[0x3E] = SCANCODE_PERIOD;   /* > */
    keycode_scancode[0x3F] = SCANCODE_SLASH;    /* ? */
    keycode_scancode[0x40] = SCANCODE_2;        /* @ */

    keycode_scancode[0x41] = SCANCODE_A;
    keycode_scancode[0x42] = SCANCODE_B;
    keycode_scancode[0x43] = SCANCODE_C;
    keycode_scancode[0x44] = SCANCODE_D;
    keycode_scancode[0x45] = SCANCODE_E;
    keycode_scancode[0x46] = SCANCODE_F;
    keycode_scancode[0x47] = SCANCODE_G;
    keycode_scancode[0x48] = SCANCODE_H;
    keycode_scancode[0x49] = SCANCODE_I;
    keycode_scancode[0x4A] = SCANCODE_J;
    keycode_scancode[0x4B] = SCANCODE_K;
    keycode_scancode[0x4C] = SCANCODE_L;
    keycode_scancode[0x4D] = SCANCODE_M;
    keycode_scancode[0x4E] = SCANCODE_N;
    keycode_scancode[0x4F] = SCANCODE_O;
    keycode_scancode[0x50] = SCANCODE_P;
    keycode_scancode[0x51] = SCANCODE_Q;
    keycode_scancode[0x52] = SCANCODE_R;
    keycode_scancode[0x53] = SCANCODE_S;
    keycode_scancode[0x54] = SCANCODE_T;
    keycode_scancode[0x55] = SCANCODE_U;
    keycode_scancode[0x56] = SCANCODE_V;
    keycode_scancode[0x57] = SCANCODE_W;
    keycode_scancode[0x58] = SCANCODE_X;
    keycode_scancode[0x59] = SCANCODE_Y;
    keycode_scancode[0x5A] = SCANCODE_Z;

    keycode_scancode[0x5B] = SCANCODE_BRACKET_LEFT;     /* [ */
    keycode_scancode[0x5C] = SCANCODE_BACKSLASH;        /* \ */
    keycode_scancode[0x5D] = SCANCODE_BRACKET_RIGHT;    /* ] */
    keycode_scancode[0x5E] = SCANCODE_6;        /* ^ */
    keycode_scancode[0x5F] = SCANCODE_MINUS;    /* _ */
    keycode_scancode[0x60] = SCANCODE_GRAVE;    /* ` */

    keycode_scancode[0x61] = SCANCODE_A;
    keycode_scancode[0x62] = SCANCODE_B;
    keycode_scancode[0x63] = SCANCODE_C;
    keycode_scancode[0x64] = SCANCODE_D;
    keycode_scancode[0x65] = SCANCODE_E;
    keycode_scancode[0x66] = SCANCODE_F;
    keycode_scancode[0x67] = SCANCODE_G;
    keycode_scancode[0x68] = SCANCODE_H;
    keycode_scancode[0x69] = SCANCODE_I;
    keycode_scancode[0x6A] = SCANCODE_J;
    keycode_scancode[0x6B] = SCANCODE_K;
    keycode_scancode[0x6C] = SCANCODE_L;
    keycode_scancode[0x6D] = SCANCODE_M;
    keycode_scancode[0x6E] = SCANCODE_N;
    keycode_scancode[0x6F] = SCANCODE_O;
    keycode_scancode[0x70] = SCANCODE_P;
    keycode_scancode[0x71] = SCANCODE_Q;
    keycode_scancode[0x72] = SCANCODE_R;
    keycode_scancode[0x73] = SCANCODE_S;
    keycode_scancode[0x74] = SCANCODE_T;
    keycode_scancode[0x75] = SCANCODE_U;
    keycode_scancode[0x76] = SCANCODE_V;
    keycode_scancode[0x77] = SCANCODE_W;
    keycode_scancode[0x78] = SCANCODE_X;
    keycode_scancode[0x79] = SCANCODE_Y;
    keycode_scancode[0x7A] = SCANCODE_Z;

    keycode_scancode[0x7B] = SCANCODE_BRACKET_LEFT;     /* { */
    keycode_scancode[0x7C] = SCANCODE_BACKSLASH;        /* | */
    keycode_scancode[0x7D] = SCANCODE_BRACKET_RIGHT;    /* } */
    keycode_scancode[0x7E] = SCANCODE_GRAVE;    /* ~ */
}

/* send mouse data  */
void SendMouseData (int x, int y, int buttons)
{
    int maxx, maxy, tmp;
    GVFBEventData event;
    GVFBHeader *hdr;

    /* get gvfb header */
    hdr = gvfbruninfo.hdr;

    /* fix position */
    maxx = max (x, hdr->width);
    maxy = max (y, hdr->height);

    switch (gvfbruninfo.rotation) {
    case 1:
        x = gvfbruninfo.actual_h - x;
        tmp = y;
        y = x;
        x = tmp;
        maxx = maxy;
        maxy = maxx;
        break;
    case 2:
        x = gvfbruninfo.actual_w - x;
        y = gvfbruninfo.actual_h - y;
        break;
    case 3:
        y = gvfbruninfo.actual_w - y;
        tmp = y;
        y = x;
        x = tmp;
        maxx = maxy;
        maxy = maxx;
        break;
    default:
        break;
    }

    x = max (x, 0);
    y = max (y, 0);
    x = min (x, maxx);
    y = min (y, maxy);

    event.event_type = MOUSE_TYPE;
    event.data.mouse.x = x;
    event.data.mouse.y = y;
    event.data.mouse.button = buttons;

    Send (gvfbruninfo.sockfd, (const void *) &event, sizeof (GVFBEventData), 0);
}

/* receive key from gtk */
void SendKeyboardData (unsigned int keycode, BOOL press, BOOL repeat)
{
    unsigned char scancode;
    unsigned int state;

    if ((keycode_to_scancode (keycode, &scancode)) < 0) {
        return;
    }

    state = press | repeat << 8;

    if (state == 0 && s_key_press_flag[scancode] == 0) {
        return;
    }

    if (state == 1) {
        s_key_press_flag[scancode] = 1;
    }

    if (last_key != scancode) {
        if (s_wait == 1) {
            send_key (last_key, 0);
            last_time = 0;
            s_wait = 0;
        }

        send_key (scancode, state);
        last_key = scancode;

        return;
    }

    if (s_wait == 0 && state == 0) {
        last_time = GVFBGetCurrentTime ();
        s_wait = 1;

        return;
    }

    if (s_wait == 1) {
        if (state == 1) {
            s_wait = 0;
            last_time = 0;
        }

        return;
    }

    send_key (scancode, state);
}

void SendUnpressedKeys (void)
{
    int i;

    for (i = 0; (i < MAX_KEY_LEN) && unpressedKey[i]; i++) {
        SendKeyboardData (unpressedKey[i], 0, 0);
        unpressedKey[i] = 0;
    }
}

void SendIMData (char *str)
{
    int len;
    char szbuff[1024];
    int ch;
    GVFBIMEventData *im = (GVFBIMEventData *) szbuff;

    if (str == NULL)
        return;

    if ((len = strlen (str)) <= 0)
        return;

    if (len == 1) {
        if (IsKeypadData) {
            IsKeypadData--;
            ch = char_to_keycode (str[0]);
        }
        else {
            ch = str[0];
        }

        SendKeyboardData (ch, 1, 0);
    }
    else {
        im->event_type = IME_MESSAGE_TYPE;
        im->size = len;

        strcpy (im->buff, str);

        Send (gvfbruninfo.sockfd, (const void *) im,
              sizeof (GVFBIMEventData) + len, 0);
    }
}

int SetCtrlKey (unsigned int keycode)
{
    int i;

    for (i = 0; (i < MAX_KEY_LEN) && unpressedKey[i]; i++) {
        if (unpressedKey[i] == keycode) {
            return 0;
        }
    }

    unpressedKey[i] = keycode;
    unpressedKey[++i] = 0;

    return 1;
}

int ClearCtrlKey (unsigned int keycode)
{
    int i;
    int find_idx = -1;

    for (i = 0; (i < MAX_KEY_LEN) && unpressedKey[i]; i++) {
        if (keycode == unpressedKey[i]) {
            find_idx = i;
        }
    }

    if (find_idx == -1) {
        return 0;
    }

    if (--i >= 0) {
        if (find_idx != i) {
            unpressedKey[find_idx] = unpressedKey[i];
        }

        unpressedKey[i] = 0;
    }

    return 1;
}

int PressedCtrlKey (void)
{
    int i;

    for (i = 0; i < MAX_KEY_LEN; i++) {
        if (unpressedKey[i]) {
            return 1;
        }
    }

    return 0;
}

gboolean KeyPressTimeout (gpointer data)
{
    if (s_wait == 1) {
        gulong now = GVFBGetCurrentTime ();

        if (now - last_time >= 10) {
            send_key (last_key, 0);
            last_key = -1;
            last_time = 0;
            s_wait = 0;
        }
    }

    return TRUE;
}

gboolean CheckKeycode (unsigned int keycode)
{
    unsigned char scancode;

    if (keycode_to_scancode (keycode, &scancode) != 0) {
        return FALSE;
    }

    return TRUE;
}

/* static function */
static unsigned int char_to_keycode (unsigned char c)
{
    unsigned int keycode = 0;

    switch (c) {
    case '0':
    case '1':
    case '2':
    case '3':
    case '4':
    case '5':
    case '6':
    case '7':
    case '8':
    case '9':
        keycode = 0xffb0 + c - '0';
        break;
    case '/':
        keycode = 0xffaf;
        break;
    case '-':
        keycode = 0xffad;
        break;
    case '.':
        keycode = 0xffae;
        break;
    default:
        break;
    }

    return keycode;
}

/* keycode to scancode */
static int keycode_to_scancode (unsigned int keycode, unsigned char *scancode)
{
    assert (scancode != NULL);

    if (keycode < 256) {
        /* normal key */
        //printf ("%x\n", keycode);
        *scancode = keycode_scancode[keycode];
        if (*scancode == scancode_not_exist) {
            msg_out (LEVEL_0, "warning, bad keycode 0x%x, ignore it.", keycode);

            return -1;
        }
    }
    else {
        /* control key */
        switch (keycode) {
        case 0xffbe:
        case 0xffbf:
        case 0xffc0:
        case 0xffc1:
        case 0xffc2:
        case 0xffc3:
        case 0xffc4:
        case 0xffc5:
        case 0xffc6:
        case 0xffc7:
            *scancode = SCANCODE_F1 + keycode - 0xffbe;
            break;
        case 0xffc8:
        case 0xffc9:
            *scancode = SCANCODE_F11 + keycode - 0xffc8;
            break;
        case 0xff1b:
            *scancode = SCANCODE_ESCAPE;
            break;
        case 0xff09:
            *scancode = SCANCODE_TAB;
            break;
        case 0xfe20:
            /* fix shift + tab */
            *scancode = SCANCODE_TAB;
            break;
        case 0xffe5:
            *scancode = SCANCODE_CAPSLOCK;
            break;
        case 0xffe1:
            *scancode = SCANCODE_LEFTSHIFT;
            break;
        case 0xffe2:
            *scancode = SCANCODE_RIGHTSHIFT;
            break;
        case 0xffe3:
            *scancode = SCANCODE_LEFTCONTROL;
            break;
        case 0xffe4:
            *scancode = SCANCODE_RIGHTCONTROL;
            break;
        case 0xffe9:
            *scancode = SCANCODE_LEFTALT;
            break;
        case 0xffea:
            *scancode = SCANCODE_RIGHTALT;
            break;
        case 0xff08:
            *scancode = SCANCODE_BACKSPACE;
            break;
        case 0xff0d:
            *scancode = SCANCODE_ENTER;
            break;
        case 0xff51:
            *scancode = SCANCODE_CURSORBLOCKLEFT;
            break;
        case 0xff52:
            *scancode = SCANCODE_CURSORBLOCKUP;
            break;
        case 0xff53:
            *scancode = SCANCODE_CURSORBLOCKRIGHT;
            break;
        case 0xff54:
            *scancode = SCANCODE_CURSORBLOCKDOWN;
            break;
        case 0xfd1d:
            *scancode = SCANCODE_PRINTSCREEN;
            break;
        case 0xff7f:
            *scancode = SCANCODE_NUMLOCK;
            break;
        case 0xff6b:
            *scancode = SCANCODE_BREAK;
            break;
        case 0xff13:
            *scancode = SCANCODE_PAUSE;
            break;
        case 0xff14:
            *scancode = SCANCODE_SCROLLLOCK;
            break;
        case 0xff63:
            *scancode = SCANCODE_INSERT;
            break;
        case 0xffff:
            *scancode = SCANCODE_REMOVE;
            break;
        case 0xff50:
            *scancode = SCANCODE_HOME;
            break;
        case 0xff57:
            *scancode = SCANCODE_END;
            break;
        case 0xff55:
            *scancode = SCANCODE_PAGEUP;
            break;
        case 0xff56:
            *scancode = SCANCODE_PAGEDOWN;
            break;
        case 0xffb0:
            *scancode = SCANCODE_KEYPAD0;
            break;
        case 0xffb1:
            *scancode = SCANCODE_KEYPAD1;
            break;
        case 0xffb2:
            *scancode = SCANCODE_KEYPAD2;
            break;
        case 0xffb3:
            *scancode = SCANCODE_KEYPAD3;
            break;
        case 0xffb4:
            *scancode = SCANCODE_KEYPAD4;
            break;
        case 0xffb5:
            *scancode = SCANCODE_KEYPAD5;
            break;
        case 0xffb6:
            *scancode = SCANCODE_KEYPAD6;
            break;
        case 0xffb7:
            *scancode = SCANCODE_KEYPAD7;
            break;
        case 0xffb8:
            *scancode = SCANCODE_KEYPAD8;
            break;
        case 0xffb9:
            *scancode = SCANCODE_KEYPAD9;
            break;
        case 0xff8d:
            *scancode = SCANCODE_KEYPADENTER;
            break;
        case 0xffaf:
            *scancode = SCANCODE_KEYPADDIVIDE;
            break;
        case 0xffaa:
            *scancode = SCANCODE_KEYPADMULTIPLY;
            break;
        case 0xffab:
            *scancode = SCANCODE_KEYPADPLUS;
            break;
        case 0xffad:
            *scancode = SCANCODE_KEYPADMINUS;
            break;
        case 0xffae:
            *scancode = SCANCODE_KEYPADPERIOD;
            break;
        case 0xff96:
            *scancode = SCANCODE_CURSORLEFT;
            break;
        case 0xff97:
            *scancode = SCANCODE_CURSORUP;
            break;
        case 0xff98:
            *scancode = SCANCODE_CURSORRIGHT;
            break;
        case 0xff99:
            *scancode = SCANCODE_CURSORDOWN;
            break;
        default:
            return -1;
        }       /* end switch */
    }   /* end if */

    return 0;
}

static void send_key (unsigned char scancode, unsigned int state)
{
    GVFBEventData event;

    event.event_type = KB_TYPE;
    event.data.key.key_code = scancode;
    event.data.key.key_state = state;

    if (state == 0) {
        s_key_press_flag[scancode] = 0;
    }

    Send (gvfbruninfo.sockfd, (const void *) &event, sizeof (GVFBEventData), 0);
}
