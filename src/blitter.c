/*
* UAE - The Un*x Amiga Emulator
*
* Custom chip emulation
*
* (c) 1995 Bernd Schmidt, Alessandro Bissacco
* (c) 2002 - 2005 Toni Wilen
*/

//#define BLITTER_DEBUG_NOWAIT
//#define BLITTER_DEBUG
//#define BLITTER_DEBUG_NO_D
//#define BLITTER_INSTANT

#define SPEEDUP

#include "sysconfig.h"
#include "sysdeps.h"

#include "options.h"
#include "uae.h"
#include "memory.h"
#include "custom.h"
#include "events.h"
#include "newcpu.h"
#include "blitter.h"
#include "blit.h"
#include "savestate.h"
#include "debug.h"

/* we must not change ce-mode while blitter is running.. */
static int blitter_cycle_exact;

uae_u16 bltcon0, bltcon1;
uae_u32 bltapt, bltbpt, bltcpt, bltdpt;
int blitter_nasty;

static int original_ch, original_fill, original_line;

static unsigned int blinea_shift;
static uae_u16 blinea, blineb;
static unsigned int blitline, blitfc, blitfill, blitife, blitsing, blitdesc;
static unsigned int blitonedot, blitsign, blitlinepixel;
static int blit_add;
static int blit_modadda, blit_modaddb, blit_modaddc, blit_modaddd;
static unsigned int blit_ch;
static int vblitsize, hblitsize;

#ifdef BLITTER_DEBUG
static int blitter_dontdo;
static int blitter_delayed_debug;
#endif
#ifdef BLITTER_SLOWDOWNDEBUG
static int blitter_slowdowndebug;
#endif

struct bltinfo blt_info;

static uae_u8 blit_filltable[256][4][2];
uae_u32 blit_masktable[BLITTER_MAX_WORDS];
enum blitter_states bltstate;

static unsigned int blit_cyclecounter, blit_waitcyclecounter;
static unsigned int blit_maxcyclecounter, blit_totalcyclecounter;
static unsigned int blit_startcycles, blit_misscyclecounter;
static int blit_slowdown;

#ifdef CPUEMU_12
extern uae_u8 cycle_line[256];
#endif

static unsigned long blit_firstline_cycles;
static unsigned long blit_first_cycle;
static unsigned int blit_last_cycle, blit_dmacount, blit_dmacount2;
static int blit_linecycles, blit_extracycles;
static const int *blit_diag;
static int blit_frozen, blit_faulty;
static int blit_final;
static int blt_delayed_irq;
static uae_u16 ddat1, ddat2;
static int ddat1use, ddat2use;

static unsigned int blit_nod;

int blit_interrupt;

static int last_blitter_hpos;

#define BLITTER_STARTUP_CYCLES 2

/*
Blitter Idle Cycle:

Cycles that are free cycles (available for CPU) and
are not used by any other Agnus DMA channel. Blitter
idle cycle is not "used" by blitter, CPU can still use
it normally if it needs the bus.

same in both block and line modes

number of cycles, initial cycle, main cycle
*/

static const int blit_cycle_diagram[][10] =
{
	{ 2, 0,0,	    0,0 },		/* 0 */
	{ 2, 0,0,	    0,4 },		/* 1 */
	{ 2, 0,3,	    0,3 },		/* 2 */
	{ 3, 0,3,0,	    0,3,4 },    /* 3 */
	{ 3, 0,2,0,	    0,2,0 },    /* 4 */
	{ 3, 0,2,0,	    0,2,4 },    /* 5 */
	{ 3, 0,2,3,	    0,2,3 },    /* 6 */
	{ 4, 0,2,3,0,   0,2,3,4 },  /* 7 */
	{ 2, 1,0,	    1,0 },		/* 8 */
	{ 2, 1,0,	    1,4 },		/* 9 */
	{ 2, 1,3,	    1,3 },		/* A */
	{ 3, 1,3,0,	    1,3,4, },	/* B */
	{ 3, 1,2,0,	    1,2,0 },	/* C */
	{ 3, 1,2,0,	    1,2,4 },	/* D */
	{ 3, 1,2,3,	    1,2,3 },	/* E */
	{ 4, 1,2,3,0,   1,2,3,4 }	/* F */
};

/*

following 4 channel combinations in fill mode have extra
idle cycle added (still requires free bus cycle)

*/

static const uae_s8 blit_cycle_diagram_fill[][10] =
{
	{ 0 },						/* 0 */
	{ 3, 0,0,0,	    0,4,0 },	/* 1 */
	{ 0 },						/* 2 */
	{ 0 },						/* 3 */
	{ 0 },						/* 4 */
	{ 4, 0,2,0,0,   0,2,4,0 },	/* 5 */
	{ 0 },						/* 6 */
	{ 0 },						/* 7 */
	{ 0 },						/* 8 */
	{ 3, 1,0,0,	    1,4,0 },	/* 9 */
	{ 0 },						/* A */
	{ 0 },						/* B */
	{ 0 },						/* C */
	{ 4, 1,2,0,0,   1,2,4,0 },	/* D */
	{ 0 },						/* E */
	{ 0 },						/* F */
};

/*
-C-D C-D- ... C-D- --

line draw takes 4 cycles (-C-D)
idle cycles do the same as above, 2 dma fetches
(read from C, write to D, but see below)

Oddities:

- first word is written to address pointed by BLTDPT
but all following writes go to address pointed by BLTCPT!
(some kind of internal copy because all bus cyles are
using normal BLTDDAT)
- BLTDMOD is ignored by blitter (BLTCMOD is used)
- state of D-channel enable bit does not matter!
- disabling A-channel freezes the content of BPLAPT
- C-channel disabled: nothing is written

There is one tricky situation, writing to DFF058 just before
last D write cycle (which is normally free) does not disturb
blitter operation, final D is still written correctly before
blitter starts normally (after 2 idle cycles)

There is at least one demo that does this..

*/

// 5 = internal "processing cycle"
static const int blit_cycle_diagram_line[] =
{
	4, 0,3,5,4,	    0,3,5,4
};

static const int blit_cycle_diagram_finald[] =
{
	2, 0,4,	    0,4
};

static const int blit_cycle_diagram_finalld[] =
{
	2, 0,0,	    0,0
};


void build_blitfilltable (void)
{
	unsigned int d, fillmask;
	unsigned int i;

	for (i = 0; i < BLITTER_MAX_WORDS; i++)
		blit_masktable[i] = 0xFFFF;

	for (d = 0; d < 256; d++) {
		for (i = 0; i < 4; i++) {
			unsigned int fc = i & 1;
			uae_u8 data = d;
			for (fillmask = 1; fillmask != 0x100; fillmask <<= 1) {
				uae_u16 tmp = data;
				if (fc) {
					if (i & 2)
						data |= fillmask;
					else
						data ^= fillmask;
				}
				if (tmp & fillmask) fc = !fc;
			}
			blit_filltable[d][i][0] = data;
			blit_filltable[d][i][1] = fc;
		}
	}
}

STATIC_INLINE void record_dma_blit (uae_u16 reg, uae_u16 dat, uae_u32 addr, unsigned int hpos)
{
#ifdef DEBUGGER
	int type;

	if (blitline)
		type = DMARECORD_BLITTER_LINE;
	else
		type = DMARECORD_BLITTER;
	if (debug_dma)
		record_dma (reg, dat, addr, hpos, vpos, type);
#endif
}

static void blitter_dump (void)
{
	write_log ("PT A=%08X B=%08X C=%08X D=%08X\n", bltapt, bltbpt, bltcpt, bltdpt);
	write_log ("CON0=%04X CON1=%04X DAT A=%04X B=%04X C=%04X\n",
		bltcon0, bltcon1, blt_info.bltadat, blt_info.bltbdat, blt_info.bltcdat);
	write_log ("AFWM=%04X ALWM=%04X MOD A=%04X B=%04X C=%04X D=%04X\n",
		blt_info.bltafwm, blt_info.bltalwm,
		blt_info.bltamod & 0xffff, blt_info.bltbmod & 0xffff, blt_info.bltcmod & 0xffff, blt_info.bltdmod & 0xffff);
}

STATIC_INLINE const int *get_ch (void)
{
	if (blit_faulty)
		return &blit_diag[0];
	if (blit_final)
		return blitline ? blit_cycle_diagram_finalld : blit_cycle_diagram_finald;
	return blit_diag;
}

STATIC_INLINE int channel_state (unsigned int cycles)
{
	const int *diag;
	if ((int)cycles < 0)
		return 0;
	diag = get_ch ();
	if ((int)cycles < diag[0])
		return diag[1 + cycles];
	cycles -= diag[0];
	cycles %= diag[0];
	return diag[1 + diag[0] + cycles];
}
STATIC_INLINE int channel_pos (unsigned int cycles)
{
	const int *diag;
	if ((int)cycles < 0)
		return 0;
	diag =  get_ch ();
	if ((int)cycles < diag[0])
		return cycles;
	cycles -= diag[0];
	cycles %= diag[0];
	return cycles;
}

int blitter_channel_state (void)
{
	return channel_state (blit_cyclecounter);
}

extern int is_bitplane_dma (unsigned int hpos);
STATIC_INLINE int canblit (unsigned int hpos)
{
	if (is_bitplane_dma (hpos))
		return 0;
	if (cycle_line[hpos])
		return 0;
	return 1;
}

// blitter interrupt is set when last "main" cycle
// has been finished, any non-linedraw D-channel blit
// still needs 2 more cycles before final D is written
static void blitter_interrupt (unsigned int hpos, int done)
{
	if (blit_interrupt)
		return;
	if (!done && (!currprefs.blitter_cycle_exact || (currprefs.chipset_mask & CSMASK_AGA)))
		return;
	blit_interrupt = 1;
	send_interrupt (6, 3);
	if (debug_dma)
		record_dma_event (DMA_EVENT_BLITIRQ, hpos, vpos);
}

static void blitter_done (unsigned int hpos)
{
	ddat1use = ddat2use = 0;
	bltstate = blit_startcycles == 0 || !currprefs.blitter_cycle_exact ? BLT_done : BLT_init;
	blitter_interrupt (hpos, 1);
	blitter_done_notify (hpos);
	if (debug_dma)
		record_dma_event (DMA_EVENT_BLITFINISHED, hpos, vpos);
	event2_remevent (ev2_blitter);
	unset_special (&regs, SPCFLAG_BLTNASTY);
#ifdef BLITTER_DEBUG
	write_log ("cycles %d, missed %d, total %d\n",
		blit_totalcyclecounter, blit_misscyclecounter, blit_totalcyclecounter + blit_misscyclecounter);
#endif
}

STATIC_INLINE void chipmem_agnus_wput2 (uaecptr addr, uae_u32 w)
{
	last_custom_value1 = w;
#ifndef BLITTER_DEBUG_NO_D
	chipmem_agnus_wput (addr, w);
#endif
}

static void blitter_dofast (void)
{
	int i,j;
	uaecptr bltadatptr = 0, bltbdatptr = 0, bltcdatptr = 0, bltddatptr = 0;
	uae_u8 mt = bltcon0 & 0xFF;

	blit_masktable[0] = blt_info.bltafwm;
	blit_masktable[hblitsize - 1] &= blt_info.bltalwm;

	if (bltcon0 & 0x800) {
		bltadatptr = bltapt;
		bltapt += (hblitsize * 2 + blt_info.bltamod) * vblitsize;
	}
	if (bltcon0 & 0x400) {
		bltbdatptr = bltbpt;
		bltbpt += (hblitsize * 2 + blt_info.bltbmod) * vblitsize;
	}
	if (bltcon0 & 0x200) {
		bltcdatptr = bltcpt;
		bltcpt += (hblitsize * 2 + blt_info.bltcmod) * vblitsize;
	}
	if (bltcon0 & 0x100) {
		bltddatptr = bltdpt;
		bltdpt += (hblitsize * 2 + blt_info.bltdmod) * vblitsize;
	}

#ifdef SPEEDUP
	if (blitfunc_dofast[mt] && !blitfill) {
		(*blitfunc_dofast[mt])(bltadatptr, bltbdatptr, bltcdatptr, bltddatptr, &blt_info);
	} else
#endif
	{
		uae_u32 blitbhold = blt_info.bltbhold;
		uae_u32 preva = 0, prevb = 0;
		uaecptr dstp = 0;
		int dodst = 0;

		for (j = 0; j < vblitsize; j++) {
			blitfc = !!(bltcon1 & 0x4);
			for (i = 0; i < hblitsize; i++) {
				uae_u32 bltadat, blitahold;
				uae_u16 bltbdat;
				if (bltadatptr) {
					blt_info.bltadat = bltadat = chipmem_agnus_wget (bltadatptr);
					bltadatptr += 2;
				} else
					bltadat = blt_info.bltadat;
				bltadat &= blit_masktable[i];
				blitahold = (((uae_u32)preva << 16) | bltadat) >> blt_info.blitashift;
				preva = bltadat;

				if (bltbdatptr) {
					blt_info.bltbdat = bltbdat = chipmem_agnus_wget (bltbdatptr);
					bltbdatptr += 2;
					blitbhold = (((uae_u32)prevb << 16) | bltbdat) >> blt_info.blitbshift;
					prevb = bltbdat;
				}

				if (bltcdatptr) {
					blt_info.bltcdat = chipmem_agnus_wget (bltcdatptr);
					bltcdatptr += 2;
				}
				if (dodst)
					chipmem_agnus_wput2 (dstp, blt_info.bltddat);
				blt_info.bltddat = blit_func (blitahold, blitbhold, blt_info.bltcdat, mt) & 0xFFFF;
				if (blitfill) {
					uae_u16 d = blt_info.bltddat;
					int ifemode = blitife ? 2 : 0;
					int fc1 = blit_filltable[d & 255][ifemode + blitfc][1];
					blt_info.bltddat = (blit_filltable[d & 255][ifemode + blitfc][0]
						+ (blit_filltable[d >> 8][ifemode + fc1][0] << 8));
					blitfc = blit_filltable[d >> 8][ifemode + fc1][1];
				}
				if (blt_info.bltddat)
					blt_info.blitzero = 0;
				if (bltddatptr) {
					dodst = 1;
					dstp = bltddatptr;
					bltddatptr += 2;
				}
			}
			if (bltadatptr)
				bltadatptr += blt_info.bltamod;
			if (bltbdatptr)
				bltbdatptr += blt_info.bltbmod;
			if (bltcdatptr)
				bltcdatptr += blt_info.bltcmod;
			if (bltddatptr)
				bltddatptr += blt_info.bltdmod;
		}
		if (dodst)
			chipmem_agnus_wput2 (dstp, blt_info.bltddat);
		blt_info.bltbhold = blitbhold;
	}
	blit_masktable[0] = 0xFFFF;
	blit_masktable[hblitsize - 1] = 0xFFFF;

	bltstate = BLT_done;
}

static void blitter_dofast_desc (void)
{
	int i,j;
	uaecptr bltadatptr = 0, bltbdatptr = 0, bltcdatptr = 0, bltddatptr = 0;
	uae_u8 mt = bltcon0 & 0xFF;

	blit_masktable[0] = blt_info.bltafwm;
	blit_masktable[hblitsize - 1] &= blt_info.bltalwm;

	if (bltcon0 & 0x800) {
		bltadatptr = bltapt;
		bltapt -= (hblitsize*2 + blt_info.bltamod)*vblitsize;
	}
	if (bltcon0 & 0x400) {
		bltbdatptr = bltbpt;
		bltbpt -= (hblitsize*2 + blt_info.bltbmod)*vblitsize;
	}
	if (bltcon0 & 0x200) {
		bltcdatptr = bltcpt;
		bltcpt -= (hblitsize*2 + blt_info.bltcmod)*vblitsize;
	}
	if (bltcon0 & 0x100) {
		bltddatptr = bltdpt;
		bltdpt -= (hblitsize*2 + blt_info.bltdmod)*vblitsize;
	}
#ifdef SPEEDUP
	if (blitfunc_dofast_desc[mt] && !blitfill) {
		(*blitfunc_dofast_desc[mt])(bltadatptr, bltbdatptr, bltcdatptr, bltddatptr, &blt_info);
	} else
#endif
	{
		uae_u32 blitbhold = blt_info.bltbhold;
		uae_u32 preva = 0, prevb = 0;
		uaecptr dstp = 0;
		int dodst = 0;

		for (j = 0; j < vblitsize; j++) {
			blitfc = !!(bltcon1 & 0x4);
			for (i = 0; i < hblitsize; i++) {
				uae_u32 bltadat, blitahold;
				uae_u16 bltbdat;
				if (bltadatptr) {
					bltadat = blt_info.bltadat = chipmem_agnus_wget (bltadatptr);
					bltadatptr -= 2;
				} else
					bltadat = blt_info.bltadat;
				bltadat &= blit_masktable[i];
				blitahold = (((uae_u32)bltadat << 16) | preva) >> blt_info.blitdownashift;
				preva = bltadat;

				if (bltbdatptr) {
					blt_info.bltbdat = bltbdat = chipmem_agnus_wget (bltbdatptr);
					bltbdatptr -= 2;
					blitbhold = (((uae_u32)bltbdat << 16) | prevb) >> blt_info.blitdownbshift;
					prevb = bltbdat;
				}

				if (bltcdatptr) {
					blt_info.bltcdat = blt_info.bltbdat = chipmem_agnus_wget (bltcdatptr);
					bltcdatptr -= 2;
				}
				if (dodst)
					chipmem_agnus_wput2 (dstp, blt_info.bltddat);
				blt_info.bltddat = blit_func (blitahold, blitbhold, blt_info.bltcdat, mt) & 0xFFFF;
				if (blitfill) {
					uae_u16 d = blt_info.bltddat;
					int ifemode = blitife ? 2 : 0;
					int fc1 = blit_filltable[d & 255][ifemode + blitfc][1];
					blt_info.bltddat = (blit_filltable[d & 255][ifemode + blitfc][0]
					+ (blit_filltable[d >> 8][ifemode + fc1][0] << 8));
					blitfc = blit_filltable[d >> 8][ifemode + fc1][1];
				}
				if (blt_info.bltddat)
					blt_info.blitzero = 0;
				if (bltddatptr) {
					dstp = bltddatptr;
					dodst = 1;
					bltddatptr -= 2;
				}
			}
			if (bltadatptr)
				bltadatptr -= blt_info.bltamod;
			if (bltbdatptr)
				bltbdatptr -= blt_info.bltbmod;
			if (bltcdatptr)
				bltcdatptr -= blt_info.bltcmod;
			if (bltddatptr)
				bltddatptr -= blt_info.bltdmod;
		}
		if (dodst)
			chipmem_agnus_wput2 (dstp, blt_info.bltddat);
		blt_info.bltbhold = blitbhold;
	}
	blit_masktable[0] = 0xFFFF;
	blit_masktable[hblitsize - 1] = 0xFFFF;

	bltstate = BLT_done;
}

STATIC_INLINE void blitter_read (void)
{
	if (bltcon0 & 0x200) {
		if (!dmaen (DMA_BLITTER))
			return;
		blt_info.bltcdat = chipmem_bank.wget (bltcpt);
		last_custom_value1 = blt_info.bltcdat;
	}
	bltstate = BLT_work;
}

STATIC_INLINE void blitter_write (void)
{
	if (blt_info.bltddat)
		blt_info.blitzero = 0;
	/* D-channel state has no effect on linedraw, but C must be enabled or nothing is drawn! */
	if (bltcon0 & 0x200) {
		if (!dmaen (DMA_BLITTER))
			return;
		last_custom_value1 = blt_info.bltddat;
		chipmem_bank.wput (bltdpt, blt_info.bltddat);
	}
	bltstate = BLT_next;
}

STATIC_INLINE void blitter_line_incx (void)
{
	if (++blinea_shift == 16) {
		blinea_shift = 0;
		bltcpt += 2;
	}
}

STATIC_INLINE void blitter_line_decx (void)
{
	if (blinea_shift-- == 0) {
		blinea_shift = 15;
		bltcpt -= 2;
	}
}

STATIC_INLINE void blitter_line_decy (void)
{
	bltcpt -= blt_info.bltcmod;
	blitonedot = 0;
}

STATIC_INLINE void blitter_line_incy (void)
{
	bltcpt += blt_info.bltcmod;
	blitonedot = 0;
}

static void blitter_line (void)
{
	uae_u16 blitahold = (blinea & blt_info.bltafwm) >> blinea_shift;
	uae_u16 blitbhold = blineb & 1 ? 0xFFFF : 0;
	uae_u16 blitchold = blt_info.bltcdat;

	blitlinepixel = !blitsing || (blitsing && !blitonedot);
	blt_info.bltddat = blit_func (blitahold, blitbhold, blitchold, bltcon0 & 0xFF);
	blitonedot++;
}

static void blitter_line_proc (void)
{
	if (bltcon0 & 0x800) {
		if (blitsign)
			bltapt += (uae_s16)blt_info.bltbmod;
		else
			bltapt += (uae_s16)blt_info.bltamod;
	}

	if (!blitsign) {
		if (bltcon1 & 0x10) {
			if (bltcon1 & 0x8)
				blitter_line_decy ();
			else
				blitter_line_incy ();
		} else {
			if (bltcon1 & 0x8)
				blitter_line_decx ();
			else
				blitter_line_incx ();
		}
	}
	if (bltcon1 & 0x10) {
		if (bltcon1 & 0x4)
			blitter_line_decx ();
		else
			blitter_line_incx ();
	} else {
		if (bltcon1 & 0x4)
			blitter_line_decy ();
		else
			blitter_line_incy ();
	}

	blitsign = 0 > (uae_s16)bltapt;
	bltstate = BLT_write;
}

STATIC_INLINE void blitter_nxline (void)
{
	blineb = (blineb << 1) | (blineb >> 15);
	vblitsize--;
	bltstate = BLT_read;
}

#ifdef CPUEMU_12

static unsigned int blit_last_hpos;

static unsigned int blitter_cyclecounter;
static int blitter_hcounter1, blitter_hcounter2;
static int blitter_vcounter1, blitter_vcounter2;

static void decide_blitter_line (unsigned int hsync, unsigned int hpos)
{
	if (blit_final && vblitsize)
		blit_final = 0;
	while (last_blitter_hpos < (int)hpos) {
		int c = channel_state (blit_cyclecounter);

		for (;;) {
			int v = canblit (last_blitter_hpos);

			if (blit_waitcyclecounter) {
				blit_waitcyclecounter = 0;
				break;
			}

			if (!v) {
				blit_misscyclecounter++;
				blitter_nasty++;
				break;
			}

			if ((!dmaen (DMA_BLITTER) || v <= 0) && (c == 3 || c == 4)) {
				blit_misscyclecounter++;
				blitter_nasty++;
				break;
			}

			blit_cyclecounter++;
			blit_totalcyclecounter++;

			// final 2 idle cycles?
			if (blit_final) {
				if (blit_cyclecounter > get_ch ()[0]) {
					blitter_done (last_blitter_hpos);
					return;
				}
				break;
			}

			if (c == 3) {

				blitter_read ();
				alloc_cycle_ext (last_blitter_hpos, CYCLE_BLITTER);
				record_dma_blit (0x70, blt_info.bltcdat, bltcpt, last_blitter_hpos);

			} else if (c == 5) {

				if (ddat1use) {
					bltdpt = bltcpt;
				}
				ddat1use = 1;
				blitter_line ();
				blitter_line_proc ();
				blitter_nxline ();

			} else if (c == 4) {

				/* onedot mode and no pixel = bus write access is skipped */
				if (blitlinepixel) {
					blitter_write ();
					alloc_cycle_ext (last_blitter_hpos, CYCLE_BLITTER);
					record_dma_blit (0x00, blt_info.bltddat, bltdpt, last_blitter_hpos);
					blitlinepixel = 0;
				}
				if (vblitsize == 0) {
					bltdpt = bltcpt;
					blit_final = 1;
					blit_cyclecounter = 0;
					blit_waitcyclecounter = 0;
					break;
				}

			}

			break;
		}
		last_blitter_hpos++;
	}
	if (hsync)
		last_blitter_hpos = 0;
}

#endif

static void actually_do_blit (void)
{
	if (blitline) {
		do {
			blitter_read ();
			if (ddat1use)
				bltdpt = bltcpt;
			ddat1use = 1;
			blitter_line ();
			blitter_line_proc ();
			blitter_nxline ();
			if (blitlinepixel) {
				blitter_write ();
				blitlinepixel = 0;
			}
			if (vblitsize == 0)
				bltstate = BLT_done;
		} while (bltstate != BLT_done);
		bltdpt = bltcpt;
	} else {
		if (blitdesc)
			blitter_dofast_desc ();
		else
			blitter_dofast ();
		bltstate = BLT_done;
	}
}

void blitter_handler (uae_u32 data)
{
	static int blitter_stuck;

	if (!dmaen (DMA_BLITTER)) {
		event2_newevent (ev2_blitter, 10, 0);
		blitter_stuck++;
		if (blitter_stuck < 20000 || !currprefs.immediate_blits)
			return; /* gotta come back later. */
		/* "free" blitter in immediate mode if it has been "stuck" ~3 frames
		* fixes some JIT game incompatibilities
		*/
		write_log ("Blitter force-unstuck!\n");
	}
	blitter_stuck = 0;
	if (blit_slowdown > 0 && !currprefs.immediate_blits) {
		event2_newevent (ev2_blitter, blit_slowdown, 0);
		blit_slowdown = -1;
		return;
	}
#ifdef BLITTER_DEBUG
	if (!blitter_dontdo)
		actually_do_blit ();
	else
		bltstate = BLT_done;
#else
	actually_do_blit ();
#endif
	blitter_done (current_hpos ());
}

#ifdef CPUEMU_12

static uae_u32 preva, prevb;
STATIC_INLINE uae_u16 blitter_doblit (void)
{
	uae_u32 blitahold;
	uae_u16 bltadat, ddat;
	uae_u8 mt = bltcon0 & 0xFF;

	bltadat = blt_info.bltadat;
	if (blitter_hcounter1 == 0)
		bltadat &= blt_info.bltafwm;
	if (blitter_hcounter1 == hblitsize - 1)
		bltadat &= blt_info.bltalwm;
	if (blitdesc)
		blitahold = (((uae_u32)bltadat << 16) | preva) >> blt_info.blitdownashift;
	else
		blitahold = (((uae_u32)preva << 16) | bltadat) >> blt_info.blitashift;
	preva = bltadat;

	ddat = blit_func (blitahold, blt_info.bltbhold, blt_info.bltcdat, mt) & 0xFFFF;

	if (bltcon1 & 0x18) {
		uae_u16 d = ddat;
		int ifemode = blitife ? 2 : 0;
		int fc1 = blit_filltable[d & 255][ifemode + blitfc][1];
		ddat = (blit_filltable[d & 255][ifemode + blitfc][0]
			+ (blit_filltable[d >> 8][ifemode + fc1][0] << 8));
		blitfc = blit_filltable[d >> 8][ifemode + fc1][1];
	}

	if (ddat)
		blt_info.blitzero = 0;

	return ddat;
}


STATIC_INLINE void blitter_doddma (unsigned int hpos)
{
	int wd;
	uae_u16 d;

	wd = 0;
	if (blit_dmacount2 == 0) {
		d =  blitter_doblit ();
		wd = -1;
	} else if (ddat2use) {
		d = ddat2;
		ddat2use = 0;
		wd = 2;
	} else if (ddat1use) {
		d = ddat1;
		ddat1use = 0;
		wd = 1;
	} else {
		write_log ("BLITTER: D-channel without nothing to do?\n");
		return;
	}
	alloc_cycle_ext (hpos, CYCLE_BLITTER);
	record_dma_blit (0x00, d, bltdpt, hpos);
	last_custom_value1 = d;
	chipmem_agnus_wput2 (bltdpt, d);
	bltdpt += blit_add;
	blitter_hcounter2++;
	if (blitter_hcounter2 == hblitsize) {
		blitter_hcounter2 = 0;
		bltdpt += blit_modaddd;
		blitter_vcounter2++;
		if (blitter_vcounter2 > blitter_vcounter1)
			blitter_vcounter1 = blitter_vcounter2;
	}
	if (blit_ch == 1)
		blitter_hcounter1 = blitter_hcounter2;
}

STATIC_INLINE void blitter_dodma (unsigned int ch, unsigned int hpos)
{
	uae_u16 dat, reg;
	uae_u32 addr;

	switch (ch)
	{
	case 1:
		blt_info.bltadat = dat = chipmem_agnus_wget (bltapt);
		last_custom_value1 = blt_info.bltadat;
		addr = bltapt;
		bltapt += blit_add;
		reg = 0x74;
		break;
	case 2:
		blt_info.bltbdat = dat = chipmem_agnus_wget (bltbpt);
		last_custom_value1 = blt_info.bltbdat;
		addr = bltbpt;
		bltbpt += blit_add;
		if (blitdesc)
			blt_info.bltbhold = (((uae_u32)blt_info.bltbdat << 16) | prevb) >> blt_info.blitdownbshift;
		else
			blt_info.bltbhold = (((uae_u32)prevb << 16) | blt_info.bltbdat) >> blt_info.blitbshift;
		prevb = blt_info.bltbdat;
		reg = 0x72;
		break;
	case 3:
		blt_info.bltcdat = dat = chipmem_agnus_wget (bltcpt);
		last_custom_value1 = blt_info.bltcdat;
		addr = bltcpt;
		bltcpt += blit_add;
		reg = 0x70;
		break;
	default:
		abort ();
	}

	blitter_cyclecounter++;
	if (blitter_cyclecounter >= blit_dmacount2) {
		blitter_cyclecounter = 0;
		ddat2 = ddat1;
		ddat2use = ddat1use;
		ddat1use = 0;
		ddat1 = blitter_doblit ();
		if (bltcon0 & 0x100)
			ddat1use = 1;
		blitter_hcounter1++;
		if (blitter_hcounter1 == hblitsize) {
			blitter_hcounter1 = 0;
			if (bltcon0 & 0x800)
				bltapt += blit_modadda;
			if (bltcon0 & 0x400)
				bltbpt += blit_modaddb;
			if (bltcon0 & 0x200)
				bltcpt += blit_modaddc;
			blitter_vcounter1++;
			blitfc = !!(bltcon1 & 0x4);
		}
	}
	alloc_cycle_ext (hpos, CYCLE_BLITTER);
	record_dma_blit (reg, dat, addr, hpos);
}

int blitter_need (unsigned int hpos)
{
	unsigned int c;
	if (bltstate == BLT_done)
		return 0;
	if (!dmaen (DMA_BLITTER))
		return 0;
	c = channel_state (blit_cyclecounter);
	return c;
}

static void do_startcycles (unsigned int hpos)
{
	unsigned int vhpos = last_blitter_hpos;
	while (vhpos < hpos) {
		unsigned int v = canblit (vhpos);
		vhpos++;
		if (v) {
			blit_startcycles--;
			if (blit_startcycles == 0) {
				if (blit_faulty)
					blit_faulty = -1;
				bltstate = BLT_done;
				blit_final = 0;
				do_blitter (vhpos, 0);
				blit_startcycles = 0;
				blit_cyclecounter = 0;
				blit_waitcyclecounter = 0;
				if (blit_faulty)
					blit_faulty = 1;
				return;
			}
		}
	}
}

void decide_blitter (unsigned int hpos)
{
	int hsync = (int)hpos < 0;

	if (blit_startcycles > 0)
		do_startcycles (hpos);

	if (blt_delayed_irq > 0 && hsync) {
		blt_delayed_irq--;
		if (!blt_delayed_irq)
			send_interrupt (6, 2);
	}

	if (bltstate == BLT_done)
		return;
#ifdef BLITTER_DEBUG
	if (blitter_delayed_debug) {
		blitter_delayed_debug = 0;
		blitter_dump ();
	}
#endif
	if (!blitter_cycle_exact)
		return;

	if ((int)hpos < 0)
		hpos = maxhpos;

	if (blitline) {
		blt_info.got_cycle = 1;
		decide_blitter_line (hsync, hpos);
		return;
	}

	while (last_blitter_hpos < (int)hpos) {
		int c;

		c = channel_state (blit_cyclecounter);

		for (;;) {
			unsigned int v;

			v = canblit (last_blitter_hpos);

			// copper bltsize write needs one cycle (any cycle) delay
			if (blit_waitcyclecounter) {
				blit_waitcyclecounter = 0;
				break;
			}
			// idle cycles require free bus..
			// (CPU can still use this cycle)
			if (c == 0 && v == 0) {
				blitter_nasty++;
				blit_misscyclecounter++;
				break;
			}

			if (blit_frozen) {
				blit_misscyclecounter++;
				break;
			}

			if (c == 0) {
				blt_info.got_cycle = 1;
				blit_cyclecounter++;
				if (blit_cyclecounter == 0)
					blit_final = 0;
				blit_totalcyclecounter++;
				/* check if blit with zero channels has ended  */
				if (blit_ch == 0 && blit_cyclecounter >= blit_maxcyclecounter) {
					blitter_done (last_blitter_hpos);
					return;
				}
				break;
			}

			blitter_nasty++;

			if (!dmaen (DMA_BLITTER) || v <= 0) {
				blit_misscyclecounter++;
				break;
			}

			blt_info.got_cycle = 1;
			if (c == 4) {
				blitter_doddma (last_blitter_hpos);
				blit_cyclecounter++;
				blit_totalcyclecounter++;
			} else {
				if (blitter_vcounter1 < vblitsize) {
					blitter_dodma (c, last_blitter_hpos);
				}
				blit_cyclecounter++;
				blit_totalcyclecounter++;
			}

			if (blitter_vcounter1 >= vblitsize && blitter_vcounter2 >= vblitsize) {
				if (!ddat1use && !ddat2use) {
					blitter_done (last_blitter_hpos);
					return;
				}
			}
			break;
		}

		if (!blit_final && blitter_vcounter1 == vblitsize && channel_pos (blit_cyclecounter - 1) == blit_diag[0] - 1) {
			blitter_interrupt (last_blitter_hpos, 0);
			blit_cyclecounter = 0;
			blit_final = 1;
		}
		last_blitter_hpos++;
	}
	if (hsync)
		last_blitter_hpos = 0;
}
#else
void decide_blitter (unsigned int hpos) { }
#endif

static void blitter_force_finish (void)
{
    uae_u16 odmacon;
    if (bltstate == BLT_done)
	return;
    if (bltstate != BLT_done) {
	 /* blitter is currently running
	  * force finish (no blitter state support yet)
	  */
	odmacon = dmacon;
	dmacon |= DMA_MASTER | DMA_BLITTER;
	write_log ("forcing blitter finish\n");
#ifdef CPUEMU_6
	if (currprefs.blitter_cycle_exact) {
	    int rounds = 10000;
	    while (bltstate != BLT_done && rounds > 0) {
		memset (cycle_line, 0, maxhpos);
		decide_blitter (maxhpos);
		rounds--;
	    }
	    if (rounds == 0)
		write_log ("blitter froze!?\n");
			blit_startcycles = 0;
	} else {
#endif
	    actually_do_blit ();
	}
	blitter_done (current_hpos ());
	dmacon = odmacon;
    }
}

static void blit_bltset (unsigned int con)
{
	int i;
	const int *olddiag = blit_diag;

	if (con & 2) {
		blitdesc = bltcon1 & 2;
		blt_info.blitbshift = bltcon1 >> 12;
		blt_info.blitdownbshift = 16 - blt_info.blitbshift;
	}

	if (con & 1) {
		blt_info.blitashift = bltcon0 >> 12;
		blt_info.blitdownashift = 16 - blt_info.blitashift;
	}

	blit_ch = (bltcon0 & 0x0f00) >> 8;
	blitline = bltcon1 & 1;
	blitfill = !!(bltcon1 & 0x18);

	if (blitline) {
		if (hblitsize != 2)
			write_log ("weird hblitsize in linemode: %d vsize=%d\n",
			hblitsize, vblitsize);
		blit_diag = blit_cycle_diagram_line;
	} else {
		if (con & 2) {
			blitfc = !!(bltcon1 & 0x4);
			blitife = !!(bltcon1 & 0x8);
			if ((bltcon1 & 0x18) == 0x18) {
				write_log ("weird fill mode\n");
				blitife = 0;
			}
		}
		if (blitfill && !blitdesc)
			write_log ("fill without desc\n");
		blit_diag = blitfill &&  blit_cycle_diagram_fill[blit_ch][0] ? blit_cycle_diagram_fill[blit_ch] : blit_cycle_diagram[blit_ch];
	}
	if ((bltcon1 & 0x80) && (currprefs.chipset_mask & CSMASK_ECS_AGNUS))
		write_log ("ECS BLTCON1 DOFF-bit set\n");

	// on the fly switching fillmode from extra cycle to non-extra: blitter freezes
	// non-extra cycle to extra cycle: does not freeze but cycle diagram goes weird,
	// extra free cycle changes to another D write..
	// (Absolute Inebriation vector cube inside semi-filled vector object requires freezing blitter.)
	if (!savestate_state && bltstate != BLT_done) {
		static int freezes = 10;
		int isen = blit_diag >= &blit_cycle_diagram_fill[0][0] && blit_diag <= &blit_cycle_diagram_fill[15][0];
		int iseo = olddiag >= &blit_cycle_diagram_fill[0][0] && olddiag <= &blit_cycle_diagram_fill[15][0];
		if (iseo != isen) {
			if (freezes > 0) {
				write_log ("BLITTER: on the fly %d (%d) -> %d (%d) switch! PC=%08x\n", original_ch, iseo, blit_ch, isen, M68K_GETPC);
				freezes--;
			}
		}
		if (original_fill == isen) {
			blit_frozen = 0; // switched back to original fill mode? unfreeze
		} else if (iseo && !isen) {
			blit_frozen = 1;
			write_log ("BLITTER: frozen! %d (%d) -> %d (%d) %08X\n", original_ch, iseo, blit_ch, isen, M68K_GETPC);
		} else if (!iseo && isen) {
#ifdef BLITTER_DEBUG_NOWAIT
			write_log ("BLITTER: on the fly %d (%d) -> %d (%d) switch\n", original_ch, iseo, blit_ch, isen);
#endif
		}
	}

	// on the fly switching from CH=1 to CH=D -> blitter stops writing (Rampage/TEK)
	// currently just switch to no-channels mode, better than crashing the demo..
	if (!savestate_state && bltstate != BLT_done) {
		static uae_u8 changetable[32 * 32];
		int o = original_ch + (original_fill ? 16 : 0);
		int n = blit_ch + (blitfill ? 16 : 0);
		if (o != n) {
			if (changetable[o * 32 + n] < 10) {
				changetable[o * 32 + n]++;
				write_log ("BLITTER: channel mode changed while active (%02X->%02X) PC=%08x\n", o, n, M68K_GETPC);
			}
		}
		if (blit_ch == 13 && original_ch == 1) {
			blit_faulty = 1;
		}
	}

	if (blit_faulty) {
		blit_ch = 0;
		blit_diag = blit_cycle_diagram[blit_ch];
	}

	blit_dmacount = blit_dmacount2 = 0;
	blit_nod = 1;
	for (i = 0; i < blit_diag[0]; i++) {
		int v = blit_diag[1 + blit_diag[0] + i];
		if (v <= 4)
			blit_dmacount++;
		if (v > 0 && v < 4)
			blit_dmacount2++;
		if (v == 4)
			blit_nod = 0;
	}

}

static void blit_modset (void)
{
	int mult;

	blit_add = blitdesc ? -2 : 2;
	mult = blitdesc ? -1 : 1;
	blit_modadda = mult * blt_info.bltamod;
	blit_modaddb = mult * blt_info.bltbmod;
	blit_modaddc = mult * blt_info.bltcmod;
	blit_modaddd = mult * blt_info.bltdmod;
}

void reset_blit (unsigned int bltcon)
{
	if (bltcon & 1)
		blinea_shift = bltcon0 >> 12;
	if (bltcon & 2)
		blitsign = bltcon1 & 0x40;
	if (bltstate == BLT_done)
		return;
	if (bltcon)
		blit_bltset (bltcon);
	blit_modset ();
}

static void do_blitter2 (unsigned int hpos, int copper)
{
	unsigned int cycles;
	int cleanstart;

#ifdef BLITTER_DEBUG_NOWAIT
	if (bltstate != BLT_done) {
		if (blit_final)
			write_log ("blitter was already active! PC=%08x\n", M68K_GETPC);
	}
#endif
	cleanstart = 0;
	if (bltstate == BLT_done) {
		if (blit_faulty > 0)
			blit_faulty = 0;
		cleanstart = 1;
	}

	bltstate = BLT_done;

	hblitsize = blt_info.hblitsize;
	vblitsize = blt_info.vblitsize;
	blitter_cycle_exact = currprefs.blitter_cycle_exact;
	blt_info.blitzero = 1;
	preva = 0;
	prevb = 0;
	blt_info.got_cycle = 0;
	blit_frozen = 0;

	blit_firstline_cycles = blit_first_cycle = get_cycles ();
	blit_misscyclecounter = 0;
	blit_last_cycle = 0;
	blit_maxcyclecounter = 0;
	last_blitter_hpos = hpos + 1;
	blit_cyclecounter = 0;
	blit_totalcyclecounter = 0;

	blit_bltset (1 | 2);
	blit_modset ();
	ddat1use = ddat2use = 0;
	blit_interrupt = 0;

	if (blitline) {
		blinea = blt_info.bltadat;
		blineb = (blt_info.bltbdat >> blt_info.blitbshift) | (blt_info.bltbdat << (16 - blt_info.blitbshift));
		blitonedot = 0;
		blitlinepixel = 0;
		blitsing = bltcon1 & 0x2;
		cycles = vblitsize;
	} else {
		blit_firstline_cycles = blit_first_cycle + (blit_diag[0] * hblitsize + cpu_cycles) * CYCLE_UNIT;
		cycles = vblitsize * hblitsize;
	}

	if (cleanstart) {
		original_ch = blit_ch;
		original_fill = blitfill;
		original_line = blitline;
	}

#ifdef BLITTER_DEBUG
	blitter_dontdo = 0;
	if (1) {
		int ch = 0;
		if (blit_ch & 1)
			ch++;
		if (blit_ch & 2)
			ch++;
		if (blit_ch & 4)
			ch++;
		if (blit_ch & 8)
			ch++;
		write_log ("blitstart: %dx%d ch=%d %d*%d=%d d=%d f=%02X n=%d pc=%p l=%d dma=%04X\n",
			hblitsize, vblitsize, ch, blit_diag[0], cycles, blit_diag[0] * cycles,
			blitdesc ? 1 : 0, blitfill, dmaen (DMA_BLITPRI) ? 1 : 0, M68K_GETPC, blitline, dmacon);
		blitter_dump ();
	}
#endif

	bltstate = BLT_init;
	blit_slowdown = 0;

	unset_special (&regs,SPCFLAG_BLTNASTY);
	if (dmaen (DMA_BLITPRI))
		set_special (&regs, SPCFLAG_BLTNASTY);

#if 0
	if (M68K_GETPC >= 0x00070554 && M68K_GETPC <= 0x000706B0) {
		blitter_done ();
		return;
	}
	if (M68K_GETPC >= 0x00070838) {
		blitter_done ();
		return;
	}
#endif

	if (dmaen (DMA_BLITTER))
		bltstate = BLT_work;

	blit_maxcyclecounter = 0x7fffffff;
	if (blitter_cycle_exact) {
#ifdef BLITTER_INSTANT
		blitter_handler (0);
#else
		blitter_hcounter1 = blitter_hcounter2 = 0;
		blitter_vcounter1 = blitter_vcounter2 = 0;
		if (blit_nod)
			blitter_vcounter2 = vblitsize;
		blit_cyclecounter = -BLITTER_STARTUP_CYCLES;
		blit_waitcyclecounter = copper;
		blit_startcycles = 0;
		blit_maxcyclecounter = hblitsize * vblitsize + 2;
#endif
		return;
	}

	if (vblitsize == 0 || (blitline && hblitsize != 2)) {
		blitter_done (hpos);
		return;
	}

	blt_info.got_cycle = 1;
	if (currprefs.immediate_blits)
		cycles = 1;

	blit_waitcyclecounter = 0;
	blit_cyclecounter = cycles * (blit_dmacount2 + (blit_nod ? 0 : 1)); 
	event2_newevent (ev2_blitter, blit_cyclecounter, 0);
}

void do_blitter (unsigned int hpos, int copper)
{
	if (bltstate == BLT_done || !currprefs.blitter_cycle_exact) {
		do_blitter2 (hpos, copper);
		return;
	}
	// previous blit may have last write cycle left
	// and we must let it finish
	blit_startcycles = BLITTER_STARTUP_CYCLES;
	blit_waitcyclecounter = copper;
}

void maybe_blit (unsigned int hpos, int hack)
{
	static int warned = 10;

	if (bltstate == BLT_done)
		return;
	if (savestate_state)
		return;

	if (warned && dmaen (DMA_BLITTER) && blt_info.got_cycle) {
		warned--;
		write_log ("program does not wait for blitter tc=%d\n",
			blit_cyclecounter);
#ifdef BLITTER_DEBUG
		warned = 0;
#endif
#ifdef BLITTER_DEBUG_NOWAIT
		warned = 10;
		write_log ("program does not wait for blitter PC=%08x\n", M68K_GETPC);
		//activate_debugger ();
		//blitter_done (hpos);
#endif
	}

	if (blitter_cycle_exact) {
		decide_blitter (hpos);
		goto end;
	}

	if (hack == 1 && get_cycles() < blit_firstline_cycles)
		goto end;

	blitter_handler (0);
end:;
#ifdef BLITTER_DEBUG
	blitter_delayed_debug = 1;
#endif
}

unsigned int blitnasty (void)
{
	unsigned int cycles, ccnt;
	if (bltstate == BLT_done)
		return 0;
	if (!dmaen (DMA_BLITTER))
		return 0;
	if ((int)blit_last_cycle >= blit_diag[0] && (int)blit_dmacount == blit_diag[0])
		return 0;
	cycles = (get_cycles () - blit_first_cycle) / CYCLE_UNIT;
	ccnt = 0;
	while (blit_last_cycle < cycles) {
		int c = channel_state (blit_last_cycle++);
		if (!c)
			ccnt++;
	}
	return ccnt;
}

/* very approximate emulation of blitter slowdown caused by bitplane DMA */
void blitter_slowdown (int ddfstrt, int ddfstop, unsigned int totalcycles, unsigned int freecycles)
{
    static int oddfstrt, oddfstop;
    static unsigned ototal, ofree;
    static int slow;

	if (!totalcycles || ddfstrt < 0 || ddfstop < 0)
		return;
	if (ddfstrt != oddfstrt || ddfstop != oddfstop || totalcycles != ototal || ofree != freecycles) {
		int linecycles = ((ddfstop - ddfstrt + totalcycles - 1) / totalcycles) * totalcycles;
		int freelinecycles = ((ddfstop - ddfstrt + totalcycles - 1) / totalcycles) * freecycles;
		int dmacycles = (linecycles * blit_dmacount) / blit_diag[0];
		oddfstrt = ddfstrt;
		oddfstop = ddfstop;
		ototal = totalcycles;
		ofree = freecycles;
		slow = 0;
		if (dmacycles > freelinecycles)
			slow = dmacycles - freelinecycles;
	}
	if (blit_slowdown < 0 || blitline)
		return;
	blit_slowdown += slow;
#ifdef BLITTER_DEBUG
	blit_misscyclecounter += slow;
#endif
}

#ifdef SAVESTATE

const uae_u8 *restore_blitter (const uae_u8 *src)
{
	uae_u32 flags = restore_u32();

	blt_delayed_irq = 0;
	bltstate = BLT_done;
	if (flags & 4) {
		bltstate = (flags & 1) ? BLT_done : BLT_init;
	}
	if (flags & 2) {
		write_log ("blitter was force-finished when this statefile was saved\n");
		write_log ("contact the author if restored program freezes\n");
		// there is a problem. if system ks vblank is active, we must not activate
		// "old" blit's intreq until vblank is handled or ks 1.x thinks it was blitter
		// interrupt..
		blt_delayed_irq = -1;
	}
	return src;
}

void restore_blitter_finish (void)
{
	record_dma_reset ();
	record_dma_reset ();
	blit_interrupt = 1;
	if (bltstate == BLT_init) {
		write_log ("blitter was started but DMA was inactive during save\n");
		//do_blitter (0);
	}
	if (blt_delayed_irq < 0) {
		if (intreq & 0x0040)
			blt_delayed_irq = 3;
		intreq &= 0x0040;
	}
}

uae_u8 *save_blitter (uae_u32 *len, uae_u8 *dstptr)
{
	uae_u8 *dstbak,*dst;
	int forced;

	forced = 0;
	if (bltstate != BLT_done && bltstate != BLT_init) {
		write_log ("blitter is active, forcing immediate finish\n");
		/* blitter is active just now but we don't have blitter state support yet */
		blitter_force_finish ();
		forced = 2;
	}
	if (dstptr)
		dstbak = dst = dstptr;
	else
		dstbak = dst = xmalloc (uae_u8, 16);
	save_u32 (((bltstate != BLT_done) ? 0 : 1) | forced | 4);
	*len = dst - dstbak;
	return dstbak;

}

#endif /* SAVESTATE */
