/*
 * UAE - The Un*x Amiga Emulator
 *
 * Enforcer Like Support
 *
 * Copyright 2000-2003 Bernd Roesch and Sebastian Bauer
 * Copyright 2004      Richard Drummond
 */

#ifdef ENFORCER

#include <stdlib.h>

#include "sysconfig.h"
#include "sysdeps.h"

#include "options.h"
#include "memory.h"
#include "custom.h"
#include "newcpu.h"
#include "uae.h"
#include "xwin.h"
#include "enforcer.h"
#include "sleep.h"

#ifndef _WIN32
#define console_out printf
#else
#define console_out(...) do {;} while (0)
#endif

#if defined(JIT)
#define special_mem_r special_mem |= S_READ
#define special_mem_w special_mem |= S_WRITE
#define NMEM_OFFSET NATMEM_OFFSET
#else
#define special_mem_r
#define special_mem_w
#define NMEM_OFFSET 0
#endif

/* Configurable options */
#define ENFORCESIZE 1024
#define STACKLINES 5
#define BACKTRACELONGS 500
#define INSTRUCTIONLINES 17

#define ISEXEC(addr) ((addr) >= 4 && (addr) <= 7)
#define ISILLEGAL_LONG(addr) ((addr) < 4 || ((addr) > 4 && (addr) < ENFORCESIZE))
#define ISILLEGAL_WORD(addr) ((addr) < ENFORCESIZE)
#define ISILLEGAL_BYTE(addr) ((addr) < ENFORCESIZE)

extern uae_u32 *natmem_offset;

int flashscreen = 0;
static int enforcer_installed = 0;
int enforcermode = 0;
static int enforcer_hit = 0; /* set to 1 if displaying the hit */

#define ENFORCER_BUF_SIZE 8192
static char enforcer_buf[ENFORCER_BUF_SIZE];

uae_u32 (REGPARAM2 *saved_chipmem_lget) (uaecptr addr);
uae_u32 (REGPARAM2 *saved_chipmem_wget) (uaecptr addr);
uae_u32 (REGPARAM2 *saved_chipmem_bget) (uaecptr addr);
void (REGPARAM2 *saved_chipmem_lput) (uaecptr addr, uae_u32 l);
void (REGPARAM2 *saved_chipmem_wput) (uaecptr addr, uae_u32 w);
void (REGPARAM2 *saved_chipmem_bput) (uaecptr addr, uae_u32 b);
int (REGPARAM2 *saved_chipmem_check) (uaecptr addr, uae_u32 size);
uae_u8 *(REGPARAM2 *saved_chipmem_xlate) (uaecptr addr);
uae_u32 (REGPARAM2 *saved_dummy_lget) (uaecptr addr);
uae_u32 (REGPARAM2 *saved_dummy_wget) (uaecptr addr);
uae_u32 (REGPARAM2 *saved_dummy_bget) (uaecptr addr);
void (REGPARAM2 *saved_dummy_lput) (uaecptr addr, uae_u32 l);
void (REGPARAM2 *saved_dummy_wput) (uaecptr addr, uae_u32 w);
void (REGPARAM2 *saved_dummy_bput) (uaecptr addr, uae_u32 b);
int (REGPARAM2 *saved_dummy_check) (uaecptr addr, uae_u32 size);

#define GET_PC m68k_getpc (&regs)

static addrbank saved_dummy_bank;
static addrbank saved_chipmem_bank;


/*************************************************************
 Returns the first node entry of an exec list or 0 if
 empty
*************************************************************/
static uae_u32 amiga_list_first (uae_u32 list)
{
    uae_u32 node = get_long (list);      /* lh_Head */
    if (!node)
	return 0;
    if (!get_long (node))
	return 0;   /* ln_Succ */
    return node;
}

/*************************************************************
 Returns the next node of an exec node or 0 if it was the
 last element
*************************************************************/
static uae_u32 amiga_node_next (uae_u32 node)
{
    uae_u32 next = get_long (node);    /* ln_Succ */
    if (!next)
	return 0;
    if (!get_long (next))
	return 0; /* ln_Succ */
    return next;
}

/*************************************************************
 Converts an amiga address to a native one or NULL if this
 is not possible, Size specified the number of bytes you
 want to access
*************************************************************/
static uae_u8 *amiga2native (uae_u32 aptr, int size)
{
    addrbank bank = get_mem_bank (aptr);

    /* Check if the address can be translated to native */
    if (bank.check (aptr,size)) {
	return bank.xlateaddr (aptr);
    }
    return NULL;
}

/*************************************************************
 Writes the Hunk and Offset of the given Address into buf
*************************************************************/
static int enforcer_decode_hunk_and_offset (char *buf, uae_u32 pc)
{
    uae_u32 sysbase = get_long(4);
    uae_u32 semaphore_list = sysbase + 532;

    /* First step is searching for the SegTracker semaphore */
    uae_u32 node = amiga_list_first (semaphore_list);
    while (node) {
	uae_u32 string = get_long (node + 10); /* ln_Name */
	uae_u8 *native_string = amiga2native (string, 100);

	if (native_string) {
	    if (!strcmp ((char *) native_string, "SegTracker"))
		break;
	}
	node = amiga_node_next (node);
    }

    if (node) {
	/* We have found the segtracker semaphore. Soon after the
	 * public documented semaphore structure Segtracker holds
	 * an own list of all segements. We will use this list to
	 * find out the hunk and offset (simliar to segtracker).
	 *
	 * Source of segtracker can be found at:
	 *    http://www.sinz.org/Michael.Sinz/Enforcer/SegTracker.c.html
	 */
	uae_u32 seg_list = node + 46 + 4; /* sizeof(struct SignalSemaphore) + seg find */

	node = amiga_list_first (seg_list);
	while (node) {
	    uae_u32 seg_entry = node + 12;
	    uae_u32 address, size;
	    int hunk = 0;

	    /* Go through all entries until an address is 0
	     * or the segment has been found */
	    while ((address = get_long (seg_entry))) {
		size = get_long (seg_entry + 4);

				if (pc > address && pc < address + size) {
				    uae_u32 name, offset;
					const char *native_name;

				    offset = pc - address - 4;
				    name = get_long (node + 8); /* ln_Name */
				    if (name) {
						native_name = (char*)amiga2native(name,100);
						if (!native_name)
							native_name = strdup ("Unknown");
				    } else {
						native_name = strdup ("Unknown");
					}
					sprintf (buf, "----> %08lx - \"%s\" Hunk %04lx Offset %08lx\n", pc, native_name, hunk, offset);
					xfree (native_name);
				    return 1;
				}

				seg_entry += 8;
				hunk++;
		    }
		    node = amiga_node_next (node);
		}
    }
    return 0;
}

/*************************************************************
 Display the enforcer hit
*************************************************************/
static void enforcer_display_hit (const char *addressmode, uae_u32 pc, uaecptr addr)
{
    uae_u32 a7;
    uae_u32 sysbase;
    uae_u32 this_task;
    uae_u32 task_name;
	const char *native_task_name = NULL;
    int i, j;
	static char buf[256],instrcode[256];
	static char lines[INSTRUCTIONLINES/2][256];
    static uaecptr bestpc_array[INSTRUCTIONLINES/2][5];
    static int bestpc_idxs[INSTRUCTIONLINES/2];
	char *enforcer_buf_ptr = enforcer_buf;
    uaecptr bestpc, pospc, nextpc, temppc;

	if (enforcer_hit)
		return; /* our function itself generated a hit ;), avoid endless loop */
	if (regs.vbr < 0x100 && addr >= 0x0c && addr < 0x80)
	return;
    if (!(sysbase = get_long (4)))
	return;
    if (!(this_task = get_long (sysbase + 276)))
	return;


	enforcer_hit = 1;

	sysbase = get_long (4);
	if (sysbase < 0x100 || !valid_address (sysbase, 1000))
		goto end;
	this_task = get_long (sysbase + 276);
	if (this_task < 0x100 || !valid_address (this_task, 1000))
		goto end;

	task_name = get_long (this_task + 10); /* ln_Name */
	native_task_name = ((char*)amiga2native (task_name, 100));
	/*if (strcmp(native_task_name,"c:MCP")!=0)
	{
	Exception (0x2d,0);
	}*/
	_tcscpy (enforcer_buf_ptr, "Enforcer Hit! Bad program\n");
	enforcer_buf_ptr += _tcslen (enforcer_buf_ptr);

	_stprintf (buf, "Illegal %s: %08lx", addressmode, addr);
	_stprintf (enforcer_buf_ptr, "%-48sPC: %0lx\n", buf, pc);
	enforcer_buf_ptr += _tcslen (enforcer_buf_ptr);

    /* Data registers */
	_stprintf (enforcer_buf_ptr, "Data: %08lx %08lx %08lx %08lx %08lx %08lx %08lx %08lx\n",
	m68k_dreg (&regs, 0), m68k_dreg (&regs, 1), m68k_dreg (&regs, 2), m68k_dreg (&regs, 3),
	m68k_dreg (&regs, 4), m68k_dreg (&regs, 5), m68k_dreg (&regs, 6), m68k_dreg (&regs, 7));
	enforcer_buf_ptr += _tcslen (enforcer_buf_ptr);

    /* Address registers */
	_stprintf (enforcer_buf_ptr, "Addr: %08lx %08lx %08lx %08lx %08lx %08lx %08lx %08lx\n",
	m68k_areg (&regs, 0), m68k_areg (&regs, 1), m68k_areg (&regs, 2), m68k_areg (&regs, 3),
	m68k_areg (&regs, 4), m68k_areg (&regs, 5), m68k_areg (&regs, 6), m68k_areg (&regs, 7));
	enforcer_buf_ptr += _tcslen (enforcer_buf_ptr);

    /* Stack */
	a7 = m68k_areg (&regs, 7);
    for (i = 0; i < 8 * STACKLINES; i++) {
		a7 += 4;
	if (!(i % 8)) {
			_tcscpy (enforcer_buf_ptr, "Stck:");
			enforcer_buf_ptr += _tcslen (enforcer_buf_ptr);
	}
		_stprintf (enforcer_buf_ptr, " %08lx",get_long (a7));
		enforcer_buf_ptr += _tcslen (enforcer_buf_ptr);

	if (i%8 == 7)
	    *enforcer_buf_ptr++ = '\n';
    }

    /* Segtracker output */
	a7 = m68k_areg (&regs, 7);
    if (get_long (a7-4) != pc) {
	if (enforcer_decode_hunk_and_offset (buf, pc)) {
			_tcscpy (enforcer_buf_ptr, buf);
			enforcer_buf_ptr += _tcslen (enforcer_buf_ptr);
	}
    }

	for (i = 0; i < BACKTRACELONGS; i++) {
		a7 += 4;
	if (enforcer_decode_hunk_and_offset (buf, get_long (a7))) {
			int l = _tcslen (buf);

	    if (ENFORCER_BUF_SIZE - (enforcer_buf_ptr - enforcer_buf) > l + 256) {
				_tcscpy (enforcer_buf_ptr, buf);
		enforcer_buf_ptr += l;
	    }
	}
    }

    /* Decode the instructions around the pc where the enforcer hit was caused.
     *
     * At first, the area before the pc, this not always done correctly because
     * it's done backwards */
    temppc = pc;

    memset (bestpc_array, 0, sizeof (bestpc_array));
    for (i = 0; i < INSTRUCTIONLINES / 2; i++)
		bestpc_idxs[i] = -1;

    for (i = 0; i < INSTRUCTIONLINES / 2; i++) {
		pospc = temppc;
		bestpc = 0;

		if (bestpc_idxs[i] == -1) {
		    for (j = 0; j < 5; j++) {
				pospc -= 2;
				sm68k_disasm (buf, NULL, pospc, &nextpc);
				if (nextpc == temppc) {
				    bestpc_idxs[i] = j;
				    bestpc_array[i][j] = bestpc = pospc;
				}
		    }
		} else {
		    bestpc = bestpc_array[i][bestpc_idxs[i]];
		}

		if (!bestpc) {
			/* there was no best pc found, so it is high probable that
			* a former used best pc was wrong.
			*
			* We trace back and use the former best pc instead
			*/

			int former_idx;
			int leave = 0;

			do {
				if (!i) {
					leave = 1;
					break;
				}
				i--;
				former_idx = bestpc_idxs[i];
				bestpc_idxs[i] = -1;
				bestpc_array[i][former_idx] = 0;

				for (j = former_idx - 1; j >= 0; j--) {
					if (bestpc_array[i][j]) {
						bestpc_idxs[i] = j;
						break;
					}
				}
			} while (bestpc_idxs[i] == -1);
			if (leave)
				break;
			if (i)
				temppc = bestpc_array[i-1][bestpc_idxs[i-1]];
			else
				temppc = pc;
			i--; /* will be increased in after continue */
			continue;
		}

		sm68k_disasm (buf, instrcode, bestpc, NULL);
		sprintf (lines[i], "%08lx :   %-20s %s\n", bestpc, instrcode, buf);
		temppc = bestpc;
    }

    i--;
    for (; i>=0; i--) {
		strcpy (enforcer_buf_ptr,lines[i]);
		enforcer_buf_ptr += strlen (enforcer_buf_ptr);
    }

    /* Now the instruction after the pc including the pc */
    temppc = pc;
    for (i=0; i < (INSTRUCTIONLINES+1) / 2; i++) {
		sm68k_disasm (buf, instrcode, temppc, &nextpc);
		sprintf (enforcer_buf_ptr, "%08lx : %s %-20s %s\n", temppc,
			(i == 0 ? "*" : " "), instrcode, buf);
		enforcer_buf_ptr += strlen (enforcer_buf_ptr);
		temppc = nextpc;
    }

    if (!native_task_name)
		native_task_name = "Unknown";
		sprintf (enforcer_buf_ptr, "Name: \"%s\"\n\n", native_task_name);
		enforcer_buf_ptr += strlen (enforcer_buf_ptr);

    console_out (enforcer_buf);
    write_log (enforcer_buf);
	uae_msleep (5);
	//doflashscreen ();

end:
	xfree (native_task_name);
    enforcer_hit = 0;
}

static uae_u32 REGPARAM2 chipmem_lget2 (uaecptr addr)
{
    uae_u32 *m;

    addr -= chipmem_start & chipmem_mask;
    addr &= chipmem_mask;
    m = (uae_u32 *)(chipmemory + addr);

	if (ISILLEGAL_LONG (addr))
	{
		enforcer_display_hit (_T("LONG READ from"), m68k_getpc (&regs), addr);
		if (enforcermode & 1)
			set_special (&regs, SPCFLAG_TRAP);
	}
    return do_get_mem_long (m);
}

static uae_u32 REGPARAM2 chipmem_wget2 (uaecptr addr)
{
    uae_u16 *m;

    addr -= chipmem_start & chipmem_mask;
    addr &= chipmem_mask;
    m = (uae_u16 *)(chipmemory + addr);

	if (ISILLEGAL_WORD (addr))
	{
		enforcer_display_hit (_T("WORD READ from"), m68k_getpc (&regs), addr);
		if (enforcermode & 1)
			set_special (&regs, SPCFLAG_TRAP);
	}
    return do_get_mem_word (m);
}

static uae_u32 REGPARAM2 chipmem_bget2 (uaecptr addr)
{
    addr -= chipmem_start & chipmem_mask;
    addr &= chipmem_mask;

	if (ISILLEGAL_BYTE (addr))
	{
		enforcer_display_hit (_T("BYTE READ from"), m68k_getpc (&regs), addr);
		if (enforcermode & 1)
			set_special (&regs, SPCFLAG_TRAP);
	}

    return chipmemory[addr];
}

static void REGPARAM2 chipmem_lput2 (uaecptr addr, uae_u32 l)
{
    uae_u32 *m;

    addr -= chipmem_start & chipmem_mask;
    addr &= chipmem_mask;
    m = (uae_u32 *)(chipmemory + addr);

	if (ISILLEGAL_LONG (addr))
	{
		enforcer_display_hit (_T("LONG WRITE to"), m68k_getpc (&regs), addr);
		if (enforcermode & 1)
			if (addr != 0x100)
				set_special (&regs, SPCFLAG_TRAP);
	}
	if (ISEXEC (addr) || ISEXEC (addr + 1) || ISEXEC (addr + 2) || ISEXEC (addr + 3))
		return;
    do_put_mem_long (m, l);
}

static void REGPARAM2 chipmem_wput2 (uaecptr addr, uae_u32 w)
{
    uae_u16 *m;

    addr -= chipmem_start & chipmem_mask;
    addr &= chipmem_mask;
    m = (uae_u16 *)(chipmemory + addr);

	if (ISILLEGAL_WORD (addr))
	{
		enforcer_display_hit (_T("WORD WRITE to"), m68k_getpc (&regs), addr);
		if (enforcermode & 1)
			set_special (&regs, SPCFLAG_TRAP);
	}
	if (ISEXEC (addr) || ISEXEC (addr + 1))
		return;
    do_put_mem_word (m, w);
}

static void REGPARAM2 chipmem_bput2 (uaecptr addr, uae_u32 b)
{
    addr -= chipmem_start & chipmem_mask;
    addr &= chipmem_mask;

	if (ISILLEGAL_BYTE (addr))
	{
		enforcer_display_hit (_T("BYTE WRITE to"), m68k_getpc (&regs), addr);
		if (enforcermode & 1)
			set_special (&regs, SPCFLAG_TRAP);
	}
	if (ISEXEC (addr))
		return;
    chipmemory[addr] = b;
}

static int REGPARAM2 chipmem_check2 (uaecptr addr, uae_u32 size)
{
    addr -= chipmem_start & chipmem_mask;
    addr &= chipmem_mask;
    return (addr + size) <= allocated_chipmem;
}

static uae_u8 * REGPARAM2 chipmem_xlate2 (uaecptr addr)
{
    addr -= chipmem_start & chipmem_mask;
    addr &= chipmem_mask;
    return chipmemory + addr;
}

/*
 * Replacement dummy memory accessors
 */
static uae_u32  dummy_lget2    (uaecptr addr) REGPARAM;
static uae_u32  dummy_wget2    (uaecptr addr) REGPARAM;
static uae_u32  dummy_bget2    (uaecptr addr) REGPARAM;
static void     dummy_lput2    (uaecptr addr, uae_u32 l) REGPARAM;
static void     dummy_wput2    (uaecptr addr, uae_u32 w) REGPARAM;
static void     dummy_bput2    (uaecptr addr, uae_u32 b) REGPARAM;
static int      dummy_check2   (uaecptr addr, uae_u32 size) REGPARAM;

static uae_u32 REGPARAM2 dummy_lget2 (uaecptr addr)
{
	special_mem_r;
	enforcer_display_hit (_T("LONG READ from"), m68k_getpc (&regs), addr);
	if (enforcermode & 1) {
		set_special (&regs, SPCFLAG_TRAP);
		return 0;
	}
    return 0xbadedeef;
}

#ifdef JIT
static int warned_JIT_0xF10000 = 0;
#endif

static uae_u32 REGPARAM2 dummy_wget2 (uaecptr addr)
{
	special_mem_r;

#ifdef JIT
    if (addr >= 0x00F10000 && addr <= 0x00F7FFFF) {
	if (!warned_JIT_0xF10000) {
	    warned_JIT_0xF10000 = 1;
			enforcer_display_hit ("LONG READ from", m68k_getpc(&regs) ,addr);
	}
	return 0;
    }
#endif
	enforcer_display_hit (_T("WORD READ from"), m68k_getpc (&regs), addr);
	if (enforcermode & 1) {
		set_special (&regs, SPCFLAG_TRAP);
    return 0;
	}
	return 0xbadf;
}

static uae_u32 REGPARAM2 dummy_bget2 (uaecptr addr)
{
	special_mem_r;
	enforcer_display_hit (_T("BYTE READ from"), m68k_getpc (&regs), addr);
	if (enforcermode & 1) {
		set_special (&regs, SPCFLAG_TRAP);
		return 0;
	}
	return 0xbadedeef;
}

static void REGPARAM2 dummy_lput2 (uaecptr addr, uae_u32 l)
{
	special_mem_w;
	enforcer_display_hit (_T("LONG WRITE to"), m68k_getpc (&regs), addr);
	if (enforcermode & 1) {
		set_special (&regs, SPCFLAG_TRAP);
		return;
	}
}

static void REGPARAM2 dummy_wput2 (uaecptr addr, uae_u32 w)
{
	special_mem_w;
	enforcer_display_hit (_T("WORD WRITE to"), m68k_getpc (&regs), addr);
	if (enforcermode & 1) {
		set_special (&regs, SPCFLAG_TRAP);
		return;
	}
}

static void REGPARAM2 dummy_bput2 (uaecptr addr, uae_u32 b)
{
	special_mem_w;
	enforcer_display_hit (_T("BYTE WRITE to"), m68k_getpc (&regs), addr);
	if (enforcermode & 1) {
		set_special (&regs, SPCFLAG_TRAP);
		return;
	}
}

static int REGPARAM2 dummy_check2 (uaecptr addr, uae_u32 size)
{
	special_mem_r;
	enforcer_display_hit (_T("CHECK from "), m68k_getpc (&regs), addr);
	return 0;
}


/*************************************************************
 enable the enforcer like support, maybe later this make MMU
 exceptions so enforcer can use it. Returns 1 if enforcer
 is enabled
*************************************************************/
int enforcer_enable (int enfmode)
{
    extern addrbank chipmem_bank, dummy_bank;
	enforcermode = enfmode;
    if (!enforcer_installed) {
		saved_dummy_lget = dummy_bank.lget;
		saved_dummy_wget = dummy_bank.wget;
		saved_dummy_bget = dummy_bank.bget;
		saved_dummy_lput = dummy_bank.lput;
		saved_dummy_wput = dummy_bank.wput;
		saved_dummy_bput = dummy_bank.bput;
		saved_chipmem_lget = chipmem_bank.lget;
		saved_chipmem_wget = chipmem_bank.wget;
		saved_chipmem_bget = chipmem_bank.bget;
		saved_chipmem_lput = chipmem_bank.lput;
		saved_chipmem_wput = chipmem_bank.wput;
		saved_chipmem_bput = chipmem_bank.bput;
		saved_chipmem_xlate = chipmem_bank.xlateaddr;
		saved_chipmem_check = chipmem_bank.check;

		dummy_bank.lget = dummy_lget2;
		dummy_bank.wget = dummy_wget2;
		dummy_bank.bget = dummy_bget2;
		dummy_bank.lput = dummy_lput2;
		dummy_bank.wput = dummy_wput2;
		dummy_bank.bput = dummy_bput2;
		chipmem_bank.lget = chipmem_lget2;
		chipmem_bank.wget = chipmem_wget2;
		chipmem_bank.bget = chipmem_bget2;
		chipmem_bank.lput = chipmem_lput2;
		chipmem_bank.wput = chipmem_wput2;
		chipmem_bank.bput = chipmem_bput2;
		chipmem_bank.xlateaddr = chipmem_xlate2;
		chipmem_bank.check = chipmem_check2;

	enforcer_installed = 1;
    }
    write_log ("Enforcer enabled\n");
    return 1;
}

/*************************************************************
 Disable Enforcer like support
*************************************************************/
int enforcer_disable (void)
{
    if (enforcer_installed) {
		dummy_bank.lget = saved_dummy_lget;
		dummy_bank.wget = saved_dummy_wget;
		dummy_bank.bget = saved_dummy_bget;
		dummy_bank.lput = saved_dummy_lput;
		dummy_bank.wput = saved_dummy_wput;
		dummy_bank.bput = saved_dummy_bput;
		chipmem_bank.lget = saved_chipmem_lget;
		chipmem_bank.wget = saved_chipmem_wget;
		chipmem_bank.bget = saved_chipmem_bget;
		chipmem_bank.lput = saved_chipmem_lput;
		chipmem_bank.wput = saved_chipmem_wput;
		chipmem_bank.bput = saved_chipmem_bput;
		chipmem_bank.xlateaddr = saved_chipmem_xlate;
		chipmem_bank.check = saved_chipmem_check;

	enforcer_installed = 0;
    }
    return 1;
}

#endif
