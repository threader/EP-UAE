/*
  * UAE - The Un*x Amiga Emulator
  *
  * Definitions for accessing cycle counters on a given machine, if possible.
  *
  * Copyright 1998 Bernd Schmidt
  * Copyright 2011 Mustafa TUFAN
  */

#ifndef EUAE_MACHDEP_RPT_H
#define EUAE_MACHDEP_RPT_H

STATIC_INLINE uae_s64 uae_time(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return ts.tv_sec * 1000000000LL + ts.tv_nsec;
}

STATIC_INLINE uae_s32 read_processor_time (void)
{
    return uae_time();
}

STATIC_INLINE frame_time_t machdep_gethrtime (void)
{
    return read_processor_time ();
}

frame_time_t machdep_gethrtimebase (void);
int          machdep_inithrtimer   (void);

#endif /* EUAE_MACHDEP_RPT_H */
