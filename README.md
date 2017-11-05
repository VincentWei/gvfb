# GVFB

GVFB Version 1.2.1

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
	window_caption=GVFB-for-MiniGUI-3.0-(Gtk-Version)
	exec_file=/usr/local/bin/gvfb

