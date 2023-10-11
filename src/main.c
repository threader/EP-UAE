 /*
  * UAE - The Un*x Amiga Emulator
  *
  * Main program
  *
  * Copyright 1995 Ed Hanway
  * Copyright 1995, 1996, 1997 Bernd Schmidt
  * Copyright 2006-2007 Richard Drummond
  * Copyright 2010 Mustafa Tufan
  */

#include "sysconfig.h"
#include "sysdeps.h"
#include <assert.h>

#include "options.h"
#include "threaddep/thread.h"
#include "uae.h"
#include "audio.h"
#include "events.h"
#include "memory.h"
#include "custom.h"
#include "serial.h"
#include "newcpu.h"
#include "disk.h"
#include "debug.h"
#include "xwin.h"
#include "drawing.h"
#include "inputdevice.h"
#include "keybuf.h"
#include "gui.h"
#include "zfile.h"
#include "autoconf.h"
#include "traps.h"
#include "osemu.h"
#include "picasso96.h"
#include "bsdsocket.h"
#include "uaeexe.h"
#include "native2amiga.h"
#include "scsidev.h"
#include "savestate.h"
#include "hrtimer.h"
#include "sleep.h"
#include "uaeserial.h"
#include "akiko.h"
#include "cdtv.h"
#include "savestate.h"
#include "filesys.h"
#include "parallel.h"
#include "a2091.h"
#include "a2065.h"
#include "ncr_scsi.h"
#include "scsi.h"
#include "sana2.h"
#include "blkdev.h"
#include "gfxfilter.h"
#include "uaeresource.h"
#include "dongle.h"
#include "sleep.h"
#include "consolehook.h"
#include "version.h"

#ifdef USE_SDL
#include "SDL.h"
#endif

#ifdef WIN32
//FIXME: This shouldn't be necessary
#include "windows.h"
#endif

struct uae_prefs currprefs, changed_prefs;

static int restart_program;
int log_scsi;

struct gui_info gui_data;

#if 0
static void fixup_prefs_joysticks (struct uae_prefs *prefs)
{
    int joy_count = inputdevice_get_device_total (IDTYPE_JOYSTICK);

    /* If either port is configured to use a non-existent joystick, try
     * to use a sensible alternative.
     */
    if (prefs->jport0 >= JSEM_JOYS && prefs->jport0 < JSEM_MICE) {
	if (prefs->jport0 - JSEM_JOYS >= joy_count)
	    prefs->jport0 = (prefs->jport1 != JSEM_MICE) ? JSEM_MICE : JSEM_NONE;
    }
    if (prefs->jport1 >= JSEM_JOYS && prefs->jport1 < JSEM_MICE) {
	if (prefs->jport1 - JSEM_JOYS >= joy_count)
	    prefs->jport1 = (prefs->jport0 != JSEM_KBDLAYOUT) ? JSEM_KBDLAYOUT : JSEM_NONE;
    }
}
#endif 
static void fix_options (void)
{
    int err = 0;

    if ((currprefs.chipmem_size & (currprefs.chipmem_size - 1)) != 0
	|| currprefs.chipmem_size < 0x40000
	|| currprefs.chipmem_size > 0x800000)
    {
	currprefs.chipmem_size = 0x200000;
	write_log ("Unsupported chipmem size!\n");
	err = 1;
    }
    if (currprefs.chipmem_size > 0x80000)
	currprefs.chipset_mask |= CSMASK_ECS_AGNUS;

    if ((currprefs.fastmem_size & (currprefs.fastmem_size - 1)) != 0
	|| (currprefs.fastmem_size != 0 && (currprefs.fastmem_size < 0x100000 || currprefs.fastmem_size > 0x800000)))
    {
	currprefs.fastmem_size = 0;
	write_log ("Unsupported fastmem size!\n");
	err = 1;
    }
    if ((currprefs.gfxmem_size & (currprefs.gfxmem_size - 1)) != 0
	|| (currprefs.gfxmem_size != 0 && (currprefs.gfxmem_size < 0x100000 || currprefs.gfxmem_size > 0x2000000)))
    {
	write_log ("Unsupported graphics card memory size %lx!\n", currprefs.gfxmem_size);
	currprefs.gfxmem_size = 0;
	err = 1;
    }
    if ((currprefs.z3fastmem_size & (currprefs.z3fastmem_size - 1)) != 0
	|| (currprefs.z3fastmem_size != 0 && (currprefs.z3fastmem_size < 0x100000 || currprefs.z3fastmem_size > 0x20000000)))
    {
	currprefs.z3fastmem_size = 0;
	write_log ("Unsupported Zorro III fastmem size!\n");
	err = 1;
    }
    if (currprefs.address_space_24 && (currprefs.gfxmem_size != 0 || currprefs.z3fastmem_size != 0)) {
	currprefs.z3fastmem_size = currprefs.gfxmem_size = 0;
	write_log ("Can't use a graphics card or Zorro III fastmem when using a 24 bit\n"
		 "address space - sorry.\n");
    }
    if (currprefs.bogomem_size != 0 && currprefs.bogomem_size != 0x80000 && currprefs.bogomem_size != 0x100000 && currprefs.bogomem_size != 0x1C0000)
    {
	currprefs.bogomem_size = 0;
	write_log ("Unsupported bogomem size!\n");
	err = 1;
    }

    if (currprefs.chipmem_size > 0x200000 && currprefs.fastmem_size != 0) {
	write_log ("You can't use fastmem and more than 2MB chip at the same time!\n");
	currprefs.fastmem_size = 0;
	err = 1;
    }
#if 0
    if (currprefs.m68k_speed < -1 || currprefs.m68k_speed > 20) {
	write_log ("Bad value for -w parameter: must be -1, 0, or within 1..20.\n");
	currprefs.m68k_speed = 4;
	err = 1;
    }
#endif

    if (currprefs.produce_sound < 0 || currprefs.produce_sound > 3) {
	write_log ("Bad value for -S parameter: enable value must be within 0..3\n");
	currprefs.produce_sound = 0;
	err = 1;
    }
#ifdef JIT
    if (currprefs.comptrustbyte < 0 || currprefs.comptrustbyte > 3) {
	write_log ("Bad value for comptrustbyte parameter: value must be within 0..2\n");
	currprefs.comptrustbyte = 1;
	err = 1;
    }
    if (currprefs.comptrustword < 0 || currprefs.comptrustword > 3) {
	write_log ("Bad value for comptrustword parameter: value must be within 0..2\n");
	currprefs.comptrustword = 1;
	err = 1;
    }
    if (currprefs.comptrustlong < 0 || currprefs.comptrustlong > 3) {
	write_log ("Bad value for comptrustlong parameter: value must be within 0..2\n");
	currprefs.comptrustlong = 1;
	err = 1;
    }
    if (currprefs.comptrustnaddr < 0 || currprefs.comptrustnaddr > 3) {
	write_log ("Bad value for comptrustnaddr parameter: value must be within 0..2\n");
	currprefs.comptrustnaddr = 1;
	err = 1;
    }
    if (currprefs.compnf < 0 || currprefs.compnf > 1) {
	write_log ("Bad value for compnf parameter: value must be within 0..1\n");
	currprefs.compnf = 1;
	err = 1;
    }
    if (currprefs.comp_hardflush < 0 || currprefs.comp_hardflush > 1) {
	write_log ("Bad value for comp_hardflush parameter: value must be within 0..1\n");
	currprefs.comp_hardflush = 1;
	err = 1;
    }
    if (currprefs.comp_constjump < 0 || currprefs.comp_constjump > 1) {
	write_log ("Bad value for comp_constjump parameter: value must be within 0..1\n");
	currprefs.comp_constjump = 1;
	err = 1;
    }
    if (currprefs.comp_oldsegv < 0 || currprefs.comp_oldsegv > 1) {
	write_log ("Bad value for comp_oldsegv parameter: value must be within 0..1\n");
	currprefs.comp_oldsegv = 1;
	err = 1;
    }
    if (currprefs.cachesize < 0 || currprefs.cachesize > 16384) {
	write_log ("Bad value for cachesize parameter: value must be within 0..16384\n");
	currprefs.cachesize = 0;
	err = 1;
    }
#endif
    if (currprefs.cpu_level < 2 && currprefs.z3fastmem_size > 0) {
	write_log ("Z3 fast memory can't be used with a 68000/68010 emulation. It\n"
		 "requires a 68020 emulation. Turning off Z3 fast memory.\n");
	currprefs.z3fastmem_size = 0;
	err = 1;
    }
    if (currprefs.gfxmem_size > 0 && (currprefs.cpu_level < 2 || currprefs.address_space_24)) {
	write_log ("Picasso96 can't be used with a 68000/68010 or 68EC020 emulation. It\n"
		 "requires a 68020 emulation. Turning off Picasso96.\n");
	currprefs.gfxmem_size = 0;
	err = 1;
    }
#ifndef BSDSOCKET
    if (currprefs.socket_emu) {
	write_log ("Compile-time option of BSDSOCKET was not enabled.  You can't use bsd-socket emulation.\n");
	currprefs.socket_emu = 0;
	err = 1;
    }
#endif

    if (currprefs.nr_floppies < 0 || currprefs.nr_floppies > 4) {
	write_log ("Invalid number of floppies.  Using 4.\n");
	currprefs.nr_floppies = 4;
	currprefs.dfxtype[0] = 0;
	currprefs.dfxtype[1] = 0;
	currprefs.dfxtype[2] = 0;
	currprefs.dfxtype[3] = 0;
	err = 1;
    }

    if (currprefs.floppy_speed > 0 && currprefs.floppy_speed < 10) {
	currprefs.floppy_speed = 100;
    }
    if (currprefs.input_mouse_speed < 1 || currprefs.input_mouse_speed > 1000) {
	currprefs.input_mouse_speed = 100;
    }

    if (currprefs.collision_level < 0 || currprefs.collision_level > 3) {
	write_log ("Invalid collision support level.  Using 1.\n");
	currprefs.collision_level = 1;
	err = 1;
    }
    fixup_prefs_dimensions (&currprefs);

#ifdef CPU_68000_ONLY
    currprefs.cpu_level = 0;
#endif
#ifndef CPUEMU_0
    currprefs.cpu_compatible = 1;
    currprefs.address_space_24 = 1;
#endif
#if !defined(CPUEMU_5) && !defined (CPUEMU_6)
    currprefs.cpu_compatible = 0;
    currprefs.address_space_24 = 0;
#endif
#if !defined (CPUEMU_6)
    currprefs.cpu_cycle_exact = currprefs.blitter_cycle_exact = 0;
#endif
#ifndef AGA
    currprefs.chipset_mask &= ~CSMASK_AGA;
#endif
#ifndef AUTOCONFIG
    currprefs.z3fastmem_size = 0;
    currprefs.fastmem_size = 0;
    currprefs.gfxmem_size = 0;
#endif
#if !defined (BSDSOCKET)
    currprefs.socket_emu = 0;
#endif
#if !defined (SCSIEMU)
    currprefs.scsi = 0;
#ifdef _WIN32
    currprefs.win32_aspi = 0;
#endif
#endif

   // fixup_prefs_joysticks (&currprefs);

    if (err)
	write_log ("Please use \"uae -h\" to get usage information.\n");
}

struct uae_prefs currprefs, changed_prefs;
int config_changed;

int pissoff_value = 25000;
int pause_emulation;
char start_path_data[MAX_DPATH];
int no_gui = 0, quit_to_gui = 0;
int cloanto_rom = 0;
int kickstart_rom = 1;
int console_emulation = 0;

struct gui_info gui_data;

char warning_buffer[256];

char optionsfile[256];

static void hr (void)
{
    write_log ("------------------------------------------------------------------------------------\n");

}

static void show_version (void)
{
#ifdef PACKAGE_VERSION
    write_log (PACKAGE_NAME " " PACKAGE_VERSION "\n");
#else
    write_log ("EP-UAE %d.%d.%d\n", UAEMAJOR, UAEMINOR, UAESUBREV);
#endif
    write_log ("Build date: " __DATE__ " " __TIME__ "\n");
}

static void show_version_full (void)
{

	hr ();
    show_version ();
	hr ();
    write_log ("Copyright 1995-2002 Bernd Schmidt\n");
    write_log ("          1999-2010 Toni Wilen\n");
    write_log ("          TODO n");

    write_log ("          2003-2007 Richard Drummond\n");
    write_log ("          2006-2010 Mustafa Tufan\n\n");
    write_log ("See the source for a full list of contributors.\n");
    write_log ("This is free software; see the file COPYING for copying conditions.  There is NO\n");
    write_log ("warranty; not even for MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.\n");
	hr ();
}

int uaerand (void)
{
	return rand ();
}

void discard_prefs (struct uae_prefs *p, int type)
{
	struct strlist **ps = &p->all_lines;
	while (*ps) {
		struct strlist *s = *ps;
		*ps = s->next;
		xfree (s->value);
		xfree (s->option);
		xfree (s);
	}
#ifdef FILESYS
	filesys_cleanup ();
#endif
}

static void fixup_prefs_dim2 (struct wh *wh)
{
	if (wh->width < 160)
		wh->width = 160;
	if (wh->height < 128)
		wh->height = 128;
	if (wh->width > 3072)
		wh->width = 3072;
	if (wh->height > 2048)
		wh->height = 2048;
}

void fixup_prefs_dimensions (struct uae_prefs *prefs)
{
    if (prefs->gfx_width_fs < 320)
		prefs->gfx_width_fs = 320;
    if (prefs->gfx_height_fs < 200)
		prefs->gfx_height_fs = 200;
    if (prefs->gfx_height_fs > 1280)
		prefs->gfx_height_fs = 1280;
    prefs->gfx_width_fs += 7;
    prefs->gfx_width_fs &= ~7;

    if (prefs->gfx_width_win < 320)
		prefs->gfx_width_win = 320;
    if (prefs->gfx_height_win < 200)
		prefs->gfx_height_win = 200;
    if (prefs->gfx_height_win > 1280)
		prefs->gfx_height_win = 1280;
    prefs->gfx_width_win += 7;
    prefs->gfx_width_win &= ~7;
}

void fixup_cpu (struct uae_prefs *p)
{
	if (p->cpu_frequency == 1000000)
		p->cpu_frequency = 0;
	switch (p->cpu_model)
	{
	case 68000:
		p->address_space_24 = 1;
		if (p->cpu_compatible || p->cpu_cycle_exact)
			p->fpu_model = 0;
		break;
	case 68010:
		p->address_space_24 = 1;
		if (p->cpu_compatible || p->cpu_cycle_exact)
			p->fpu_model = 0;
		break;
	case 68020:
		break;
	case 68030:
		p->address_space_24 = 0;
		break;
	case 68040:
		p->address_space_24 = 0;
		if (p->fpu_model)
			p->fpu_model = 68040;
		break;
	case 68060:
		p->address_space_24 = 0;
		if (p->fpu_model)
			p->fpu_model = 68060;
		break;
	}
	if (p->cpu_model != 68040)
		p->mmu_model = 0;
}


void fixup_prefs (struct uae_prefs *p)
{
    int err = 0;

	built_in_chipset_prefs (p);
	fixup_cpu (p);

	if (((p->chipmem_size & (p->chipmem_size - 1)) != 0 && p->chipmem_size != 0x180000)
		|| p->chipmem_size < 0x20000
		|| p->chipmem_size > 0x800000)
    {
		write_log ("Unsupported chipmem size %x!\n", p->chipmem_size);
		p->chipmem_size = 0x200000;
		err = 1;
    }
	if ((p->fastmem_size & (p->fastmem_size - 1)) != 0
		|| (p->fastmem_size != 0 && (p->fastmem_size < 0x100000 || p->fastmem_size > 0x800000)))
    {
		write_log ("Unsupported fastmem size %x!\n", p->fastmem_size);
		err = 1;
    }
	if ((p->gfxmem_size & (p->gfxmem_size - 1)) != 0
		|| (p->gfxmem_size != 0 && (p->gfxmem_size < 0x100000 || p->gfxmem_size > max_z3fastmem / 2)))
    {
		write_log ("Unsupported graphics card memory size %x (%x)!\n", p->gfxmem_size, max_z3fastmem / 2);
		if (p->gfxmem_size > max_z3fastmem / 2)
			p->gfxmem_size = max_z3fastmem / 2;
		else
			p->gfxmem_size = 0;
		err = 1;
    }
	if ((p->z3fastmem_size & (p->z3fastmem_size - 1)) != 0
		|| (p->z3fastmem_size != 0 && (p->z3fastmem_size < 0x100000 || p->z3fastmem_size > max_z3fastmem)))
    {
		write_log ("Unsupported Zorro III fastmem size %x (%x)!\n", p->z3fastmem_size, max_z3fastmem);
		if (p->z3fastmem_size > max_z3fastmem)
			p->z3fastmem_size = max_z3fastmem;
		else
			p->z3fastmem_size = 0;
	err = 1;
    }
	if ((p->z3fastmem2_size & (p->z3fastmem2_size - 1)) != 0
		|| (p->z3fastmem2_size != 0 && (p->z3fastmem2_size < 0x100000 || p->z3fastmem2_size > max_z3fastmem)))
	{
		write_log ("Unsupported Zorro III fastmem size %x (%x)!\n", p->z3fastmem2_size, max_z3fastmem);
		if (p->z3fastmem2_size > max_z3fastmem)
			p->z3fastmem2_size = max_z3fastmem;
		else
			p->z3fastmem2_size = 0;
		err = 1;
	}
	p->z3fastmem_start &= ~0xffff;
	if (p->z3fastmem_start < 0x1000000)
		p->z3fastmem_start = 0x1000000;

	if (p->address_space_24 && (p->gfxmem_size != 0 || p->z3fastmem_size != 0)) {
		p->z3fastmem_size = p->gfxmem_size = 0;
		write_log ("Can't use a graphics card or Zorro III fastmem when using a 24 bit\n"
			"address space - sorry.\n");
	}
	if (p->bogomem_size != 0 && p->bogomem_size != 0x80000 && p->bogomem_size != 0x100000 && p->bogomem_size != 0x180000 && p->bogomem_size != 0x1c0000) {
		p->bogomem_size = 0;
		write_log ("Unsupported bogomem size!\n");
		err = 1;
	}
	if (p->bogomem_size > 0x100000 && (p->cs_fatgaryrev >= 0 || p->cs_ide || p->cs_ramseyrev >= 0)) {
		p->bogomem_size = 0x100000;
		write_log ("Possible Gayle bogomem conflict fixed\n");
	}
	if (p->chipmem_size > 0x200000 && p->fastmem_size != 0) {
		write_log ("You can't use fastmem and more than 2MB chip at the same time!\n");
		p->fastmem_size = 0;
		err = 1;
	}
	if (p->mbresmem_low_size > 0x04000000 || (p->mbresmem_low_size & 0xfffff)) {
		p->mbresmem_low_size = 0;
		write_log ("Unsupported A3000 MB RAM size\n");
	}
	if (p->mbresmem_high_size > 0x04000000 || (p->mbresmem_high_size & 0xfffff)) {
		p->mbresmem_high_size = 0;
		write_log ("Unsupported Motherboard RAM size\n");
	}

#if 0
	if (p->m68k_speed < -1 || p->m68k_speed > 20) {
		write_log ("Bad value for -w parameter: must be -1, 0, or within 1..20.\n");
		p->m68k_speed = 4;
		err = 1;
    }
#endif

	if (p->produce_sound < 0 || p->produce_sound > 3) {
		write_log ("Bad value for -S parameter: enable value must be within 0..3\n");
		p->produce_sound = 0;
		err = 1;
    }
#ifdef JIT
	if (p->comptrustbyte < 0 || p->comptrustbyte > 3) {
		write_log ("Bad value for comptrustbyte parameter: value must be within 0..2\n");
		p->comptrustbyte = 1;
		err = 1;
	}
	if (p->comptrustword < 0 || p->comptrustword > 3) {
		write_log ("Bad value for comptrustword parameter: value must be within 0..2\n");
		p->comptrustword = 1;
		err = 1;
	}
	if (p->comptrustlong < 0 || p->comptrustlong > 3) {
		write_log ("Bad value for comptrustlong parameter: value must be within 0..2\n");
		p->comptrustlong = 1;
		err = 1;
	}
	if (p->comptrustnaddr < 0 || p->comptrustnaddr > 3) {
		write_log ("Bad value for comptrustnaddr parameter: value must be within 0..2\n");
		p->comptrustnaddr = 1;
		err = 1;
	}
	if (p->compnf < 0 || p->compnf > 1) {
		write_log ("Bad value for compnf parameter: value must be within 0..1\n");
		p->compnf = 1;
		err = 1;
	}
	if (p->comp_hardflush < 0 || p->comp_hardflush > 1) {
		write_log ("Bad value for comp_hardflush parameter: value must be within 0..1\n");
		p->comp_hardflush = 1;
		err = 1;
	}
	if (p->comp_constjump < 0 || p->comp_constjump > 1) {
		write_log ("Bad value for comp_constjump parameter: value must be within 0..1\n");
		p->comp_constjump = 1;
		err = 1;
	}
	if (p->comp_oldsegv < 0 || p->comp_oldsegv > 1) {
		write_log ("Bad value for comp_oldsegv parameter: value must be within 0..1\n");
		p->comp_oldsegv = 1;
		err = 1;
	}
	if (p->cachesize < 0 || p->cachesize > 16384) {
		write_log ("Bad value for cachesize parameter: value must be within 0..16384\n");
		p->cachesize = 0;
		err = 1;
	}
#endif
	if (p->z3fastmem_size > 0 && (p->address_space_24 || p->cpu_model < 68020)) {
		write_log ("Z3 fast memory can't be used with a 68000/68010 emulation. It\n"
			"requires a 68020 emulation. Turning off Z3 fast memory.\n");
		p->z3fastmem_size = 0;
		err = 1;
    }
	if (p->gfxmem_size > 0 && (p->cpu_model < 68020 || p->address_space_24)) {
		write_log ("Picasso96 can't be used with a 68000/68010 or 68EC020 emulation. It\n"
			"requires a 68020 emulation. Turning off Picasso96.\n");
		p->gfxmem_size = 0;
		err = 1;
    }
#if !defined (BSDSOCKET)
	if (p->socket_emu) {
		write_log ("Compile-time option of BSDSOCKET_SUPPORTED was not enabled.  You can't use bsd-socket emulation.\n");
		p->socket_emu = 0;
		err = 1;
    }
#endif

	if (p->nr_floppies < 0 || p->nr_floppies > 4) {
		write_log ("Invalid number of floppies.  Using 4.\n");
		p->nr_floppies = 4;
		p->dfxtype[0] = 0;
		p->dfxtype[1] = 0;
		p->dfxtype[2] = 0;
		p->dfxtype[3] = 0;
		err = 1;
	}
	if (p->floppy_speed > 0 && p->floppy_speed < 10) {
		p->floppy_speed = 100;
	}
	if (p->input_mouse_speed < 1 || p->input_mouse_speed > 1000) {
		p->input_mouse_speed = 100;
    }
	if (p->collision_level < 0 || p->collision_level > 3) {
		write_log ("Invalid collision support level.  Using 1.\n");
		p->collision_level = 1;
		err = 1;
    }
	if (p->parallel_postscript_emulation)
		p->parallel_postscript_detection = 1;
	if (p->cs_compatible == 1) {
		p->cs_fatgaryrev = p->cs_ramseyrev = p->cs_mbdmac = -1;
		p->cs_ide = 0;
		if (p->cpu_model >= 68020) {
			p->cs_fatgaryrev = 0;
			p->cs_ide = -1;
			p->cs_ramseyrev = 0x0f;
			p->cs_mbdmac = 0;
		}
	}
    fixup_prefs_dimensions (p);

#ifdef JIT
	p->cachesize = 0;
#endif
#ifdef CPU_68000_ONLY
	p->cpu_model = 68000;
	p->fpu_model = 0;
#endif
#ifndef CPUEMU_0
	p->cpu_compatible = 1;
	p->address_space_24 = 1;
#endif
#if !defined(CPUEMU_11) && !defined (CPUEMU_12)
	p->cpu_compatible = 0;
	p->address_space_24 = 0;
#endif
#if !defined (CPUEMU_12)
	p->cpu_cycle_exact = p->blitter_cycle_exact = 0;
#endif
#ifndef AGA
	p->chipset_mask &= ~CSMASK_AGA;
#endif
#ifndef AUTOCONFIG
	p->z3fastmem_size = 0;
	p->fastmem_size = 0;
	p->gfxmem_size = 0;
#endif
#if !defined (BSDSOCKET)
	p->socket_emu = 0;
#endif
#if !defined (SCSIEMU)
	p->scsi = 0;
#ifdef _WIN32
	p->win32_aspi = 0;
#endif
#endif
#if !defined (SANA2)
	p->sana2 = 0;
#endif
#if !defined (UAESERIAL)
	p->uaeserial = 0;
#endif
#if defined (CPUEMU_12)
	if (p->cpu_cycle_exact) {
		p->gfx_framerate = 1;
#ifdef JIT
		p->cachesize = 0;
#endif
		p->m68k_speed = 0;
	}
#endif
	if (p->maprom && !p->address_space_24)
		p->maprom = 0x0f000000;
	if (p->tod_hack && p->cs_ciaatod == 0)
		p->cs_ciaatod = p->ntscmode ? 2 : 1;
	//target_fixup_options (p);
}

int quit_program = 0;
static int restart_program;
static char restart_config[MAX_DPATH];
static int default_config;

static int uae_state;
static int uae_target_state;

void uae_reset (int hardreset)
{
    switch (uae_target_state) {
	case UAE_STATE_QUITTING:
	case UAE_STATE_STOPPED:
	case UAE_STATE_COLD_START:
	case UAE_STATE_WARM_START:
	    /* Do nothing */
	    break;
	default:
	    uae_target_state = hardreset ? UAE_STATE_COLD_START : UAE_STATE_WARM_START;
    }
	if (quit_program == 0) {
		quit_program = -2;
		if (hardreset)
			quit_program = -3;
	}

}

void uae_quit (void)
{
    if (uae_target_state != UAE_STATE_QUITTING) {
	uae_target_state = UAE_STATE_QUITTING;
    }
	deactivate_debugger ();
	if (quit_program != -1)
		quit_program = -1;
	//target_quit ();
}

/* 0 = normal, 1 = nogui, -1 = disable nogui */
void uae_restart (int opengui, char *cfgfile)
{
	uae_quit ();
	restart_program = opengui > 0 ? 1 : (opengui == 0 ? 2 : 3);
	restart_config[0] = 0;
	default_config = 0;
	if (cfgfile)
		strcpy (restart_config, cfgfile);
}

#ifndef DONT_PARSE_CMDLINE

void usage (void)
{
}
static void parse_cmdline_2 (int argc, char **argv)
{
	int i;

	cfgfile_addcfgparam (0);
	for (i = 1; i < argc; i++) {
		if (_tcsncmp (argv[i], "-cfgparam=", 10) == 0) {
			cfgfile_addcfgparam (argv[i] + 10);
		} else if (_tcscmp (argv[i], "-cfgparam") == 0) {
			if (i + 1 == argc)
				write_log ("Missing argument for '-cfgparam' option.\n");
			else
				cfgfile_addcfgparam (argv[++i]);
		}
	}
}

static void parse_diskswapper (char *s)
{
	char *tmp = my_strdup (s);
	char *delim = ",";
	char *p1, *p2;
	int num = 0;

	p1 = tmp;
	for (;;) {
		p2 = strtok (p1, delim);
		if (!p2)
			break;
		p1 = NULL;
		if (num >= MAX_SPARE_DRIVES)
			break;
		_tcsncpy (currprefs.dfxlist[num], p2, 255);
		num++;
	}
	free (tmp);
}

static char *parsetext (const char *s)
{
	if (*s == '"' || *s == '\'') {
		char *d;
		char c = *s++;
		int i;
		d = my_strdup (s);
		for (i = 0; i < _tcslen (d); i++) {
			if (d[i] == c) {
				d[i] = 0;
				break;
			}
		}
		return d;
	} else {
		return my_strdup (s);
	}
}

static void parse_cmdline (int argc, char **argv)
{
    int i;

    for (i = 1; i < argc; i++) {
	if (strcmp (argv[i], "-cfgparam") == 0) {
	    if (i + 1 < argc)
		i++;
	} else if (strncmp (argv[i], "-config=", 8) == 0) {
#ifdef FILESYS
	    free_mountinfo (currprefs.mountinfo);
#endif
	    if (target_cfgfile_load (&currprefs, argv[i] + 8, 0))
		strcpy (optionsfile, argv[i] + 8);
	}
	/* Check for new-style "-f xxx" argument, where xxx is config-file */
	else if (strcmp (argv[i], "-f") == 0) {
	    if (i + 1 == argc) {
		write_log ("Missing argument for '-f' option.\n");
	    } else {
#ifdef FILESYS
		free_mountinfo (currprefs.mountinfo);
#endif
		if (target_cfgfile_load (&currprefs, argv[++i], 0))
		    strcpy (optionsfile, argv[i]);
	    }
	} else if (strcmp (argv[i], "-s") == 0) {
	    if (i + 1 == argc)
		write_log ("Missing argument for '-s' option.\n");
	    else
		cfgfile_parse_line (&currprefs, argv[++i], 0);
	} else if (strcmp (argv[i], "-h") == 0 || strcmp (argv[i], "-help") == 0) {
	    usage ();
	    exit (0);
	} else if (strcmp (argv[i], "-version") == 0) {
	    show_version_full ();
	    exit (0);
	} else if (strcmp (argv[i], "-scsilog") == 0) {
	    log_scsi = 1;
	} else {
	    if (argv[i][0] == '-' && argv[i][1] != '\0') {
		const char *arg = argv[i] + 2;
		int extra_arg = *arg == '\0';
		if (extra_arg)
		    arg = i + 1 < argc ? argv[i + 1] : 0;
		if (parse_cmdline_option (&currprefs, argv[i][1], (char*)arg) && extra_arg)
		    i++;
	    }
	}
		if (!_tcsncmp (argv[i], "-diskswapper=", 13)) {
			char *txt = parsetext (argv[i] + 13);
			parse_diskswapper (txt);
			xfree (txt);
		} else if (_tcsncmp (argv[i], "-cfgparam=", 10) == 0) {
			;
		} else if (_tcscmp (argv[i], "-cfgparam") == 0) {
		    if (i + 1 < argc)
				i++;
		} else if (_tcsncmp (argv[i], "-config=", 8) == 0) {
			char *txt = parsetext (argv[i] + 8);
			currprefs.mountitems = 0;
			target_cfgfile_load (&currprefs, txt, -1, 0);
			xfree (txt);
		} else if (_tcsncmp (argv[i], "-statefile=", 11) == 0) {
			char *txt = parsetext (argv[i] + 11);
			savestate_state = STATE_DORESTORE;
			_tcscpy (savestate_fname, txt);
			xfree (txt);
		} else if (_tcscmp (argv[i], "-f") == 0) {
			/* Check for new-style "-f xxx" argument, where xxx is config-file */
		    if (i + 1 == argc) {
				write_log ("Missing argument for '-f' option.\n");
		    } else {
				currprefs.mountitems = 0;
				target_cfgfile_load (&currprefs, argv[++i], -1, 0);
		    }
		} else if (_tcscmp (argv[i], "-s") == 0) {
		    if (i + 1 == argc)
				write_log ("Missing argument for '-s' option.\n");
		    else
				cfgfile_parse_line (&currprefs, argv[++i], 0);
		} else if (_tcscmp (argv[i], "-h") == 0 || _tcscmp (argv[i], "-help") == 0) {
		    usage ();
		    exit (0);
		} else if (_tcsncmp (argv[i], "-cdimage=", 9) == 0) {
			char *txt = parsetext (argv[i] + 9);
			cfgfile_parse_option (&currprefs, "cdimage0", txt, 0);
			xfree (txt);
		} else {
		    if (argv[i][0] == '-' && argv[i][1] != '\0') {
				const char *arg = argv[i] + 2;
				int extra_arg = *arg == '\0';
				if (extra_arg)
				    arg = i + 1 < argc ? argv[i + 1] : 0;
				if (parse_cmdline_option (&currprefs, argv[i][1], arg) && extra_arg)
				    i++;
		    }
		}
    }
}
#endif

static void parse_cmdline_and_init_file (int argc, char **argv)
{

	strcpy (optionsfile, "");

#ifdef OPTIONS_IN_HOME
	{
	    char *home = getenv ("HOME");
    	if (home != NULL && strlen (home) < 240)
    	{
			_tcscpy (optionsfile, home);
			_tcscat (optionsfile, "/");
    	}
	}
#endif

	parse_cmdline_2 (argc, argv);

	_tcscat (optionsfile, restart_config);

	if (! target_cfgfile_load (&currprefs, optionsfile, 0, default_config)) {
		write_log ("failed to load config '%s'\n", optionsfile);
#ifdef OPTIONS_IN_HOME
	/* sam: if not found in $HOME then look in current directory */
	char *saved_path = strdup (optionsfile);
	strcpy (optionsfile, OPTIONSFILENAME);
	if (! target_cfgfile_load (&currprefs, optionsfile, 0) ) {
	    /* If not in current dir either, change path back to home
	     * directory - so that a GUI can save a new config file there */
	    strcpy (optionsfile, saved_path);
	}
	free (saved_path);
#endif
    }
    fixup_prefs (&currprefs);

    parse_cmdline (argc, argv);
    fixup_prefs (&currprefs);
}

/*
 * Save the currently loaded configuration.
 */
void uae_save_config (void)
{
    FILE *f;
    char tmp[257];

    /* Back up the old file.  */
    strcpy (tmp, optionsfile);
    strcat (tmp, "~");
    write_log ("Backing-up config file '%s' to '%s'\n", optionsfile, tmp);
    rename (optionsfile, tmp);

    write_log ("Writing new config file '%s'\n", optionsfile);
    f = fopen (optionsfile, "w");
    if (f == NULL) {
	gui_message ("Error saving configuration file.!\n"); // FIXME - better error msg.
	return;
    }

    // FIXME  - either fix this nonsense, or only allow config to be saved when emulator is stopped.
    if (uae_get_state () == UAE_STATE_STOPPED)
	cfgfile_save_options (f, &changed_prefs, 0);
    else
	cfgfile_save_options (f, &currprefs, 0);

    fclose (f);
}


/*
 * A first cut at better state management...
 */

static int uae_state;
static int uae_target_state;

int uae_get_state (void)
{
    return uae_state;
}

static void set_state (int state)
{
    uae_state = state;
    gui_notify_state (state);
    graphics_notify_state (state);
}

int uae_state_change_pending (void)
{
    return uae_state != uae_target_state;
}

void uae_start (void)
{
    uae_target_state = UAE_STATE_COLD_START;
}

void uae_pause (void)
{
    if (uae_target_state == UAE_STATE_RUNNING)
	uae_target_state = UAE_STATE_PAUSED;
}

void uae_resume (void)
{
    if (uae_target_state == UAE_STATE_PAUSED)
	uae_target_state = UAE_STATE_RUNNING;
}

void uae_stop (void)
{
    if (uae_target_state != UAE_STATE_QUITTING && uae_target_state != UAE_STATE_STOPPED) {
	uae_target_state = UAE_STATE_STOPPED;
	restart_config[0] = 0;
    }
}


/*
 * Early initialization of emulator, parsing of command-line options,
 * and loading of config files, etc.
 *
 * TODO: Need better cohesion! Break this sucker up!
 */
static int do_preinit_machine (int argc, char **argv)
{
    if (! graphics_setup ()) {
	exit (1);
    }
    if (restart_config[0]) {
#ifdef FILESYS
	free_mountinfo (currprefs.mountinfo);
#endif
	default_prefs (&currprefs, 0);
	fix_options ();
    }

#ifdef NATMEM_OFFSET
    init_shm ();
#endif

#ifdef FILESYS
    rtarea_init ();
    hardfile_install ();
#endif

    if (restart_config[0])
        parse_cmdline_and_init_file (argc, argv);
    else
	currprefs = changed_prefs;

    uae_inithrtimer ();

    machdep_init ();

    if (! audio_setup ()) {
	write_log ("Sound driver unavailable: Sound output disabled\n");
	currprefs.produce_sound = 0;
    }
    inputdevice_init ();

    return 1;
}

/*
 * Initialization of emulator proper
 */
static int do_init_machine (void)
{
#ifdef JIT
    if (!(( currprefs.cpu_level >= 2 ) && ( currprefs.address_space_24 == 0 ) && ( currprefs.cachesize )))
	canbang = 0;
#endif

#ifdef _WIN32
    logging_init(); /* Yes, we call this twice - the first case handles when the user has loaded
		       a config using the cmd-line.  This case handles loads through the GUI. */
#endif

#ifdef SAVESTATE
    savestate_init ();
#endif
#ifdef SCSIEMU
    scsidev_install ();
#endif
#ifdef AUTOCONFIG
    /* Install resident module to get 8MB chipmem, if requested */
    rtarea_setup ();
#endif

    keybuf_init (); /* Must come after init_joystick */

#ifdef AUTOCONFIG
    expansion_init ();
#endif
    memory_init ();
    memory_reset ();

#ifdef FILESYS
    filesys_install ();
#endif
#ifdef AUTOCONFIG
    bsdlib_install ();
    emulib_install ();
    uaeexe_install ();
    native2amiga_install ();
#endif

    if (custom_init ()) { /* Must come after memory_init */
#ifdef SERIAL_PORT
	serial_init ();
#endif
	DISK_init ();

	reset_frame_rate_hack ();
	init_m68k(); /* must come after reset_frame_rate_hack (); */

	gui_update ();

	if (graphics_init ()) {

#ifdef DEBUGGER
	    setup_brkhandler ();

	    if (currprefs.start_debugger && debuggable ())
		activate_debugger ();
#endif

#ifdef WIN32
#ifdef FILESYS
	    filesys_init (); /* New function, to do 'add_filesys_unit()' calls at start-up */
#endif
#endif
	    if (sound_available && currprefs.produce_sound > 1 && ! audio_init ()) {
		write_log ("Sound driver unavailable: Sound output disabled\n");
		currprefs.produce_sound = 0;
	    }

	    return 1;
	}
    }
    return 0;
}

/*
 * Reset emulator
 */
static void do_reset_machine (int hardreset)
{
#ifdef SAVESTATE
    if (savestate_state == STATE_RESTORE)
	restore_state (savestate_fname);
    else if (savestate_state == STATE_REWIND)
	savestate_rewind ();
#endif
    /* following three lines must not be reordered or
     * fastram state restore breaks
     */
    reset_all_systems ();
    customreset (hardreset);
    m68k_reset (hardreset);
    if (hardreset) {
	memset (chipmemory, 0, allocated_chipmem);
	write_log ("chipmem cleared\n");
    }
#ifdef SAVESTATE
    /* We may have been restoring state, but we're done now.  */
    if (savestate_state == STATE_RESTORE || savestate_state == STATE_REWIND)
    {
	map_overlay (1);
	fill_prefetch_slow (&regs); /* compatibility with old state saves */
    }
    savestate_restore_finish ();
#endif

    fill_prefetch_slow (&regs);
    if (currprefs.produce_sound == 0)
	eventtab[ev_audio].active = 0;
    handle_active_events ();

    inputdevice_updateconfig (&currprefs);
}

/*
 * Run emulator
 */
static void do_run_machine (void)
{
#if defined (NATMEM_OFFSET) && defined( _WIN32 ) && !defined( NO_WIN32_EXCEPTION_HANDLER )
    extern int EvalException ( LPEXCEPTION_POINTERS blah, int n_except );
    __try
#endif
    {
	m68k_go (1);
    }
#if defined (NATMEM_OFFSET) && defined( _WIN32 ) && !defined( NO_WIN32_EXCEPTION_HANDLER )
    __except( EvalException( GetExceptionInformation(), GetExceptionCode() ) )
    {
	// EvalException does the good stuff...
    }
#endif
}

/*
 * Exit emulator
 */
static void do_exit_machine (void)
{
    graphics_leave ();
    inputdevice_close ();

#ifdef SCSIEMU
    scsidev_exit ();
#endif
    DISK_free ();
    audio_close ();
    dump_counts ();
#ifdef SERIAL_PORT
    serial_exit ();
#endif
#ifdef CD32
    akiko_free ();
#endif
    gui_exit ();

#ifdef AUTOCONFIG
    expansion_cleanup ();
#endif
#ifdef FILESYS
    filesys_cleanup ();
#endif
#ifdef SAVESTATE
    savestate_free ();
#endif
    memory_cleanup ();
    cfgfile_addcfgparam (0);
}

#if 0
/*
 * Here's where all the action takes place!
 */
void real_main (int argc, char **argv)
{
    show_version ();

    currprefs.mountinfo = changed_prefs.mountinfo = &options_mountinfo;
    restart_program = 1;
#ifdef _WIN32
    sprintf (restart_config, "%sConfigurations\\", start_path);
#endif
    strcat (restart_config, OPTIONSFILENAME);

    /* Initial state is stopped */
    uae_target_state = UAE_STATE_STOPPED;

    while (uae_target_state != UAE_STATE_QUITTING) {
	int want_gui;

	set_state (uae_target_state);

	do_preinit_machine (argc, argv);

        /* Should we open the GUI? TODO: This mess needs to go away */
	want_gui = currprefs.start_gui;
        if (restart_program == 2)
	    want_gui = 0;
        else if (restart_program == 3)
	    want_gui = 1;

	changed_prefs = currprefs;


	if (want_gui) {
	    /* Handle GUI at start-up */
	    int err = gui_open ();

	    if (err >= 0) {
		do {
		    gui_handle_events ();

		    uae_msleep (10);

		} while (!uae_state_change_pending ());
	    } else if (err == - 1) {
		if (restart_program == 3) {
		    restart_program = 0;
		    uae_quit ();
		}
	    } else
		uae_quit ();

	    currprefs = changed_prefs;
	    fix_options ();
	    inputdevice_init ();
	}

	restart_program = 0;

	if (uae_target_state == UAE_STATE_QUITTING)
	    break;

	uae_target_state = UAE_STATE_COLD_START;

	/* Start emulator proper. */
	if (!do_init_machine ())
	    break;

	while (uae_target_state != UAE_STATE_QUITTING && uae_target_state != UAE_STATE_STOPPED) {
	    /* Reset */
	    set_state (uae_target_state);
	    do_reset_machine (uae_state == UAE_STATE_COLD_START);

	    /* Running */
	    uae_target_state = UAE_STATE_RUNNING;

	    /*
 	     * Main Loop
	     */
	    do {
		set_state (uae_target_state);

		/* Run emulator. */
		do_run_machine ();

		if (uae_target_state == UAE_STATE_PAUSED) {
		    /* Paused */
		    set_state (uae_target_state);

		    audio_pause ();

		    /* While UAE is paused we have to handle
		     * input events, etc. ourselves.
		     */
		    do {
			gui_handle_events ();
			handle_events ();

			/* Manually pump input device */
                	inputdevicefunc_keyboard.read ();
			inputdevicefunc_mouse.read ();
			inputdevicefunc_joystick.read ();
			inputdevice_handle_inputcode ();

			/* Don't busy wait. */
			uae_msleep (10);

		    } while (!uae_state_change_pending ());

		    audio_resume ();
		}

	    } while (uae_target_state == UAE_STATE_RUNNING);

	    /*
	     * End of Main Loop
	     *
	     * We're no longer running or paused.
	     */

	    set_inhibit_frame (IHF_QUIT_PROGRAM);

#ifdef FILESYS
	    /* Ensure any cached changes to virtual filesystem are flushed before
	     * resetting or exitting. */
	    filesys_prepare_reset ();
#endif

	} /* while (!QUITTING && !STOPPED) */

	do_exit_machine ();

        /* TODO: This stuff is a hack. What we need to do is
	 * check whether a config GUI is available. If not,
	 * then quit.
	 */
	restart_program = 3;
    }
    zfile_exit ();
}
#endif
#ifdef USE_SDL
int init_sdl (void)
{
    int result = (SDL_Init (SDL_INIT_TIMER | SDL_INIT_NOPARACHUTE /*| SDL_INIT_AUDIO*/) == 0);
    if (result)
        atexit (SDL_Quit);

    return result;
}
#else
#define init_sdl()
#endif
void reset_all_systems (void)
{
	init_eventtab ();

#ifdef PICASSO96
	//picasso_reset ();
#endif
#ifdef SCSIEMU
	scsi_reset ();
	scsidev_reset ();
	scsidev_start_threads ();
#endif
#ifdef A2065
	a2065_reset ();
#endif
#ifdef SANA2
	netdev_reset ();
	netdev_start_threads ();
#endif
#ifdef FILESYS
	filesys_prepare_reset ();
	filesys_reset ();
#endif
	memory_reset ();
#if defined (BSDSOCKET)
	bsdlib_reset ();
#endif
#ifdef FILESYS
	filesys_start_threads ();
	hardfile_reset ();
#endif
#ifdef UAESERIAL
	uaeserialdev_reset ();
	uaeserialdev_start_threads ();
#endif
#if defined (PARALLEL_PORT)
	initparallel ();
#endif
	native2amiga_reset ();
	dongle_reset ();
	//sampler_init ();
}

/* Okay, this stuff looks strange, but it is here to encourage people who
* port UAE to re-use as much of this code as possible. Functions that you
* should be using are do_start_program () and do_leave_program (), as well
* as real_main (). Some OSes don't call main () (which is braindamaged IMHO,
* but unfortunately very common), so you need to call real_main () from
* whatever entry point you have. You may want to write your own versions
* of start_program () and leave_program () if you need to do anything special.
* Add #ifdefs around these as appropriate.
 */
extern unsigned int pause_uae;
void do_start_program (void)
{
	if (quit_program == -1)
		return;
#ifdef JIT
	if (!canbang && candirect < 0)
		candirect = 0;
	if (canbang && candirect < 0)
		candirect = 1;
#endif
	/* Do a reset on startup. Whether this is elegant is debatable. */
	inputdevice_updateconfig (&currprefs);
	if (quit_program >= 0)
		quit_program = 2;
	m68k_go (1);
}

void do_leave_program (void)
{
	//sampler_free ();
	graphics_leave ();
	inputdevice_close ();
	DISK_free ();
	close_sound ();
	dump_counts ();
#ifdef SERIAL_PORT
	serial_exit ();
#endif
#ifdef CDTV
	cdtv_free ();
#endif
#ifdef A2091
	a2091_free ();
#endif
#ifdef NCR
	ncr_free ();
#endif
#ifdef CD32
	akiko_free ();
#endif
	if (! no_gui)
		gui_exit ();
#ifdef USE_SDL
	SDL_Quit ();
#endif
#ifdef AUTOCONFIG
	expansion_cleanup ();
#endif
#ifdef FILESYS
	filesys_cleanup ();
#endif
#ifdef BSDSOCKET
	bsdlib_reset ();
#endif
//	device_func_reset (); //blkdev
	savestate_free ();
	memory_cleanup ();
	cfgfile_addcfgparam (0);
	machdep_free ();
}

void start_program (void)
{
	do_start_program ();
}

void leave_program (void)
{
	do_leave_program ();
}

#if (defined (_WIN32) || defined (_WIN64)) && !defined (NO_WIN32_EXCEPTION_HANDLER)
#ifndef JIT
extern int DummyException (LPEXCEPTION_POINTERS blah, int n_except)
{
	return EXCEPTION_CONTINUE_SEARCH;
}
#endif
#endif

static int real_main2 (int argc, char **argv)
{
#if (defined (_WIN32) || defined (_WIN64)) && !defined (NO_WIN32_EXCEPTION_HANDLER)
	extern int EvalException (LPEXCEPTION_POINTERS blah, int n_except);
	__try
#endif
	{

#ifdef USE_SDL
    int result = (SDL_Init (SDL_INIT_TIMER | /*SDL_INIT_AUDIO |*/ SDL_INIT_JOYSTICK | SDL_INIT_NOPARACHUTE) == 0);
    if (result)
		atexit (SDL_Quit);
#endif
		config_changed = 1;
		if (restart_config[0]) {
			default_prefs (&currprefs, 0);
			fixup_prefs (&currprefs);
	    }

		if (!graphics_setup ()) {
			write_log ("Graphics Setup Failed\n");
			exit (1);
		}

#ifdef NATMEM_OFFSET
		preinit_shm ();
#endif

	    if (restart_config[0])
			parse_cmdline_and_init_file (argc, argv);
    	else
			currprefs = changed_prefs;

	    uae_inithrtimer ();

		if (!machdep_init ()) {
			write_log ("Machine Init Failed.\n");
			restart_program = 0;
			return -1;
		}

		if (console_emulation) {
			consolehook_config (&currprefs);
			fixup_prefs (&currprefs);
		}

		if (! audio_setup ()) {
			write_log ("Sound driver unavailable: Sound output disabled\n");
			currprefs.produce_sound = 0;
	    }
	    inputdevice_init ();

		changed_prefs = currprefs;
		no_gui = ! currprefs.start_gui;
		if (restart_program == 2)
			no_gui = 1;
		else if (restart_program == 3)
			no_gui = 0;
		restart_program = 0;
		if (! no_gui) {
			int err = gui_init ();
			currprefs = changed_prefs;
			config_changed = 1;
			if (err == -1) {
				write_log ("Failed to initialize the GUI\n");
				return -1;
			} else if (err == -2) {
			    return 1;
			}
		}

#ifdef NATMEM_OFFSET
		init_shm ();
#endif

#ifdef JIT
		if (!(currprefs.cpu_model >= 68020 && currprefs.address_space_24 == 0 && currprefs.cachesize))
			canbang = 0;
#endif

		fixup_prefs (&currprefs);
		changed_prefs = currprefs;
		target_run ();
		/* force sound settings change */
		currprefs.produce_sound = 0;

#ifdef AUTOCONFIG
    	/* Install resident module to get 8MB chipmem, if requested */
	    rtarea_setup ();
#endif
#ifdef FILESYS
		rtarea_init ();
		uaeres_install ();
		hardfile_install ();
#endif
		savestate_init ();
#ifdef SCSIEMU
		scsi_reset ();
		scsidev_install ();
#endif
#ifdef SANA2
		netdev_install ();
#endif
#ifdef UAESERIAL
		uaeserialdev_install ();
#endif
    	keybuf_init (); /* Must come after init_joystick */

#ifdef AUTOCONFIG
    	expansion_init ();
#endif
#ifdef FILESYS
    	filesys_install ();
#endif
		memory_init ();
		memory_reset ();

#ifdef AUTOCONFIG
#if defined (BSDSOCKET)
    	bsdlib_install ();
#endif
    	emulib_install ();
    	uaeexe_install ();
    	native2amiga_install ();
#endif

		custom_init (); /* Must come after memory_init */
#ifdef SERIAL_PORT
		serial_init ();
#endif
		DISK_init ();

		reset_frame_rate_hack ();
		init_m68k(); /* must come after reset_frame_rate_hack (); */

		gui_update ();

		if (graphics_init ()) {
#ifdef DEBUGGER
		    setup_brkhandler ();
		    if (currprefs.start_debugger && debuggable ())
				activate_debugger ();
#endif

		    if (! init_audio ()) {
				if (sound_available && currprefs.produce_sound > 1) { 
					write_log ("Sound driver unavailable: Sound output disabled\n");
				}
				currprefs.produce_sound = 0;
		    }

			start_program ();
		}

    }
#if (defined (_WIN32) || defined (_WIN64)) && !defined (NO_WIN32_EXCEPTION_HANDLER)
#ifdef JIT
    __except( EvalException( GetExceptionInformation(), GetExceptionCode() ) )
#else
	__except (DummyException (GetExceptionInformation (), GetExceptionCode ()))
#endif
	{
		// EvalException does the good stuff...
	}
#endif
	return 0;
}

void real_main (int argc, char **argv)
{
	restart_program = 1;

	fetch_configurationpath (restart_config, sizeof (restart_config) / sizeof (char));
	_tcscat (restart_config, OPTIONSFILENAME);
	default_config = 1;

	while (restart_program) {
		int ret;
		changed_prefs = currprefs;
		ret = real_main2 (argc, argv);
		if (ret == 0 && quit_to_gui)
			restart_program = 1;
		leave_program ();
		quit_program = 0;
	}
	zfile_exit ();
}

#ifndef NO_MAIN_IN_MAIN_C
int main (int argc, char **argv)
{
    init_sdl ();
    gui_init ();
	show_version_full ();
    real_main (argc, argv);
    return 0;
}
#endif

#ifdef SINGLEFILE
uae_u8 singlefile_config[50000] = { "_CONFIG_STARTS_HERE" };
uae_u8 singlefile_data[1500000] = { "_DATA_STARTS_HERE" };
#endif
