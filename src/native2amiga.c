 /*
  * UAE - The Un*x Amiga Emulator
  *
  * Call Amiga Exec functions outside the main UAE thread.
  *
  * Copyright 1999 Patrick Ohly
  *
  * Uses the EXTER interrupt that is setup in filesys.c
  * and needs thread support.
  */

#include "sysconfig.h"
#include "sysdeps.h"

#include "threaddep/thread.h"
#include "options.h"
#include "memory.h"
#include "custom.h"
#include "newcpu.h"
#include "autoconf.h"
#include "traps.h"
#include "native2amiga.h"

smp_comm_pipe native2amiga_pending;
static uae_sem_t n2asem;

/*
 * to be called when setting up the hardware
 */

void native2amiga_install (void)
{
	init_comm_pipe (&native2amiga_pending, 100, 2);
    uae_sem_init (&n2asem, 0, 1);
}

void native2amiga_reset (void)
{
	smp_comm_pipe *p = &native2amiga_pending;
	p->rdp = p->wrp = 0;
	p->reader_waiting = 0;
	p->writer_waiting = 0;
};

/*
 * to be called when the Amiga boots, i.e. by filesys_diagentry()
 */
void native2amiga_startup (void)
{
}

int native2amiga_isfree (void)
{
	return comm_pipe_has_data (&native2amiga_pending) == 0;
}

#ifdef SUPPORT_THREADS

void uae_Cause (uaecptr interrupt)
{
    uae_sem_wait (&n2asem);
    write_comm_pipe_int (&native2amiga_pending, 3, 0);
    write_comm_pipe_u32 (&native2amiga_pending, interrupt, 1);
    do_uae_int_requested ();
    uae_sem_post (&n2asem);
}

void uae_ReplyMsg (uaecptr msg)
{
    uae_sem_wait (&n2asem);
    write_comm_pipe_int (&native2amiga_pending, 2, 0);
    write_comm_pipe_u32 (&native2amiga_pending, msg, 1);
    do_uae_int_requested ();
    uae_sem_post (&n2asem);
}

void uae_PutMsg (uaecptr port, uaecptr msg)
{
    uae_sem_wait (&n2asem);
    write_comm_pipe_int (&native2amiga_pending, 1, 0);
    write_comm_pipe_u32 (&native2amiga_pending, port, 0);
    write_comm_pipe_u32 (&native2amiga_pending, msg, 1);
    do_uae_int_requested ();
    uae_sem_post (&n2asem);
}

void uae_Signal (uaecptr task, uae_u32 mask)
{
    uae_sem_wait (&n2asem);
    write_comm_pipe_int (&native2amiga_pending, 0, 0);
    write_comm_pipe_u32 (&native2amiga_pending, task, 0);
    write_comm_pipe_int (&native2amiga_pending, mask, 1);
    do_uae_int_requested ();
    uae_sem_post (&n2asem);
}

void uae_NotificationHack (uaecptr port, uaecptr nr)
{
    uae_sem_wait (&n2asem);
    write_comm_pipe_int (&native2amiga_pending, 4, 0);
    write_comm_pipe_int (&native2amiga_pending, port, 0);
    write_comm_pipe_int (&native2amiga_pending, nr, 1);
    do_uae_int_requested ();
    uae_sem_post (&n2asem);
}

#endif
 /* note */
void uae_NewList (uaecptr list)
{
	put_long_slow (list, list + 4);
	put_long_slow (list + 4, 0);
	put_long_slow (list + 8, list);
}

uaecptr uae_AllocMem (TrapContext *context, uae_u32 size, uae_u32 flags)
{
    m68k_dreg (&context->regs, 0) = size;
    m68k_dreg (&context->regs, 1) = flags;
    return CallLib (context, get_long (4), -198); /* AllocMem */
}

void uae_FreeMem (TrapContext *context, uaecptr memory, uae_u32 size)
{
    m68k_dreg (&context->regs, 0) = size;
    m68k_areg (&context->regs, 1) = memory;
    CallLib (context, get_long (4), -0xD2); /* FreeMem */
}
