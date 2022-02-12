 /*
  * UAE - The Un*x Amiga Emulator
  *
  * Interface to the Tcl/Tk GUI
  *
  * Copyright 1996 Bernd Schmidt
  */

#include "sysconfig.h"
#include "sysdeps.h"

#include "options.h"
#include "gui.h"

<<<<<<< HEAD

void gui_init (int argc, char **argv)
=======
int gui_init (void)
>>>>>>> p-uae/v2.1.0
{
}

int gui_open (void)
{
    return -1;
}

void gui_notify_state (int state)
{
}

<<<<<<< HEAD
int gui_update (void)
{
    return 0;
}

void gui_exit (void)
{
}

=======
>>>>>>> p-uae/v2.1.0
void gui_fps (int fps, int idle)
{
    gui_data.fps  = fps;
    gui_data.idle = idle;
}

<<<<<<< HEAD
=======
void gui_flicker_led (int led, int unitnum, int status)
{
}

>>>>>>> p-uae/v2.1.0
void gui_led (int led, int on)
{
}

<<<<<<< HEAD
void gui_hd_led (int led)
{
    static int resetcounter;

    int old = gui_data.hd;

    if (led == 0) {
	resetcounter--;
	if (resetcounter > 0)
	    return;
    }

    gui_data.hd = led;
    resetcounter = 6;
    if (old != gui_data.hd)
	gui_led (5, gui_data.hd);
}

void gui_cd_led (int led)
{
    static int resetcounter;

    int old = gui_data.cd;
    if (led == 0) {
	resetcounter--;
	if (resetcounter > 0)
	    return;
    }

    gui_data.cd = led;
    resetcounter = 6;
    if (old != gui_data.cd)
	gui_led (6, gui_data.cd);
=======
void gui_hd_led (int unitnum, int led)
{
}

void gui_cd_led (int unitnum, int led)
{
>>>>>>> p-uae/v2.1.0
}

void gui_filename (int num, const char *name)
{
}

void gui_handle_events (void)
{
}

<<<<<<< HEAD
=======
int gui_update (void)
{
	return 0;
}

void gui_exit (void)
{
}

>>>>>>> p-uae/v2.1.0
void gui_display(int shortcut)
{
}

void gui_message (const char *format,...)
<<<<<<< HEAD
{   
=======
{
>>>>>>> p-uae/v2.1.0
       char msg[2048];
       va_list parms;

       va_start (parms,format);
       vsprintf ( msg, format, parms);
       va_end (parms);

       write_log (msg);
}
<<<<<<< HEAD
=======

void gui_disk_image_change (int unitnum, const TCHAR *name) {}
void gui_lock (void) {}
void gui_unlock (void) {}

>>>>>>> p-uae/v2.1.0
