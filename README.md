# GVFB

GVFB Version 1.0.0

This is a virtual frame buffer program running on GTK.

This program (gvfb) is compliant to MiniGUI V3.0's XVFB specification.

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

