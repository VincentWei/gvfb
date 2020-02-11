# GVFB

GVFB Version 1.2.4

This is a virtual frame buffer program running on Gtk-2.0.

This program (gvfb) is compliant to MiniGUI V3.0's XVFB specification.

## Change Log

### Version 1.2.4

- Support for double buffering.

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

## Authors

The current maintainers and/or developers:

* Vincent Wei

The following people had been involved in the development of GVFB:

* WANG Xubin

## Copying

Copyright (C) 2010 ~ 2020, Beijing FMSoft Technologies Co., Ltd.

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

### Special Statement

The above open source or free software license does
not apply to any entity in the Exception List published by
Beijing FMSoft Technologies Co., Ltd.

If you are or the entity you represent is listed in the Exception List,
the above open source or free software license does not apply to you
or the entity you represent. Regardless of the purpose, you should not
use the software in any way whatsoever, including but not limited to
downloading, viewing, copying, distributing, compiling, and running.
If you have already downloaded it, you MUST destroy all of its copies.

The Exception List is published by FMSoft and may be updated
from time to time. For more information, please see
<https://www.fmsoft.cn/exception-list>.

