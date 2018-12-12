# GVFB

GVFB Version 1.2.2

This is a virtual frame buffer program running on Gtk-2.0.

This program (gvfb) is compliant to MiniGUI V3.0's XVFB specification.

## Prerequisites

* Gtk-2.0 development headers and libraries (libgtk2.0-dev pkg on Ubuntu).

## Building

Run the following commands to install gvfb.

	$ cmake .
	$ make
	$ sudo make install

And then you should change something in MiniGUI.cfg:

	[system]
	gal_engine=pc_xvfb

	[pc_xvfb]
	defaultmode=800x600-XXbpp.argbXXXX
	window_caption=GVFB-for-MiniGUI-3.2-(Gtk-Version)
	exec_file=/usr/local/bin/gvfb

## COPYING

    Copyright (C) 2010~2018, Beijing FMSoft Technologies Co., Ltd.

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.

## AUTHORS

The following people took and/or are taking part in the development of GVFB:

* Vincent Wei
* Wang Xubin
