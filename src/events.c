 /*
  * UAE - The Un*x Amiga Emulator
  *
  * Events
  * These are best for low-frequency events. Having too many of them,
  * or using them for events that occur too frequently, can cause massive
  * slowdown.
  *
  * Copyright 1995-1998 Bernd Schmidt
  * Copyright 2004-2005 Richard Drummond
  */

#include "sysconfig.h"
#include "sysdeps.h"

#include "options.h"
#include "events.h"

#include "custom.h"
#include "cia.h"
#include "blitter.h"
#include "disk.h"
#include "audio.h"
#include "hrtimer.h"

/* Current time in cycles */
//unsigned long int currcycle;

/* Cycles to next event pending */
//unsigned long nextevent;
int event2_count;
#ifdef JIT2
/* For faster cycles handling */
signed long pissoff = 0;
#endif

//struct ev eventtab[ev_max];

#if 0
void init_eventtab (void)
{
    int i;

    nextevent = 0;
    set_cycles (0);

    for (i = 0; i < ev_max; i++) {
	eventtab[i].active = 0;
	eventtab[i].oldcycles = 0;
    }

    eventtab[ev_cia].handler     = CIA_handler;
    eventtab[ev_hsync].handler   = hsync_handler;
    eventtab[ev_hsync].evtime    = get_cycles () + HSYNCTIME;
    eventtab[ev_hsync].active    = 1;
    eventtab[ev_copper].handler  = copper_handler;
    eventtab[ev_copper].active   = 0;
    eventtab[ev_blitter].handler = blitter_handler;
    eventtab[ev_blitter].active  = 0;
    eventtab[ev_disk].handler    = DISK_handler;
    eventtab[ev_disk].active     = 0;
    eventtab[ev_audio].handler   = audio_evhandler;
    eventtab[ev_audio].active    = 0;

    events_schedule ();
}

/*
 * Determine next event pending
 */
void events_schedule (void)
{
    int i;

    unsigned long int mintime = ~0L;
    for (i = 0; i < ev_max; i++) {
	if (eventtab[i].active) {
	    unsigned long int eventtime = eventtab[i].evtime - currcycle;
	    if (eventtime < mintime)
		mintime = eventtime;
	}
    }
    nextevent = currcycle + mintime;
}
/*
 * Handle all events due at current time
 */
void handle_active_events (void)
{
    int i;
    for (i = 0; i < ev_max; i++) {
	if (eventtab[i].active && eventtab[i].evtime == currcycle) {
	    (*eventtab[i].handler)();
	}
    }
}

#endif 

void event2_newevent_xx (int no, evt t, uae_u32 data, evfunc2 func)
{
	evt et;
	static int next = ev2_misc;

	et = t + get_cycles ();
	if (no < 0) {
		no = next;
		for (;;) {
			if (!eventtab2[no].active) {
				event2_count++;
				break;
			}
			if (eventtab2[no].evtime == et && eventtab2[no].handler == func && eventtab2[no].data == data)
				break;
			no++;
			if (no == ev2_max)
				no = ev2_misc;
			if (no == next) {
				write_log (_T("out of event2's!\n"));
				return;
			}
		}
		next = no;
	}
	eventtab2[no].active = true;
	eventtab2[no].evtime = et;
	eventtab2[no].handler = func;
	eventtab2[no].data = data;
	MISC_handler ();
}
