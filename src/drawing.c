//#define XLINECHECK

 /*
  * UAE - The Un*x Amiga Emulator
  *
  * Screen drawing functions
  *
  * Copyright 1995-2000 Bernd Schmidt
  * Copyright 1995 Alessandro Bissacco
  * Copyright 2000-2008 Toni Wilen
  */

/* There are a couple of concepts of "coordinates" in this file.
   - DIW coordinates
   - DDF coordinates (essentially cycles, resolution lower than lores by a factor of 2)
   - Pixel coordinates
     * in the Amiga's resolution as determined by BPLCON0 ("Amiga coordinates")
     * in the window resolution as determined by the preferences ("window coordinates").
     * in the window resolution, and with the origin being the topmost left corner of
       the window ("native coordinates")
   One note about window coordinates.  The visible area depends on the width of the
   window, and the centering code.  The first visible horizontal window coordinate is
   often _not_ 0, but the value of VISIBLE_LEFT_BORDER instead.

   One important thing to remember: DIW coordinates are in the lowest possible
   resolution.

   To prevent extremely bad things (think pixels cut in half by window borders) from
   happening, all ports should restrict window widths to be multiples of 16 pixels.  */

#include "sysconfig.h"
#include "sysdeps.h"

#include <ctype.h>
#include <assert.h>

#include "options.h"
#include "threaddep/thread.h"
#include "uae.h"
#include "memory.h"
#include "custom.h"
#include "newcpu.h"
#include "xwin.h"
#include "autoconf.h"
#include "gui.h"
#include "picasso96.h"
#include "drawing.h"
#include "savestate.h"
#include "statusline.h"
#include "inputdevice.h"
#include "debug.h"

extern unsigned int sprite_buffer_res;
int lores_factor, lores_shift;

int debug_bpl_mask = 0xff, debug_bpl_mask_one;

static void lores_reset (void)
{
    lores_factor = currprefs.gfx_resolution ? 2 : 1;
    lores_shift = currprefs.gfx_resolution;
    if (doublescan > 0) {
		if (lores_shift < 2)
		    lores_shift++;
		lores_factor = 2;
    }
    sprite_buffer_res = currprefs.gfx_resolution;
    if (doublescan > 0 && sprite_buffer_res < RES_SUPERHIRES)
		sprite_buffer_res++;
}

int aga_mode; /* mirror of chipset_mask & CSMASK_AGA */
int direct_rgb;

/* The shift factor to apply when converting between Amiga coordinates and window
   coordinates.  Zero if the resolution is the same, positive if window coordinates
   have a higher resolution (i.e. we're stretching the image), negative if window
   coordinates have a lower resolution (i.e. we're shrinking the image).  */
static int res_shift;

static int linedbl, linedbld;

int interlace_seen = 0;
#define AUTO_LORES_FRAMES 10
static int can_use_lores = 0, frame_res, frame_res_lace, last_max_ypos;

/* Lookup tables for dual playfields.  The dblpf_*1 versions are for the case
   that playfield 1 has the priority, dbplpf_*2 are used if playfield 2 has
   priority.  If we need an array for non-dual playfield mode, it has no number.  */
/* The dbplpf_ms? arrays contain a shift value.  plf_spritemask is initialized
   to contain two 16 bit words, with the appropriate mask if pf1 is in the
   foreground being at bit offset 0, the one used if pf2 is in front being at
   offset 16.  */

static int dblpf_ms1[256], dblpf_ms2[256], dblpf_ms[256];
static int dblpf_ind1[256], dblpf_ind2[256];

static int dblpf_2nd1[256], dblpf_2nd2[256];

static int dblpfofs[] = { 0, 2, 4, 8, 16, 32, 64, 128 };

static int sprite_offs[256];

static uae_u32 clxtab[256];

/* Video buffer description structure. Filled in by the graphics system
 * dependent code. */

struct vidbuf_description gfxvidinfo;

/* OCS/ECS color lookup table. */
xcolnr xcolors[4096];

struct spritepixelsbuf {
    uae_u8 attach;
    uae_u8 stdata;
    uae_u16 data;
};
static struct spritepixelsbuf spritepixels[MAX_PIXELS_PER_LINE];
static int sprite_first_x, sprite_last_x;

#ifdef AGA
/* AGA mode color lookup tables */
unsigned int xredcolors[256], xgreencolors[256], xbluecolors[256];
static int dblpf_ind1_aga[256], dblpf_ind2_aga[256];
#else
static uae_u8 spriteagadpfpixels[1];
static int dblpf_ind1_aga[1], dblpf_ind2_aga[1];
#endif
int xredcolor_s, xredcolor_b, xredcolor_m;
int xgreencolor_s, xgreencolor_b, xgreencolor_m;
int xbluecolor_s, xbluecolor_b, xbluecolor_m;

struct color_entry colors_for_drawing;

/* The size of these arrays is pretty arbitrary; it was chosen to be "more
   than enough".  The coordinates used for indexing into these arrays are
   almost, but not quite, Amiga coordinates (there's a constant offset).  */
union {
    /* Let's try to align this thing. */
    double uupzuq;
    long int cruxmedo;
    uae_u8 apixels[MAX_PIXELS_PER_LINE * 2];
    uae_u16 apixels_w[MAX_PIXELS_PER_LINE * 2 / sizeof (uae_u16)];
    uae_u32 apixels_l[MAX_PIXELS_PER_LINE * 2 / sizeof (uae_u32)];
} pixdata;

#ifdef OS_WITHOUT_MEMORY_MANAGEMENT
uae_u16 *spixels;
#else
uae_u16 spixels[2 * MAX_SPR_PIXELS];
#endif

/* Eight bits for every pixel.  */
union sps_union spixstate;

static uae_u32 ham_linebuf[MAX_PIXELS_PER_LINE * 2];
static uae_u8 *real_bplpt[8];

static uae_u8 all_ones[MAX_PIXELS_PER_LINE];
static uae_u8 all_zeros[MAX_PIXELS_PER_LINE];

uae_u8 *xlinebuffer;

static int *amiga2aspect_line_map, *native2amiga_line_map;
static uae_u8 *row_map[MAX_VIDHEIGHT + 1];
static uae_u8 row_tmp[MAX_PIXELS_PER_LINE * 32 / 8];
static int max_drawn_amiga_line;

/* line_draw_funcs: pfield_do_linetoscr, pfield_do_fill_line, decode_ham */
typedef void (*line_draw_func)(int, int);

#define LINE_UNDECIDED 1
#define LINE_DECIDED 2
#define LINE_DECIDED_DOUBLE 3
#define LINE_AS_PREVIOUS 4
#define LINE_BLACK 5
#define LINE_REMEMBERED_AS_BLACK 6
#define LINE_DONE 7
#define LINE_DONE_AS_PREVIOUS 8
#define LINE_REMEMBERED_AS_PREVIOUS 9

static uae_u8 linestate[(MAXVPOS + 1) * 2 + 1];

uae_u8 line_data[(MAXVPOS + 1) * 2][MAX_PLANES * MAX_WORDS_PER_LINE * 2];

/* Centering variables.  */
static int min_diwstart, max_diwstop;
/* The visible window: VISIBLE_LEFT_BORDER contains the left border of the visible
   area, VISIBLE_RIGHT_BORDER the right border.  These are in window coordinates.  */
int visible_left_border, visible_right_border;
static int linetoscr_x_adjust_bytes;
static int thisframe_y_adjust;
static int thisframe_y_adjust_real, max_ypos_thisframe, min_ypos_for_screen;
static int extra_y_adjust;
int thisframe_first_drawn_line, thisframe_last_drawn_line;

/* A frame counter that forces a redraw after at least one skipped frame in
   interlace mode.  */
static int last_redraw_point;

static int first_drawn_line, last_drawn_line;
static int first_block_line, last_block_line;

#define NO_BLOCK -3

/* These are generated by the drawing code from the line_decisions array for
   each line that needs to be drawn.  These are basically extracted out of
   bit fields in the hardware registers.  */
static int bplehb, bplham, bpldualpf, bpldualpfpri, bpldualpf2of, bplplanecnt, ecsshres, issprites;
static int bplres;
static int plf1pri, plf2pri, bplxor;
static uae_u32 plf_sprite_mask;
static int sbasecol[2] = { 16, 16 };
static int brdsprt, brdblank, brdblank_changed;

int picasso_requested_on;
int picasso_on;

uae_sem_t gui_sem;
int inhibit_frame;

int framecnt = 0;
static int frame_redraw_necessary;
static int picasso_redraw_necessary;

#ifdef XLINECHECK
static void xlinecheck (unsigned int start, unsigned int end)
{
    unsigned int xstart = (unsigned int)xlinebuffer + start * gfxvidinfo.pixbytes;
    unsigned int xend = (unsigned int)xlinebuffer + end * gfxvidinfo.pixbytes;
    unsigned int end1 = (unsigned int)gfxvidinfo.bufmem + gfxvidinfo.rowbytes * gfxvidinfo.height;
    int min = linetoscr_x_adjust_bytes / gfxvidinfo.pixbytes;
    int ok = 1;

    if (xstart >= gfxvidinfo.emergmem && xstart < gfxvidinfo.emergmem + 4096 * gfxvidinfo.pixbytes &&
		xend >= gfxvidinfo.emergmem && xend < gfxvidinfo.emergmem + 4096 * gfxvidinfo.pixbytes)
		return;

    if (xstart < (unsigned int)gfxvidinfo.bufmem || xend < (unsigned int)gfxvidinfo.bufmem)
		ok = 0;
    if (xend > end1 || xstart >= end1)
		ok = 0;
    xstart -= (unsigned int)gfxvidinfo.bufmem;
    xend -= (unsigned int)gfxvidinfo.bufmem;
    if ((xstart % gfxvidinfo.rowbytes) >= gfxvidinfo.width * gfxvidinfo.pixbytes)
		ok = 0;
    if ((xend % gfxvidinfo.rowbytes) >= gfxvidinfo.width * gfxvidinfo.pixbytes)
		ok = 0;
    if (xstart >= xend)
		ok = 0;
    if (xend - xstart > gfxvidinfo.width * gfxvidinfo.pixbytes)
		ok = 0;

    if (!ok) {
		write_log ("*** %d-%d (%dx%dx%d %d) %p\n",
			start - min, end - min, gfxvidinfo.width, gfxvidinfo.height,
			gfxvidinfo.pixbytes, gfxvidinfo.rowbytes,
			xlinebuffer);
    }
}
#else
#define xlinecheck
#endif


STATIC_INLINE void count_frame (void)
{
    framecnt++;
    if (framecnt >= currprefs.gfx_framerate)
		framecnt = 0;
    if (inhibit_frame)
		framecnt = 1;
}

STATIC_INLINE int xshift (int x, int shift)
{
    if (shift < 0)
		return x >> (-shift);
    else
		return x << shift;
}

int coord_native_to_amiga_x (int x)
{
    x += visible_left_border;
    x = xshift (x, 1 - lores_shift);
    return x + 2 * DISPLAY_LEFT_SHIFT - 2 * DIW_DDF_OFFSET;
}

int coord_native_to_amiga_y (int y)
{
    return native2amiga_line_map[y] + thisframe_y_adjust - minfirstline;
}

STATIC_INLINE int res_shift_from_window (int x)
{
    if (res_shift >= 0)
		return x >> res_shift;
    return x << -res_shift;
}

STATIC_INLINE int res_shift_from_amiga (int x)
{
    if (res_shift >= 0)
		return x >> res_shift;
    return x << -res_shift;
}

void notice_screen_contents_lost (void)
{
    picasso_redraw_necessary = 1;
    frame_redraw_necessary = 2;
}


extern int plffirstline_total, plflastline_total;
extern int first_planes_vpos, last_planes_vpos;
extern int diwfirstword_total, diwlastword_total;
extern int ddffirstword_total, ddflastword_total;
extern int firstword_bplcon1;

#define MIN_DISPLAY_W 256
#define MIN_DISPLAY_H 192
#define MAX_DISPLAY_W 352
#define MAX_DISPLAY_H 276

static int gclow, gcloh, gclox, gcloy;

static void reset_custom_limits (void)
{
    gclow = gcloh = gclox = gcloy = 0;
}

int get_custom_limits (int *pw, int *ph, int *pdx, int *pdy)
{
    int w, h, dx, dy, y1, y2, dbl1, dbl2;
    int ret = 0;

    if (!pw || !ph || !pdx || !pdy) {
		reset_custom_limits ();
		return 0;
    }
    *pw = gclow;
    *ph = gcloh;
    *pdx = gclox;
    *pdy = gcloy;

    if (gclow > 0 && gcloh > 0)
	ret = -1;

    if (doublescan <= 0) {
		int min = coord_diw_to_window_x (94);
		int max = coord_diw_to_window_x (460);
		if (diwfirstword_total < min)
			diwfirstword_total = min;
		if (diwlastword_total > max)
			diwlastword_total = max;
		ddffirstword_total = coord_hw_to_window_x (ddffirstword_total * 2 + DIW_DDF_OFFSET);
		ddflastword_total = coord_hw_to_window_x (ddflastword_total * 2 + DIW_DDF_OFFSET);
		if (ddffirstword_total < min)
			ddffirstword_total = min;
		if (ddflastword_total > max)
			ddflastword_total = max;
		if (0 && !(currprefs.chipset_mask & CSMASK_AGA)) {
			if (ddffirstword_total > diwfirstword_total)
				diwfirstword_total = ddffirstword_total;
			if (ddflastword_total < diwlastword_total)
				diwlastword_total = ddflastword_total;
		}
    }

    w = diwlastword_total - diwfirstword_total;
    dx = diwfirstword_total - visible_left_border;

    y2 = plflastline_total;
    if (y2 > last_planes_vpos)
		y2 = last_planes_vpos;
    y1 = plffirstline_total;
    if (first_planes_vpos > y1)
		y1 = first_planes_vpos;
    if ((int)minfirstline > y1)
		y1 = minfirstline;

    dbl2 = dbl1 = currprefs.gfx_linedbl ? 1 : 0;
    if (doublescan > 0 && interlace_seen <= 0) {
		dbl1--;
		dbl2--;
    }

    h = y2 - y1;
    dy = y1 - minfirstline;

    if (first_planes_vpos == 0) {
		// no planes enabled during frame
		if (ret < 0)
		    return 1;
		h = currprefs.ntscmode ? 200 : 240;
		w = 320 << currprefs.gfx_resolution;
		dy = 36 / 2;
		dx = 58;
    }

    if (dx < 0)
		dx = 1;

    dy = xshift (dy, dbl2);
    h = xshift (h, dbl1);

    if (w == 0 || h == 0)
		return 0;

    if (doublescan <= 0) {
		if ((w >> currprefs.gfx_resolution) < MIN_DISPLAY_W) {
		    dx += (w - (MIN_DISPLAY_W << currprefs.gfx_resolution)) / 2;
		    w = MIN_DISPLAY_W << currprefs.gfx_resolution;
		}
		if ((h >> dbl1) < MIN_DISPLAY_H) {
		    dy += (h - (MIN_DISPLAY_H << dbl1)) / 2;
		    h = MIN_DISPLAY_H << dbl1;
		}
		if ((w >> currprefs.gfx_resolution) > MAX_DISPLAY_W) {
		    dx += (w - (MAX_DISPLAY_W << currprefs.gfx_resolution)) / 2;
		    w = MAX_DISPLAY_W << currprefs.gfx_resolution;
		}
		if ((h >> dbl1) > MAX_DISPLAY_H) {
		    dy += (h - (MAX_DISPLAY_H << dbl1)) / 2;
		    h = MAX_DISPLAY_H << dbl1;
		}
    }

    if (gclow == w && gcloh == h && gclox == dx && gcloy == dy)
		return ret;

    if (w <= 0 || h <= 0 || dx <= 0 || dy <= 0)
		return ret;
    if (doublescan <= 0) {
		if (dx > gfxvidinfo.width / 3)
		    return ret;
		if (dy > gfxvidinfo.height / 3)
		    return ret;
    }
    
    gclow = w;
    gcloh = h;
    gclox = dx;
    gcloy = dy;
    *pw = w;
    *ph = h;
    *pdx = dx;
    *pdy = dy;

	write_log ("Display Size: %dx%d Offset: %dx%d\n", w, h, dx, dy);
	write_log ("First: %d Last: %d MinV: %d MaxV: %d Min: %d\n",
		plffirstline_total, plflastline_total,
		first_planes_vpos, last_planes_vpos, minfirstline);
    return 1;
}

void get_custom_mouse_limits (int *pw, int *ph, int *pdx, int *pdy, int dbl)
{
    int delay1, delay2;
    int w, h, dx, dy, dbl1, dbl2, y1, y2;

    w = diwlastword_total - diwfirstword_total;
    dx = diwfirstword_total - visible_left_border;

    y2 = plflastline_total;
    if (y2 > last_planes_vpos)
		y2 = last_planes_vpos;
    y1 = plffirstline_total;
    if (first_planes_vpos > y1)
		y1 = first_planes_vpos;
    if ((int)minfirstline > y1)
	y1 = minfirstline;

    h = y2 - y1;
    dy = y1 - minfirstline;

    if (*pw > 0)
		w = *pw;
    
    w = xshift (w, res_shift);

    if (*ph > 0)
		h = *ph;
    
    delay1 = (firstword_bplcon1 & 0x0f) | ((firstword_bplcon1 & 0x0c00) >> 6);
    delay2 = ((firstword_bplcon1 >> 4) & 0x0f) | (((firstword_bplcon1 >> 4) & 0x0c00) >> 6);
    if (delay1 == delay2)
		;//dx += delay1;

    dx = xshift (dx, res_shift);

    dbl2 = dbl1 = currprefs.gfx_linedbl ? 1 : 0;
    if ((doublescan > 0 || interlace_seen > 0) && !dbl) {
        dbl1--;
        dbl2--;
    }
    if (interlace_seen > 0)
        dbl2++;
    if (interlace_seen <= 0 && dbl)
        dbl2--;
    h = xshift (h, dbl1);
    dy = xshift (dy, dbl2);

    if (w < 1)
		w = 1;
    if (h < 1)
		h = 1;
    if (dx < 0)
		dx = 0;
    if (dy < 0)
		dy = 0;
    *pw = w; *ph = h;
    *pdx = dx; *pdy = dy;
}

static struct decision *dp_for_drawing;
static struct draw_info *dip_for_drawing;

/* Record DIW of the current line for use by centering code.  */
void record_diw_line (unsigned int plfstrt, int first, int last)
{
    if (last > max_diwstop)
		max_diwstop = last;
    if (first < min_diwstart) {
		min_diwstart = first;
/*
	if (plfstrt * 2 > min_diwstart)
	    min_diwstart = plfstrt * 2;
*/
    }
}

/*
 * Screen update macros/functions
 */

/* The important positions in the line: where do we start drawing the left border,
   where do we start drawing the playfield, where do we start drawing the right border.
   All of these are forced into the visible window (VISIBLE_LEFT_BORDER .. VISIBLE_RIGHT_BORDER).
   PLAYFIELD_START and PLAYFIELD_END are in window coordinates.  */
static int playfield_start, playfield_end;
static int real_playfield_start, real_playfield_end;
static int linetoscr_diw_start, linetoscr_diw_end;
static int native_ddf_left, native_ddf_right;

static int pixels_offset;
static int src_pixel, ham_src_pixel;
/* How many pixels in window coordinates which are to the left of the left border.  */
static int unpainted;
static int seen_sprites;

/* Initialize the variables necessary for drawing a line.
 * This involves setting up start/stop positions and display window
 * borders.  */
static void pfield_init_linetoscr (void)
{
    /* First, get data fetch start/stop in DIW coordinates.  */
    int ddf_left = dp_for_drawing->plfleft * 2 + DIW_DDF_OFFSET;
    int ddf_right = dp_for_drawing->plfright * 2 + DIW_DDF_OFFSET;

    /* Compute datafetch start/stop in pixels; native display coordinates.  */
	native_ddf_left = coord_hw_to_window_x (ddf_left);
	native_ddf_right = coord_hw_to_window_x (ddf_right);

	linetoscr_diw_start = dp_for_drawing->diwfirstword;
	linetoscr_diw_end = dp_for_drawing->diwlastword;

    res_shift = lores_shift - bplres;

    if (dip_for_drawing->nr_sprites == 0) {
		if (linetoscr_diw_start < native_ddf_left)
		    linetoscr_diw_start = native_ddf_left;
		if (linetoscr_diw_end > native_ddf_right)
		    linetoscr_diw_end = native_ddf_right;
    }

    /* Perverse cases happen. */
    if (linetoscr_diw_end < linetoscr_diw_start)
		linetoscr_diw_end = linetoscr_diw_start;

    playfield_start = linetoscr_diw_start;
    playfield_end = linetoscr_diw_end;

    unpainted = visible_left_border < playfield_start ? 0 : visible_left_border - playfield_start;
    ham_src_pixel = MAX_PIXELS_PER_LINE + res_shift_from_window (playfield_start - native_ddf_left);
    unpainted = res_shift_from_window (unpainted);

    if (playfield_start < visible_left_border)
		playfield_start = visible_left_border;
    if (playfield_start > visible_right_border)
		playfield_start = visible_right_border;
    if (playfield_end < visible_left_border)
		playfield_end = visible_left_border;
    if (playfield_end > visible_right_border)
		playfield_end = visible_right_border;

    real_playfield_end = playfield_end;
    real_playfield_start = playfield_start;
#ifdef AGA
    if (brdsprt && dip_for_drawing->nr_sprites) {
		int min = visible_right_border, max = visible_left_border, i;
		for (i = 0; i < dip_for_drawing->nr_sprites; i++) {
		    int x;
		    x = curr_sprite_entries[dip_for_drawing->first_sprite_entry + i].pos;
		    if (x < min)
				min = x;
		    x = curr_sprite_entries[dip_for_drawing->first_sprite_entry + i].max;
		    if (x > max)
				max = x;
		}
		min = coord_hw_to_window_x (min >> sprite_buffer_res) + (DIW_DDF_OFFSET << lores_shift);
		max = coord_hw_to_window_x (max >> sprite_buffer_res) + (DIW_DDF_OFFSET << lores_shift);
		if (min < playfield_start)
		    playfield_start = min;
		if (playfield_start < visible_left_border)
		    playfield_start = visible_left_border;
		if (max > playfield_end)
		    playfield_end = max;
		if (playfield_end > visible_right_border)
		    playfield_end = visible_right_border;
    }
#endif

	if (sprite_first_x < sprite_last_x) {
		if (sprite_first_x < 0)
			sprite_first_x = 0;
		if (sprite_last_x >= MAX_PIXELS_PER_LINE - 1)
			sprite_last_x = MAX_PIXELS_PER_LINE - 2;
	    if (sprite_first_x < sprite_last_x)
			memset (spritepixels + sprite_first_x, 0, sizeof (struct spritepixelsbuf) * (sprite_last_x - sprite_first_x + 1));
	}
    sprite_last_x = 0;
    sprite_first_x = MAX_PIXELS_PER_LINE - 1;

    /* Now, compute some offsets.  */
    ddf_left -= DISPLAY_LEFT_SHIFT;
    pixels_offset = MAX_PIXELS_PER_LINE - (ddf_left << bplres);
    ddf_left <<= bplres;
    src_pixel = MAX_PIXELS_PER_LINE + res_shift_from_window (playfield_start - native_ddf_left);

    seen_sprites = 0;
    if (dip_for_drawing->nr_sprites == 0)
		return;
    seen_sprites = 1;
    /* Must clear parts of apixels.  */
    if (linetoscr_diw_start < native_ddf_left) {
		int size = res_shift_from_window (native_ddf_left - linetoscr_diw_start);
		linetoscr_diw_start = native_ddf_left;
		memset (pixdata.apixels + MAX_PIXELS_PER_LINE - size, 0, size);
    }
    if (linetoscr_diw_end > native_ddf_right) {
		int pos = res_shift_from_window (native_ddf_right - native_ddf_left);
		int size = res_shift_from_window (linetoscr_diw_end - native_ddf_right);
		linetoscr_diw_start = native_ddf_left;
		memset (pixdata.apixels + MAX_PIXELS_PER_LINE + pos, 0, size);
    }
}

STATIC_INLINE uae_u8 merge_2pixel8 (uae_u8 p1, uae_u8 p2)
{
    return p1;
}
STATIC_INLINE uae_u16 merge_2pixel16 (uae_u16 p1, uae_u16 p2)
{
    uae_u16 v = ((((p1 >> xredcolor_s) & xredcolor_m) + ((p2 >> xredcolor_s) & xredcolor_m)) / 2) << xredcolor_s;
    v |= ((((p1 >> xbluecolor_s) & xbluecolor_m) + ((p2 >> xbluecolor_s) & xbluecolor_m)) / 2) << xbluecolor_s;
    v |= ((((p1 >> xgreencolor_s) & xgreencolor_m) + ((p2 >> xgreencolor_s) & xgreencolor_m)) / 2) << xgreencolor_s;
    return v;
}
STATIC_INLINE uae_u32 merge_2pixel32 (uae_u32 p1, uae_u32 p2)
{
    uae_u32 v = ((((p1 >> 16) & 0xff) + ((p2 >> 16) & 0xff)) / 2) << 16;
    v |= ((((p1 >> 8) & 0xff) + ((p2 >> 8) & 0xff)) / 2) << 8;
    v |= ((((p1 >> 0) & 0xff) + ((p2 >> 0) & 0xff)) / 2) << 0;
    return v;
}

static void fill_line_8 (uae_u8 *buf, unsigned int start, unsigned int stop)
{
    uae_u8 *b = (uae_u8 *)buf;
    unsigned int i;
    unsigned int rem = 0;
    xcolnr col = brdblank ? 0 : colors_for_drawing.acolors[0];
    while (((long)&b[start]) & 3) {
		b[start++] = (uae_u8)col;
		if (start == stop)
		   return;
    }
    if (((long)&b[stop]) & 3) {
		rem = ((long)&b[stop]) & 3;
		stop -= rem;
    }
    for (i = start; i < stop; i += 4) {
		uae_u32 *b2 = (uae_u32 *)&b[i];
		*b2 = col;
    }
    while (rem--)
		b[stop++] = (uae_u8) col;
}

static void fill_line_16 (uae_u8 *buf, unsigned int start, unsigned int stop)
{
    uae_u16 *b = (uae_u16 *)buf;
    unsigned int i;
    unsigned int rem = 0;
    xcolnr col = brdblank ? 0 : colors_for_drawing.acolors[0];
    if (((long)&b[start]) & 1)
		b[start++] = (uae_u16) col;
    if (start >= stop)
		return;
    if (((long)&b[stop]) & 1) {
		rem++;
		stop--;
    }
    for (i = start; i < stop; i += 2) {
		uae_u32 *b2 = (uae_u32 *)&b[i];
		*b2 = col;
    }
    if (rem)
		b[stop] = (uae_u16)col;
}

static void fill_line_32 (uae_u8 *buf, unsigned int start, unsigned int stop)
{
    uae_u32 *b = (uae_u32 *)buf;
    unsigned int i;
    xcolnr col = brdblank ? 0 : colors_for_drawing.acolors[0];
    for (i = start; i < stop; i++)
		b[i] = col;
}

static void pfield_do_fill_line (int start, int stop)
{
    xlinecheck(start, stop);
    switch (gfxvidinfo.pixbytes) {
    case 1: fill_line_8 (xlinebuffer, start, stop); break;
    case 2: fill_line_16 (xlinebuffer, start, stop); break;
    case 4: fill_line_32 (xlinebuffer, start, stop); break;
    }
}

STATIC_INLINE void fill_line (void)
{
    int shift;
    int nints, nrem;
    int *start;
    xcolnr val;

/*    shift = 0;
    if (gfxvidinfo.pixbytes == 2)
		shift = 1;
    if (gfxvidinfo.pixbytes == 4)
		shift = 2;*/
    shift = gfxvidinfo.pixbytes >> 1;

    nints = gfxvidinfo.width >> (2 - shift);
    nrem = nints & 7;
    nints &= ~7;
    start = (int *)(((uae_u8*)xlinebuffer) + (visible_left_border << shift));
#ifdef ECS_DENISE
    val = brdblank ? 0 : colors_for_drawing.acolors[0];
#else
    val = colors_for_drawing.acolors[0];
#endif
    for (; nints > 0; nints -= 8, start += 8) {
		*start = val;
		*(start+1) = val;
		*(start+2) = val;
		*(start+3) = val;
		*(start+4) = val;
		*(start+5) = val;
		*(start+6) = val;
		*(start+7) = val;
    }

    switch (nrem) {
     case 7:
		*start++ = val;
     case 6:
		*start++ = val;
     case 5:
		*start++ = val;
     case 4:
		*start++ = val;
     case 3:
		*start++ = val;
     case 2:
		*start++ = val;
     case 1:
		*start = val;
    }
}


#define SPRITE_DEBUG 0
STATIC_INLINE uae_u8 render_sprites (int pos, int dualpf, uae_u8 apixel, int aga)
{
    struct spritepixelsbuf *spb = &spritepixels[pos];
    unsigned int v = spb->data;
    int *shift_lookup = dualpf ? (bpldualpfpri ? dblpf_ms2 : dblpf_ms1) : dblpf_ms;
    int maskshift, plfmask;

    /* The value in the shift lookup table is _half_ the shift count we
       need.  This is because we can't shift 32 bits at once (undefined
       behaviour in C).  */
    maskshift = shift_lookup[apixel];
    plfmask = (plf_sprite_mask >> maskshift) >> maskshift;
    v &= ~plfmask;
    if (v != 0 || SPRITE_DEBUG) {
        unsigned int vlo, vhi, col;
        unsigned int v1 = v & 255;
		/* OFFS determines the sprite pair with the highest priority that has
	   any bits set.  E.g. if we have 0xFF00 in the buffer, we have sprite
	   pairs 01 and 23 cleared, and pairs 45 and 67 set, so OFFS will
	   have a value of 4.
	   2 * OFFS is the bit number in V of the sprite pair, and it also
	   happens to be the color offset for that pair. 
		*/
		int offs;
		if (v1 == 0)
		    offs = 4 + sprite_offs[v >> 8];
		else
		    offs = sprite_offs[v1];

		/* Shift highest priority sprite pair down to bit zero.  */
		v >>= offs * 2;
		v &= 15;
#if SPRITE_DEBUG > 0
		v ^= 8;
#endif
		if (spb->attach && (spb->stdata & (3 << offs))) {
		    col = v;
		    if (aga)
		        col += sbasecol[1];
		    else
		        col += 16;
		} else {
		    /* This sequence computes the correct color value.  We have to select
		       either the lower-numbered or the higher-numbered sprite in the pair.
		       We have to select the high one if the low one has all bits zero.
		       If the lower-numbered sprite has any bits nonzero, (VLO - 1) is in
		       the range of 0..2, and with the mask and shift, VHI will be zero.
		       If the lower-numbered sprite is zero, (VLO - 1) is a mask of
		       0xFFFFFFFF, and we select the bits of the higher numbered sprite
		       in VHI.
		       This is _probably_ more efficient than doing it with branches.  */
		    vlo = v & 3;
		    vhi = (v & (vlo - 1)) >> 2;
		    col = (vlo | vhi);
		    if (aga) {
		        if (vhi > 0)
				    col += sbasecol[1];
				else
				    col += sbasecol[0];
		    } else {
		        col += 16;
		    }
		    col += offs * 2;
		}
	
		return col;
    }

    return 0;
}

#include "linetoscr.c"

#ifdef ECS_DENISE
/* ECS SuperHires special cases */

STATIC_INLINE uae_u32 shsprite (int dpix, uae_u32 spix_val, uae_u32 v, int spr)
{
    uae_u8 sprcol;
    if (!spr)
		return v;
    sprcol = render_sprites (dpix, 0, spix_val, 0);
    if (sprcol)
        return colors_for_drawing.color_regs_ecs[sprcol];
    return v;
}

static int NOINLINE linetoscr_16_sh (int spix, int dpix, int stoppos, int spr)
{
    uae_u16 *buf = (uae_u16 *) xlinebuffer;

    while (dpix < stoppos) {
		uae_u16 spix_val1, spix_val2;
		uae_u16 v;
		int off;
		spix_val1 = pixdata.apixels[spix++];
		spix_val2 = pixdata.apixels[spix++];
		off = ((spix_val2 & 3) * 4) + (spix_val1 & 3) + ((spix_val1 | spix_val2) & 16);
		v = (colors_for_drawing.color_regs_ecs[off] & 0xccc) << 0;
		v |= v >> 2;
		buf[dpix] = shsprite (dpix, spix_val1, xcolors[v], spr);
		dpix++;
		v = (colors_for_drawing.color_regs_ecs[off] & 0x333) << 2;
		v |= v >> 2;
		buf[dpix] = shsprite (dpix, spix_val2, xcolors[v], spr);
		dpix++;
    }
    return spix;
}
static int NOINLINE linetoscr_32_sh (int spix, int dpix, int stoppos, int spr)
{
    uae_u32 *buf = (uae_u32 *) xlinebuffer;

    while (dpix < stoppos) {
		uae_u32 spix_val1, spix_val2;
		uae_u16 v;
		int off;
		spix_val1 = pixdata.apixels[spix++];
		spix_val2 = pixdata.apixels[spix++];
		off = ((spix_val2 & 3) * 4) + (spix_val1 & 3) + ((spix_val1 | spix_val2) & 16);
		v = (colors_for_drawing.color_regs_ecs[off] & 0xccc) << 0;
		v |= v >> 2;
		buf[dpix] = shsprite (dpix, spix_val1, xcolors[v], spr);
		dpix++;
		v = (colors_for_drawing.color_regs_ecs[off] & 0x333) << 2;
		v |= v >> 2;
		buf[dpix] = shsprite (dpix, spix_val2, xcolors[v], spr);
		dpix++;
    }
    return spix;
}
static int NOINLINE linetoscr_32_shrink1_sh (int spix, int dpix, int stoppos, int spr)
{
    uae_u32 *buf = (uae_u32 *) xlinebuffer;

    while (dpix < stoppos) {
		uae_u32 spix_val1, spix_val2;
		uae_u16 v;
		int off;
		spix_val1 = pixdata.apixels[spix++];
		spix_val2 = pixdata.apixels[spix++];
		off = ((spix_val2 & 3) * 4) + (spix_val1 & 3) + ((spix_val1 | spix_val2) & 16);
		v = (colors_for_drawing.color_regs_ecs[off] & 0xccc) << 0;
		v |= v >> 2;
		buf[dpix] = shsprite (dpix, spix_val1, xcolors[v], spr);
		dpix++;
    }
    return spix;
}
static int NOINLINE linetoscr_32_shrink1f_sh (int spix, int dpix, int stoppos, int spr)
{
    uae_u32 *buf = (uae_u32 *) xlinebuffer;

    while (dpix < stoppos) {
		uae_u32 spix_val1, spix_val2, dpix_val1, dpix_val2;
		uae_u16 v;
		int off;
		spix_val1 = pixdata.apixels[spix++];
		spix_val2 = pixdata.apixels[spix++];
		off = ((spix_val2 & 3) * 4) + (spix_val1 & 3) + ((spix_val1 | spix_val2) & 16);
		v = (colors_for_drawing.color_regs_ecs[off] & 0xccc) << 0;
		v |= v >> 2;
		dpix_val1 = xcolors[v];
		v = (colors_for_drawing.color_regs_ecs[off] & 0x333) << 2;
		v |= v >> 2;
		dpix_val2 = xcolors[v];
		buf[dpix] = shsprite (dpix, spix_val1, merge_2pixel32 (dpix_val1, dpix_val2), spr);
		dpix++;
    }
    return spix;
}
static int NOINLINE linetoscr_16_shrink1_sh (int spix, int dpix, int stoppos, int spr)
{
    uae_u16 *buf = (uae_u16 *) xlinebuffer;

    while (dpix < stoppos) {
		uae_u16 spix_val1, spix_val2;
		uae_u16 v;
		int off;
		spix_val1 = pixdata.apixels[spix++];
		spix_val2 = pixdata.apixels[spix++];
		off = ((spix_val2 & 3) * 4) + (spix_val1 & 3) + ((spix_val1 | spix_val2) & 16);
		v = (colors_for_drawing.color_regs_ecs[off] & 0xccc) << 0;
		v |= v >> 2;
		buf[dpix] = shsprite (dpix, spix_val1, xcolors[v], spr);
		dpix++;
    }
    return spix;
}
static int NOINLINE linetoscr_16_shrink1f_sh (int spix, int dpix, int stoppos, int spr)
{
    uae_u16 *buf = (uae_u16 *) xlinebuffer;

    while (dpix < stoppos) {
		uae_u16 spix_val1, spix_val2, dpix_val1, dpix_val2;
		uae_u16 v;
		int off;
		spix_val1 = pixdata.apixels[spix++];
		spix_val2 = pixdata.apixels[spix++];
		off = ((spix_val2 & 3) * 4) + (spix_val1 & 3) + ((spix_val1 | spix_val2) & 16);
		v = (colors_for_drawing.color_regs_ecs[off] & 0xccc) << 0;
		v |= v >> 2;
		dpix_val1 = xcolors[v];
		v = (colors_for_drawing.color_regs_ecs[off] & 0x333) << 2;
		v |= v >> 2;
		dpix_val2 = xcolors[v];
		buf[dpix] = shsprite (dpix, spix_val1, merge_2pixel16 (dpix_val1, dpix_val2), spr);
		dpix++;
    }
    return spix;
}



static int NOINLINE linetoscr_32_shrink2_sh (int spix, int dpix, int stoppos, int spr)
{
    uae_u32 *buf = (uae_u32 *) xlinebuffer;

    while (dpix < stoppos) {
		uae_u32 spix_val1, spix_val2;
		uae_u16 v;
		int off;
		spix_val1 = pixdata.apixels[spix++];
		spix_val2 = pixdata.apixels[spix++];
		off = ((spix_val2 & 3) * 4) + (spix_val1 & 3) + ((spix_val1 | spix_val2) & 16);
		v = (colors_for_drawing.color_regs_ecs[off] & 0xccc) << 0;
		v |= v >> 2;
		buf[dpix] = shsprite (dpix, spix_val1, xcolors[v], spr);
		spix+=2;
		dpix++;
    }
    return spix;
}
static int NOINLINE linetoscr_32_shrink2f_sh (int spix, int dpix, int stoppos, int spr)
{
    uae_u32 *buf = (uae_u32 *) xlinebuffer;

    while (dpix < stoppos) {
		uae_u32 spix_val1, spix_val2, dpix_val1, dpix_val2, dpix_val3, dpix_val4;
		uae_u16 v;
		int off;
		spix_val1 = pixdata.apixels[spix++];
		spix_val2 = pixdata.apixels[spix++];
		off = ((spix_val2 & 3) * 4) + (spix_val1 & 3) + ((spix_val1 | spix_val2) & 16);
		v = (colors_for_drawing.color_regs_ecs[off] & 0xccc) << 0;
		v |= v >> 2;
		dpix_val1 = xcolors[v];
		v = (colors_for_drawing.color_regs_ecs[off] & 0x333) << 2;
		v |= v >> 2;
		dpix_val2 = xcolors[v];
		dpix_val3 = merge_2pixel32 (dpix_val1, dpix_val2);
		spix_val1 = pixdata.apixels[spix++];
		spix_val2 = pixdata.apixels[spix++];
		off = ((spix_val2 & 3) * 4) + (spix_val1 & 3) + ((spix_val1 | spix_val2) & 16);
		v = (colors_for_drawing.color_regs_ecs[off] & 0xccc) << 0;
		v |= v >> 2;
		dpix_val1 = xcolors[v];
		v = (colors_for_drawing.color_regs_ecs[off] & 0x333) << 2;
		v |= v >> 2;
		dpix_val2 = xcolors[v];
		dpix_val4 = merge_2pixel32 (dpix_val1, dpix_val2);
		buf[dpix] = shsprite (dpix, spix_val1, merge_2pixel32 (dpix_val3, dpix_val4), spr);
		dpix++;
    }
    return spix;
}
static int NOINLINE linetoscr_16_shrink2_sh (int spix, int dpix, int stoppos, int spr)
{
    uae_u16 *buf = (uae_u16 *) xlinebuffer;

    while (dpix < stoppos) {
		uae_u16 spix_val1, spix_val2;
		uae_u16 v;
		int off;
		spix_val1 = pixdata.apixels[spix++];
		spix_val2 = pixdata.apixels[spix++];
		off = ((spix_val2 & 3) * 4) + (spix_val1 & 3) + ((spix_val1 | spix_val2) & 16);
		v = (colors_for_drawing.color_regs_ecs[off] & 0xccc) << 0;
		v |= v >> 2;
		buf[dpix] = shsprite (dpix, spix_val1, xcolors[v], spr);
		spix+=2;
		dpix++;
    }
    return spix;
}
static int NOINLINE linetoscr_16_shrink2f_sh (int spix, int dpix, int stoppos, int spr)
{
    uae_u16 *buf = (uae_u16 *) xlinebuffer;

    while (dpix < stoppos) {
		uae_u16 spix_val1, spix_val2, dpix_val1, dpix_val2, dpix_val3, dpix_val4;
		uae_u16 v;
		int off;
		spix_val1 = pixdata.apixels[spix++];
		spix_val2 = pixdata.apixels[spix++];
		off = ((spix_val2 & 3) * 4) + (spix_val1 & 3) + ((spix_val1 | spix_val2) & 16);
		v = (colors_for_drawing.color_regs_ecs[off] & 0xccc) << 0;
		v |= v >> 2;
		dpix_val1 = xcolors[v];
		v = (colors_for_drawing.color_regs_ecs[off] & 0x333) << 2;
		v |= v >> 2;
		dpix_val2 = xcolors[v];
		dpix_val3 = merge_2pixel32 (dpix_val1, dpix_val2);
		spix_val1 = pixdata.apixels[spix++];
		spix_val2 = pixdata.apixels[spix++];
		off = ((spix_val2 & 3) * 4) + (spix_val1 & 3) + ((spix_val1 | spix_val2) & 16);
		v = (colors_for_drawing.color_regs_ecs[off] & 0xccc) << 0;
		v |= v >> 2;
		dpix_val1 = xcolors[v];
		v = (colors_for_drawing.color_regs_ecs[off] & 0x333) << 2;
		v |= v >> 2;
		dpix_val2 = xcolors[v];
		dpix_val4 = merge_2pixel32 (dpix_val1, dpix_val2);
		buf[dpix] = shsprite (dpix, spix_val1, merge_2pixel16 (dpix_val3, dpix_val4), spr);
		dpix++;
    }
    return spix;
}
#endif
/* note */
static void pfield_do_linetoscr (int start, int stop)
{
    xlinecheck(start, stop);
    if (issprites && (currprefs.chipset_mask & CSMASK_AGA)) {
	if (res_shift == 0) {
	    switch (gfxvidinfo.pixbytes) {
	    case 2: src_pixel = linetoscr_16_aga_spr (src_pixel, start, stop); break;
	    case 4: src_pixel = linetoscr_32_aga_spr (src_pixel, start, stop); break;
	    }
	} else if (res_shift == 2) {
	    switch (gfxvidinfo.pixbytes) {
	    case 2: src_pixel = linetoscr_16_stretch2_aga_spr (src_pixel, start, stop); break;
	    case 4: src_pixel = linetoscr_32_stretch2_aga_spr (src_pixel, start, stop); break;
	    }
	} else if (res_shift == 1) {
	    switch (gfxvidinfo.pixbytes) {
	    case 2: src_pixel = linetoscr_16_stretch1_aga_spr (src_pixel, start, stop); break;
	    case 4: src_pixel = linetoscr_32_stretch1_aga_spr (src_pixel, start, stop); break;
	    }
	} else if (res_shift == -1) {
	    if (currprefs.gfx_lores_mode) {
			switch (gfxvidinfo.pixbytes) {
			case 2: src_pixel = linetoscr_16_shrink1f_aga_spr (src_pixel, start, stop); break;
			case 4: src_pixel = linetoscr_32_shrink1f_aga_spr (src_pixel, start, stop); break;
			}
	    } else {
			switch (gfxvidinfo.pixbytes) {
			case 2: src_pixel = linetoscr_16_shrink1_aga_spr (src_pixel, start, stop); break;
			case 4: src_pixel = linetoscr_32_shrink1_aga_spr (src_pixel, start, stop); break;
			}
	    }
	} else if (res_shift == -2) {
	    if (currprefs.gfx_lores_mode) {
			switch (gfxvidinfo.pixbytes) {
			case 2: src_pixel = linetoscr_16_shrink2f_aga_spr (src_pixel, start, stop); break;
			case 4: src_pixel = linetoscr_32_shrink2f_aga_spr (src_pixel, start, stop); break;
			}
	    } else {
			switch (gfxvidinfo.pixbytes) {
			case 2: src_pixel = linetoscr_16_shrink2_aga_spr (src_pixel, start, stop); break;
			case 4: src_pixel = linetoscr_32_shrink2_aga_spr (src_pixel, start, stop); break;
			}
	    }
	}
    } else
#ifdef AGA
	    if (currprefs.chipset_mask & CSMASK_AGA) {
			if (res_shift == 0) {
			    switch (gfxvidinfo.pixbytes) {
			    case 2: src_pixel = linetoscr_16_aga (src_pixel, start, stop); break;
			    case 4: src_pixel = linetoscr_32_aga (src_pixel, start, stop); break;
			    }
			} else if (res_shift == 2) {
			    switch (gfxvidinfo.pixbytes) {
			    case 2: src_pixel = linetoscr_16_stretch2_aga (src_pixel, start, stop); break;
			    case 4: src_pixel = linetoscr_32_stretch2_aga (src_pixel, start, stop); break;
			    }
			} else if (res_shift == 1) {
			    switch (gfxvidinfo.pixbytes) {
			    case 2: src_pixel = linetoscr_16_stretch1_aga (src_pixel, start, stop); break;
			    case 4: src_pixel = linetoscr_32_stretch1_aga (src_pixel, start, stop); break;
			    }
			} else if (res_shift == -1) {
			    if (currprefs.gfx_lores_mode) {
					switch (gfxvidinfo.pixbytes) {
					case 2: src_pixel = linetoscr_16_shrink1f_aga (src_pixel, start, stop); break;
					case 4: src_pixel = linetoscr_32_shrink1f_aga (src_pixel, start, stop); break;
					}
			    } else {
					switch (gfxvidinfo.pixbytes) {
					case 2: src_pixel = linetoscr_16_shrink1_aga (src_pixel, start, stop); break;
					case 4: src_pixel = linetoscr_32_shrink1_aga (src_pixel, start, stop); break;
					}
			    }
			} else if (res_shift == -2) {
			    if (currprefs.gfx_lores_mode) {
					switch (gfxvidinfo.pixbytes) {
					case 2: src_pixel = linetoscr_16_shrink2f_aga (src_pixel, start, stop); break;
					case 4: src_pixel = linetoscr_32_shrink2f_aga (src_pixel, start, stop); break;
					}
			    } else {
					switch (gfxvidinfo.pixbytes) {
					case 2: src_pixel = linetoscr_16_shrink2_aga (src_pixel, start, stop); break;
					case 4: src_pixel = linetoscr_32_shrink2_aga (src_pixel, start, stop); break;
					}
			    }
			}
	    } else
#endif
#ifdef ECS_DENISE
		    if (ecsshres) {
				if (res_shift == 0) {
				    switch (gfxvidinfo.pixbytes) {
				    case 2: src_pixel = linetoscr_16_sh (src_pixel, start, stop, issprites); break;
				    case 4: src_pixel = linetoscr_32_sh (src_pixel, start, stop, issprites); break;
				    }
				} else if (res_shift == -1) {
				    if (currprefs.gfx_lores_mode) {
						switch (gfxvidinfo.pixbytes) {
						case 2: src_pixel = linetoscr_16_shrink1f_sh (src_pixel, start, stop, issprites); break;
						case 4: src_pixel = linetoscr_32_shrink1f_sh (src_pixel, start, stop, issprites); break;
						}
				    } else {
						switch (gfxvidinfo.pixbytes) {
						case 2: src_pixel = linetoscr_16_shrink1_sh (src_pixel, start, stop, issprites); break;
						case 4: src_pixel = linetoscr_32_shrink1_sh (src_pixel, start, stop, issprites); break;
						}
				    }
				} else if (res_shift == -2) {
				    if (currprefs.gfx_lores_mode) {
						switch (gfxvidinfo.pixbytes) {
						case 2: src_pixel = linetoscr_16_shrink2f_sh (src_pixel, start, stop, issprites); break;
						case 4: src_pixel = linetoscr_32_shrink2f_sh (src_pixel, start, stop, issprites); break;
						}
				    } else {
						switch (gfxvidinfo.pixbytes) {
						case 2: src_pixel = linetoscr_16_shrink2_sh (src_pixel, start, stop, issprites); break;
						case 4: src_pixel = linetoscr_32_shrink2_sh (src_pixel, start, stop, issprites); break;
						}
				    }
				}
		    } else
#endif
				if (issprites) {
					if (res_shift == 0) {
						switch (gfxvidinfo.pixbytes) {
						case 1: src_pixel = linetoscr_8_spr (src_pixel, start, stop); break;
						case 2: src_pixel = linetoscr_16_spr (src_pixel, start, stop); break;
						case 4: src_pixel = linetoscr_32_spr (src_pixel, start, stop); break;
						}
					} else if (res_shift == 2) {
						switch (gfxvidinfo.pixbytes) {
						case 1: src_pixel = linetoscr_8_stretch2_spr (src_pixel, start, stop); break;
						case 2: src_pixel = linetoscr_16_stretch2_spr (src_pixel, start, stop); break;
						case 4: src_pixel = linetoscr_32_stretch2_spr (src_pixel, start, stop); break;
						}
					} else if (res_shift == 1) {
						switch (gfxvidinfo.pixbytes) {
						case 1: src_pixel = linetoscr_8_stretch1_spr (src_pixel, start, stop); break;
						case 2: src_pixel = linetoscr_16_stretch1_spr (src_pixel, start, stop); break;
						case 4: src_pixel = linetoscr_32_stretch1_spr (src_pixel, start, stop); break;
						}
					} else if (res_shift == -1) {
						if (currprefs.gfx_lores_mode) {
							switch (gfxvidinfo.pixbytes) {
							case 1: src_pixel = linetoscr_8_shrink1f_spr (src_pixel, start, stop); break;
							case 2: src_pixel = linetoscr_16_shrink1f_spr (src_pixel, start, stop); break;
							case 4: src_pixel = linetoscr_32_shrink1f_spr (src_pixel, start, stop); break;
							}
						} else {
							switch (gfxvidinfo.pixbytes) {
							case 1: src_pixel = linetoscr_8_shrink1_spr (src_pixel, start, stop); break;
							case 2: src_pixel = linetoscr_16_shrink1_spr (src_pixel, start, stop); break;
							case 4: src_pixel = linetoscr_32_shrink1_spr (src_pixel, start, stop); break;
							}
						}
					}
				} else {
					if (res_shift == 0) {
						switch (gfxvidinfo.pixbytes) {
						case 1: src_pixel = linetoscr_8 (src_pixel, start, stop); break;
						case 2: src_pixel = linetoscr_16 (src_pixel, start, stop); break;
						case 4: src_pixel = linetoscr_32 (src_pixel, start, stop); break;
						}
					} else if (res_shift == 2) {
						switch (gfxvidinfo.pixbytes) {
						case 1: src_pixel = linetoscr_8_stretch2 (src_pixel, start, stop); break;
						case 2: src_pixel = linetoscr_16_stretch2 (src_pixel, start, stop); break;
						case 4: src_pixel = linetoscr_32_stretch2 (src_pixel, start, stop); break;
						}
					} else if (res_shift == 1) {
						switch (gfxvidinfo.pixbytes) {
						case 1: src_pixel = linetoscr_8_stretch1 (src_pixel, start, stop); break;
						case 2: src_pixel = linetoscr_16_stretch1 (src_pixel, start, stop); break;
						case 4: src_pixel = linetoscr_32_stretch1 (src_pixel, start, stop); break;
						}
					} else if (res_shift == -1) {
						if (currprefs.gfx_lores_mode) {
							switch (gfxvidinfo.pixbytes) {
							case 1: src_pixel = linetoscr_8_shrink1f (src_pixel, start, stop); break;
							case 2: src_pixel = linetoscr_16_shrink1f (src_pixel, start, stop); break;
							case 4: src_pixel = linetoscr_32_shrink1f (src_pixel, start, stop); break;
							}
						} else {
							switch (gfxvidinfo.pixbytes) {
							case 1: src_pixel = linetoscr_8_shrink1 (src_pixel, start, stop); break;
							case 2: src_pixel = linetoscr_16_shrink1 (src_pixel, start, stop); break;
							case 4: src_pixel = linetoscr_32_shrink1 (src_pixel, start, stop); break;
							}
						}
					}
				}

}

static void dummy_worker (int start, int stop)
{
}

static int ham_decode_pixel;
static unsigned int ham_lastcolor;

/* Decode HAM in the invisible portion of the display (left of VISIBLE_LEFT_BORDER),
 * but don't draw anything in.  This is done to prepare HAM_LASTCOLOR for later,
 * when decode_ham runs.
 *
 */
static void init_ham_decoding (void)
{
    int unpainted_amiga = unpainted;

    ham_decode_pixel = ham_src_pixel;
    ham_lastcolor = color_reg_get (&colors_for_drawing, 0);

    if (!bplham) {
		if (unpainted_amiga > 0) {
		    int pv = pixdata.apixels[ham_decode_pixel + unpainted_amiga - 1];
#ifdef AGA
		    if (currprefs.chipset_mask & CSMASK_AGA)
				ham_lastcolor = colors_for_drawing.color_regs_aga[pv ^ bplxor];
		    else
#endif
				ham_lastcolor = colors_for_drawing.color_regs_ecs[pv];
		}
#ifdef AGA /* note */ 
    } else if (currprefs.chipset_mask & CSMASK_AGA) {
		if (bplplanecnt >= 7) { /* AGA mode HAM8 */
		    while (unpainted_amiga-- > 0) {
				int pv = pixdata.apixels[ham_decode_pixel++] ^ bplxor;
				switch (pv & 0x3)
				{
			    case 0x0: ham_lastcolor = colors_for_drawing.color_regs_aga[pv >> 2]; break;
			    case 0x1: ham_lastcolor &= 0xFFFF03; ham_lastcolor |= (pv & 0xFC); break;
			    case 0x2: ham_lastcolor &= 0x03FFFF; ham_lastcolor |= (pv & 0xFC) << 16; break;
			    case 0x3: ham_lastcolor &= 0xFF03FF; ham_lastcolor |= (pv & 0xFC) << 8; break;
				}
		    }
		} else { /* AGA mode HAM6 */
		    while (unpainted_amiga-- > 0) {
				int pv = pixdata.apixels[ham_decode_pixel++] ^ bplxor;
				switch (pv & 0x30)
				{
			    case 0x00: ham_lastcolor = colors_for_drawing.color_regs_aga[pv]; break;
			    case 0x10: ham_lastcolor &= 0xFFFF00; ham_lastcolor |= (pv & 0xF) << 4; break;
			    case 0x20: ham_lastcolor &= 0x00FFFF; ham_lastcolor |= (pv & 0xF) << 20; break;
			    case 0x30: ham_lastcolor &= 0xFF00FF; ham_lastcolor |= (pv & 0xF) << 12; break;
				}
		    }
		}
#endif
    } else {
		/* OCS/ECS mode HAM6 */
		while (unpainted_amiga-- > 0) {
		    int pv = pixdata.apixels[ham_decode_pixel++];
		    switch (pv & 0x30)
		    {
	        case 0x00: ham_lastcolor = colors_for_drawing.color_regs_ecs[pv]; break;
	        case 0x10: ham_lastcolor &= 0xFF0; ham_lastcolor |= (pv & 0xF); break;
	        case 0x20: ham_lastcolor &= 0x0FF; ham_lastcolor |= (pv & 0xF) << 8; break;
	        case 0x30: ham_lastcolor &= 0xF0F; ham_lastcolor |= (pv & 0xF) << 4; break;
		    }
		}
    }
}

static void decode_ham (int pix, int stoppos)
{
    int todraw_amiga = res_shift_from_window (stoppos - pix);

    if (!bplham) {
		while (todraw_amiga-- > 0) {
		    int pv = pixdata.apixels[ham_decode_pixel];
#ifdef AGA
		    if (currprefs.chipset_mask & CSMASK_AGA)
				ham_lastcolor = colors_for_drawing.color_regs_aga[pv ^ bplxor];
	    	else
#endif
				ham_lastcolor = colors_for_drawing.color_regs_ecs[pv];

		    ham_linebuf[ham_decode_pixel++] = ham_lastcolor;
		}
#ifdef AGA
    } else if (currprefs.chipset_mask & CSMASK_AGA) {
		if (bplplanecnt >= 7) { /* AGA mode HAM8 */
		    while (todraw_amiga-- > 0) {
				int pv = pixdata.apixels[ham_decode_pixel] ^ bplxor;
				switch (pv & 0x3)
				{
			    case 0x0: ham_lastcolor = colors_for_drawing.color_regs_aga[pv >> 2]; break;
			    case 0x1: ham_lastcolor &= 0xFFFF03; ham_lastcolor |= (pv & 0xFC); break;
			    case 0x2: ham_lastcolor &= 0x03FFFF; ham_lastcolor |= (pv & 0xFC) << 16; break;
			    case 0x3: ham_lastcolor &= 0xFF03FF; ham_lastcolor |= (pv & 0xFC) << 8; break;
				}
				ham_linebuf[ham_decode_pixel++] = ham_lastcolor;
		    }
		} else { /* AGA mode HAM6 */
		    while (todraw_amiga-- > 0) {
				int pv = pixdata.apixels[ham_decode_pixel] ^ bplxor;
				switch (pv & 0x30)
				{
			    case 0x00: ham_lastcolor = colors_for_drawing.color_regs_aga[pv]; break;
			    case 0x10: ham_lastcolor &= 0xFFFF00; ham_lastcolor |= (pv & 0xF) << 4; break;
			    case 0x20: ham_lastcolor &= 0x00FFFF; ham_lastcolor |= (pv & 0xF) << 20; break;
			    case 0x30: ham_lastcolor &= 0xFF00FF; ham_lastcolor |= (pv & 0xF) << 12; break;
				}
				ham_linebuf[ham_decode_pixel++] = ham_lastcolor;
		    }
		}
#endif
    } else {
		/* OCS/ECS mode HAM6 */
		while (todraw_amiga-- > 0) {
		    int pv = pixdata.apixels[ham_decode_pixel];
		    switch (pv & 0x30)
		    {
	        case 0x00: ham_lastcolor = colors_for_drawing.color_regs_ecs[pv]; break;
	        case 0x10: ham_lastcolor &= 0xFF0; ham_lastcolor |= (pv & 0xF); break;
	        case 0x20: ham_lastcolor &= 0x0FF; ham_lastcolor |= (pv & 0xF) << 8; break;
	        case 0x30: ham_lastcolor &= 0xF0F; ham_lastcolor |= (pv & 0xF) << 4; break;
		    }
		    ham_linebuf[ham_decode_pixel++] = ham_lastcolor;
		}
    }
}

static void gen_pfield_tables (void)
{
    int i;

    /* For now, the AGA stuff is broken in the dual playfield case. We encode
     * sprites in dpf mode by ORing the pixel value with 0x80. To make dual
     * playfield rendering easy, the lookup tables contain are made linear for
     * values >= 128. That only works for OCS/ECS, though. */

    for (i = 0; i < 256; i++) {
		int plane1 = ((i >> 0) & 1) | ((i >> 1) & 2) | ((i >> 2) & 4) | ((i >> 3) & 8);
		int plane2 = ((i >> 1) & 1) | ((i >> 2) & 2) | ((i >> 3) & 4) | ((i >> 4) & 8);

		dblpf_2nd1[i] = plane1 == 0 && plane2 != 0;
		dblpf_2nd2[i] = plane2 != 0;

#ifdef AGA
		dblpf_ind1_aga[i] = plane1 == 0 ? plane2 : plane1;
		dblpf_ind2_aga[i] = plane2 == 0 ? plane1 : plane2;
#endif

		dblpf_ms1[i] = plane1 == 0 ? (plane2 == 0 ? 16 : 8) : 0;
		dblpf_ms2[i] = plane2 == 0 ? (plane1 == 0 ? 16 : 0) : 8;
		dblpf_ms[i] = i == 0 ? 16 : 8;

		if (plane2 > 0)
		    plane2 += 8;
		dblpf_ind1[i] = i >= 128 ? i & 0x7F : (plane1 == 0 ? plane2 : plane1);
		dblpf_ind2[i] = i >= 128 ? i & 0x7F : (plane2 == 0 ? plane1 : plane2);

		sprite_offs[i] = (i & 15) ? 0 : 2;

		clxtab[i] = ((((i & 3) && (i & 12)) << 9)
			     | (((i & 3) && (i & 48)) << 10)
			     | (((i & 3) && (i & 192)) << 11)
			     | (((i & 12) && (i & 48)) << 12)
			     | (((i & 12) && (i & 192)) << 13)
			     | (((i & 48) && (i & 192)) << 14));
	
    }

    memset (all_ones, 0xff, MAX_PIXELS_PER_LINE);

}

/* When looking at this function and the ones that inline it, bear in mind
   what an optimizing compiler will do with this code.  All callers of this
   function only pass in constant arguments (except for E).  This means
   that many of the if statements will go away completely after inlining.  */
STATIC_INLINE void draw_sprites_1 (struct sprite_entry *e, int dualpf, int has_attach)
{
    uae_u16 *buf = spixels + e->first_pixel;
    uae_u8 *stbuf = spixstate.bytes + e->first_pixel;
    int spr_pos, pos;

    buf -= e->pos;
    stbuf -= e->pos;

    spr_pos = e->pos + ((DIW_DDF_OFFSET - DISPLAY_LEFT_SHIFT) << sprite_buffer_res);

    if (spr_pos < sprite_first_x)
        sprite_first_x = spr_pos;

	for (pos = e->pos; pos < e->max; pos++, spr_pos++) {
		if (spr_pos >= 0 && spr_pos < MAX_PIXELS_PER_LINE) {
			spritepixels[spr_pos].data = buf[pos];
			spritepixels[spr_pos].stdata = stbuf[pos];
			spritepixels[spr_pos].attach = has_attach;
		}
    }

    if (spr_pos > sprite_last_x)
        sprite_last_x = spr_pos;
}

/* See comments above.  Do not touch if you don't know what's going on.
 * (We do _not_ want the following to be inlined themselves).  */
/* lores bitplane, lores sprites */
static void NOINLINE draw_sprites_normal_sp_nat (struct sprite_entry *e) { draw_sprites_1 (e, 0, 0); }
static void NOINLINE draw_sprites_normal_dp_nat (struct sprite_entry *e) { draw_sprites_1 (e, 1, 0); }
static void NOINLINE draw_sprites_normal_sp_at (struct sprite_entry *e) { draw_sprites_1 (e, 0, 1); }
static void NOINLINE draw_sprites_normal_dp_at (struct sprite_entry *e) { draw_sprites_1 (e, 1, 1); }

#ifdef AGA
/* not very optimized */
STATIC_INLINE void draw_sprites_aga (struct sprite_entry *e, int aga)
{
    draw_sprites_1 (e, bpldualpf, e->has_attached);
}
#endif

STATIC_INLINE void draw_sprites_ecs (struct sprite_entry *e)
{
    if (e->has_attached) {
		if (bpldualpf)
		    draw_sprites_normal_dp_at (e);
		else
		    draw_sprites_normal_sp_at (e);
    } else {
		if (bpldualpf)
		    draw_sprites_normal_dp_nat (e);
		else
		    draw_sprites_normal_sp_nat (e);
    }
}

#ifdef AGA
/* clear possible bitplane data outside DIW area */
static void clear_bitplane_border_aga (void)
{
    int len, shift = res_shift;
    uae_u8 v = 0;

    if (shift < 0) {
		shift = -shift;
		len = (real_playfield_start - playfield_start) << shift;
		memset (pixdata.apixels + pixels_offset + (playfield_start << shift), v, len);
		len = (playfield_end - real_playfield_end) << shift;
		memset (pixdata.apixels + pixels_offset + (real_playfield_end << shift), v, len);
    } else {
		len = (real_playfield_start - playfield_start) >> shift;
		memset (pixdata.apixels + pixels_offset + (playfield_start >> shift), v, len);
		len = (playfield_end - real_playfield_end) >> shift;
		memset (pixdata.apixels + pixels_offset + (real_playfield_end >> shift), v, len);
    }
}
#endif

/* emulate OCS/ECS only undocumented "SWIV" hardware feature */
static void weird_bitplane_fix (void)
{
    int i;
    int sh = lores_shift;
    uae_u8 *p = pixdata.apixels + pixels_offset;

    for (i = playfield_start >> sh; i < playfield_end >> sh; i++) {
        if (p[i] > 16)
    	    p[i] = 16;
    }
}

#define MERGE(a,b,mask,shift) do {\
    uae_u32 tmp = mask & (a ^ (b >> shift)); \
    a ^= tmp; \
    b ^= (tmp << shift); \
} while (0)

#define GETLONG(P) (*(uae_u32 *)P)

/* We use the compiler's inlining ability to ensure that PLANES is in effect a compile time
   constant.  That will cause some unnecessary code to be optimized away.
   Don't touch this if you don't know what you are doing.  */
STATIC_INLINE void pfield_doline_1 (uae_u32 *pixels, int wordcount, int planes)
{
    while (wordcount-- > 0) {
		uae_u32 b0, b1, b2, b3, b4, b5, b6, b7;

		b0 = 0, b1 = 0, b2 = 0, b3 = 0, b4 = 0, b5 = 0, b6 = 0, b7 = 0;
		switch (planes) {
#ifdef AGA
	case 8: b0 = GETLONG ((uae_u32 *)real_bplpt[7]); real_bplpt[7] += 4;
	case 7: b1 = GETLONG ((uae_u32 *)real_bplpt[6]); real_bplpt[6] += 4;
#endif
	case 6: b2 = GETLONG ((uae_u32 *)real_bplpt[5]); real_bplpt[5] += 4;
	case 5: b3 = GETLONG ((uae_u32 *)real_bplpt[4]); real_bplpt[4] += 4;
	case 4: b4 = GETLONG ((uae_u32 *)real_bplpt[3]); real_bplpt[3] += 4;
	case 3: b5 = GETLONG ((uae_u32 *)real_bplpt[2]); real_bplpt[2] += 4;
	case 2: b6 = GETLONG ((uae_u32 *)real_bplpt[1]); real_bplpt[1] += 4;
	case 1: b7 = GETLONG ((uae_u32 *)real_bplpt[0]); real_bplpt[0] += 4;
	}

		MERGE (b0, b1, 0x55555555, 1);
		MERGE (b2, b3, 0x55555555, 1);
		MERGE (b4, b5, 0x55555555, 1);
		MERGE (b6, b7, 0x55555555, 1);

		MERGE (b0, b2, 0x33333333, 2);
		MERGE (b1, b3, 0x33333333, 2);
		MERGE (b4, b6, 0x33333333, 2);
		MERGE (b5, b7, 0x33333333, 2);

		MERGE (b0, b4, 0x0f0f0f0f, 4);
		MERGE (b1, b5, 0x0f0f0f0f, 4);
		MERGE (b2, b6, 0x0f0f0f0f, 4);
		MERGE (b3, b7, 0x0f0f0f0f, 4);

		MERGE (b0, b1, 0x00ff00ff, 8);
		MERGE (b2, b3, 0x00ff00ff, 8);
		MERGE (b4, b5, 0x00ff00ff, 8);
		MERGE (b6, b7, 0x00ff00ff, 8);
	
		MERGE (b0, b2, 0x0000ffff, 16);
		do_put_mem_long (pixels, b0);
		do_put_mem_long (pixels + 4, b2);
		MERGE (b1, b3, 0x0000ffff, 16);
		do_put_mem_long (pixels + 2, b1);
		do_put_mem_long (pixels + 6, b3);
		MERGE (b4, b6, 0x0000ffff, 16);
		do_put_mem_long (pixels + 1, b4);
		do_put_mem_long (pixels + 5, b6);
		MERGE (b5, b7, 0x0000ffff, 16);
		do_put_mem_long (pixels + 3, b5);
		do_put_mem_long (pixels + 7, b7);
		pixels += 8;
    }
}

/* See above for comments on inlining.  These functions should _not_
   be inlined themselves.  */
static void NOINLINE pfield_doline_n1 (uae_u32 *data, int count) { pfield_doline_1 (data, count, 1); }
static void NOINLINE pfield_doline_n2 (uae_u32 *data, int count) { pfield_doline_1 (data, count, 2); }
static void NOINLINE pfield_doline_n3 (uae_u32 *data, int count) { pfield_doline_1 (data, count, 3); }
static void NOINLINE pfield_doline_n4 (uae_u32 *data, int count) { pfield_doline_1 (data, count, 4); }
static void NOINLINE pfield_doline_n5 (uae_u32 *data, int count) { pfield_doline_1 (data, count, 5); }
static void NOINLINE pfield_doline_n6 (uae_u32 *data, int count) { pfield_doline_1 (data, count, 6); }
#ifdef AGA
static void NOINLINE pfield_doline_n7 (uae_u32 *data, int count) { pfield_doline_1 (data, count, 7); }
static void NOINLINE pfield_doline_n8 (uae_u32 *data, int count) { pfield_doline_1 (data, count, 8); }
#endif

static void pfield_doline (int lineno)
{
    int wordcount = dp_for_drawing->plflinelen;
    uae_u32 *data = pixdata.apixels_l + MAX_PIXELS_PER_LINE / 4;

#ifdef SMART_UPDATE
#define DATA_POINTER(n) ((debug_bpl_mask & (1 << n)) ? (line_data[lineno] + (n) * MAX_WORDS_PER_LINE * 2) : (debug_bpl_mask_one ? all_ones : all_zeros))
    real_bplpt[0] = DATA_POINTER (0);
    real_bplpt[1] = DATA_POINTER (1);
    real_bplpt[2] = DATA_POINTER (2);
    real_bplpt[3] = DATA_POINTER (3);
    real_bplpt[4] = DATA_POINTER (4);
    real_bplpt[5] = DATA_POINTER (5);
#ifdef AGA
    real_bplpt[6] = DATA_POINTER (6);
    real_bplpt[7] = DATA_POINTER (7);
#endif
#endif

    switch (bplplanecnt) {
    default: break;
    case 0: memset (data, 0, wordcount * 32); break;
    case 1: pfield_doline_n1 (data, wordcount); break;
    case 2: pfield_doline_n2 (data, wordcount); break;
    case 3: pfield_doline_n3 (data, wordcount); break;
    case 4: pfield_doline_n4 (data, wordcount); break;
    case 5: pfield_doline_n5 (data, wordcount); break;
    case 6: pfield_doline_n6 (data, wordcount); break;
#ifdef AGA
    case 7: pfield_doline_n7 (data, wordcount); break;
    case 8: pfield_doline_n8 (data, wordcount); break;
#endif
    }
}

void init_row_map (void)
{
    int i, j;
    if (gfxvidinfo.height > MAX_VIDHEIGHT) {
		write_log ("Resolution too high, aborting\n");
		abort ();
    }
    j = 0;
    for (i = gfxvidinfo.height; i < MAX_VIDHEIGHT + 1; i++)
		row_map[i] = row_tmp;
    for (i = 0; i < gfxvidinfo.height; i++, j += gfxvidinfo.rowbytes)
		row_map[i] = gfxvidinfo.bufmem + j;
}

static void init_aspect_maps (void)
{
    int i, maxl;

    if (gfxvidinfo.height == 0)
		/* Do nothing if the gfx driver hasn't initialized the screen yet */
		return;

    linedbld = linedbl = currprefs.gfx_linedbl;
    if (doublescan > 0 && interlace_seen <= 0) {
		linedbl = 0;
		linedbld = 1;
    }

    if (native2amiga_line_map)
		xfree (native2amiga_line_map);
    if (amiga2aspect_line_map)
		xfree (amiga2aspect_line_map);

    /* At least for this array the +1 is necessary. */
	amiga2aspect_line_map = xmalloc (int, (MAXVPOS + 1) * 2 + 1);
	native2amiga_line_map = xmalloc (int, gfxvidinfo.height);

    maxl = (MAXVPOS + 1) * (linedbld ? 2 : 1);
    min_ypos_for_screen = minfirstline << (linedbl ? 1 : 0);
    max_drawn_amiga_line = -1;
    for (i = 0; i < maxl; i++) {
		int v = (int) (i - min_ypos_for_screen);
		if (v >= gfxvidinfo.height && max_drawn_amiga_line == -1)
		    max_drawn_amiga_line = i - min_ypos_for_screen;
		if (i < min_ypos_for_screen || v >= gfxvidinfo.height)
		    v = -1;
		amiga2aspect_line_map[i] = v;
    }
    if (linedbl)
		max_drawn_amiga_line >>= 1;

    if (currprefs.gfx_ycenter && !currprefs.gfx_filter_autoscale) {
		/* @@@ verify maxvpos vs. MAXVPOS */
		extra_y_adjust = (gfxvidinfo.height - (maxvpos_nom << (linedbl ? 1 : 0))) >> 1;
		if (extra_y_adjust < 0)
		    extra_y_adjust = 0;
    }

	for (i = gfxvidinfo.height; i--; )
		native2amiga_line_map[i] = -1;

    for (i = maxl - 1; i >= min_ypos_for_screen; i--) {
		int j;
		if (amiga2aspect_line_map[i] == -1)
			    continue;
		for (j = amiga2aspect_line_map[i]; j < gfxvidinfo.height && native2amiga_line_map[j] == -1; j++)
			    native2amiga_line_map[j] = i >> (linedbl ? 1 : 0);
    }
}

/*
 * A raster line has been built in the graphics buffer. Tell the graphics code
 * to do anything necessary to display it.
 */
static void do_flush_line_1 (int lineno)
{
    if (lineno < first_drawn_line)
		first_drawn_line = lineno;
    if (lineno > last_drawn_line)
		last_drawn_line = lineno;

    if (gfxvidinfo.maxblocklines == 0)
		flush_line (lineno);
    else {
		if ((last_block_line + 2) < lineno) {
		    if (first_block_line != NO_BLOCK)
				flush_block (first_block_line, last_block_line);
		    first_block_line = lineno;
		}
		last_block_line = lineno;
		if (last_block_line - first_block_line >= gfxvidinfo.maxblocklines) {
		    flush_block (first_block_line, last_block_line);
		    first_block_line = last_block_line = NO_BLOCK;
		}
    }
}

STATIC_INLINE void do_flush_line (int lineno)
{
    do_flush_line_1 (lineno);
}

/*
 * One drawing frame has been finished. Tell the graphics code about it.
 * Note that the actual flush_screen() call is a no-op for all reasonable
 * systems.
 */

STATIC_INLINE void do_flush_screen (int start, int stop)
{
    /* TODO: this flush operation is executed outside locked state!
       Should be corrected.
       (sjo 26.9.99) */

    xlinecheck (start, stop);
    if (gfxvidinfo.maxblocklines != 0 && first_block_line != NO_BLOCK) {
		flush_block (first_block_line, last_block_line);
    }
    unlockscr ();
    if (start <= stop)
		flush_screen (start, stop);
	else if (currprefs.gfx_afullscreen && currprefs.gfx_avsync)
		flush_screen (0, 0); /* vsync mode */
}

/* We only save hardware registers during the hardware frame. Now, when
 * drawing the frame, we expand the data into a slightly more useful
 * form. */
static void pfield_expand_dp_bplcon (void)
{
    int brdblank_2;
    static int b2;

    bplres = dp_for_drawing->bplres;
    bplplanecnt = dp_for_drawing->nr_planes;
    bplham = dp_for_drawing->ham_seen;
	bplehb = dp_for_drawing->ehb_seen;
	if ((currprefs.chipset_mask & CSMASK_AGA) && (dp_for_drawing->bplcon2 & 0x0200))
		bplehb = 0;
    issprites = dip_for_drawing->nr_sprites;
#ifdef ECS_DENISE
    ecsshres = bplres == RES_SUPERHIRES && (currprefs.chipset_mask & CSMASK_ECS_DENISE) && !(currprefs.chipset_mask & CSMASK_AGA);
#endif

    if (bplres > 0)
		frame_res = 1;
    if (bplres > 0)
		can_use_lores = 0;

    plf1pri = dp_for_drawing->bplcon2 & 7;
    plf2pri = (dp_for_drawing->bplcon2 >> 3) & 7;
    plf_sprite_mask = 0xFFFF0000 << (4 * plf2pri);
    plf_sprite_mask |= (0x0000FFFF << (4 * plf1pri)) & 0xFFFF;
    bpldualpf = (dp_for_drawing->bplcon0 & 0x400) == 0x400;
    bpldualpfpri = (dp_for_drawing->bplcon2 & 0x40) == 0x40;

#ifdef ECS_DENISE
    brdblank_2 = (currprefs.chipset_mask & CSMASK_ECS_DENISE) && (dp_for_drawing->bplcon0 & 1) && (dp_for_drawing->bplcon3 & 0x20);
    if (brdblank_2 != brdblank)
		brdblank_changed = 1;
    brdblank = brdblank_2;
#endif

#ifdef AGA
    bpldualpf2of = (dp_for_drawing->bplcon3 >> 10) & 7;
    sbasecol[0] = ((dp_for_drawing->bplcon4 >> 4) & 15) << 4;
    sbasecol[1] = ((dp_for_drawing->bplcon4 >> 0) & 15) << 4;
    brdsprt = !brdblank && (currprefs.chipset_mask & CSMASK_AGA) && (dp_for_drawing->bplcon0 & 1) && (dp_for_drawing->bplcon3 & 0x02);
    bplxor = dp_for_drawing->bplcon4 >> 8;
#endif
}

static int isham (uae_u16 bplcon0)
{
    int p = GET_PLANES (bplcon0);
    if (!(bplcon0 & 0x800))
		return 0;
    if (currprefs.chipset_mask & CSMASK_AGA) {
		// AGA only has 6 or 8 plane HAM
		if (p == 6 || p == 8)
		    return 1;
    } else {
		// OCS/ECS also supports 5 plane HAM
		if (GET_RES_DENISE (bplcon0) > 0)
		    return 0;
		if (p >= 5)
		    return 1;
    }
    return 0;
}

static void pfield_expand_dp_bplcon2 (int regno, int v)
{
    regno -= 0x1000;
    switch (regno)
    {
	case 0x100:
		dp_for_drawing->bplcon0 = v;
		dp_for_drawing->bplres = GET_RES_DENISE (v);
		dp_for_drawing->nr_planes = GET_PLANES (v);
		dp_for_drawing->ham_seen = isham (v);
		break;
	case 0x104:
		dp_for_drawing->bplcon2 = v;
		break;
#ifdef ECS_DENISE
	case 0x106:
		dp_for_drawing->bplcon3 = v;
		break;
#endif
#ifdef AGA
	case 0x10c:
		dp_for_drawing->bplcon4 = v;
		break;
#endif
    }
    pfield_expand_dp_bplcon ();
    res_shift = lores_shift - bplres;
}

static int drawing_color_matches;
static enum { color_match_acolors, color_match_full } color_match_type;

/* Set up colors_for_drawing to the state at the beginning of the currently drawn
   line.  Try to avoid copying color tables around whenever possible.  */
static void adjust_drawing_colors (int ctable, int need_full)
{
    if (drawing_color_matches != ctable) {
		if (need_full) {
		    color_reg_cpy (&colors_for_drawing, curr_color_tables + ctable);
		    color_match_type = color_match_full;
		} else {
		    memcpy (colors_for_drawing.acolors, curr_color_tables[ctable].acolors,
				sizeof colors_for_drawing.acolors);
		    color_match_type = color_match_acolors;
		}
		drawing_color_matches = ctable;
    } else if (need_full && color_match_type != color_match_full) {
		color_reg_cpy (&colors_for_drawing, &curr_color_tables[ctable]);
		color_match_type = color_match_full;
    }
}

STATIC_INLINE void do_color_changes (line_draw_func worker_border, line_draw_func worker_pfield)
{
    int i;
    int lastpos = visible_left_border;
    int endpos = visible_left_border + gfxvidinfo.width;
	int diff = 1 << lores_shift;

    for (i = dip_for_drawing->first_color_change; i <= dip_for_drawing->last_color_change; i++) {
		int regno = curr_color_changes[i].regno;
		unsigned int value = curr_color_changes[i].value;
		int nextpos, nextpos_in_range;

		if (i == dip_for_drawing->last_color_change)
		    nextpos = endpos;
		else
			nextpos = coord_hw_to_window_x (curr_color_changes[i].linepos);

		nextpos_in_range = nextpos;
		if (nextpos > endpos)
		    nextpos_in_range = endpos;

		if (nextpos_in_range > lastpos) {
		    if (lastpos < playfield_start) {
				int t = nextpos_in_range <= playfield_start ? nextpos_in_range : playfield_start;
				(*worker_border) (lastpos, t);
				lastpos = t;
		    }
		}
		if (nextpos_in_range > lastpos) {
		    if (lastpos >= playfield_start && lastpos < playfield_end) {
				int t = nextpos_in_range <= playfield_end ? nextpos_in_range : playfield_end;
				(*worker_pfield) (lastpos, t);
				lastpos = t;
		    }
		}
		if (nextpos_in_range > lastpos) {
		    if (lastpos >= playfield_end)
				(*worker_border) (lastpos, nextpos_in_range);
		    lastpos = nextpos_in_range;
		}
		if (i != dip_for_drawing->last_color_change) {
		    if (regno >= 0x1000) {
				pfield_expand_dp_bplcon2 (regno, value);
		    } else {
				color_reg_set (&colors_for_drawing, regno, value);
				colors_for_drawing.acolors[regno] = getxcolor (value);
		    }
		}
		if (lastpos >= endpos)
		    break;
    }
}

enum double_how {
    dh_buf,
    dh_line,
    dh_emerg
};

static void pfield_draw_line (int lineno, int gfx_ypos, int follow_ypos)
{
    static int warned = 0;
    int border = 0;
    int do_double = 0;
    enum double_how dh;

    dp_for_drawing = line_decisions + lineno;
    dip_for_drawing = curr_drawinfo + lineno;

    switch (linestate[lineno])
    {
    case LINE_REMEMBERED_AS_PREVIOUS:
		if (!warned)
			write_log ("Shouldn't get here... this is a bug.\n"), warned++;
	return;

    case LINE_BLACK:
		linestate[lineno] = LINE_REMEMBERED_AS_BLACK;
		border = 2;
		break;

    case LINE_REMEMBERED_AS_BLACK:
		return;

	case LINE_AS_PREVIOUS:
		dp_for_drawing--;
		dip_for_drawing--;
		linestate[lineno] = LINE_DONE_AS_PREVIOUS;
		if (dp_for_drawing->plfleft == -1)
			border = 1;
		break;

    case LINE_DONE_AS_PREVIOUS:
		/* fall through */
    case LINE_DONE:
		return;

    case LINE_DECIDED_DOUBLE:
		if (follow_ypos != -1) {
		    do_double = 1;
		    linestate[lineno + 1] = LINE_DONE_AS_PREVIOUS;
		}

	/* fall through */
    default:
		if (dp_for_drawing->plfleft == -1)
		    border = 1;
		linestate[lineno] = LINE_DONE;
		break;
    }

    dh = dh_line;
    xlinebuffer = gfxvidinfo.linemem;
    if (xlinebuffer == 0 && do_double
	&& (border == 0 || (border != 1 && dip_for_drawing->nr_color_changes > 0)))
	xlinebuffer = gfxvidinfo.emergmem, dh = dh_emerg;
    if (xlinebuffer == 0)
		xlinebuffer = row_map[gfx_ypos], dh = dh_buf;
    xlinebuffer -= linetoscr_x_adjust_bytes;

    if (border == 0) {

		pfield_expand_dp_bplcon ();
		pfield_init_linetoscr ();
		pfield_doline (lineno);

		adjust_drawing_colors (dp_for_drawing->ctable, dp_for_drawing->ham_seen || bplehb || ecsshres);

		/* The problem is that we must call decode_ham() BEFORE we do the
		   sprites. */
		if (dp_for_drawing->ham_seen) {
		    init_ham_decoding ();
		    if (dip_for_drawing->nr_color_changes == 0) {
				/* The easy case: need to do HAM decoding only once for the
				 * full line. */
				decode_ham (visible_left_border, visible_right_border);
		    } else /* Argh. */ {
				do_color_changes (dummy_worker, decode_ham);
				adjust_drawing_colors (dp_for_drawing->ctable, dp_for_drawing->ham_seen || bplehb);
		    }
		    bplham = dp_for_drawing->ham_at_start;
		}

		if (plf2pri > 5 && bplplanecnt == 5 && !(currprefs.chipset_mask & CSMASK_AGA))
		    weird_bitplane_fix ();

		if (dip_for_drawing->nr_sprites) {
		    int i;
#ifdef AGA
		    if (brdsprt)
				clear_bitplane_border_aga ();
#endif
		    for (i = 0; i < dip_for_drawing->nr_sprites; i++) {
#ifdef AGA
			if (currprefs.chipset_mask & CSMASK_AGA)
			    draw_sprites_aga (curr_sprite_entries + dip_for_drawing->first_sprite_entry + i, 1);
			else
#endif
			    draw_sprites_ecs (curr_sprite_entries + dip_for_drawing->first_sprite_entry + i);
		    }
		}

		do_color_changes (pfield_do_fill_line, pfield_do_linetoscr);

		if (dh == dh_emerg)
		    memcpy (row_map[gfx_ypos], xlinebuffer + linetoscr_x_adjust_bytes, gfxvidinfo.pixbytes * gfxvidinfo.width);

		do_flush_line (gfx_ypos);
		if (do_double) {
		    if (dh == dh_emerg)
				memcpy (row_map[follow_ypos], xlinebuffer + linetoscr_x_adjust_bytes, gfxvidinfo.pixbytes * gfxvidinfo.width);
		    else if (dh == dh_buf)
				memcpy (row_map[follow_ypos], row_map[gfx_ypos], gfxvidinfo.pixbytes * gfxvidinfo.width);
		    do_flush_line (follow_ypos);
		}

    } else if (border == 1) {
		int dosprites = 0;

		adjust_drawing_colors (dp_for_drawing->ctable, 0);

#ifdef AGA /* this makes things complex.. */
		if (brdsprt && dip_for_drawing->nr_sprites > 0) {
		    dosprites = 1;
		    pfield_expand_dp_bplcon ();
		    pfield_init_linetoscr ();
		    memset (pixdata.apixels + MAX_PIXELS_PER_LINE, brdblank ? 0 : colors_for_drawing.acolors[0], MAX_PIXELS_PER_LINE);
		}
#endif

		if (!dosprites && dip_for_drawing->nr_color_changes == 0) {
		    fill_line ();
		    do_flush_line (gfx_ypos);

		    if (do_double) {
				if (dh == dh_buf) {
				    xlinebuffer = row_map[follow_ypos] - linetoscr_x_adjust_bytes;
				    fill_line ();
				}
				/* If dh == dh_line, do_flush_line will re-use the rendered line
				 * from linemem.  */
				do_flush_line (follow_ypos);
		    }
		    return;
		}


		if (dosprites) {

		    int i;
		    for (i = 0; i < dip_for_drawing->nr_sprites; i++)
				draw_sprites_aga (curr_sprite_entries + dip_for_drawing->first_sprite_entry + i, 1);
		    do_color_changes (pfield_do_fill_line, pfield_do_linetoscr);

		} else {

		    playfield_start = visible_right_border;
		    playfield_end = visible_right_border;
		    do_color_changes (pfield_do_fill_line, pfield_do_fill_line);

		}

		if (dh == dh_emerg)
		    memcpy (row_map[gfx_ypos], xlinebuffer + linetoscr_x_adjust_bytes, gfxvidinfo.pixbytes * gfxvidinfo.width);

		do_flush_line (gfx_ypos);
		if (do_double) {
		    if (dh == dh_emerg)
				memcpy (row_map[follow_ypos], xlinebuffer + linetoscr_x_adjust_bytes, gfxvidinfo.pixbytes * gfxvidinfo.width);
		    else if (dh == dh_buf)
	    /* If dh == dh_line, do_flush_line will re-use the rendered line
	     * from linemem.  */
				memcpy (row_map[follow_ypos], row_map[gfx_ypos], gfxvidinfo.pixbytes * gfxvidinfo.width);
		    do_flush_line (follow_ypos);
		}

    } else {

		xcolnr tmp = colors_for_drawing.acolors[0];
		colors_for_drawing.acolors[0] = getxcolor (0);
		fill_line ();
		do_flush_line (gfx_ypos);
		colors_for_drawing.acolors[0] = tmp;

    }
}

static void center_image (void)
{
    int prev_x_adjust = visible_left_border;
    int prev_y_adjust = thisframe_y_adjust;
    int tmp;

    if (currprefs.gfx_xcenter && !currprefs.gfx_filter_autoscale) {
		int w = gfxvidinfo.width;

		if (max_diwstop - min_diwstart < w && currprefs.gfx_xcenter == 2)
		    /* Try to center. */
		    visible_left_border = (max_diwstop - min_diwstart - w) / 2 + min_diwstart;
		else
		    visible_left_border = max_diwstop - w - (max_diwstop - min_diwstart - w) / 2;
		visible_left_border &= ~((xshift (1, lores_shift)) - 1);

		/* Would the old value be good enough? If so, leave it as it is if we want to
		 * be clever. */
		if (currprefs.gfx_xcenter == 2) {
		    if (visible_left_border < prev_x_adjust && prev_x_adjust < min_diwstart && min_diwstart - visible_left_border <= 32)
				visible_left_border = prev_x_adjust;
		}
    } else {
		if (beamcon0 & 0x80) {
		    int w = gfxvidinfo.width;
		    if (max_diwstop - min_diwstart < w)
				visible_left_border = (max_diwstop - min_diwstart - w) / 2 + min_diwstart;
		    else
				visible_left_border = max_diwstop - w - (max_diwstop - min_diwstart - w) / 2;
		} else {
			visible_left_border = max_diwlastword - gfxvidinfo.width;
		}
    }
    if (currprefs.gfx_xcenter_pos >= 0) {
		int val = currprefs.gfx_xcenter_pos >> RES_MAX;
#if 0
		if (currprefs.gfx_xcenter_size > 0) {
		    int diff = ((gfxvidinfo.width << (RES_MAX - currprefs.gfx_resolution)) - currprefs.gfx_xcenter_size) / 1;
		    write_log ("%d %d\n", currprefs.gfx_xcenter_size, gfxvidinfo.width);
		    val -= diff >> RES_MAX;
		}
#endif
		if (val < 56)
		    val = 56;
		visible_left_border = val + (DIW_DDF_OFFSET << currprefs.gfx_resolution) - (DISPLAY_LEFT_SHIFT * 2 - (DISPLAY_LEFT_SHIFT << currprefs.gfx_resolution));
    }

    if (visible_left_border > max_diwlastword - 32)
		visible_left_border = max_diwlastword - 32;
    if (visible_left_border < 0)
		visible_left_border = 0;
    visible_left_border &= ~((xshift (1, lores_shift)) - 1);

    linetoscr_x_adjust_bytes = visible_left_border * gfxvidinfo.pixbytes;

    visible_right_border = visible_left_border + gfxvidinfo.width;
    if (visible_right_border > max_diwlastword)
		visible_right_border = max_diwlastword;

    thisframe_y_adjust = minfirstline;
    if (currprefs.gfx_ycenter && thisframe_first_drawn_line != -1 && !currprefs.gfx_filter_autoscale) {

		if (thisframe_last_drawn_line - thisframe_first_drawn_line < max_drawn_amiga_line && currprefs.gfx_ycenter == 2)
		    thisframe_y_adjust = (thisframe_last_drawn_line - thisframe_first_drawn_line - max_drawn_amiga_line) / 2 + thisframe_first_drawn_line;
		else
		    thisframe_y_adjust = thisframe_first_drawn_line + ((thisframe_last_drawn_line - thisframe_first_drawn_line) - max_drawn_amiga_line) / 2;

		/* Would the old value be good enough? If so, leave it as it is if we want to
		 * be clever. */
		if (currprefs.gfx_ycenter == 2) {
		    if (thisframe_y_adjust != prev_y_adjust
				&& prev_y_adjust <= thisframe_first_drawn_line
				&& prev_y_adjust + max_drawn_amiga_line > thisframe_last_drawn_line)
				thisframe_y_adjust = prev_y_adjust;
		}
    }
    if (currprefs.gfx_ycenter_pos >= 0) {
		thisframe_y_adjust = currprefs.gfx_ycenter_pos >> 1;
#if 0
		if (currprefs.gfx_ycenter_size > 0) {
		    int diff = (currprefs.gfx_ycenter_size - (gfxvidinfo.height << (linedbl ? 0 : 1))) / 2;
		    thisframe_y_adjust += diff >> 1;
		}
#endif
		if (thisframe_y_adjust + max_drawn_amiga_line > 2 * (int)maxvpos_nom)
			thisframe_y_adjust = 2 * maxvpos_nom - max_drawn_amiga_line;
		if (thisframe_y_adjust < 0)
		    thisframe_y_adjust = 0;
    } else {
		/* Make sure the value makes sense */
		if (thisframe_y_adjust + max_drawn_amiga_line > (int)maxvpos_nom)
			thisframe_y_adjust = (int)maxvpos_nom - max_drawn_amiga_line;
		if (thisframe_y_adjust < (int)minfirstline)
		    thisframe_y_adjust = (int)minfirstline;
    }
    thisframe_y_adjust_real = thisframe_y_adjust << (linedbl ? 1 : 0);
	tmp = (maxvpos_nom - thisframe_y_adjust) << (linedbl ? 1 : 0);
    if (tmp != max_ypos_thisframe) {
		last_max_ypos = tmp;
		if (last_max_ypos < 0)
		    last_max_ypos = 0;
    }
    max_ypos_thisframe = tmp;

    /* @@@ interlace_seen used to be (bplcon0 & 4), but this is probably
     * better.  */
    if (prev_x_adjust != visible_left_border || prev_y_adjust != thisframe_y_adjust)
		frame_redraw_necessary |= (interlace_seen > 0 && linedbl) ? 2 : 1;

    max_diwstop = 0;
    min_diwstart = 10000;
}

#define FRAMES_UNTIL_RES_SWITCH 5
static int frame_res_cnt;
static void init_drawing_frame (void)
{
    int i, maxline;
    static int frame_res_old;

	if (FRAMES_UNTIL_RES_SWITCH > 0 && frame_res_old == frame_res * 2 + frame_res_lace) {
		frame_res_cnt--;
		if (frame_res_cnt == 0) {
			int ar = currprefs.gfx_autoresolution;
			int m = frame_res * 2 + frame_res_lace;
			struct wh *dst = currprefs.gfx_afullscreen ? &changed_prefs.gfx_size_fs : &changed_prefs.gfx_size_win;
			while (m < 4) {
				struct wh *src = currprefs.gfx_afullscreen ? &currprefs.gfx_size_fs_xtra[m] : &currprefs.gfx_size_win_xtra[m];
				if ((src->width > 0 && src->height > 0) || (ar && currprefs.gfx_filter > 0)) {
					int nr = (m & 2) == 0 ? 0 : 1;
					int nl = (m & 1) == 0 ? 0 : 1;
					if (changed_prefs.gfx_resolution != nr || changed_prefs.gfx_linedbl != nl) {
						changed_prefs.gfx_resolution = nr;
						changed_prefs.gfx_linedbl = nl;
						write_log ("RES -> %d LINE -> %d\n", nr, nl);
						config_changed = 1;
						if (ar) {
							changed_prefs.gfx_filter_horiz_zoom_mult = (nr + 1) * 500;
							changed_prefs.gfx_filter_vert_zoom_mult = (nl + 1) * 500;
						}
					}
					if (src->width > 0 && src->height > 0) {
						if (memcmp (dst, src, sizeof *dst)) {
							*dst = *src;
							config_changed = 1;
						}
					}
					break;
				}
				m++;
			}
			frame_res_cnt = FRAMES_UNTIL_RES_SWITCH;
		}
    } else {
		frame_res_old = frame_res * 2 + frame_res_lace;
		frame_res_cnt = FRAMES_UNTIL_RES_SWITCH;
    }
    frame_res = 0;
    frame_res_lace = 0;

    if (can_use_lores > AUTO_LORES_FRAMES && 0) {
		lores_factor = 1;
		lores_shift = 0;
    } else {
		can_use_lores++;
		lores_reset ();
    }

    init_hardware_for_drawing_frame ();

    if (thisframe_first_drawn_line == -1)
		thisframe_first_drawn_line = minfirstline;
    if (thisframe_first_drawn_line > thisframe_last_drawn_line)
		thisframe_last_drawn_line = thisframe_first_drawn_line;

	maxline = linedbl ? (maxvpos_nom + 1) * 2 + 1 : (maxvpos_nom + 1) + 1;
    maxline++;
#ifdef SMART_UPDATE
    for (i = 0; i < maxline; i++) {
		switch (linestate[i]) {
		 case LINE_DONE_AS_PREVIOUS:
		    linestate[i] = LINE_REMEMBERED_AS_PREVIOUS;
		    break;
		 case LINE_REMEMBERED_AS_BLACK:
		    break;
		 default:
		    linestate[i] = LINE_UNDECIDED;
		    break;
		}
    }
#else
    memset (linestate, LINE_UNDECIDED, maxline);
#endif
    last_drawn_line = 0;
    first_drawn_line = 32767;

    first_block_line = last_block_line = NO_BLOCK;
    if (currprefs.test_drawing_speed)
		frame_redraw_necessary = 1;
    else if (frame_redraw_necessary)
		frame_redraw_necessary--;

    center_image ();

    thisframe_first_drawn_line = -1;
    thisframe_last_drawn_line = -1;

    drawing_color_matches = -1;
    seen_sprites = -1;
}

/*
 * Some code to put status information on the screen.
 */

static const char *numbers = { /* ugly  0123456789CHD%+- */
	"+++++++--++++-+++++++++++++++++-++++++++++++++++++++++++++++++++++++++++++++-++++++-++++----++---+--------------"
	"+xxxxx+--+xx+-+xxxxx++xxxxx++x+-+x++xxxxx++xxxxx++xxxxx++xxxxx++xxxxx++xxxx+-+x++x+-+xxx++-+xx+-+x---+----------"
	"+x+++x+--++x+-+++++x++++++x++x+++x++x++++++x++++++++++x++x+++x++x+++x++x++++-+x++x+-+x++x+--+x++x+--+x+----+++--"
	"+x+-+x+---+x+-+xxxxx++xxxxx++xxxxx++xxxxx++xxxxx+--++x+-+xxxxx++xxxxx++x+----+xxxx+-+x++x+----+x+--+xxx+--+xxx+-"
	"+x+++x+---+x+-+x++++++++++x++++++x++++++x++x+++x+--+x+--+x+++x++++++x++x++++-+x++x+-+x++x+---+x+x+--+x+----+++--"
	"+xxxxx+---+x+-+xxxxx++xxxxx+----+x++xxxxx++xxxxx+--+x+--+xxxxx++xxxxx++xxxx+-+x++x+-+xxx+---+x++xx--------------"
	"+++++++---+++-++++++++++++++----+++++++++++++++++--+++--++++++++++++++++++++-++++++-++++------------------------"
};
/* note */
STATIC_INLINE void putpixel (uae_u8 *buf, int bpp, int x, xcolnr c8, int opaq)
{
    if (x <= 0)
		return;

    switch (bpp) {
    case 1:
		buf[x] = (uae_u8)c8;
		break;
    case 2:
	    {
			uae_u16 *p = (uae_u16*)buf + x;
			*p = (uae_u16)c8;
			break;
    }
    case 3:
		/* no 24 bit yet */
		break;
    case 4:
		{
			int i;
			if (1 || opaq || currprefs.gfx_filter == 0) {
				uae_u32 *p = (uae_u32*)buf + x;
				*p = c8;
			} else {
				for (i = 0; i < 4; i++) {
					int v1 = buf[i + bpp * x];
					int v2 = (c8 >> (i * 8)) & 255;
					v1 = (v1 * 2 + v2 * 3) / 5;
					if (v1 > 255)
						v1 = 255;
					buf[i + bpp * x] = v1;
				}
			}
			break;
		}
	}
}

STATIC_INLINE uae_u32 ledcolor (uae_u32 c, uae_u32 *rc, uae_u32 *gc, uae_u32 *bc, uae_u32 *a)
{
	uae_u32 v = rc[(c >> 16) & 0xff] | gc[(c >> 8) & 0xff] | bc[(c >> 0) & 0xff];
	if (a)
		v |= a[255 - ((c >> 24) & 0xff)];
	return v;
}

static void write_tdnumber (uae_u8 *buf, int bpp, int x, int y, int num, uae_u32 c1, uae_u32 c2)
{
    int j;
    const char *numptr;

    numptr = numbers + num * TD_NUM_WIDTH + NUMBERS_NUM * TD_NUM_WIDTH * y;
    for (j = 0; j < TD_NUM_WIDTH; j++) {
		if (*numptr == 'x')
			putpixel (buf, bpp, x + j, c1, 1);
		else if (*numptr == '+')
			putpixel (buf, bpp, x + j, c2, 0);
		numptr++;
    }
}

void draw_status_line_single (uae_u8 *buf, int bpp, int y, int totalwidth, uae_u32 *rc, uae_u32 *gc, uae_u32 *bc, uae_u32 *alpha)
{
    int x_start, j, led, border;
    uae_u32 c1, c2, cb;

	c1 = ledcolor (0x00ffffff, rc, gc, bc, alpha);
	c2 = ledcolor (0x00000000, rc, gc, bc, alpha);
	cb = ledcolor (TD_BORDER, rc, gc, bc, alpha);

    if (td_pos & TD_RIGHT)
		x_start = totalwidth - TD_PADX - VISIBLE_LEDS * TD_WIDTH;
    else
		x_start = TD_PADX;

	for (led = 0; led < LED_MAX; led++) {
		int side, pos, num1 = -1, num2 = -1, num3 = -1, num4 = -1;
		int x, c, on = 0, am = 2;
		xcolnr on_rgb, on_rgb2, off_rgb, pen_rgb;
		int half = 0;

		pen_rgb = c1;
		if (led >= LED_DF0 && led <= LED_DF3) {
			int pled = led - LED_DF0;
			int track = gui_data.drive_track[pled];
			pos = 6 + pled;
			on_rgb = 0x00cc00;
			on_rgb2 = 0x006600;
			off_rgb = 0x003300;
			if (!gui_data.drive_disabled[pled]) {
				num1 = -1;
				num2 = track / 10;
				num3 = track % 10;
				on = gui_data.drive_motor[pled];
				if (gui_data.drive_writing[pled]) {
					on_rgb = 0xcc0000;
					on_rgb2 = 0x880000;
				}
				half = gui_data.drive_side ? 1 : -1;
				if (gui_data.df[pled][0] == 0)
					pen_rgb = ledcolor (0x00aaaaaa, rc, gc, bc, alpha);
			}
			side = gui_data.drive_side;
		} else if (led == LED_POWER) {
			pos = 3;
			//on = gui_data.powerled_brightness > 0;
			on_rgb = ((gui_data.powerled_brightness * 10 / 16) + 0x33) << 16;
			on = gui_data.powerled;
			//on_rgb = 0xcc0000;
			off_rgb = 0x330000;
		} else if (led == LED_CD) {
			pos = 5;
			on = gui_data.cd & (LED_CD_AUDIO | LED_CD_ACTIVE);
			on_rgb = (on & LED_CD_AUDIO) ? 0x00cc00 : 0x0000cc;
			if ((gui_data.cd & LED_CD_ACTIVE2) && !(gui_data.cd & LED_CD_AUDIO)) {
				on_rgb &= 0xfefefe;
				on_rgb >>= 1;
			}
			off_rgb = 0x000033;
			num1 = -1;
			num2 = 10;
			num3 = 12;
		} else if (led == LED_HD) {
			pos = 4;
			on = gui_data.hd;
			on_rgb = on == 2 ? 0xcc0000 : 0x0000cc;
			off_rgb = 0x000033;
			num1 = -1;
			num2 = 11;
			num3 = 12;
		} else if (led == LED_FPS) {
			int fps = (gui_data.fps + 5) / 10;
			pos = 2;
			on_rgb = 0x000000;
			off_rgb = 0x000000;
			if (fps > 999)
				fps = 999;
			num1 = fps / 100;
			num2 = (fps - num1 * 100) / 10;
			num3 = fps % 10;
			am = 3;
			if (num1 == 0)
				am = 2;
		} else if (led == LED_CPU) {
			int idle = (gui_data.idle + 5) / 10;
			pos = 1;
			on = framecnt && !picasso_on;
			on_rgb = 0xcc0000;
			off_rgb = 0x000000;
			num1 = idle / 100;
			num2 = (idle - num1 * 100) / 10;
			num3 = idle % 10;
			num4 = num1 == 0 ? 13 : -1;
			am = 3;
		} else if (led == LED_SND) {
			int snd = abs(gui_data.sndbuf + 5) / 10;
			if (snd > 99)
				snd = 99;
			pos = 0;
			on = gui_data.sndbuf_status;
			if (on < 3) {
				num1 = gui_data.sndbuf < 0 ? 15 : 14;
				num2 = snd / 10;
				num3 = snd % 10;
			}
			on_rgb = 0x000000;
			if (on < 0)
				on_rgb = 0xcccc00; // underflow
			else if (on == 2)
				on_rgb = 0xcc0000; // really big overflow
			else if (on == 1)
				on_rgb = 0x0000cc; // "normal" overflow
			off_rgb = 0x000000;
			am = 3;
		} else if (led == LED_MD && gui_data.drive_disabled[3]) {
			// DF3 reused as internal non-volatile ram led (cd32/cdtv)
			pos = 6 + 3;
			on = gui_data.md;
			on_rgb = on == 2 ? 0xcc0000 : 0x00cc00;
			off_rgb = 0x003300;
			num1 = -1;
			num2 = -1;
			num3 = -1;
		} else
			return;
		on_rgb |= 0x33000000;
		off_rgb |= 0x33000000;
		if (half > 0) {
			c = ledcolor (on ? (y >= TD_TOTAL_HEIGHT / 2 ? on_rgb2 : on_rgb) : off_rgb, rc, gc, bc, alpha);
		} else if (half < 0) {
			c = ledcolor (on ? (y < TD_TOTAL_HEIGHT / 2 ? on_rgb2 : on_rgb) : off_rgb, rc, gc, bc, alpha);
		} else {
			c = ledcolor (on ? on_rgb : off_rgb, rc, gc, bc, alpha);
		}
		border = 0;
		if (y == 0 || y == TD_TOTAL_HEIGHT - 1) {
			c = ledcolor (TD_BORDER, rc, gc, bc, alpha);
			border = 1;
		}

		x = x_start + pos * TD_WIDTH;
		if (!border)
			putpixel (buf, bpp, x - 1, cb, 0);
		for (j = 0; j < TD_LED_WIDTH; j++)
			putpixel (buf, bpp, x + j, c, 0);
		if (!border)
			putpixel (buf, bpp, x + j, cb, 0);

		if (y >= TD_PADY && y - TD_PADY < TD_NUM_HEIGHT) {
			if (num3 >= 0) {
				x += (TD_LED_WIDTH - am * TD_NUM_WIDTH) / 2;
				if (num1 > 0) {
					write_tdnumber (buf, bpp, x, y - TD_PADY, num1, pen_rgb, c2);
					x += TD_NUM_WIDTH;
				}
				write_tdnumber (buf, bpp, x, y - TD_PADY, num2, pen_rgb, c2);
				x += TD_NUM_WIDTH;
				write_tdnumber (buf, bpp, x, y - TD_PADY, num3, pen_rgb, c2);
				x += TD_NUM_WIDTH;
				if (num4 > 0)
					write_tdnumber (buf, bpp, x, y - TD_PADY, num4, pen_rgb, c2);
			}
		}
	}
}

static void draw_status_line (int line)
{
    int bpp, y;
    uae_u8 *buf;

    if (!(currprefs.leds_on_screen & STATUSLINE_CHIPSET) || (currprefs.leds_on_screen & STATUSLINE_TARGET))
		return;
    bpp = gfxvidinfo.pixbytes;
    y = line - (gfxvidinfo.height - TD_TOTAL_HEIGHT);
    xlinebuffer = gfxvidinfo.linemem;
    if (xlinebuffer == 0)
		xlinebuffer = row_map[line];
    buf = xlinebuffer;
	draw_status_line_single (buf, bpp, y, gfxvidinfo.width, xredcolors, xgreencolors, xbluecolors, NULL);
}

static void draw_debug_status_line (int line)
{
	xlinebuffer = gfxvidinfo.linemem;
	if (xlinebuffer == 0)
		xlinebuffer = row_map[line];
	debug_draw_cycles (xlinebuffer, gfxvidinfo.pixbytes, line, gfxvidinfo.width, gfxvidinfo.height, xredcolors, xgreencolors, xbluecolors);
}

#define LIGHTPEN_HEIGHT 12
#define LIGHTPEN_WIDTH 17

static const char *lightpen_cursor = {
    "------.....------"
    "------.xxx.------"
    "------.xxx.------"
    "------.xxx.------"
    ".......xxx......."
    ".xxxxxxxxxxxxxxx."
    ".xxxxxxxxxxxxxxx."
    ".......xxx......."
    "------.xxx.------"
    "------.xxx.------"
    "------.xxx.------"
    "------.....------"
};

static void draw_lightpen_cursor (int x, int y, int line, int onscreen)
{
    int i;
    const char *p;
    int color1 = onscreen ? 0xff0 : 0xf00;
    int color2 = 0x000;

    xlinebuffer = gfxvidinfo.linemem;
    if (xlinebuffer == 0)
	xlinebuffer = row_map[line];

    p = lightpen_cursor + y * LIGHTPEN_WIDTH;
    for (i = 0; i < LIGHTPEN_WIDTH; i++) {
	int xx = x + i - LIGHTPEN_WIDTH / 2;
	if (*p != '-' && xx >= 0 && xx < gfxvidinfo.width)
	    putpixel(xlinebuffer, gfxvidinfo.pixbytes, xx, *p == 'x' ? xcolors[color1] : xcolors[color2], 1);
	p++;
    }
}

static int lightpen_y1, lightpen_y2;

static void lightpen_update (void)
{
    int i;

    if (lightpen_x < LIGHTPEN_WIDTH + 1)
	lightpen_x = LIGHTPEN_WIDTH + 1;
    if (lightpen_x >= gfxvidinfo.width - LIGHTPEN_WIDTH - 1)
	lightpen_x = gfxvidinfo.width - LIGHTPEN_WIDTH - 2;
    if (lightpen_y < LIGHTPEN_HEIGHT + 1)
	lightpen_y = LIGHTPEN_HEIGHT + 1;
    if (lightpen_y >= gfxvidinfo.height - LIGHTPEN_HEIGHT - 1)
	lightpen_y = gfxvidinfo.height - LIGHTPEN_HEIGHT - 2;
    if (lightpen_y >= max_ypos_thisframe - LIGHTPEN_HEIGHT - 1)
	lightpen_y = max_ypos_thisframe - LIGHTPEN_HEIGHT - 2;

    lightpen_cx = (((lightpen_x + visible_left_border) >> lores_shift) >> 1) + DISPLAY_LEFT_SHIFT - DIW_DDF_OFFSET;

    lightpen_cy = lightpen_y;
    if (linedbl)
	lightpen_cy >>= 1;
    lightpen_cy += minfirstline;

    if (lightpen_cx < 0x18)
	lightpen_cx = 0x18;
    if (lightpen_cx >= (int)maxhpos)
	lightpen_cx -= (int)maxhpos;
    if (lightpen_cy < (int)minfirstline)
	lightpen_cy = (int)minfirstline;
	if (lightpen_cy >= (int)maxvpos)
		lightpen_cy = (int)maxvpos - 1;

    for (i = 0; i < LIGHTPEN_HEIGHT; i++) {
	int line = lightpen_y + i - LIGHTPEN_HEIGHT / 2;
	if (line >= 0 || line < max_ypos_thisframe) {
	    draw_lightpen_cursor(lightpen_x, i, line, lightpen_cx > 0);
	    flush_line (line);
	}
    }
    lightpen_y1 = lightpen_y - LIGHTPEN_HEIGHT / 2 - 1 + min_ypos_for_screen;
    lightpen_y2 = lightpen_y1 + LIGHTPEN_HEIGHT + 2;
}

void finish_drawing_frame (void)
{
    int i;

	if (! lockscr ()) {
		notice_screen_contents_lost ();
		return;
    }

#ifndef SMART_UPDATE
    /* @@@ This isn't exactly right yet. FIXME */
    if (!interlace_seen)
	do_flush_screen (first_drawn_line, last_drawn_line);
    else
	unlockscr ();
    return;
#endif

    for (i = 0; i < max_ypos_thisframe; i++) {
	int i1 = i + min_ypos_for_screen;
	int line = i + thisframe_y_adjust_real;
	int where2;

	where2 = amiga2aspect_line_map[i1];
	if (where2 >= gfxvidinfo.height)
	    break;
	if (where2 < 0)
	    continue;

	pfield_draw_line (line, where2, amiga2aspect_line_map[i1 + 1]);
    }

    /* clear possible old garbage at the bottom if emulated area become smaller */
    for (i = last_max_ypos; i < gfxvidinfo.height; i++) {
		int i1 = i + min_ypos_for_screen;
		int line = i + thisframe_y_adjust_real;
		int where2 = amiga2aspect_line_map[i1];
		xcolnr tmp;

		if (where2 >= gfxvidinfo.height)
			break;
		if (where2 < 0)
			continue;
		tmp = colors_for_drawing.acolors[0];
		colors_for_drawing.acolors[0] = getxcolor (0);
		xlinebuffer = gfxvidinfo.linemem;
		if (xlinebuffer == 0)
			xlinebuffer = row_map[where2];
		xlinebuffer -= linetoscr_x_adjust_bytes;
		fill_line ();
		linestate[line] = LINE_UNDECIDED;
		do_flush_line (where2);
		colors_for_drawing.acolors[0] = tmp;
    }

    if (currprefs.leds_on_screen) {
		for (i = 0; i < TD_TOTAL_HEIGHT; i++) {
			int line = gfxvidinfo.height - TD_TOTAL_HEIGHT + i - bottom_crop_global;//koko
		    draw_status_line (line);
		    do_flush_line (line);
		}
    }
#ifdef DEBUGGER
	if (debug_dma > 1) {
		for (i = 0; i < gfxvidinfo.height; i++) {
			int line = i;
			draw_debug_status_line (line);
			do_flush_line (line);
		}
	}
#endif

    if (lightpen_x > 0 || lightpen_y > 0)
		lightpen_update ();

    do_flush_screen (first_drawn_line, last_drawn_line);

#ifdef ECS_DENISE
    if (brdblank_changed) {
		last_max_ypos = max_ypos_thisframe;
		notice_screen_contents_lost ();
		brdblank_changed = 0;
    }
#endif
}

void hardware_line_completed (int lineno)
{
#ifndef SMART_UPDATE
    {
		int i, where;
		/* l is the line that has been finished for drawing. */
		i = lineno - thisframe_y_adjust_real;
		if (i >= 0 && i < max_ypos_thisframe) {
		    where = amiga2aspect_line_map[i+min_ypos_for_screen];
		    if (where < gfxvidinfo.height && where != -1)
				pfield_draw_line (lineno, where, amiga2aspect_line_map[i+min_ypos_for_screen+1]);
		}
    }
#endif
}

STATIC_INLINE void check_picasso (void)
{
#ifdef PICASSO96
    if (picasso_on && picasso_redraw_necessary)
		picasso_refresh (1);
    picasso_redraw_necessary = 0;

    if (picasso_requested_on == picasso_on)
		return;

    picasso_on = picasso_requested_on;

    if (!picasso_on)
		clear_inhibit_frame (IHF_PICASSO);
    else
		set_inhibit_frame (IHF_PICASSO);

    gfx_set_picasso_state (picasso_on);
    picasso_enablescreen (picasso_requested_on);

    notice_screen_contents_lost ();
    notice_new_xcolors ();
    count_frame ();
#endif
}

void redraw_frame (void)
{
    last_drawn_line = 0;
    first_drawn_line = 32767;
    finish_drawing_frame ();
    flush_screen (0, 0);
}

void vsync_handle_redraw (int long_frame, int lof_changed)
{
    last_redraw_point++;
    if (lof_changed || interlace_seen <= 0 || last_redraw_point >= 2 || long_frame || doublescan < 0) {
		last_redraw_point = 0;

		if (framecnt == 0)
		    finish_drawing_frame ();
		if (interlace_seen > 0) {
		    interlace_seen = -1;
		} else if (interlace_seen == -1) {
		    interlace_seen = 0;
		    if (currprefs.gfx_scandoubler && currprefs.gfx_linedbl)
				notice_screen_contents_lost ();
		}

		/* At this point, we have finished both the hardware and the
		 * drawing frame. Essentially, we are outside of all loops and
		 * can do some things which would cause confusion if they were
		 * done at other times.
		 */

#ifdef SAVESTATE
		if (savestate_state == STATE_DORESTORE) {
		    savestate_state = STATE_RESTORE;
		    reset_drawing ();
		    uae_reset (0);
		} else if (savestate_state == STATE_DOREWIND) {
		    savestate_state = STATE_REWIND;
		    reset_drawing ();
		    uae_reset (0);
		}
#endif

		if (quit_program < 0) {
		    quit_program = -quit_program;
		    set_inhibit_frame (IHF_QUIT_PROGRAM);
		    set_special (&regs, SPCFLAG_BRK);
		    return;
		}

#ifdef SAVESTATE
		savestate_capture (0);
#endif
		count_frame ();
		check_picasso ();

		int changed = check_prefs_changed_gfx ();
		if (changed > 0) {
		    reset_drawing ();
		    init_row_map ();
		    init_aspect_maps ();
		    notice_screen_contents_lost ();
		    notice_new_xcolors ();
		} else if (changed < 0) {
			reset_drawing ();
			init_row_map ();
			init_aspect_maps ();
			notice_screen_contents_lost ();
			notice_new_xcolors ();
		}

		check_prefs_changed_audio ();
		check_prefs_changed_custom ();
		check_prefs_changed_cpu ();

		if (framecnt == 0)
		    init_drawing_frame ();
		else if (currprefs.cpu_cycle_exact)
			init_hardware_for_drawing_frame ();
	} else {
		if (currprefs.gfx_afullscreen && currprefs.gfx_avsync)
	    	flush_screen (0, 0); /* vsync mode */
    }
	gui_flicker_led (-1, 0, 0);
#ifdef AVIOUTPUT
    frame_drawn ();
#endif
}

void hsync_record_line_state (int lineno, enum nln_how how, int changed)
{
    uae_u8 *state;

    if (framecnt != 0)
		return;

    state = linestate + lineno;
    changed += frame_redraw_necessary + ((lineno >= lightpen_y1 && lineno <= lightpen_y2) ? 1 : 0);

    switch (how) {
	case nln_normal:
		*state = changed ? LINE_DECIDED : LINE_DONE;
		break;
	case nln_doubled:
		*state = changed ? LINE_DECIDED_DOUBLE : LINE_DONE;
		changed += state[1] != LINE_REMEMBERED_AS_PREVIOUS;
		state[1] = changed ? LINE_AS_PREVIOUS : LINE_DONE_AS_PREVIOUS;
		break;
	case nln_nblack:
		*state = changed ? LINE_DECIDED : LINE_DONE;
		if (state[1] != LINE_REMEMBERED_AS_BLACK)
		    state[1] = LINE_BLACK;
		break;
	case nln_lower:
		if (state[-1] == LINE_UNDECIDED)
		    state[-1] = LINE_BLACK;
		*state = changed ? LINE_DECIDED : LINE_DONE;
		break;
	case nln_upper:
		*state = changed ? LINE_DECIDED : LINE_DONE;
		if (state[1] == LINE_UNDECIDED
		    || state[1] == LINE_REMEMBERED_AS_PREVIOUS
		    || state[1] == LINE_AS_PREVIOUS)
		    state[1] = LINE_BLACK;
		break;
    }
}

static void dummy_flush_line (struct vidbuf_description *gfxinfo, int line_no)
{
}

static void dummy_flush_block (struct vidbuf_description *gfxinfo, int first_line, int last_line)
{
}

static void dummy_flush_screen (struct vidbuf_description *gfxinfo, int first_line, int last_line)
{
}

static void dummy_flush_clear_screen (struct vidbuf_description *gfxinfo)
{
}

static int  dummy_lock (struct vidbuf_description *gfxinfo)
{
    return 1;
}

static void dummy_unlock (struct vidbuf_description *gfxinfo)
{
}

static void gfxbuffer_reset (void)
{
    gfxvidinfo.flush_line         = dummy_flush_line;
    gfxvidinfo.flush_block        = dummy_flush_block;
    gfxvidinfo.flush_screen       = dummy_flush_screen;
    gfxvidinfo.flush_clear_screen = dummy_flush_clear_screen;
    gfxvidinfo.lockscr            = dummy_lock;
    gfxvidinfo.unlockscr          = dummy_unlock;
}

void notice_interlace_seen (void)
{
    // non-lace to lace switch (non-lace active at least one frame)?
    if (interlace_seen == 0)
		frame_redraw_necessary = 2;
    interlace_seen = 1;
    frame_res_lace = 1;
}

void reset_drawing (void)
{
    unsigned int i;

    max_diwstop = 0;

    lores_reset ();
/* note */
    for (i = 0; i < sizeof linestate / sizeof *linestate; i++)
	linestate[i] = LINE_UNDECIDED;

    init_aspect_maps ();

    init_row_map();

    last_redraw_point = 0;

    memset (spixels, 0, sizeof spixels);
    memset (&spixstate, 0, sizeof spixstate);

    init_drawing_frame ();

    notice_screen_contents_lost ();
    frame_res_cnt = FRAMES_UNTIL_RES_SWITCH;
    lightpen_y1 = lightpen_y2 = -1;

    reset_custom_limits ();
}

void drawing_init (void)
{
    gen_pfield_tables ();

	uae_sem_init (&gui_sem, 0, 1);
#ifdef PICASSO96
    if (savestate_state != STATE_RESTORE) {
		InitPicasso96 ();
		picasso_on = 0;
		picasso_requested_on = 0;
		gfx_set_picasso_state (0);
    }
#endif
    xlinebuffer = gfxvidinfo.bufmem;
    inhibit_frame = 0;

    //gfxbuffer_reset ();
    reset_drawing ();
}

