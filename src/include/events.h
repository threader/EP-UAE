#ifndef EVENTS_H
#define EVENTS_H

 /*
  * UAE - The Un*x Amiga Emulator
  *
  * Events
  * These are best for low-frequency events. Having too many of them,
  * or using them for events that occur too frequently, can cause massive
  * slowdown.
  *
  * Copyright 1995-1998 Bernd Schmidt
  */

#undef EVENT_DEBUG

#include "machdep/rpt.h"
#include "hrtimer.h"

/* Every Amiga hardware clock cycle takes this many "virtual" cycles.  This
 * used to be hardcoded as 1, but using higher values allows us to time some
 * stuff more precisely.
 * 512 is the official value from now on - it can't change, unless we want
 * _another_ config option "finegrain2_m68k_speed".
 *
 * We define this value here rather than in events.h so that gencpu.c sees
 * it.
 */
#define CYCLE_UNIT 512

/* This one is used by cfgfile.c.  We could reduce the CYCLE_UNIT back to 1,
 * I'm not 100% sure this code is bug free yet.
 */
#define OFFICIAL_CYCLE_UNIT 512

extern volatile frame_time_t vsynctime, vsyncmintime;
extern int  syncbase;
extern void reset_frame_rate_hack (void);
//extern frame_time_t syncbase;
extern int event2_count;

extern void compute_vsynctime (void);
extern void init_eventtab (void);
extern void do_cycles_ce (long cycles);
extern void do_cycles_ce020 (int clocks);
extern void do_cycles_ce020_mem (int clocks);
extern void do_cycles_ce000 (int clocks);
extern int is_cycle_ce (void);

extern unsigned long currcycle, nextevent, is_lastline;
typedef void (*evfunc)(void);
typedef void (*evfunc2)(uae_u32);

typedef unsigned long int evt;

struct ev
{
    int active;
    evt evtime, oldcycles;
    evfunc handler;
};

struct ev2
{
    int active;
    evt evtime;
    uae_u32 data;
    evfunc2 handler;
};

enum {
    ev_hsync, ev_copper, ev_audio, ev_cia, ev_blitter, ev_disk, ev_misc,
    ev_max
};

enum {
    ev2_blitter, ev2_disk, ev2_misc,
    ev2_max = 12
};

extern int pissoff_value;
extern signed long pissoff;

#define countdown pissoff
#define do_cycles do_cycles_slow

extern struct ev eventtab[ev_max];
extern struct ev2 eventtab2[ev2_max];


#ifdef JIT
#include "events_jit.h"
#else
#include "events_normal.h"
#endif


STATIC_INLINE void do_cycles_slow (unsigned long cycles_to_add)
{
    if (is_lastline && eventtab[ev_hsync].evtime - currcycle <= cycles_to_add
		&& (long int)(uae_gethrtime () - vsyncmintime) < 0)
		return;

    while ((nextevent - currcycle) <= cycles_to_add) {
		int i;
		cycles_to_add -= (nextevent - currcycle);
		currcycle = nextevent;

		for (i = 0; i < ev_max; i++) {
		    if (eventtab[i].active && eventtab[i].evtime == currcycle) {
				(*eventtab[i].handler)();
		    }
		}
		events_schedule();
    }
    currcycle += cycles_to_add;
}
STATIC_INLINE void do_extra_cycles (unsigned long cycles_to_add)
{
	pissoff -= cycles_to_add;
}

STATIC_INLINE unsigned long int get_cycles (void)
{
	return currcycle;
}

STATIC_INLINE void set_cycles (unsigned long int x)
{
	currcycle = x;
	eventtab[ev_hsync].oldcycles = x;
#ifdef EVT_DEBUG
	if (currcycle & (CYCLE_UNIT - 1))
		write_log (_T("%x\n"), currcycle);
#endif
}

STATIC_INLINE int current_hpos_safe (void)
{
    int hp = (get_cycles () - eventtab[ev_hsync].oldcycles) / CYCLE_UNIT;
	return hp;
}

STATIC_INLINE int current_hpos (void)
{
    return (get_cycles () - eventtab[ev_hsync].oldcycles) / CYCLE_UNIT;
}
extern void MISC_handler (void);
extern void event2_newevent_xx (int no, evt t, uae_u32 data, evfunc2 func);

STATIC_INLINE void event2_newevent_x (int no, evt t, uae_u32 data, evfunc2 func)
{
	if (((int)t) <= 0) {
		func (data);
		return;
	}
	event2_newevent_xx (no, t * CYCLE_UNIT, data, func);
}

STATIC_INLINE void event2_newevent (int no, evt t, uae_u32 data)
{
	event2_newevent_x (no, t, data, eventtab2[no].handler);
}
STATIC_INLINE void event2_newevent2 (evt t, uae_u32 data, evfunc2 func)
{
	event2_newevent_x (-1, t, data, func);
}

STATIC_INLINE void event2_remevent (int no)
{
	eventtab2[no].active = 0;
}


#endif
