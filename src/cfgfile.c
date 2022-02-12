/*
* UAE - The Un*x Amiga Emulator
*
* Config file handling
* This still needs some thought before it's complete...
*
* Copyright 1998 Brian King, Bernd Schmidt
* Copyright 2006 Richard Drummond
* Copyright 2008 Mustafa Tufan
*/

#include "sysconfig.h"
#include "sysdeps.h"

#include <ctype.h>

#include "options.h"
#include "uae.h"
#include "audio.h"
#include "filesys.h"
#include "events.h"
#include "custom.h"
#include "inputdevice.h"
#include "gfxfilter.h"
#include "gfxdep/gfx.h"
#include "sounddep/sound.h"
#include "savestate.h"
#include "memory.h"
#include "rommgr.h"
#include "gui.h"
#include "newcpu.h"
#include "zfile.h"
#include "fsdb.h"
#include "disk.h"
#include "version.h"
#include "statusline.h"

static int config_newfilesystem;
static struct strlist *temp_lines;
static struct zfile *default_file;
static int uaeconfig;
static int unicode_config = 0;

/* @@@ need to get rid of this... just cut part of the manual and print that
* as a help text.  */
struct cfg_lines
{
	const char *config_label, *config_help;
};

static const struct cfg_lines opttable[] =
{
    {"config_description", "" },
    {"config_info", "" },
    {"use_gui", "Enable the GUI?  If no, then goes straight to emulator" },
#ifdef DEBUGGER
    {"use_debugger", "Enable the debugger?" },
#endif
    {"cpu_speed", "can be max, real, or a number between 1 and 20" },
    {"cpu_model", "Can be 68000, 68010, 68020, 68030, 68040, 68060" },
    {"fpu_model", "Can be 68881, 68882, 68040, 68060" },
    {"cpu_compatible", "yes enables compatibility-mode" },
    {"cpu_24bit_addressing", "must be set to 'no' in order for Z3mem or P96mem to work" },
    {"autoconfig", "yes = add filesystems and extra ram" },
    {"log_illegal_mem", "print illegal memory access by Amiga software?" },
    {"fastmem_size", "Size in megabytes of fast-memory" },
    {"chipmem_size", "Size in megabytes of chip-memory" },
    {"bogomem_size", "Size in megabytes of bogo-memory at 0xC00000" },
    {"a3000mem_size", "Size in megabytes of A3000 memory" },
    {"gfxcard_size", "Size in megabytes of Picasso96 graphics-card memory" },
    {"z3mem_size", "Size in megabytes of Zorro-III expansion memory" },
    {"gfx_test_speed", "Test graphics speed?" },
    {"gfx_framerate", "Print every nth frame" },
    {"gfx_width", "Screen width" },
    {"gfx_height", "Screen height" },
    {"gfx_refreshrate", "Fullscreen refresh rate" },
    {"gfx_linemode", "Can be none, double, or scanlines" },
    {"gfx_fullscreen_amiga", "Amiga screens are fullscreen?" },
    {"gfx_fullscreen_picasso", "Picasso screens are fullscreen?" },
    {"gfx_center_horizontal", "Center display horizontally?" },
    {"gfx_center_vertical", "Center display vertically?" },

    {"gfx_gl_x_offset", "horizontal panning in gl mode (+/- integer hint: -37" },//koko
    {"gfx_gl_y_offset", "vertical panning in gl mode (+/- integer hint: -2)" }, 	//koko
    {"gfx_gl_panscan", "Zoom in/out in gl mode (+/- integer)" }, 	//koko
  /*{"gfx_gl_top_crop", "crop image in gl mode (+ integer)" }, 		//koko
    {"gfx_gl_bottom_crop", "crop image in gl mode (+ integer)" },	//koko
    {"gfx_gl_left_crop", "crop image in gl mode (+ integer)" },		//koko
    {"gfx_gl_right_crop", "crop image in gl mode (+ integer)" },*/	//koko
    {"gfx_gl_smoothing", "Linear smoothing in gl mode (true/false)" },	//koko
    
    {"gfx_colour_mode", "" },
    {"32bit_blits", "Enable 32 bit blitter emulation" },
    {"immediate_blits", "Perform blits immediately" },
    {"hide_cursor", "Whether to hide host window manager's cursor"},
    {"show_leds", "LED display" },
    {"keyboard_leds", "Keyboard LEDs" },
    {"gfxlib_replacement", "Use graphics.library replacement?" },
    {"sound_output", "" },
    {"sound_frequency", "" },
    {"sound_bits", "" },
    {"sound_channels", "" },
    {"sound_latency", "" },
    {"sound_max_buff", "" },
#ifdef JIT
    {"comp_trustbyte", "How to access bytes in compiler (direct/indirect/indirectKS/afterPic" },
    {"comp_trustword", "How to access words in compiler (direct/indirect/indirectKS/afterPic" },
    {"comp_trustlong", "How to access longs in compiler (direct/indirect/indirectKS/afterPic" },
    {"comp_nf", "Whether to optimize away flag generation where possible" },
    {"comp_fpu", "Whether to provide JIT FPU emulation" },
    {"cachesize", "How many MB to use to buffer translated instructions"},
#endif
    {"override_dga_address", "Address from which to map the frame buffer (upper 16 bits) (DANGEROUS!)"},
    {"avoid_cmov", "Set to yes on machines that lack the CMOV instruction" },
    {"avoid_dga", "Set to yes if the use of DGA extension creates problems" },
    {"avoid_vid", "Set to yes if the use of the Vidmode extension creates problems" },
    {"parallel_on_demand", "" },
    {"serial_on_demand", "" },
    {"scsi", "scsi.device emulation" },
    {"joyport0", "" },
    {"joyport1", "" },
    {"kickstart_rom_file", "Kickstart ROM image, (C) Copyright Amiga, Inc." },
    {"kickstart_ext_rom_file", "Extended Kickstart ROM image, (C) Copyright Amiga, Inc." },
    {"kickstart_key_file", "Key-file for encrypted ROM images (from Cloanto's Amiga Forever)" },
    {"flash_ram_file", "Flash/battery backed RAM image file." },
#ifdef ACTION_REPLAY
    {"cart_file", "Freezer cartridge ROM image file." },
#endif
    {"floppy0", "Diskfile for drive 0" },
    {"floppy1", "Diskfile for drive 1" },
    {"floppy2", "Diskfile for drive 2" },
    {"floppy3", "Diskfile for drive 3" },
#ifdef FILESYS
    {"hardfile", "access,sectors, surfaces, reserved, blocksize, path format" },
    {"filesystem", "access,'Amiga volume-name':'host directory path' - where 'access' can be 'read-only' or 'read-write'" },
#endif
#ifdef CATWEASEL
    {"catweasel","Catweasel board io base address" }
#endif
};

static const char *guimode1[] = { "no", "yes", "nowait", 0 };
static const char *guimode2[] = { "false", "true", "nowait", 0 };
static const char *guimode3[] = { "0", "1", "nowait", 0 };
static const char *csmode[] = { "ocs", "ecs_agnus", "ecs_denise", "ecs", "aga", 0 };
static const char *linemode1[] = { "none", "double", "scanlines", 0 };
static const char *linemode2[] = { "n", "d", "s", 0 };
static const char *speedmode[] = { "max", "real", 0 };
static const char *colormode1[] = { "8bit", "15bit", "16bit", "8bit_dither", "4bit_dither", "32bit", 0 };
static const char *colormode2[] = { "8", "15", "16", "8d", "4d", "32", 0 };
static const char *soundmode1[] = { "none", "interrupts", "normal", "exact", 0 };
static const char *soundmode2[] = { "none", "interrupts", "good", "best", 0 };
static const char *centermode1[] = { "none", "simple", "smart", 0 };
static const char *centermode2[] = { "false", "true", "smart", 0 };
static const char *stereomode[] = { "mono", "stereo", "clonedstereo", "4ch", "clonedstereo6ch", "6ch", "mixed", 0 };
static const char *interpolmode[] = { "none", "anti", "sinc", "rh", "crux", 0 };
static const char *collmode[] = { "none", "sprites", "playfields", "full", 0 };
static const char *compmode[] = { "direct", "indirect", "indirectKS", "afterPic", 0 };
static const char *flushmode[] = { "soft", "hard", 0 };
static const char *kbleds[] = { "none", "POWER", "DF0", "DF1", "DF2", "DF3", "HD", "CD", 0 };
static const char *onscreenleds[] = { "false", "true", "rtg", "both", 0 };
static const char *soundfiltermode1[] = { "off", "emulated", "on", 0 };
static const char *soundfiltermode2[] = { "standard", "enhanced", 0 };
static const char *lorestype1[] = { "lores", "hires", "superhires" };
static const char *lorestype2[] = { "true", "false" };
static const char *loresmode[] = { "normal", "filtered", 0 };
#ifdef GFXFILTER
static const char *filtermode2[] = { "0x", "1x", "2x", "3x", "4x", 0 };
#endif
static const char *cartsmode[] = { "none", "hrtmon", 0 };
static const char *idemode[] = { "none", "a600/a1200", "a4000", 0 };
static const char *rtctype[] = { "none", "MSM6242B", "RP5C01A", 0 };
static const char *ciaatodmode[] = { "vblank", "50hz", "60hz", 0 };
static const char *ksmirrortype[] = { "none", "e0", "a8+e0", 0 };
static const char *cscompa[] = {
	"-", "Generic", "CDTV", "CD32", "A500", "A500+", "A600",
	"A1000", "A1200", "A2000", "A3000", "A3000T", "A4000", "A4000T", 0
};
static const char *qsmodes[] = {
	"A500", "A500+", "A600", "A1000", "A1200", "A3000", "A4000", "", "CD32", "CDTV", "ARCADIA", NULL };
/* 3-state boolean! */
static const char *fullmodes[] = { "false", "true", /* "FILE_NOT_FOUND", */ "fullwindow", 0 };
/* bleh for compatibility */
static const char *scsimode[] = { "false", "true", "scsi", 0 };
static const char *maxhoriz[] = { "lores", "hires", "superhires", 0 };
static const char *maxvert[] = { "nointerlace", "interlace", 0 };
static const char *abspointers[] = { "none", "mousehack", "tablet", 0 };
static const char *magiccursors[] = { "both", "native", "host", 0 };
static const char *autoscale[] = { "none", "scale", "resize", 0 };
static const char *joyportmodes[] = { NULL, "mouse", "djoy", "ajoy", "cdtvjoy", "cd32joy", "lightpen", 0 };
static const char *epsonprinter[] = { "none", "ascii", "epson_matrix_9pin", "epson_matrix_24pin", "epson_matrix_48pin", 0 };
static const char *aspects[] = { "none", "vga", "tv", 0 };
static const char *vsyncmodes[] = { "false", "true", "autoswitch", 0 };
static const char *filterapi[] = { "directdraw", "direct3d", 0 };
static const char *dongles[] =
{
	"none",
	"robocop 3", "leaderboard", "b.a.t. ii", "italy'90 soccer", "dames grand maitre",
	"rugby coach", "cricket captain", "leviathan",
	NULL
};

static const char *obsolete[] = {
	"accuracy", "gfx_opengl", "gfx_32bit_blits", "32bit_blits",
	"gfx_immediate_blits", "gfx_ntsc", "win32", "gfx_filter_bits",
	"sound_pri_cutoff", "sound_pri_time", "sound_min_buff", "sound_bits",
	"gfx_test_speed", "gfxlib_replacement", "enforcer", "catweasel_io",
	"kickstart_key_file", "fast_copper", "sound_adjust",
	"serial_hardware_dtrdsr", "gfx_filter_upscale",
	"gfx_correct_aspect", "gfx_autoscale",
	NULL
};

#define UNEXPANDED "$(FILE_PATH)"

/*
 * The beginnings of a less brittle, more easily maintainable way of handling
 * prefs options.
 *
 * We maintain a key/value table of options.
 *
 * TODO:
 *  Make this a hash table.
 *  Add change notification.
 *  Support other value data types.
 *  Migrate more options.
 */

typedef struct {
    const char *key;
    int         target_specific;
    const char *value;
    const char *help;
} prefs_attr_t;

static prefs_attr_t prefs_attr_table[] = {
    {"floppy_path",            1, 0, "Default directory for floppy disk images"},
    {"rom_path",               1, 0, "Default directory for ROM images"},
    {"hardfile_path",          1, 0, "Default directory for hardfiles and filesystems"},
    {"savestate_path",         1, 0, "Default directory for saved-state images"},
    {0,                        0, 0, 0}
};

static prefs_attr_t *lookup_attr (const char *key)
{
    prefs_attr_t *attr = &prefs_attr_table[0];

    while (attr->key) {
	if (0 == strcmp (key, attr->key))
	    return attr;
	attr++;
    }
    return 0;
}

void prefs_set_attr (const char *key, const char *value)
{
    prefs_attr_t *attr = lookup_attr (key);

    if (attr) {
	if (attr->value)
	    free ((void *)attr->value);
	attr->value = value;
    }
}

const char *prefs_get_attr (const char *key)
{
    prefs_attr_t *attr = lookup_attr (key);

    if (attr)
	return attr->value;
    else
	return 0;
}

static void trimwsa (char *s)
{
    /* Delete trailing whitespace.  */
    int len = strlen (s);
    while (len > 0 && strcspn (s + len - 1, "\t \r\n") == 0)
        s[--len] = '\0';
}

static int match_string (const char *table[], const char *str)
{
    int i;
    for (i = 0; table[i] != 0; i++)
		if (strcasecmp (table[i], str) == 0)
		    return i;
    return -1;
}

char *cfgfile_subst_path (const char *path, const char *subst, const char *file)
{
    /* @@@ use strcasecmp for some targets.  */
	if (_tcslen (path) > 0 && _tcsncmp (file, path, _tcslen (path)) == 0) {
		int l;
		char *p = xmalloc (char, _tcslen (file) + _tcslen (subst) + 2);
		_tcscpy (p, subst);
		l = _tcslen (p);
		while (l > 0 && p[l - 1] == '/')
			p[--l] = '\0';
		l = _tcslen (path);
		while (file[l] == '/')
			l++;
		_tcscat (p, "/"); _tcscat (p, file + l);
		return p;
    }
    return my_strdup (file);
}

static int isdefault (const char *s)
{
	char tmp[MAX_DPATH];
	if (!default_file || uaeconfig)
		return 0;
	zfile_fseek (default_file, 0, SEEK_SET);
	while (zfile_fgets (tmp, sizeof tmp / sizeof (char), default_file)) {
		if (!_tcscmp (tmp, s))
			return 1;
	}
	return 0;
}

static size_t cfg_write (void *b, struct zfile *z)
{
	size_t v;
	if (unicode_config) {
		char lf = 10;
		v = zfile_fwrite (b, _tcslen ((char*)b), sizeof (char), z);
		zfile_fwrite (&lf, 1, 1, z);
	} else {
		char lf = 10;
		v = zfile_fwrite (((char)b), strlen (((char)b)), 1, z);
		zfile_fwrite (&lf, 1, 1, z);
	}
	return v;
}

#define UTF8NAME ".utf8"

void cfgfile_write (FILE *f, const char *format,...)
{
    va_list parms;
    char tmp[CONFIG_BLEN];

    va_start (parms, format);
    vsprintf (tmp, format, parms);
    fprintf (f, tmp);
    va_end (parms);
}

void cfgfile_write_bool (FILE *f, const char *option, int b)
{
	cfgfile_write (f, "%s=%s\n", option, b ? "true" : "false");
}

void cfgfile_write_str (FILE *f, const char *option, const char *value)
{
	cfgfile_write (f, "%s=%s\n", option, value);
}

static void subst_home (char *f, int n)
{
    const char *home = getenv ("HOME");

    if (home) {
	char *str = cfgfile_subst_path ("~", home, f);
	strncpy (f, str, n - 1);
	f[n - 1] = '\0';
	free (str);
    }
}

void cfgfile_subst_home (char *path, unsigned int maxlen)
{
    subst_home (path, maxlen);
}

void do_cfgfile_write (FILE *f, const char *format,...)
{
    va_list parms;
    char tmp[CONFIG_BLEN];

    va_start (parms, format);
    vsprintf (tmp, format, parms);
    fprintf (f, tmp);
    va_end (parms);
}

static void cfgfile_write_path_option (FILE *f, const char *key)
{
    const char *home = getenv ("HOME");
    const char *path = prefs_get_attr (key);
    char *out_path = 0;

    if (path)
	out_path = cfgfile_subst_path (home, "~", path);

    cfgfile_write (f, "%s.%s=%s\n", TARGET_NAME, key, out_path ? out_path : "");

    if (out_path)
	free (out_path);
}

static void cfgfile_write_file_option (FILE *f, const char *option, const char *subst_key, const char *value)
{
    const char *subst_path = prefs_get_attr (subst_key);
    char *out_path = 0;

    if (subst_path)
	out_path = cfgfile_subst_path (subst_path, UNEXPANDED, value);

    cfgfile_write (f, "%s=%s\n", option, out_path ? out_path : value);

    if (out_path)
	free (out_path);
}

static void write_compatibility_cpu (struct zfile *f, struct uae_prefs *p)
{
	char tmp[100];
	int model;

	model = p->cpu_model;
	if (model == 68030)
		model = 68020;
	if (model == 68060)
		model = 68040;
	if (p->address_space_24 && model == 68020)
		_tcscpy (tmp, "68ec020");
	else
		_stprintf (tmp, "%d", model);
	if (model == 68020 && (p->fpu_model == 68881 || p->fpu_model == 68882))
		_tcscat (tmp, "/68881");
	cfgfile_write (f, "cpu_type=%s\n", tmp);
}

void cfgfile_save_options (FILE *f, const struct uae_prefs *p, int type)
{
    struct strlist *sl;
	char *str, tmp[MAX_DPATH];
    int i;

	cfgfile_write_str (f, "config_description", p->description);
	cfgfile_write_bool (f, "config_hardware", type & CONFIG_TYPE_HARDWARE);
	cfgfile_write_bool (f, "config_host", type & CONFIG_TYPE_HOST);
    if (p->info[0])
		cfgfile_write (f, "config_info=%s\n", p->info);
    cfgfile_write (f, "config_version=%d.%d.%d\n", UAEMAJOR, UAEMINOR, UAESUBREV);
	cfgfile_write_str (f, "config_hardware_path", p->config_hardware_path);
	cfgfile_write_str (f, "config_host_path", p->config_host_path);

    for (sl = p->all_lines; sl; sl = sl->next) {
	if (sl->unknown)
			cfgfile_write_str (f, sl->option, sl->value);
    }

    cfgfile_write_path_option (f, "rom_path");
    cfgfile_write_path_option (f, "floppy_path");
    cfgfile_write_path_option (f, "hardfile_path");
#ifdef SAVESTATE
    cfgfile_write_path_option (f, "savestate_path");
#endif

	cfgfile_write (f, "; host-specific");

    machdep_save_options (f, p);
    target_save_options (f, p);
    gfx_save_options (f, p);
    audio_save_options (f, p);

	cfgfile_write (f,"; common");

	cfgfile_write_str (f, "use_gui", guimode1[p->start_gui]);
#ifdef DEBUGGER
	cfgfile_write_bool (f, "use_debugger", p->start_debugger);
#endif

    cfgfile_write_file_option (f, "kickstart_rom_file",     "rom_path", p->romfile);
    cfgfile_write_file_option (f, "kickstart_ext_rom_file", "rom_path", p->romextfile);
    cfgfile_write_file_option (f, "kickstart_key_file",     "rom_path", p->keyfile);
    cfgfile_write_file_option (f, "flash_file",             "rom_path", p->flashfile);
#ifdef ACTION_REPLAY
    cfgfile_write_file_option (f, "cart_file",              "rom_path", p->cartfile);
#endif

	cfgfile_write_bool (f, "kickshifter", p->kickshifter);

	//p->nr_floppies = 4;
    for (i = 0; i < 4; i++) {
	char tmp_option[] = "floppy0";
	tmp_option[6] = '0' + i;
	cfgfile_write_file_option (f, tmp_option, "floppy_path", p->df[i]);
	cfgfile_write (f, "floppy%dtype=%d\n", i, p->dfxtype[i]);
#ifdef DRIVESOUND
	cfgfile_write (f, "floppy%dsound=%d\n", i, p->dfxclick[i]);
	if (p->dfxclick[i] < 0 && p->dfxclickexternal[i][0])
	    cfgfile_write (f, "floppy%dsoundext=%s\n", i, p->dfxclickexternal[i]);
#endif
    }
    for (i = 0; i < MAX_SPARE_DRIVES; i++) {
	if (p->dfxlist[i][0])
	    cfgfile_write (f, "diskimage%d=%s\n", i, p->dfxlist[i]);
    }

    cfgfile_write (f, "nr_floppies=%d\n", p->nr_floppies);
    cfgfile_write (f, "floppy_speed=%d\n", p->floppy_speed);
#ifdef DRIVESOUND
    cfgfile_write (f, "floppy_volume=%d\n", p->dfxclickvolume);
#endif
	cfgfile_write_bool (f, "parallel_on_demand", p->parallel_demand);
	cfgfile_write_bool (f, "serial_on_demand", p->serial_demand);
	cfgfile_write_bool (f, "serial_hardware_ctsrts", p->serial_hwctsrts);
	cfgfile_write_bool (f, "serial_direct", p->serial_direct);
	cfgfile_write_str (f, "scsi", scsimode[p->scsi]);
	cfgfile_write_bool (f, "uaeserial", p->uaeserial);
	cfgfile_write_bool (f, "sana2", p->sana2);

	cfgfile_write_str (f, "sound_output", soundmode1[p->produce_sound]);
	cfgfile_write_str (f, "sound_channels", stereomode[p->sound_stereo]);
    cfgfile_write (f, "sound_stereo_separation=%d\n", p->sound_stereo_separation);
    cfgfile_write (f, "sound_stereo_mixing_delay=%d\n", p->sound_mixed_stereo_delay >= 0 ? p->sound_mixed_stereo_delay : 0);
	cfgfile_write (f, "sound_max_buff=%d\n", p->sound_maxbsiz);
    cfgfile_write (f, "sound_frequency=%d\n", p->sound_freq);
	cfgfile_write (f, "sound_latency=%d", p->sound_latency);
	cfgfile_write_str (f, "sound_interpol", interpolmode[p->sound_interpol]);
	cfgfile_write_str (f, "sound_filter", soundfiltermode1[p->sound_filter]);
	cfgfile_write_str (f, "sound_filter_type", soundfiltermode2[p->sound_filter_type]);
    cfgfile_write (f, "sound_volume=%d\n", p->sound_volume);
	cfgfile_write_bool (f, "sound_auto", p->sound_auto);
	cfgfile_write_bool (f, "sound_stereo_swap_paula", p->sound_stereo_swap_paula);
	cfgfile_write_bool (f, "sound_stereo_swap_ahi", p->sound_stereo_swap_ahi);

#ifdef JIT
    cfgfile_write (f, "comp_trustbyte=%s\n", compmode[p->comptrustbyte]);
    cfgfile_write (f, "comp_trustword=%s\n", compmode[p->comptrustword]);
    cfgfile_write (f, "comp_trustlong=%s\n", compmode[p->comptrustlong]);
    cfgfile_write (f, "comp_trustnaddr=%s\n", compmode[p->comptrustnaddr]);
    cfgfile_write (f, "comp_nf=%s\n", p->compnf ? "true" : "false");
    cfgfile_write (f, "comp_constjump=%s\n", p->comp_constjump ? "true" : "false");
    cfgfile_write (f, "comp_oldsegv=%s\n", p->comp_oldsegv ? "true" : "false");

    cfgfile_write (f, "comp_flushmode=%s\n", flushmode[p->comp_hardflush]);
    cfgfile_write (f, "compfpu=%s\n", p->compfpu ? "true" : "false");
    cfgfile_write (f, "cachesize=%d\n", p->cachesize);
#endif

	for (i = 0; i < MAX_JPORTS; i++) {
		struct jport *jp = &p->jports[i];
		int v = jp->id;
		char tmp1[MAX_DPATH], tmp2[MAX_DPATH];
		if (v < 0) {
			_tcscpy (tmp2, "none");
		} else if (v < JSEM_JOYS) {
			_stprintf (tmp2, "kbd%d", v + 1);
		} else if (v < JSEM_MICE) {
			_stprintf (tmp2, "joy%d", v - JSEM_JOYS);
		} else {
			_tcscpy (tmp2, "mouse");
		    if (v - JSEM_MICE > 0)
				_stprintf (tmp2, "mouse%d", v - JSEM_MICE);
		}
		if (i < 2 || jp->id >= 0) {
			_stprintf (tmp1, "joyport%d", i);
			cfgfile_write (f, tmp1, tmp2);
			if (i < 2 && jp->mode > 0) {
				_stprintf (tmp1, "joyport%dmode", i);
				cfgfile_write (f, tmp1, joyportmodes[jp->mode]);
			}
			if (jp->name) {
				_stprintf (tmp1, "joyportfriendlyname%d", i);
				cfgfile_write (f, tmp1, jp->name);
			}
			if (jp->configname) {
				_stprintf (tmp1, "joyportname%d", i);
				cfgfile_write (f, tmp1, jp->configname);
			}
		}
	}
	if (p->dongle) {
		if (p->dongle + 1 >= sizeof (dongles) / sizeof (char*))
			cfgfile_write (f, "dongle=%d", p->dongle);
		else
			cfgfile_write_str (f, "dongle", dongles[p->dongle]);
	}

	cfgfile_write_bool (f, "bsdsocket_emu", p->socket_emu);
	if (p->a2065name[0])
		cfgfile_write_str (f, "a2065", p->a2065name);

	cfgfile_write_bool (f, "synchronize_clock", p->tod_hack);
    cfgfile_write (f, "maprom=0x%x\n", p->maprom);

    cfgfile_write (f, "gfx_framerate=%d\n", p->gfx_framerate);
    cfgfile_write (f, "gfx_width=%d\n", p->gfx_width_win); /* compatibility with old versions */
    cfgfile_write (f, "gfx_height=%d\n", p->gfx_height_win); /* compatibility with old versions */
    cfgfile_write (f, "gfx_width_windowed=%d\n", p->gfx_width_win);
    cfgfile_write (f, "gfx_height_windowed=%d\n", p->gfx_height_win);
    cfgfile_write (f, "gfx_width_fullscreen=%d\n", p->gfx_width_fs);
    cfgfile_write (f, "gfx_height_fullscreen=%d\n", p->gfx_height_fs);
    cfgfile_write (f, "gfx_refreshrate=%d\n", p->gfx_refreshrate);
    cfgfile_write (f, "gfx_linemode=%s\n", linemode1[p->gfx_linedbl]);
    cfgfile_write (f, "gfx_fullscreen_amiga=%s\n", p->gfx_afullscreen ? "true" : "false");
    cfgfile_write (f, "gfx_fullscreen_picasso=%s\n", p->gfx_pfullscreen ? "true" : "false");
    cfgfile_write (f, "gfx_center_horizontal=%s\n", centermode1[p->gfx_xcenter]);
    cfgfile_write (f, "gfx_center_vertical=%s\n", centermode1[p->gfx_ycenter]);
    cfgfile_write (f, "gfx_colour_mode=%s\n", colormode1[p->color_mode]);
    cfgfile_write (f, "gfx_gl_x_offset=%d\n", p->gfx_gl_x_offset); //koko
    cfgfile_write (f, "gfx_gl_y_offset=%d\n", p->gfx_gl_y_offset); //koko
    cfgfile_write (f, "gfx_gl_panscan=%d\n", p->gfx_gl_y_offset); //koko
    cfgfile_write (f, "gfx_gl_smoothing=%s\n", p->gfx_gl_smoothing ? "true" : "false"); //koko

#ifdef GFXFILTER
    if (p->gfx_filter > 0) {
	int i = 0;
	struct uae_filter *uf;
	while (uaefilters[i].name) {
	    uf = &uaefilters[i];
	    if (uf->type == p->gfx_filter) {
		cfgfile_write (f, "gfx_filter=%s\n", uf->cfgname);
		if (uf->type == p->gfx_filter) {
		    if (uf->x[0]) {
			cfgfile_write (f, "gfx_filter_mode=%s\n", filtermode1[p->gfx_filter_filtermode]);
		    } else {
			int mt[4], i = 0;
						if (uf->x[1])
							mt[i++] = 1;
						if (uf->x[2])
							mt[i++] = 2;
						if (uf->x[3])
							mt[i++] = 3;
						if (uf->x[4])
							mt[i++] = 4;
			cfgfile_write (f, "gfx_filter_mode=%dx\n", mt[p->gfx_filter_filtermode]);
		    }
		}
	    }
	    i++;
	}
    } else {
		cfgfile_write (f, "gfx_filter=no\n");
    }

    cfgfile_write (f, "gfx_filter_vert_zoom=%d\n", p->gfx_filter_vert_zoom);
    cfgfile_write (f, "gfx_filter_horiz_zoom=%d\n", p->gfx_filter_horiz_zoom);
    cfgfile_write (f, "gfx_filter_vert_offset=%d\n", p->gfx_filter_vert_offset);
    cfgfile_write (f, "gfx_filter_horiz_offset=%d\n", p->gfx_filter_horiz_offset);
    cfgfile_write (f, "gfx_filter_scanlines=%d\n", p->gfx_filter_scanlines);
    cfgfile_write (f, "gfx_filter_scanlinelevel=%d\n", p->gfx_filter_scanlinelevel);
    cfgfile_write (f, "gfx_filter_scanlineratio=%d\n", p->gfx_filter_scanlineratio);
#endif

	cfgfile_write_bool (f, "immediate_blits", p->immediate_blits);
	cfgfile_write_bool (f, "ntsc", p->ntscmode);
	cfgfile_write_bool (f, "genlock", p->genlock);
    cfgfile_write (f, "hide_cursor=%s\n", p->hide_cursor ? "true" : "false");
    cfgfile_write (f, "show_leds=%s\n", p->leds_on_screen ? "true" : "false");
    cfgfile_write (f, "keyboard_leds=numlock:%s,capslock:%s,scrolllock:%s\n",
	kbleds[p->keyboard_leds[0]], kbleds[p->keyboard_leds[1]], kbleds[p->keyboard_leds[2]]);
    if (p->chipset_mask & CSMASK_AGA)
		cfgfile_write (f, "chipset=aga\n");
    else if ((p->chipset_mask & CSMASK_ECS_AGNUS) && (p->chipset_mask & CSMASK_ECS_DENISE))
		cfgfile_write (f, "chipset=ecs\n");
    else if (p->chipset_mask & CSMASK_ECS_AGNUS)
		cfgfile_write (f, "chipset=ecs_agnus\n");
    else if (p->chipset_mask & CSMASK_ECS_DENISE)
		cfgfile_write (f, "chipset=ecs_denise\n");
    else
		cfgfile_write (f, "chipset=ocs\n");
	cfgfile_write (f, "chipset_refreshrate=%d\n", p->chipset_refreshrate);
	cfgfile_write_str (f, "collision_level", collmode[p->collision_level]);

	cfgfile_write_str(f, "chipset_compatible", cscompa[p->cs_compatible]);
	cfgfile_write_str (f, "ciaatod", ciaatodmode[p->cs_ciaatod]);
	cfgfile_write_str (f, "rtc", rtctype[p->cs_rtc]);
	//cfgfile_dwrite (f, "chipset_rtc_adjust", "%d", p->cs_rtc_adjust);

    cfgfile_write (f, "fastmem_size=%d\n", p->fastmem_size / 0x100000);
    cfgfile_write (f, "a3000mem_size=%d\n", p->mbresmem_low_size / 0x100000);
	cfgfile_write (f, "mbresmem_size=%d\n", p->mbresmem_high_size / 0x100000);
    cfgfile_write (f, "z3mem_size=%d\n", p->z3fastmem_size / 0x100000);
	cfgfile_write (f, "z3mem2_size=%d\n", p->z3fastmem2_size / 0x100000);
	cfgfile_write (f, "z3mem_start=0x%x\n", p->z3fastmem_start);
    cfgfile_write (f, "bogomem_size=%d\n", p->bogomem_size / 0x40000);
    cfgfile_write (f, "gfxcard_size=%d\n", p->gfxmem_size / 0x100000);
    cfgfile_write (f, "chipmem_size=%d\n", p->chipmem_size == 0x20000 ? -1 : (p->chipmem_size == 0x40000 ? 0 : p->chipmem_size / 0x80000));

    if (p->m68k_speed > 0)
		cfgfile_write (f, "finegrain_cpu_speed=%d\n", p->m68k_speed);
    else
		cfgfile_write_str (f, "cpu_speed", p->m68k_speed == -1 ? "max" : "real");

	/* do not reorder start */
	write_compatibility_cpu(f, p);
	cfgfile_write (f, "cpu_model=%d\n", p->cpu_model);
	if (p->fpu_model)
		cfgfile_write (f, "fpu_model=%d\n", p->fpu_model);
	if (p->mmu_model)
		cfgfile_write (f, "mmu_model=%d\n", p->mmu_model);
	cfgfile_write (f, "cpu_compatible=%s\n", p->cpu_compatible ? "true" : "false");
	cfgfile_write_bool (f, "cpu_24bit_addressing", p->address_space_24);
	/* do not reorder end */

	if (p->cpu_cycle_exact) {
		if (p->cpu_frequency)
			cfgfile_write (f, "cpu_frequency=%d\n", p->cpu_frequency);
		if (p->cpu_clock_multiplier) {
			if (p->cpu_clock_multiplier >= 256)
				cfgfile_write (f, "cpu_multiplier=%d\n", p->cpu_clock_multiplier >> 8);
		}
	}

	cfgfile_write_bool (f, "cpu_cycle_exact", p->cpu_cycle_exact);
	cfgfile_write_bool (f, "blitter_cycle_exact", p->blitter_cycle_exact);
	cfgfile_write_bool (f, "cycle_exact", p->cpu_cycle_exact && p->blitter_cycle_exact ? 1 : 0);
	cfgfile_write_bool (f, "rtg_nocustom", p->picasso96_nocustom);
	cfgfile_write (f, "rtg_modes=0x%x\n", p->picasso96_modeflags);

	cfgfile_write_bool (f, "log_illegal_mem", p->illegal_mem);
	if (p->catweasel >= 100)
		cfgfile_write (f, "catweasel=0x%x\n", p->catweasel);
	else
		cfgfile_write (f, "catweasel=%d\n", p->catweasel);

    cfgfile_write (f, "kbd_lang=%s\n", (p->keyboard_lang == KBD_LANG_DE ? "de"
				  : p->keyboard_lang == KBD_LANG_DK ? "dk"
				  : p->keyboard_lang == KBD_LANG_ES ? "es"
				  : p->keyboard_lang == KBD_LANG_US ? "us"
				  : p->keyboard_lang == KBD_LANG_SE ? "se"
				  : p->keyboard_lang == KBD_LANG_FR ? "fr"
				  : p->keyboard_lang == KBD_LANG_IT ? "it"
				  : "FOO"));

#ifdef SAVESTATE
    cfgfile_write (f, "state_replay=%s\n", p->statecapture ? "yes" : "no");
    cfgfile_write (f, "state_replay_rate=%d\n", p->statecapturerate);
    cfgfile_write (f, "state_replay_buffer=%d\n", p->statecapturebuffersize);
#endif
	cfgfile_write_bool (f, "warp", p->turbo_emulation);

#ifdef FILESYS
    //write_filesys_config (currprefs.mountinfo, UNEXPANDED, prefs_get_attr ("hardfile_path"), f);
    if (p->filesys_no_uaefsdb)
		cfgfile_write_bool (f, "filesys_no_fsdb", p->filesys_no_uaefsdb);
#endif
    write_inputdevice_config (p, f);

    /* Don't write gfxlib/gfx_test_speed options.  */
}

int cfgfile_yesno (const char *option, const char *value, const char *name, int *location)
{
	if (_tcscmp (option, name) != 0)
		return 0;
    if (strcasecmp (value, "yes") == 0 || strcasecmp (value, "y") == 0
		|| strcasecmp (value, "true") == 0 || strcasecmp (value, "t") == 0)
		*location = 1;
    else if (strcasecmp (value, "no") == 0 || strcasecmp (value, "n") == 0
		|| strcasecmp (value, "false") == 0 || strcasecmp (value, "f") == 0)
		*location = 0;
    else {
		write_log ("Option `%s' requires a value of either `yes' or `no'.\n", option);
		return -1;
    }
    return 1;
}

int cfgfile_intval (const char *option, const char *value, const char *name, int *location, int scale)
{
    int base = 10;
    char *endptr;
	if (_tcscmp (option, name) != 0)
		return 0;
    /* I guess octal isn't popular enough to worry about here...  */
    if (value[0] == '0' && value[1] == 'x')
		value += 2, base = 16;
	*location = _tcstol (value, &endptr, base) * scale;

    if (*endptr != '\0' || *value == '\0') {
		if (strcasecmp (value, "false") == 0 || strcasecmp (value, "no") == 0) {
			*location = 0;
			return 1;
		}
		if (strcasecmp (value, "true") == 0 || strcasecmp (value, "yes") == 0) {
			*location = 1;
			return 1;
		}
		write_log ("Option '%s' requires a numeric argument but got '%s'\n", option, value);
	return -1;
    }
    return 1;
}
int cfgfile_intvalx (const char *option, const char *value, const char *name, int *location, int scale)
{
	unsigned int v = 0;
	int r = cfgfile_intval (option, value, name, &v, scale);
	if (!r)
		return 0;
	*location = (int)v;
	return r;
}



int cfgfile_strval (const char *option, const char *value, const char *name, int *location, const char *table[], int more)
{
    int val;
	if (_tcscmp (option, name) != 0)
	return 0;
    val = match_string (table, value);
    if (val == -1) {
	if (more)
	    return 0;

		write_log ("Unknown value ('%s') for option '%s'.\n", value, option);
	return -1;
    }
    *location = val;
    return 1;
}

int cfgfile_string (const char *option, const char *value, const char *name, char *location, int maxsz)
{
	if (_tcscmp (option, name) != 0)
		return 0;
	_tcsncpy (location, value, maxsz - 1);
    location[maxsz - 1] = '\0';
    return 1;
}

static int getintval (char **p, int *result, int delim)
{
	char *value = *p;
	int base = 10;
	char *endptr;
	char *p2 = _tcschr (*p, delim);

	if (p2 == 0)
		return 0;

	*p2++ = '\0';

	if (value[0] == '0' && toupper (value[1]) == 'X')
		value += 2, base = 16;
	*result = _tcstol (value, &endptr, base);
	*p = p2;

	if (*endptr != '\0' || *value == '\0')
		return 0;

	return 1;
}

static int getintval2 (char **p, int *result, int delim)
{
	char *value = *p;
	int base = 10;
	char *endptr;
	char *p2 = _tcschr (*p, delim);

	if (p2 == 0) {
		p2 = _tcschr (*p, 0);
		if (p2 == 0) {
			*p = 0;
			return 0;
		}
	}
	if (*p2 != 0)
		*p2++ = '\0';

	if (value[0] == '0' && toupper (value[1]) == 'X')
		value += 2, base = 16;
	*result = _tcstol (value, &endptr, base);
	*p = p2;

	if (*endptr != '\0' || *value == '\0') {
		*p = 0;
		return 0;
	}

	return 1;
}

static void set_chipset_mask (struct uae_prefs *p, int val)
{
	p->chipset_mask = (val == 0 ? 0
		: val == 1 ? CSMASK_ECS_AGNUS
		: val == 2 ? CSMASK_ECS_DENISE
		: val == 3 ? CSMASK_ECS_DENISE | CSMASK_ECS_AGNUS
		: CSMASK_AGA | CSMASK_ECS_DENISE | CSMASK_ECS_AGNUS);
}

/*
 * Duplicate the path 'src'. If 'src' begins with '~/' substitue
 * the home directory.
 *
 * TODO: Clean this up.
 * TODO: Collect path handling tools in one place and cleanly
 * handle platform-specific differences.
 */
static const char *strdup_path_expand (const char *src)
{
    char *path = 0;
    unsigned int srclen, destlen;
    int need_separator = 0;
    const char *home = getenv ("HOME");

    srclen = strlen (src);

    if (srclen > 0) {
	if (src[srclen - 1] != '/' && src[srclen - 1] != '\\'
#ifdef TARGET_AMIGAOS
	    && src[srclen - 1] != ':'
#endif
	    ) {
	    need_separator = 1;
	}
    }

    destlen = srclen + need_separator;

    if (src[0] == '~' && src[1] == '/' && home) {
	destlen += srclen + strlen (home);
	src++;
	srclen--;
    } else
	home = 0;

    path = malloc (destlen + 1); path[0]=0;

    if (path) {
	if (home)
	    strcpy (path, home);

	strcat (path, src);

	if (need_separator)
	    strcat (path, "/");
    }

    return path;
}

static int cfgfile_path (const char *option, const char *value, const char *key)
{
    if (strcmp (option, key) == 0) {
	const char *path = strdup_path_expand (value);

	if (path)
	    prefs_set_attr (key, path);

	return 1;
    }
    return 0;
}

static int cfgfile_parse_host (struct uae_prefs *p, char *option, char *value)
{
    int i, v;
    char *section = 0;
    char *tmpp;
    char tmpbuf[CONFIG_BLEN];

    if (memcmp (option, "input.", 6) == 0) {
		read_inputdevice_config (p, option, value);
		return 1;
    }

    for (tmpp = option; *tmpp != '\0'; tmpp++)
		if (isupper (*tmpp))
			*tmpp = tolower (*tmpp);
	tmpp = strchr (option, '.');
    if (tmpp) {
		section = option;
		option = tmpp + 1;
		*tmpp = '\0';
		if (strcmp (section, TARGET_NAME) == 0) {
	    	/* We special case the various path options here.  */
			if (cfgfile_path (option, value, "rom_path"))
				return 1;
		    if (cfgfile_path (option, value, "floppy_path"))
				return 1;
		    if (cfgfile_path (option, value, "hardfile_path"))
				return 1;
#ifdef SAVESTATE
		    if (cfgfile_path (option, value, "savestate_path"))
				return 1;
#endif
		    if (target_parse_option (p, option, value))
				return 1;
		}
		if (strcmp (section, MACHDEP_NAME) == 0)
		    return machdep_parse_option (p, option, value);
		if (strcmp (section, GFX_NAME) == 0)
		    return gfx_parse_option (p, option, value);
		if (strcmp (section, AUDIO_NAME) == 0)
		    return audio_parse_option (p, option, value);

		return 0;
    }
    for (i = 0; i < MAX_SPARE_DRIVES; i++) {
		sprintf (tmpbuf, "diskimage%d", i);
		if (cfgfile_string (option, value, tmpbuf, p->dfxlist[i], sizeof p->dfxlist[i] / sizeof (char))) {
#if 0
	    if (i < 4 && !p->df[i][0])
				_tcscpy (p->df[i], p->dfxlist[i]);
#endif
		return 1;
	}
    }

	if (cfgfile_intval (option, value, "sound_frequency", &p->sound_freq, 1)) {
		/* backwards compatibility */
		p->sound_latency = 1000 * (p->sound_maxbsiz >> 1) / p->sound_freq;
		return 1;
	}

	if (cfgfile_intval (option, value, "sound_latency", &p->sound_latency, 1)
		|| cfgfile_intval (option, value, "sound_max_buff", &p->sound_maxbsiz, 1)
		|| cfgfile_intval (option, value, "state_replay_rate", &p->statecapturerate, 1)
		|| cfgfile_intval (option, value, "state_replay_buffer", &p->statecapturebuffersize, 1)
		|| cfgfile_intval (option, value, "sound_frequency", &p->sound_freq, 1)
		|| cfgfile_intval (option, value, "sound_volume", &p->sound_volume, 1)
		|| cfgfile_intval (option, value, "sound_stereo_separation", &p->sound_stereo_separation, 1)
		|| cfgfile_intval (option, value, "sound_stereo_mixing_delay", &p->sound_mixed_stereo_delay, 1)

		|| cfgfile_intval (option, value, "gfx_display", &p->gfx_display, 1)
		|| cfgfile_intval (option, value, "gfx_framerate", &p->gfx_framerate, 1)
		|| cfgfile_intval (option, value, "gfx_width_windowed", &p->gfx_size_win.width, 1)
		|| cfgfile_intval (option, value, "gfx_height_windowed", &p->gfx_size_win.height, 1)
		|| cfgfile_intval (option, value, "gfx_top_windowed", &p->gfx_size_win.x, 1)
		|| cfgfile_intval (option, value, "gfx_left_windowed", &p->gfx_size_win.y, 1)
		|| cfgfile_intval (option, value, "gfx_width_fullscreen", &p->gfx_size_fs.width, 1)
		|| cfgfile_intval (option, value, "gfx_height_fullscreen", &p->gfx_size_fs.height, 1)
		|| cfgfile_intval (option, value, "gfx_refreshrate", &p->gfx_refreshrate, 1)
		|| cfgfile_intval (option, value, "gfx_autoresolution", &p->gfx_autoresolution, 1)
		|| cfgfile_intval (option, value, "gfx_backbuffers", &p->gfx_backbuffers, 1)

		|| cfgfile_intval (option, value, "gfx_center_horizontal_position", &p->gfx_xcenter_pos, 1)
		|| cfgfile_intval (option, value, "gfx_center_vertical_position", &p->gfx_ycenter_pos, 1)
		|| cfgfile_intval (option, value, "gfx_center_horizontal_size", &p->gfx_xcenter_size, 1)
		|| cfgfile_intval (option, value, "gfx_center_vertical_size", &p->gfx_ycenter_size, 1)

#ifdef GFXFILTER
		|| cfgfile_intval (option, value, "gfx_filter_vert_zoom", &p->gfx_filter_vert_zoom, 1)
		|| cfgfile_intval (option, value, "gfx_filter_horiz_zoom", &p->gfx_filter_horiz_zoom, 1)
		|| cfgfile_intval (option, value, "gfx_filter_vert_zoom_mult", &p->gfx_filter_vert_zoom_mult, 1)
		|| cfgfile_intval (option, value, "gfx_filter_horiz_zoom_mult", &p->gfx_filter_horiz_zoom_mult, 1)
		|| cfgfile_intval (option, value, "gfx_filter_vert_offset", &p->gfx_filter_vert_offset, 1)
		|| cfgfile_intval (option, value, "gfx_filter_horiz_offset", &p->gfx_filter_horiz_offset, 1)
		|| cfgfile_intval (option, value, "gfx_filter_scanlines", &p->gfx_filter_scanlines, 1)
		|| cfgfile_intval (option, value, "gfx_filter_scanlinelevel", &p->gfx_filter_scanlinelevel, 1)
		|| cfgfile_intval (option, value, "gfx_filter_scanlineratio", &p->gfx_filter_scanlineratio, 1)
		|| cfgfile_intval (option, value, "gfx_filter_luminance", &p->gfx_filter_luminance, 1)
		|| cfgfile_intval (option, value, "gfx_filter_contrast", &p->gfx_filter_contrast, 1)
		|| cfgfile_intval (option, value, "gfx_filter_saturation", &p->gfx_filter_saturation, 1)
		|| cfgfile_intval (option, value, "gfx_filter_gamma", &p->gfx_filter_gamma, 1)
		|| cfgfile_intval (option, value, "gfx_filter_blur", &p->gfx_filter_blur, 1)
		|| cfgfile_intval (option, value, "gfx_filter_noise", &p->gfx_filter_noise, 1)
		|| cfgfile_intval (option, value, "gfx_filter_bilinear", &p->gfx_filter_bilinear, 1)
		|| cfgfile_intval (option, value, "gfx_luminance", &p->gfx_luminance, 1)
		|| cfgfile_intval (option, value, "gfx_contrast", &p->gfx_contrast, 1)
		|| cfgfile_intval (option, value, "gfx_gamma", &p->gfx_gamma, 1)
		|| cfgfile_string (option, value, "gfx_filter_mask", p->gfx_filtermask, sizeof p->gfx_filtermask / sizeof (TCHAR))
#endif
#ifdef DRIVESOUND
		|| cfgfile_intval (option, value, "floppy0sound", &p->dfxclick[0], 1)
		|| cfgfile_intval (option, value, "floppy1sound", &p->dfxclick[1], 1)
		|| cfgfile_intval (option, value, "floppy2sound", &p->dfxclick[2], 1)
		|| cfgfile_intval (option, value, "floppy3sound", &p->dfxclick[3], 1)
		|| cfgfile_intval (option, value, "floppy_volume", &p->dfxclickvolume, 1)
#endif
		|| cfgfile_intval (option, value, "override_dga_address", &p->override_dga_address, 1))
		return 1;

	if (
#ifdef DRIVESOUND
		cfgfile_string (option, value, "floppy0soundext", p->dfxclickexternal[0], sizeof p->dfxclickexternal[0] / sizeof (TCHAR))
		|| cfgfile_string (option, value, "floppy1soundext", p->dfxclickexternal[1], sizeof p->dfxclickexternal[1] / sizeof (TCHAR))
		|| cfgfile_string (option, value, "floppy2soundext", p->dfxclickexternal[2], sizeof p->dfxclickexternal[2] / sizeof (TCHAR))
		|| cfgfile_string (option, value, "floppy3soundext", p->dfxclickexternal[3], sizeof p->dfxclickexternal[3] / sizeof (TCHAR))
		|| 
#endif
		   cfgfile_string (option, value, "gfx_display_name", p->gfx_display_name, sizeof p->gfx_display_name / sizeof (TCHAR))
		|| cfgfile_string (option, value, "config_info", p->info, sizeof p->info / sizeof (TCHAR))
		|| cfgfile_string (option, value, "config_description", p->description, sizeof p->description / sizeof (TCHAR)))
		return 1;

	if (cfgfile_yesno (option, value, "use_debugger", &p->start_debugger)
		|| cfgfile_yesno (option, value, "sound_auto", &p->sound_auto)
		|| cfgfile_yesno (option, value, "sound_stereo_swap_paula", &p->sound_stereo_swap_paula)
		|| cfgfile_yesno (option, value, "sound_stereo_swap_ahi", &p->sound_stereo_swap_ahi)
		|| cfgfile_yesno (option, value, "state_replay", &p->statecapture)
		|| cfgfile_yesno (option, value, "avoid_cmov", &p->avoid_cmov)
		|| cfgfile_yesno (option, value, "avoid_dga", &p->avoid_dga)
		|| cfgfile_yesno (option, value, "avoid_vid", &p->avoid_vid)
		|| cfgfile_yesno (option, value, "log_illegal_mem", &p->illegal_mem)
		|| cfgfile_yesno (option, value, "filesys_no_fsdb", &p->filesys_no_uaefsdb)
		|| cfgfile_yesno (option, value, "gfx_vsync_picasso", &p->gfx_pvsync)
		|| cfgfile_yesno (option, value, "gfx_blacker_than_black", &p->gfx_blackerthanblack)
		|| cfgfile_yesno (option, value, "gfx_flickerfixer", &p->gfx_scandoubler)
		|| cfgfile_yesno (option, value, "synchronize_clock", &p->tod_hack)
		|| cfgfile_yesno (option, value, "magic_mouse", &p->input_magic_mouse)
		|| cfgfile_yesno (option, value, "warp", &p->turbo_emulation)
		|| cfgfile_yesno (option, value, "headless", &p->headless)
		|| cfgfile_yesno (option, value, "bsdsocket_emu", &p->socket_emu))
		return 1;

	if    (cfgfile_intval (option, value, "gfx_gl_x_offset", &p->gfx_gl_x_offset, 1)   //koko
		|| cfgfile_intval (option, value, "gfx_gl_y_offset", &p->gfx_gl_y_offset, 1)  //koko
		|| cfgfile_intval (option, value, "gfx_gl_panscan", &p->gfx_gl_panscan, 1))  //koko
		return 1;

	if (cfgfile_strval (option, value, "sound_output", &p->produce_sound, soundmode1, 1)
		|| cfgfile_strval (option, value, "sound_output", &p->produce_sound, soundmode2, 0)
		|| cfgfile_strval (option, value, "sound_interpol", &p->sound_interpol, interpolmode, 0)
		|| cfgfile_strval (option, value, "sound_filter", &p->sound_filter, soundfiltermode1, 0)
		|| cfgfile_strval (option, value, "sound_filter_type", &p->sound_filter_type, soundfiltermode2, 0)
		|| cfgfile_strval (option, value, "use_gui", &p->start_gui, guimode1, 1)
		|| cfgfile_strval (option, value, "use_gui", &p->start_gui, guimode2, 1)
		|| cfgfile_strval (option, value, "use_gui", &p->start_gui, guimode3, 0)
		|| cfgfile_strval (option, value, "gfx_resolution", &p->gfx_resolution, lorestype1, 0)
		|| cfgfile_strval (option, value, "gfx_lores", &p->gfx_resolution, lorestype2, 0)
		|| cfgfile_strval (option, value, "gfx_lores_mode", &p->gfx_lores_mode, loresmode, 0)
		|| cfgfile_strval (option, value, "gfx_fullscreen_amiga", &p->gfx_afullscreen, fullmodes, 0)
		|| cfgfile_strval (option, value, "gfx_fullscreen_picasso", &p->gfx_pfullscreen, fullmodes, 0)
		|| cfgfile_strval (option, value, "gfx_linemode", &p->gfx_linedbl, linemode1, 1)
		|| cfgfile_strval (option, value, "gfx_linemode", &p->gfx_linedbl, linemode2, 0)
		|| cfgfile_strval (option, value, "gfx_center_horizontal", &p->gfx_xcenter, centermode1, 1)
		|| cfgfile_strval (option, value, "gfx_center_vertical", &p->gfx_ycenter, centermode1, 1)
		|| cfgfile_strval (option, value, "gfx_center_horizontal", &p->gfx_xcenter, centermode2, 0)
		|| cfgfile_strval (option, value, "gfx_center_vertical", &p->gfx_ycenter, centermode2, 0)
		|| cfgfile_strval (option, value, "gfx_colour_mode", &p->color_mode, colormode1, 1)
		|| cfgfile_strval (option, value, "gfx_colour_mode", &p->color_mode, colormode2, 0)
		|| cfgfile_strval (option, value, "gfx_color_mode", &p->color_mode, colormode1, 1)
		|| cfgfile_strval (option, value, "gfx_color_mode", &p->color_mode, colormode2, 0)
		|| cfgfile_strval (option, value, "gfx_max_horizontal", &p->gfx_max_horizontal, maxhoriz, 0)
		|| cfgfile_strval (option, value, "gfx_max_vertical", &p->gfx_max_vertical, maxvert, 0)
		|| cfgfile_strval (option, value, "gfx_filter_autoscale", &p->gfx_filter_autoscale, autoscale, 0)
		|| cfgfile_strval (option, value, "gfx_api", &p->gfx_api, filterapi, 0)
		|| cfgfile_strval (option, value, "magic_mousecursor", &p->input_magic_mouse_cursor, magiccursors, 0)
		|| cfgfile_strval (option, value, "gfx_filter_keep_aspect", &p->gfx_filter_keep_aspect, aspects, 0)
		|| cfgfile_strval (option, value, "absolute_mouse", &p->input_tablet, abspointers, 0))
		return 1;


	if (_tcscmp (option, "gfx_vsync") == 0) {
		if (cfgfile_strval (option, value, "gfx_vsync", &p->gfx_avsync, vsyncmodes, 0) >= 0)
			return 1;
		return cfgfile_yesno (option, value, "gfx_vsync", &p->gfx_avsync);
	}

	if (cfgfile_yesno (option, value, "show_leds", &v)) {
		if (v)
			p->leds_on_screen |= STATUSLINE_CHIPSET;
		return 1;
	}
	if (cfgfile_yesno (option, value, "show_leds_rtg", &v)) {
		if (v)
			p->leds_on_screen |= STATUSLINE_RTG;
		return 1;
	}

#ifdef GFXFILTER
	if (_tcscmp (option, "gfx_filter") == 0) {
	int i = 0;
		char *s = _tcschr (value, ':');
		p->gfx_filtershader[0] = 0;
	p->gfx_filter = 0;
		if (s) {
			*s++ = 0;
			if (!_tcscmp (value, "D3D")) {
				p->gfx_api = 1;
				_tcscpy (p->gfx_filtershader, s);
			}
		}
		if (!_tcscmp (value, "direct3d")) {
			p->gfx_api = 1; // forwards compatibiity
		} else {
	while(uaefilters[i].name) {
				if (!_tcscmp (uaefilters[i].cfgname, value)) {
		p->gfx_filter = uaefilters[i].type;
		break;
	    }
	    i++;
	}
		}
	return 1;
    }
	if (_tcscmp (option, "gfx_filter_mode") == 0) {
	p->gfx_filter_filtermode = 0;
	if (p->gfx_filter > 0) {
	    struct uae_filter *uf;
	    int i = 0;
	    while(uaefilters[i].name) {
		uf = &uaefilters[i];
		if (uf->type == p->gfx_filter) {
					if (!uf->x[0]) {
			int mt[4], j;
			i = 0;
						if (uf->x[1])
							mt[i++] = 1;
						if (uf->x[2])
							mt[i++] = 2;
						if (uf->x[3])
							mt[i++] = 3;
						if (uf->x[4])
							mt[i++] = 4;
						cfgfile_strval (option, value, "gfx_filter_mode", &i, filtermode2, 0);
			for (j = 0; j < i; j++) {
			    if (mt[j] == i)
				p->gfx_filter_filtermode = j;
			}
		    }
		    break;
		}
		i++;
	    }
	}
	return 1;
    }
	if (cfgfile_string (option, value, "gfx_filter_aspect_ratio", tmpbuf, sizeof tmpbuf / sizeof (TCHAR))) {
		int v1, v2;
		TCHAR *s;

		p->gfx_filter_aspect = -1;
		v1 = _tstol (tmpbuf);
		s = _tcschr (tmpbuf, ':');
		if (s) {
			v2 = _tstol (s + 1);
			if (v1 < 0 || v2 < 0)
				p->gfx_filter_aspect = -1;
			else if (v1 == 0 || v2 == 0)
				p->gfx_filter_aspect = 0;
			else
				p->gfx_filter_aspect = (v1 << 8) | v2;
		}
		return 1;
	}
#endif

	if (_tcscmp (option, "gfx_width") == 0 || _tcscmp (option, "gfx_height") == 0) {
		cfgfile_intval (option, value, "gfx_width", &p->gfx_width_win, 1);
		cfgfile_intval (option, value, "gfx_height", &p->gfx_height_win, 1);
		p->gfx_width_fs = p->gfx_width_win;
		p->gfx_height_fs = p->gfx_height_win;
		return 1;
    }

	if (_tcscmp (option, "gfx_fullscreen_multi") == 0 || _tcscmp (option, "gfx_windowed_multi") == 0) {
		TCHAR tmp[256], *tmpp, *tmpp2;
		struct wh *wh = p->gfx_size_win_xtra;
		if (_tcscmp (option, "gfx_fullscreen_multi") == 0)
			wh = p->gfx_size_fs_xtra;
		_stprintf (tmp, ",%s,", value);
		tmpp2 = tmp;
		for (i = 0; i < 4; i++) {
			tmpp = _tcschr (tmpp2, ',');
			tmpp++;
			wh[i].width = _tstol (tmpp);
			while (*tmpp != ',' && *tmpp != 'x')
				tmpp++;
			wh[i].height = _tstol (tmpp + 1);
			tmpp2 = tmpp;
		}
		return 1;
	}

	if (_tcscmp (option, "joyportfriendlyname0") == 0 || _tcscmp (option, "joyportfriendlyname1") == 0) {
		inputdevice_joyport_config (p, value, _tcscmp (option, "joyportfriendlyname0") == 0 ? 0 : 1, 0, 2);
		return 1;
	}
	if (_tcscmp (option, "joyportfriendlyname2") == 0 || _tcscmp (option, "joyportfriendlyname3") == 0) {
		inputdevice_joyport_config (p, value, _tcscmp (option, "joyportfriendlyname2") == 0 ? 2 : 3, 0, 2);
		return 1;
	}
	if (_tcscmp (option, "joyportname0") == 0 || _tcscmp (option, "joyportname1") == 0) {
		inputdevice_joyport_config (p, value, _tcscmp (option, "joyportname0") == 0 ? 0 : 1, 0, 1);
		return 1;
	}
	if (_tcscmp (option, "joyportname2") == 0 || _tcscmp (option, "joyportname3") == 0) {
		inputdevice_joyport_config (p, value, _tcscmp (option, "joyportname2") == 0 ? 2 : 3, 0, 1);
		return 1;
	}
	if (_tcscmp (option, "joyport0") == 0 || _tcscmp (option, "joyport1") == 0) {
		inputdevice_joyport_config (p, value, _tcscmp (option, "joyport0") == 0 ? 0 : 1, 0, 0);
		return 1;
	}
	if (_tcscmp (option, "joyport2") == 0 || _tcscmp (option, "joyport3") == 0) {
		inputdevice_joyport_config (p, value, _tcscmp (option, "joyport2") == 0 ? 2 : 3, 0, 0);
		return 1;
	}
	if (cfgfile_strval (option, value, "joyport0mode", &p->jports[0].mode, joyportmodes, 0))
		return 1;
	if (cfgfile_strval (option, value, "joyport1mode", &p->jports[1].mode, joyportmodes, 0))
		return 1;
	if (cfgfile_strval (option, value, "joyport2mode", &p->jports[2].mode, joyportmodes, 0))
		return 1;
	if (cfgfile_strval (option, value, "joyport3mode", &p->jports[3].mode, joyportmodes, 0))
		return 1;

#ifdef SAVESTATE
    if (cfgfile_string (option, value, "statefile", tmpbuf, sizeof (tmpbuf) / sizeof (char))) {
		strcpy (savestate_fname, tmpbuf);
		if (zfile_exists (savestate_fname)) {
			savestate_state = STATE_DORESTORE;
		} else {
			int ok = 0;
			if (savestate_fname[0]) {
				for (;;) {
					char *p;
					if (my_existsdir (savestate_fname)) {
						ok = 1;
						break;
					}
					p = _tcsrchr (savestate_fname, '\\');
					if (!p)
						p = _tcsrchr (savestate_fname, '/');
					if (!p)
						break;
					*p = 0;
				}
			}
			if (!ok)
				savestate_fname[0] = 0;
		}
	return 1;
    }
#endif

	if (cfgfile_strval (option, value, "sound_channels", &p->sound_stereo, stereomode, 1)) {
		if (p->sound_stereo == SND_NONE) { /* "mixed stereo" compatibility hack */
			p->sound_stereo = SND_STEREO;
			p->sound_mixed_stereo_delay = 5;
			p->sound_stereo_separation = 7;
		}
	return 1;
    }

	if (_tcscmp (option, "kbd_lang") == 0) {
		KbdLang l;
		if ((l = KBD_LANG_DE, strcasecmp (value, "de") == 0)
		    || (l = KBD_LANG_DK, strcasecmp (value, "dk") == 0)
		    || (l = KBD_LANG_SE, strcasecmp (value, "se") == 0)
		    || (l = KBD_LANG_US, strcasecmp (value, "us") == 0)
		    || (l = KBD_LANG_FR, strcasecmp (value, "fr") == 0)
		    || (l = KBD_LANG_IT, strcasecmp (value, "it") == 0)
		    || (l = KBD_LANG_ES, strcasecmp (value, "es") == 0))
		    p->keyboard_lang = l;
		else
		    write_log ("Unknown keyboard language\n");
		return 1;
    }

    if (cfgfile_string (option, value, "config_version", tmpbuf, sizeof (tmpbuf) / sizeof (char))) {
		char *tmpp2;
		tmpp = strchr (value, '.');
	if (tmpp) {
	    *tmpp++ = 0;
	    tmpp2 = tmpp;
	    p->config_version = atol (tmpbuf) << 16;
	    tmpp = strchr (tmpp, '.');
	    if (tmpp) {
		*tmpp++ = 0;
		p->config_version |= atol (tmpp2) << 8;
		p->config_version |= atol (tmpp);
	    }
	}
	return 1;
    }

    if (cfgfile_string (option, value, "keyboard_leds", tmpbuf, sizeof (tmpbuf) / sizeof (char))) {
	char *tmpp2 = tmpbuf;
	int i, num;
	p->keyboard_leds[0] = p->keyboard_leds[1] = p->keyboard_leds[2] = 0;
	p->keyboard_leds_in_use = 0;
		_tcscat (tmpbuf, ",");
	for (i = 0; i < 3; i++) {
			tmpp = _tcschr (tmpp2, ':');
	    if (!tmpp)
		break;
	    *tmpp++= 0;
	    num = -1;
			if (!strcasecmp (tmpp2, "numlock"))
				num = 0;
			if (!strcasecmp (tmpp2, "capslock"))
				num = 1;
			if (!strcasecmp (tmpp2, "scrolllock"))
				num = 2;
	    tmpp2 = tmpp;
			tmpp = _tcschr (tmpp2, ',');
	    if (!tmpp)
		break;
	    *tmpp++= 0;
	    if (num >= 0) {
		p->keyboard_leds[num] = match_string (kbleds, tmpp2);
				if (p->keyboard_leds[num])
					p->keyboard_leds_in_use = 1;
	    }
	    tmpp2 = tmpp;
	}
	return 1;
    }

    return 0;
}

static void decode_rom_ident (char *romfile, int maxlen, char *ident)
{
	char *p;
	int ver, rev, subver, subrev, round, i;
	char model[64], *modelp;
	struct romlist **rl;
	char *romtxt;

	if (!ident[0])
		return;
	romtxt = xmalloc (char, 10000);
	romtxt[0] = 0;
	for (round = 0; round < 2; round++) {
		ver = rev = subver = subrev = -1;
		modelp = NULL;
		memset (model, 0, sizeof model);
		p = ident;
		while (*p) {
			char c = *p++;
			int *pp1 = NULL, *pp2 = NULL;
			if (toupper(c) == 'V' && _istdigit(*p)) {
				pp1 = &ver;
				pp2 = &rev;
			} else if (toupper(c) == 'R' && _istdigit(*p)) {
				pp1 = &subver;
				pp2 = &subrev;
			} else if (!_istdigit(c) && c != ' ') {
				_tcsncpy (model, p - 1, (sizeof model) - 1);
				p += _tcslen (model);
				modelp = model;
			}
			if (pp1) {
				*pp1 = _tstol(p);
				while (*p != 0 && *p != '.' && *p != ' ')
					p++;
				if (*p == '.') {
					p++;
					if (pp2)
						*pp2 = _tstol(p);
				}
			}
			if (*p == 0 || *p == ';') {
				rl = getromlistbyident (ver, rev, subver, subrev, modelp, round);
				if (rl) {
					for (i = 0; rl[i]; i++) {
						if (round) {
							char romname[MAX_DPATH];
							getromname(rl[i]->rd, romname);
							_tcscat (romtxt, romname);
							_tcscat (romtxt, "\n");
						} else {
							_tcsncpy (romfile, rl[i]->path, maxlen);
							goto end;
						}
					}
					xfree (rl);
				}
			}
		}
	}
end:
	if (round && romtxt[0]) {
        gui_message("One of the following system ROMs is required:\n\n%s\n\nCheck the System ROM path in the Paths panel and click Rescan ROMs.\n");
		;//notify_user_parms (NUMSG_ROMNEED, romtxt, romtxt);
	}
	xfree (romtxt);
}

static struct uaedev_config_info *getuci(struct uae_prefs *p)
{
	if (p->mountitems < MOUNT_CONFIG_SIZE)
		return &p->mountconfig[p->mountitems++];
	return NULL;
}

struct uaedev_config_info *add_filesys_config (struct uae_prefs *p, int index,
	char *devname, char *volname, char *rootdir, int readonly,
	int secspertrack, int surfaces, int reserved,
	int blocksize, int bootpri,
	char *filesysdir, int hdc, int flags)
{
	struct uaedev_config_info *uci;
	int i;
	char *s;

	if (index < 0 && devname && _tcslen (devname) > 0) {
		for (i = 0; i < p->mountitems; i++) {
			if (p->mountconfig[i].devname && !_tcscmp (p->mountconfig[i].devname, devname))
				return 0;
		}
	}

	if (index < 0) {
		uci = getuci(p);
		uci->configoffset = -1;
	} else {
		uci = &p->mountconfig[index];
	}
	if (!uci)
		return 0;

	uci->ishdf = volname == NULL ? 1 : 0;
	_tcscpy (uci->devname, devname ? devname : "");
	_tcscpy (uci->volname, volname ? volname : "");
	_tcscpy (uci->rootdir, rootdir ? rootdir : "");
	validatedevicename (uci->devname);
	validatevolumename (uci->volname);
	uci->readonly = readonly;
	uci->sectors = secspertrack;
	uci->surfaces = surfaces;
	uci->reserved = reserved;
	uci->blocksize = blocksize;
	uci->bootpri = bootpri;
	uci->donotmount = 0;
	uci->autoboot = 0;
	if (bootpri < -128)
		uci->donotmount = 1;
	else if (bootpri >= -127)
		uci->autoboot = 1;
	uci->controller = hdc;
	_tcscpy (uci->filesys, filesysdir ? filesysdir : "");
	if (!uci->devname[0]) {
		char base[32];
		char base2[32];
		int num = 0;
		if (uci->rootdir[0] == 0 && !uci->ishdf)
			_tcscpy (base, "RDH");
		else
			_tcscpy (base, "DH");
		_tcscpy (base2, base);
		for (i = 0; i < p->mountitems; i++) {
			_stprintf (base2, "%s%d", base, num);
			if (!_tcscmp(base2, p->mountconfig[i].devname)) {
				num++;
				i = -1;
				continue;
			}
		}
		_tcscpy (uci->devname, base2);
		validatedevicename (uci->devname);
	}
	s = filesys_createvolname (volname, rootdir, "Harddrive");
	_tcscpy (uci->volname, s);
	xfree (s);
	return uci;
}

static void parse_addmem (struct uae_prefs *p, char *buf, int num)
{
	int size = 0, addr = 0;

	if (!getintval2 (&buf, &addr, ','))
		return;
	if (!getintval2 (&buf, &size, 0))
		return;
	if (addr & 0xffff)
		return;
	if ((size & 0xffff) || (size & 0xffff0000) == 0)
		return;
	p->custom_memory_addrs[num] = addr;
	p->custom_memory_sizes[num] = size;
}

static int cfgfile_parse_hardware (struct uae_prefs *p, char *option, char *value)
{
	int tmpval, dummyint, i;
	char *section = 0;
	char tmpbuf[CONFIG_BLEN];

	if (cfgfile_yesno (option, value, "cpu_cycle_exact", &p->cpu_cycle_exact)
		|| cfgfile_yesno (option, value, "blitter_cycle_exact", &p->blitter_cycle_exact)) {
#ifdef JIT
			if (p->cpu_model >= 68020 && p->cachesize > 0)
				p->cpu_cycle_exact = p->blitter_cycle_exact = 0;
			/* we don't want cycle-exact in 68020/40+JIT modes */
#endif
			return 1;
	}
	if (cfgfile_yesno (option, value, "cycle_exact", &tmpval)) {
		p->cpu_cycle_exact = p->blitter_cycle_exact = tmpval ? 1 : 0;
#ifdef JIT
		if (p->cpu_model >= 68020 && p->cachesize > 0)
			p->cpu_cycle_exact = p->blitter_cycle_exact = 0;
#endif
		return 1;
	}

	if (cfgfile_string (option, value, "cpu_multiplier", tmpbuf, sizeof tmpbuf / sizeof (char))) {
		p->cpu_clock_multiplier = (int)(_tstof (tmpbuf) * 256.0);
		return 1;
	}


	if (cfgfile_yesno (option, value, "scsi_a3000", &dummyint)) {
		if (dummyint)
			p->cs_mbdmac = 1;
		return 1;
	}
	if (cfgfile_yesno (option, value, "scsi_a4000t", &dummyint)) {
		if (dummyint)
			p->cs_mbdmac = 2;
		return 1;
	}

	if (cfgfile_string (option, value, "a2065", p->a2065name, sizeof p->a2065name / sizeof (char)))
		return 1;

	if (cfgfile_yesno (option, value, "immediate_blits", &p->immediate_blits)
		|| cfgfile_yesno (option, value, "cd32cd", &p->cs_cd32cd)
		|| cfgfile_yesno (option, value, "cd32c2p", &p->cs_cd32c2p)
		|| cfgfile_yesno (option, value, "cd32nvram", &p->cs_cd32nvram)
		|| cfgfile_yesno (option, value, "cdtvcd", &p->cs_cdtvcd)
		|| cfgfile_yesno (option, value, "cdtvram", &p->cs_cdtvram)
		|| cfgfile_yesno (option, value, "a1000ram", &p->cs_a1000ram)
		|| cfgfile_yesno (option, value, "pcmcia", &p->cs_pcmcia)
		|| cfgfile_yesno (option, value, "scsi_cdtv", &p->cs_cdtvscsi)
		|| cfgfile_yesno (option, value, "scsi_a4091", &p->cs_a4091)
		|| cfgfile_yesno (option, value, "scsi_a2091", &p->cs_a2091)
		|| cfgfile_yesno (option, value, "cia_overlay", &p->cs_ciaoverlay)
		|| cfgfile_yesno (option, value, "bogomem_fast", &p->cs_slowmemisfast)
		|| cfgfile_yesno (option, value, "ksmirror_e0", &p->cs_ksmirror_e0)
		|| cfgfile_yesno (option, value, "ksmirror_a8", &p->cs_ksmirror_a8)
		|| cfgfile_yesno (option, value, "resetwarning", &p->cs_resetwarning)
		|| cfgfile_yesno (option, value, "denise_noehb", &p->cs_denisenoehb)
		|| cfgfile_yesno (option, value, "ics_agnus", &p->cs_dipagnus)
		|| cfgfile_yesno (option, value, "agnus_bltbusybug", &p->cs_agnusbltbusybug)

		|| cfgfile_yesno (option, value, "kickshifter", &p->kickshifter)
		|| cfgfile_yesno (option, value, "ntsc", &p->ntscmode)
		|| cfgfile_yesno (option, value, "sana2", &p->sana2)
		|| cfgfile_yesno (option, value, "genlock", &p->genlock)
		|| cfgfile_yesno (option, value, "cpu_compatible", &p->cpu_compatible)
		|| cfgfile_yesno (option, value, "cpu_24bit_addressing", &p->address_space_24)
		|| cfgfile_yesno (option, value, "parallel_on_demand", &p->parallel_demand)
		|| cfgfile_yesno (option, value, "parallel_postscript_emulation", &p->parallel_postscript_emulation)
		|| cfgfile_yesno (option, value, "parallel_postscript_detection", &p->parallel_postscript_detection)
		|| cfgfile_yesno (option, value, "serial_on_demand", &p->serial_demand)
		|| cfgfile_yesno (option, value, "serial_hardware_ctsrts", &p->serial_hwctsrts)
		|| cfgfile_yesno (option, value, "serial_direct", &p->serial_direct)
#ifdef JIT
		|| cfgfile_yesno (option, value, "comp_nf", &p->compnf)
		|| cfgfile_yesno (option, value, "comp_constjump", &p->comp_constjump)
		|| cfgfile_yesno (option, value, "comp_oldsegv", &p->comp_oldsegv)
		|| cfgfile_yesno (option, value, "compforcesettings", &dummyint)
		|| cfgfile_yesno (option, value, "compfpu", &p->compfpu)
#endif
#ifdef FPU
		|| cfgfile_yesno (option, value, "fpu_strict", &p->fpu_strict)
#endif
#ifdef JIT
		|| cfgfile_yesno (option, value, "comp_midopt", &p->comp_midopt)
		|| cfgfile_yesno (option, value, "comp_lowopt", &p->comp_lowopt)
#endif
		|| cfgfile_yesno (option, value, "rtg_nocustom", &p->picasso96_nocustom)
		|| cfgfile_yesno (option, value, "uaeserial", &p->uaeserial))
		return 1;

	if (cfgfile_intval (option, value, "cpu060_revision", &p->cpu060_revision, 1)
		|| cfgfile_intval (option, value, "fpu_revision", &p->fpu_revision, 1)
		|| cfgfile_intval (option, value, "cdtvramcard", &p->cs_cdtvcard, 1)
		|| cfgfile_intval (option, value, "fatgary", &p->cs_fatgaryrev, 1)
		|| cfgfile_intval (option, value, "ramsey", &p->cs_ramseyrev, 1)
		|| cfgfile_intval (option, value, "chipset_refreshrate", &p->chipset_refreshrate, 1)
		|| cfgfile_intval (option, value, "fastmem_size", &p->fastmem_size, 0x100000)
		|| cfgfile_intval (option, value, "a3000mem_size", &p->mbresmem_low_size, 0x100000)
		|| cfgfile_intval (option, value, "mbresmem_size", &p->mbresmem_high_size, 0x100000)
		|| cfgfile_intval (option, value, "z3mem_size", &p->z3fastmem_size, 0x100000)
		|| cfgfile_intval (option, value, "z3mem2_size", &p->z3fastmem2_size, 0x100000)
		|| cfgfile_intval (option, value, "z3mem_start", &p->z3fastmem_start, 1)
		|| cfgfile_intval (option, value, "bogomem_size", &p->bogomem_size, 0x40000)
		|| cfgfile_intval (option, value, "gfxcard_size", &p->gfxmem_size, 0x100000)
		|| cfgfile_intval (option, value, "rtg_modes", &p->picasso96_modeflags, 1)
		|| cfgfile_intval (option, value, "floppy_speed", &p->floppy_speed, 1)
		|| cfgfile_intval (option, value, "floppy_write_length", &p->floppy_write_length, 1)
		|| cfgfile_intval (option, value, "nr_floppies", &p->nr_floppies, 1)
		|| cfgfile_intval (option, value, "floppy0type", &p->dfxtype[0], 1)
		|| cfgfile_intval (option, value, "floppy1type", &p->dfxtype[1], 1)
		|| cfgfile_intval (option, value, "floppy2type", &p->dfxtype[2], 1)
		|| cfgfile_intval (option, value, "floppy3type", &p->dfxtype[3], 1)
		|| cfgfile_intval (option, value, "maprom", &p->maprom, 1)
		|| cfgfile_intval (option, value, "parallel_autoflush", &p->parallel_autoflush_time, 1)
		|| cfgfile_intval (option, value, "uae_hide", &p->uae_hide, 1)
		|| cfgfile_intval (option, value, "cpu_frequency", &p->cpu_frequency, 1)
		|| cfgfile_intval (option, value, "catweasel", &p->catweasel, 1))
	return 1;

#ifdef JIT
    if (cfgfile_intval (option, value, "cachesize", &p->cachesize, 1)
# ifdef NATMEM_OFFSET
	|| cfgfile_strval (option, value, "comp_trustbyte",  &p->comptrustbyte,  compmode, 0)
	|| cfgfile_strval (option, value, "comp_trustword",  &p->comptrustword,  compmode, 0)
	|| cfgfile_strval (option, value, "comp_trustlong",  &p->comptrustlong,  compmode, 0)
	|| cfgfile_strval (option, value, "comp_trustnaddr", &p->comptrustnaddr, compmode, 0)
# else
	|| cfgfile_strval (option, value, "comp_trustbyte",  &p->comptrustbyte,  compmode, 1)
	|| cfgfile_strval (option, value, "comp_trustword",  &p->comptrustword,  compmode, 1)
	|| cfgfile_strval (option, value, "comp_trustlong",  &p->comptrustlong,  compmode, 1)
	|| cfgfile_strval (option, value, "comp_trustnaddr", &p->comptrustnaddr, compmode, 1)
# endif
	|| cfgfile_strval (option, value, "comp_flushmode", &p->comp_hardflush, flushmode, 0))
	return 1;
#endif

	if (cfgfile_string (option, value, "kickstart_rom_file", p->romfile, sizeof p->romfile / sizeof (char))
		|| cfgfile_string (option, value, "kickstart_ext_rom_file", p->romextfile, sizeof p->romextfile / sizeof (char))
		|| cfgfile_string (option, value, "amax_rom_file", p->amaxromfile, sizeof p->amaxromfile / sizeof (char))
		|| cfgfile_string (option, value, "flash_file", p->flashfile, sizeof p->flashfile / sizeof (char))
		|| cfgfile_string (option, value, "cart_file", p->cartfile, sizeof p->cartfile / sizeof (char))
		|| cfgfile_string (option, value, "pci_devices", p->pci_devices, sizeof p->pci_devices / sizeof (char))
		|| cfgfile_string (option, value, "ghostscript_parameters", p->ghostscript_parameters, sizeof p->ghostscript_parameters / sizeof (char)))
		return 1;

	if (cfgfile_strval (option, value, "cart_internal", &p->cart_internal, cartsmode, 0)) {
		if (p->cart_internal) {
			struct romdata *rd = getromdatabyid (63);
			if (rd)
				sprintf (p->cartfile, ":%s", rd->configname);
		}
		return 1;
	}
	if (cfgfile_string (option, value, "kickstart_rom", p->romident, sizeof p->romident / sizeof (char))) {
		decode_rom_ident (p->romfile, sizeof p->romfile / sizeof (char), p->romident);
		return 1;
	}
	if (cfgfile_string (option, value, "kickstart_ext_rom", p->romextident, sizeof p->romextident / sizeof (char))) {
		decode_rom_ident (p->romextfile, sizeof p->romextfile / sizeof (char), p->romextident);
		return 1;
	}
	if (cfgfile_string (option, value, "cart", p->cartident, sizeof p->cartident / sizeof (char))) {
		decode_rom_ident (p->cartfile, sizeof p->cartfile / sizeof (char), p->cartident);
		return 1;
	}

    for (i = 0; i < 4; i++) {
		_stprintf (tmpbuf, "floppy%d", i);
		if (cfgfile_string (option, value, tmpbuf, p->df[i], sizeof p->df[i] / sizeof (char)))
			return 1;
	}

	if (cfgfile_intval (option, value, "chipmem_size", &dummyint, 1)) {
		if (dummyint < 0)
			p->chipmem_size = 0x20000; /* 128k, prototype support */
		else if (dummyint == 0)
			p->chipmem_size = 0x40000; /* 256k */
		else
			p->chipmem_size = dummyint * 0x80000;
		return 1;
    }

	if (cfgfile_string (option, value, "addmem1", tmpbuf, sizeof tmpbuf / sizeof (char))) {
		parse_addmem (p, tmpbuf, 0);
		return 1;
	}
	if (cfgfile_string (option, value, "addmem2", tmpbuf, sizeof tmpbuf / sizeof (char))) {
		parse_addmem (p, tmpbuf, 1);
		return 1;
	}

    if (cfgfile_strval (option, value, "chipset", &tmpval, csmode, 0)) {
		set_chipset_mask (p, tmpval);
		return 1;
    }

	if (cfgfile_string (option, value, "mmu_model", tmpbuf, sizeof tmpbuf / sizeof (char))) {
		p->mmu_model = _tstol(tmpbuf);
		return 1;
	}

	if (cfgfile_string (option, value, "fpu_model", tmpbuf, sizeof tmpbuf / sizeof (char))) {
		p->fpu_model = _tstol(tmpbuf);
		return 1;
	}

	if (cfgfile_string (option, value, "cpu_model", tmpbuf, sizeof tmpbuf / sizeof (char))) {
		p->cpu_model = _tstol(tmpbuf);
		p->fpu_model = 0;
		return 1;
	}

	/* old-style CPU configuration */
	if (cfgfile_string (option, value, "cpu_type", tmpbuf, sizeof tmpbuf / sizeof (char))) {
		p->fpu_model = 0;
		p->address_space_24 = 0;
		p->cpu_model = 680000;
		if (!_tcscmp (tmpbuf, "68000")) {
			p->cpu_model = 68000;
		} else if (!_tcscmp (tmpbuf, "68010")) {
			p->cpu_model = 68010;
		} else if (!_tcscmp (tmpbuf, "68ec020")) {
			p->cpu_model = 68020;
			p->address_space_24 = 1;
		} else if (!_tcscmp (tmpbuf, "68020")) {
			p->cpu_model = 68020;
		} else if (!_tcscmp (tmpbuf, "68ec020/68881")) {
			p->cpu_model = 68020;
			p->fpu_model = 68881;
			p->address_space_24 = 1;
		} else if (!_tcscmp (tmpbuf, "68020/68881")) {
			p->cpu_model = 68020;
			p->fpu_model = 68881;
		} else if (!_tcscmp (tmpbuf, "68040")) {
			p->cpu_model = 68040;
			p->fpu_model = 68040;
		} else if (!_tcscmp (tmpbuf, "68060")) {
			p->cpu_model = 68060;
			p->fpu_model = 68060;
		}
		return 1;
	}

    if (p->config_version < (21 << 16)) {
		if (cfgfile_strval (option, value, "cpu_speed", &p->m68k_speed, speedmode, 1)
		    /* Broken earlier versions used to write this out as a string.  */
			|| cfgfile_strval (option, value, "finegraincpu_speed", &p->m68k_speed, speedmode, 1))
		{
		    p->m68k_speed--;
		    return 1;
		}
    }

	if (cfgfile_intval (option, value, "cpu_speed", &p->m68k_speed, 1)) {
		p->m68k_speed *= CYCLE_UNIT;
		return 1;
    }

	if (cfgfile_intval (option, value, "finegrain_cpu_speed", &p->m68k_speed, 1)) {
		if (OFFICIAL_CYCLE_UNIT > CYCLE_UNIT) {
		    int factor = OFFICIAL_CYCLE_UNIT / CYCLE_UNIT;
		    p->m68k_speed = (p->m68k_speed + factor - 1) / factor;
		}
		if (strcasecmp (value, "max") == 0)
		    p->m68k_speed = -1;
		return 1;
    }

	if (cfgfile_intval (option, value, "dongle", &p->dongle, 1)) {
		if (p->dongle == 0)
			cfgfile_strval (option, value, "dongle", &p->dongle, dongles, 0);
		return 1;
	}

	for (i = 0; i < MAX_FILESYSTEM_UNITS; i++) {
		char tmp[100];
		_stprintf (tmp, "uaehf%d", i);
		if (_tcscmp (option, tmp) == 0)
			return 1;
	}

	if (_tcscmp (option, "filesystem") == 0
		|| _tcscmp (option, "hardfile") == 0)
    {
		int secs, heads, reserved, bs, ro;
		char *aname, *root;
		char *tmpp = _tcschr (value, ',');
		char *str;

		if (config_newfilesystem)
		    return 1;

		if (tmpp == 0)
		    goto invalid_fs;

		*tmpp++ = '\0';
		if (strcmp (value, "1") == 0 || strcasecmp (value, "ro") == 0
		    || strcasecmp (value, "readonly") == 0
		    || strcasecmp (value, "read-only") == 0)
		    ro = 1;
		else if (strcmp (value, "0") == 0 || strcasecmp (value, "rw") == 0
			 || strcasecmp (value, "readwrite") == 0
			 || strcasecmp (value, "read-write") == 0)
		    ro = 0;
		else
		    goto invalid_fs;
		secs = 0; heads = 0; reserved = 0; bs = 0;

		value = tmpp;
		if (strcmp (option, "filesystem") == 0) {
		    tmpp = strchr (value, ':');
		    if (tmpp == 0)
				goto invalid_fs;
		    *tmpp++ = '\0';
		    aname = value;
		    root = tmpp;
		} else {
		    if (! getintval (&value, &secs, ',')
				|| ! getintval (&value, &heads, ',')
				|| ! getintval (&value, &reserved, ',')
				|| ! getintval (&value, &bs, ','))
				goto invalid_fs;
		    root = value;
		    aname = 0;
		}
		str = cfgfile_subst_path (UNEXPANDED, p->path_hardfile, root);
#ifdef FILESYS
	{
	    const char *err_msg;
	    char *str;

	    str = cfgfile_subst_path (UNEXPANDED, prefs_get_attr ("hardfile_path"), root);
/*
add_filesys_unit (struct uaedev_mount_info *mountinfo, const char *devname, const char *volname, const char *rootdir, int readonly, 
	int secspertrack, int surfaces, int reserved,
	int blocksize, int bootpri, int donotmount, int autoboot,
	const char *filesysdir, int hdc, int flags)
*/
	    err_msg = add_filesys_unit (currprefs.mountinfo, 0, aname, str, ro, secs, heads, reserved, bs, 0, 0, 0, 0, 0, 0);
	write_log ("-- ADD FILESYS UNIT --\n");
	    if (err_msg)
			write_log ("Error: %s\n", err_msg);

	    free (str);
	}
#endif
	return 1;
    }

	if (_tcscmp (option, "filesystem2") == 0
		|| _tcscmp (option, "hardfile2") == 0)
    {
		int secs, heads, reserved, bs, ro, bp, hdcv;
		char *dname = NULL, *aname = "", *root = NULL, *fs = NULL, *hdc;
		char *tmpp = _tcschr (value, ',');
		char *str = NULL;

		config_newfilesystem = 1;
		if (tmpp == 0)
		    goto invalid_fs;

		*tmpp++ = '\0';
		if (strcasecmp (value, "ro") == 0)
		    ro = 1;
		else if (strcasecmp (value, "rw") == 0)
		    ro = 0;
		else
		    goto invalid_fs;
		secs = 0; heads = 0; reserved = 0; bs = 0; bp = 0;
		fs = 0; hdc = 0; hdcv = 0;

		value = tmpp;
		if (_tcscmp (option, "filesystem2") == 0) {
			tmpp = _tcschr (value, ':');
			if (tmpp == 0)
				goto empty_fs;
			*tmpp++ = 0;
			dname = value;
			aname = tmpp;
			tmpp = _tcschr (tmpp, ':');
			if (tmpp == 0)
				goto empty_fs;
			*tmpp++ = 0;
			root = tmpp;
			tmpp = _tcschr (tmpp, ',');
			if (tmpp == 0)
				goto empty_fs;
			*tmpp++ = 0;
			if (! getintval (&tmpp, &bp, 0))
				goto empty_fs;
	} else {
			tmpp = _tcschr (value, ':');
			if (tmpp == 0)
				goto invalid_fs;
			*tmpp++ = '\0';
			dname = value;
			root = tmpp;
			tmpp = _tcschr (tmpp, ',');
			if (tmpp == 0)
				goto invalid_fs;
			*tmpp++ = 0;
			aname = 0;
			if (! getintval (&tmpp, &secs, ',')
				|| ! getintval (&tmpp, &heads, ',')
				|| ! getintval (&tmpp, &reserved, ',')
				|| ! getintval (&tmpp, &bs, ','))
				goto invalid_fs;
			if (getintval2 (&tmpp, &bp, ',')) {
				fs = tmpp;
				tmpp = _tcschr (tmpp, ',');
				if (tmpp != 0) {
					*tmpp++ = 0;
					hdc = tmpp;
					if(_tcslen (hdc) >= 4 && !_tcsncmp (hdc, "ide", 3)) {
						hdcv = hdc[3] - '0' + HD_CONTROLLER_IDE0;
						if (hdcv < HD_CONTROLLER_IDE0 || hdcv > HD_CONTROLLER_IDE3)
							hdcv = 0;
					}
					if(_tcslen (hdc) >= 5 && !_tcsncmp (hdc, "scsi", 4)) {
						hdcv = hdc[4] - '0' + HD_CONTROLLER_SCSI0;
						if (hdcv < HD_CONTROLLER_SCSI0 || hdcv > HD_CONTROLLER_SCSI6)
							hdcv = 0;
					}
					if (_tcslen (hdc) >= 6 && !_tcsncmp (hdc, "scsram", 6))
						hdcv = HD_CONTROLLER_PCMCIA_SRAM;
				}
	    }
	}
empty_fs:
		if (root)
			str = cfgfile_subst_path (UNEXPANDED, p->path_hardfile, root);
#ifdef FILESYS
		add_filesys_config (p, -1, dname, aname, str, ro, secs, heads, reserved, bs, bp, fs, hdcv, 0);
#endif
	    free (str);
		return 1;

invalid_fs:
		write_log ("Invalid filesystem/hardfile specification.\n");
		return 1;
    }

    return 0;
}

int cfgfile_parse_option (struct uae_prefs *p, char *option, char *value, int type)
{
	if (!_tcscmp (option, "config_hardware"))
		return 1;
	if (!_tcscmp (option, "config_host"))
		return 1;
	if (cfgfile_string (option, value, "config_hardware_path", p->config_hardware_path, sizeof p->config_hardware_path / sizeof (char)))
		return 1;
	if (cfgfile_string (option, value, "config_host_path", p->config_host_path, sizeof p->config_host_path / sizeof (char)))
		return 1;
	if (type == 0 || (type & CONFIG_TYPE_HARDWARE)) {
		if (cfgfile_parse_hardware (p, option, value))
			return 1;
	}
	if (type == 0 || (type & CONFIG_TYPE_HOST)) {
		if (cfgfile_parse_host (p, option, value))
			return 1;
	}
	if (type > 0)
		return 1;
	return 0;
}

static int isutf8ext (TCHAR *s)
{
	if (_tcslen (s) > _tcslen (UTF8NAME) && !_tcscmp (s + _tcslen (s) - _tcslen (UTF8NAME), UTF8NAME)) {
		s[_tcslen (s) - _tcslen (UTF8NAME)] = 0;
		return 1;
	}
	return 0;
}

static int cfgfile_separate_linea (char *line, char *line1b, char *line2b)
{
    char *line1, *line2;
    int i;

    line1 = line;
    line2 = strchr (line, '=');
    if (! line2) {
		write_log ("CFGFILE: linea was incomplete with only %s\n", line1);
		return 0;
    }
    *line2++ = '\0';

    /* Get rid of whitespace.  */
    i = strlen (line2);
    while (i > 0 && (line2[i - 1] == '\t' || line2[i - 1] == ' '
		     || line2[i - 1] == '\r' || line2[i - 1] == '\n'))
	line2[--i] = '\0';
    line2 += strspn (line2, "\t \r\n");

    i = strlen (line);
    while (i > 0 && (line[i - 1] == '\t' || line[i - 1] == ' '
		     || line[i - 1] == '\r' || line[i - 1] == '\n'))
	line[--i] = '\0';
    line += strspn (line, "\t \r\n");
	memcpy (line1b, line, MAX_DPATH);
	if (isutf8ext (line1b)) {
		if (line2[0]) {
			//char *s = utf8u (line2);
			_tcscpy (line2b, line2);
			//xfree (s);
		}
	} else {
		memcpy (line2b, line2, MAX_DPATH);
	}
    return 1;
}

static int cfgfile_separate_line (char *line, char *line1b, char *line2b)
{
	char *line1, *line2;
	int i;

	line1 = line;
	line2 = _tcschr (line, '=');
	if (! line2) {
		write_log ("CFGFILE: line was incomplete with only %s\n", line1);
		return 0;
	}
	*line2++ = '\0';

	/* Get rid of whitespace.  */
	i = _tcslen (line2);
	while (i > 0 && (line2[i - 1] == '\t' || line2[i - 1] == ' '
		|| line2[i - 1] == '\r' || line2[i - 1] == '\n'))
		line2[--i] = '\0';
	line2 += strspn (line2, "\t \r\n");
	_tcscpy (line2b, line2);
	i = _tcslen (line);
	while (i > 0 && (line[i - 1] == '\t' || line[i - 1] == ' '
		|| line[i - 1] == '\r' || line[i - 1] == '\n'))
		line[--i] = '\0';
	line += strspn (line, "\t \r\n");
	_tcscpy (line1b, line);

	if (line2b[0] == '"' || line2b[0] == '\"') {
		char c = line2b[0];
		int i = 0;
		memmove (line2b, line2b + 1, (_tcslen (line2b) + 1) * sizeof (char));
		while (line2b[i] != 0 && line2b[i] != c)
			i++;
		line2b[i] = 0;
	}

	if (isutf8ext (line1b))
		return 0;
	return 1;
}

static int isobsolete (char *s)
{
    int i = 0;
    while (obsolete[i]) {
	if (!strcasecmp (s, obsolete[i])) {
	    write_log ("obsolete config entry '%s'\n", s);
	    return 1;
	}
	i++;
    }
    if (strlen (s) >= 10 && !memcmp (s, "gfx_opengl", 10)) {
		write_log ("obsolete config entry '%s\n", s);
		return 1;
    }
    if (strlen (s) >= 6 && !memcmp (s, "gfx_3d", 6)) {
		write_log ("obsolete config entry '%s\n", s);
		return 1;
    }
    return 0;
}

static void cfgfile_parse_separated_line (struct uae_prefs *p, char *line1b, char *line2b, int type)
{
    char line3b[CONFIG_BLEN], line4b[CONFIG_BLEN];
    struct strlist *sl;
    int ret;

	_tcscpy (line3b, line1b);
	_tcscpy (line4b, line2b);
    ret = cfgfile_parse_option (p, line1b, line2b, type);
	if (!isobsolete (line3b)) {
		for (sl = p->all_lines; sl; sl = sl->next) {
			if (sl->option && !strcasecmp (line1b, sl->option)) break;
		}
		if (!sl) {
			struct strlist *u = xcalloc (struct strlist, 1);
			u->option = my_strdup (line3b);
			u->value = my_strdup (line4b);
			u->next = p->all_lines;
			p->all_lines = u;
			if (!ret) {
				u->unknown = 1;
				write_log ("unknown config entry: '%s=%s'\n", u->option, u->value);
		    }
		}
    }
}

void cfgfile_parse_line (struct uae_prefs *p, char *line, int type)
{
    char line1b[CONFIG_BLEN], line2b[CONFIG_BLEN];

    if (!cfgfile_separate_line (line, line1b, line2b))
	return;
    cfgfile_parse_separated_line (p, line1b, line2b, type);
    return;
}

static void subst (const char *p, char *f, int n)
{
    char *str = cfgfile_subst_path (UNEXPANDED, p, f);
    strncpy (f, str, n - 1);
    f[n - 1] = '\0';
    free (str);
}

static char *cfg_fgets (char *line, int max, FILE *fh)
{
#ifdef SINGLEFILE
    extern char singlefile_config[];
    static char *sfile_ptr;
    char *p;
#endif

    if (fh)
	return fgets (line, max, fh);
#ifdef SINGLEFILE
    if (sfile_ptr == 0) {
	sfile_ptr = singlefile_config;
	if (*sfile_ptr) {
	    write_log ("singlefile config found\n");
	    while (*sfile_ptr++);
	}
    }
    if (*sfile_ptr == 0) {
	sfile_ptr = singlefile_config;
	return 0;
    }
    p = sfile_ptr;
    while (*p != 13 && *p != 10 && *p != 0) p++;
    memset (line, 0, max);
    memcpy (line, sfile_ptr, p - sfile_ptr);
    sfile_ptr = p + 1;
    if (*sfile_ptr == 13)
	sfile_ptr++;
    if (*sfile_ptr == 10)
	sfile_ptr++;
    return line;
#endif
    return 0;
}

static int cfgfile_load_2 (struct uae_prefs *p, const char *filename, int real, int *type)
{
    int i;
    FILE *fh;
	char linea[CONFIG_BLEN];
    char line[CONFIG_BLEN], line1b[CONFIG_BLEN], line2b[CONFIG_BLEN];
    struct strlist *sl;
    int type1 = 0, type2 = 0, askedtype = 0;

    if (type) {
		askedtype = *type;
		*type = 0;
    }
    if (real) {
		p->config_version = 0;
		config_newfilesystem = 0;
		reset_inputdevice_config (p);
    }

    write_log ("Opening cfgfile '%s': ", filename);
    fh = fopen (filename, "r");
#ifndef SINGLEFILE
    if (! fh) {
		write_log ("failed\n");
		return 0;
    }
#endif
    write_log ("okay.\n");

    while (cfg_fgets (linea, sizeof (linea), fh) != 0) {
		trimwsa (linea);
	    //write_log ("%s\n",linea);
		if (strlen (linea) > 0) {
		    if (linea[0] == '#' || linea[0] == ';')
				continue;
		    if (!cfgfile_separate_linea (linea, line1b, line2b))
				continue;
		    type1 = type2 = 0;
			if (cfgfile_yesno (line1b, line2b, "config_hardware", &type1) ||
				cfgfile_yesno (line1b, line2b, "config_host", &type2)) {
					if (type1 && type)
						*type |= CONFIG_TYPE_HARDWARE;
					if (type2 && type)
						*type |= CONFIG_TYPE_HOST;
					continue;
			}
		    if (real) {
				cfgfile_parse_separated_line (p, line1b, line2b, askedtype);
		    } else {
				cfgfile_string (line1b, line2b, "config_description", p->description, sizeof p->description / sizeof (char));
				cfgfile_string (line1b, line2b, "config_hardware_path", p->config_hardware_path, sizeof p->config_hardware_path / sizeof (char));
				cfgfile_string (line1b, line2b, "config_host_path", p->config_host_path, sizeof p->config_host_path / sizeof (char));
		    }
		}
    }

    if (type && *type == 0)
		*type = CONFIG_TYPE_HARDWARE | CONFIG_TYPE_HOST;
    if (fh)
		fclose (fh);

    if (!real)
		return 1;

    for (sl = temp_lines; sl; sl = sl->next) {
		sprintf (line, "%s=%s", sl->option, sl->value);
		cfgfile_parse_line (p, line, 0);
    }

    for (i = 0; i < 4; i++)
		subst (prefs_get_attr("floppy_path"), p->df[i], sizeof p->df[i]);

    subst (prefs_get_attr("rom_path"), p->romfile, sizeof p->romfile);
    subst (prefs_get_attr("rom_path"), p->romextfile, sizeof p->romextfile);
    subst (prefs_get_attr("rom_path"), p->keyfile, sizeof p->keyfile);

    return 1;
}

int cfgfile_load (struct uae_prefs *p, const char *filename, int *type, int ignorelink, int userconfig)
{
    return cfgfile_load_2 (p, filename, 1, type);
}

int cfgfile_save (const struct uae_prefs *p, const char *filename, int type)
{
    FILE *fh = fopen (filename, "w");
    write_log ("save config '%s'\n", filename);
    if (! fh)
	return 0;

    if (!type)
	type = CONFIG_TYPE_HARDWARE | CONFIG_TYPE_HOST;
    cfgfile_save_options (fh, p, type);
    fclose (fh);
    return 1;
}

int cfgfile_get_description (const char *filename, char *description, char *hostlink, char *hardwarelink, int *type)
{
    int result = 0;
    struct uae_prefs *p = xmalloc (struct uae_prefs, 1);

    p->description[0] = 0;
	p->config_host_path[0] = 0;
	p->config_hardware_path[0] = 0;
    if (cfgfile_load_2 (p, filename, 0, type)) {
		result = 1;
		if (description)
		    strcpy (description, p->description);
		if (hostlink)
			strcpy (hostlink, p->config_host_path);
		if (hardwarelink)
			strcpy (hardwarelink, p->config_hardware_path);
    }
	xfree (p);
    return result;
}

int cfgfile_configuration_change (int v)
{
	static int mode;
	if (v >= 0)
		mode = v;
	return mode;
}

void cfgfile_show_usage (void)
{
    unsigned int i;
    write_log ("UAE Configuration Help:\n" \
	       "=======================\n");
    for (i = 0; i < sizeof opttable / sizeof *opttable; i++)
		write_log ("%s: %s\n", opttable[i].config_label, opttable[i].config_help);
}

/* This implements the old commandline option parsing.  I've re-added this
   because the new way of doing things is painful for me (it requires me
   to type a couple hundred characters when invoking UAE).  The following
   is far less annoying to use.  */
static void parse_gfx_specs (struct uae_prefs *p, char *spec)
{
    char *x0 = my_strdup (spec);
    char *x1, *x2;

    x1 = strchr (x0, ':');
    if (x1 == 0)
		goto argh;
    x2 = strchr (x1+1, ':');
    if (x2 == 0)
		goto argh;
    *x1++ = 0; *x2++ = 0;

    p->gfx_width_win = p->gfx_width_fs = atoi (x0);
    p->gfx_height_win = p->gfx_height_fs = atoi (x1);
    p->gfx_size_win.width = p->gfx_size_fs.width = atoi (x0);
    p->gfx_size_win.height = p->gfx_size_fs.height = atoi (x1);
    p->gfx_resolution = strchr (x2, 'l') != 0 ? 1 : 0;
    p->gfx_xcenter = strchr (x2, 'x') != 0 ? 1 : strchr (x2, 'X') != 0 ? 2 : 0;
    p->gfx_ycenter = strchr (x2, 'y') != 0 ? 1 : strchr (x2, 'Y') != 0 ? 2 : 0;
    p->gfx_linedbl = strchr (x2, 'd') != 0;
    p->gfx_linedbl += 2 * (strchr (x2, 'D') != 0);
    p->gfx_afullscreen = strchr (x2, 'a') != 0;
    p->gfx_pfullscreen = strchr (x2, 'p') != 0;

    if (p->gfx_linedbl == 3) {
		write_log ("You can't use both 'd' and 'D' modifiers in the display mode specification.\n");
    }

    free (x0);
    return;

    argh:
    write_log ("Bad display mode specification.\n");
    write_log ("The format to use is: \"width:height:modifiers\"\n");
    write_log ("Type \"uae -h\" for detailed help.\n");
    free (x0);
}

static void parse_sound_spec (struct uae_prefs *p, char *spec)
{
    char *x0 = my_strdup (spec);
    char *x1, *x2 = NULL, *x3 = NULL, *x4 = NULL, *x5 = NULL;

	x1 = _tcschr (x0, ':');
	if (x1 != NULL) {
		*x1++ = '\0';
		x2 = _tcschr (x1 + 1, ':');
		if (x2 != NULL) {
			*x2++ = '\0';
			x3 = _tcschr (x2 + 1, ':');
			if (x3 != NULL) {
				*x3++ = '\0';
				x4 = _tcschr (x3 + 1, ':');
				if (x4 != NULL) {
					*x4++ = '\0';
					x5 = _tcschr (x4 + 1, ':');
				}
			}
		}
	}
	p->produce_sound = _tstoi (x0);
	if (x1) {
		p->sound_stereo_separation = 0;
		if (*x1 == 'S') {
			p->sound_stereo = SND_STEREO;
			p->sound_stereo_separation = 7;
		} else if (*x1 == 's')
			p->sound_stereo = SND_STEREO;
		else
			p->sound_stereo = SND_MONO;
	}
	if (x3)
		p->sound_freq = _tstoi (x3);
	if (x4)
		p->sound_maxbsiz = _tstoi (x4);
	free (x0);
    return;
}


static void parse_joy_spec (struct uae_prefs *p, char *spec)
{
    int v0 = 2, v1 = 0;
	if (_tcslen(spec) != 2)
		goto bad;

    switch (spec[0]) {
     case '0': v0 = JSEM_JOYS; break;
     case '1': v0 = JSEM_JOYS + 1; break;
     case 'M': case 'm': v0 = JSEM_MICE; break;
     case 'A': case 'a': v0 = JSEM_KBDLAYOUT; break;
     case 'B': case 'b': v0 = JSEM_KBDLAYOUT + 1; break;
     case 'C': case 'c': v0 = JSEM_KBDLAYOUT + 2; break;
     default: goto bad;
    }

    switch (spec[1]) {
     case '0': v1 = JSEM_JOYS; break;
     case '1': v1 = JSEM_JOYS + 1; break;
     case 'M': case 'm': v1 = JSEM_MICE; break;
     case 'A': case 'a': v1 = JSEM_KBDLAYOUT; break;
     case 'B': case 'b': v1 = JSEM_KBDLAYOUT + 1; break;
     case 'C': case 'c': v1 = JSEM_KBDLAYOUT + 2; break;
     default: goto bad;
    }
    if (v0 == v1)
	goto bad;
    /* Let's scare Pascal programmers */
    if (0)
bad:
    write_log ("Bad joystick mode specification. Use -J xy, where x and y\n"
	     "can be 0 for joystick 0, 1 for joystick 1, M for mouse, and\n"
	     "a, b or c for different keyboard settings.\n");

	p->jports[0].id = v0;
	p->jports[1].id = v1;
}

static void parse_filesys_spec (int readonly, const char *spec)
{
    char buf[256];
    char *s2;

	_tcsncpy (buf, spec, 255); buf[255] = 0;
	s2 = _tcschr (buf, ':');
	if (s2) {
		*s2++ = '\0';
#ifdef __DOS__
	{
	    char *tmp;

			while ((tmp = _tcschr (s2, '\\')))
		*tmp = '/';
	}
#endif
	s2 = 0;
#ifdef FILESYS
	{
	    const char *err;
/*
add_filesys_unit (struct uaedev_mount_info *mountinfo, const char *devname, const char *volname, const char *rootdir, int readonly, 
	int secspertrack, int surfaces, int reserved,
	int blocksize, int bootpri, int donotmount, int autoboot,
	const char *filesysdir, int hdc, int flags)
*/
	    err = add_filesys_unit (currprefs.mountinfo, 0, buf, s2, readonly, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0);

	    if (err)
		write_log ("%s\n", s2);
	}
#endif
    } else {
	write_log ("Usage: [-m | -M] VOLNAME:mount_point\n");
    }
}

static void parse_hardfile_spec (char *spec)
{
    char *x0 = my_strdup (spec);
    char *x1, *x2, *x3, *x4;

	x1 = _tcschr (x0, ':');
	if (x1 == NULL)
		goto argh;
	*x1++ = '\0';
	x2 = _tcschr (x1 + 1, ':');
	if (x2 == NULL)
		goto argh;
	*x2++ = '\0';
	x3 = _tcschr (x2 + 1, ':');
	if (x3 == NULL)
		goto argh;
	*x3++ = '\0';
	x4 = _tcschr (x3 + 1, ':');
	if (x4 == NULL)
		goto argh;
	*x4++ = '\0';
#ifdef FILESYS
    {
       const char *err_msg;
/*
add_filesys_unit (struct uaedev_mount_info *mountinfo, const char *devname, const char *volname, const char *rootdir, int readonly, 
	int secspertrack, int surfaces, int reserved,
	int blocksize, int bootpri, int donotmount, int autoboot,
	const char *filesysdir, int hdc, int flags)
*/
       err_msg = add_filesys_unit (currprefs.mountinfo, 0, 0, x4, 0, atoi (x0), atoi (x1), atoi (x2), atoi (x3), 0, 0, 0, 0, 0, 0);

       if (err_msg)
		   write_log ("%s\n", err_msg);
    }
#endif
    free (x0);
    return;

 argh:
    free (x0);
    write_log ("Bad hardfile parameter specified - type \"uae -h\" for help.\n");
    return;
}

static void parse_cpu_specs (struct uae_prefs *p, char *spec)
{
	if (*spec < '0' || *spec > '4') {
		write_log ("CPU parameter string must begin with '0', '1', '2', '3' or '4'.\n");
		return;
    }

	p->cpu_model = (*spec++) * 10 + 68000;
    p->address_space_24 = p->cpu_model < 68020;
    p->cpu_compatible = 0;
	while (*spec != '\0') {
		switch (*spec) {
		case 'a':
			if (p->cpu_model < 68020)
				write_log ("In 68000/68010 emulation, the address space is always 24 bit.\n");
			else if (p->cpu_model >= 68040)
				write_log ("In 68040/060 emulation, the address space is always 32 bit.\n");
			else
				p->address_space_24 = 1;
			break;
		case 'c':
			if (p->cpu_model != 68000)
				write_log ("The more compatible CPU emulation is only available for 68000\n"
				"emulation, not for 68010 upwards.\n");
			else
				p->cpu_compatible = 1;
			break;
		default:
			write_log ("Bad CPU parameter specified - type \"uae -h\" for help.\n");
			break;
		}
		spec++;
	}
}

/* Returns the number of args used up (0 or 1).  */
int parse_cmdline_option (struct uae_prefs *p, char c, char *arg)
{
    struct strlist *u = xcalloc (struct strlist, 1);
    const char arg_required[] = "0123rKpImWSAJwNCZUFcblOdHRv";

    if (strchr (arg_required, c) && ! arg) {
		write_log ("Missing argument for option `-%c'!\n", c);
		return 0;
    }

	u->option = xmalloc (char, 2);
    u->option[0] = c;
    u->option[1] = 0;
    u->value = arg ? my_strdup (arg) : NULL;
    u->next = p->all_lines;
    p->all_lines = u;

    switch (c) {
    case 'h': usage (); exit (0);

    case '0': strncpy (p->df[0], arg, 255); p->df[0][255] = 0; break;
    case '1': strncpy (p->df[1], arg, 255); p->df[1][255] = 0; break;
    case '2': strncpy (p->df[2], arg, 255); p->df[2][255] = 0; break;
    case '3': strncpy (p->df[3], arg, 255); p->df[3][255] = 0; break;
    case 'r': strncpy (p->romfile, arg, 255); p->romfile[255] = 0; break;
    case 'K': strncpy (p->keyfile, arg, 255); p->keyfile[255] = 0; break;
    case 'p': strncpy (p->prtname, arg, 255); p->prtname[255] = 0; break;
	/*     case 'I': strncpy (p->sername, arg, 255); p->sername[255] = 0; currprefs.use_serial = 1; break; */
    case 'm': case 'M': parse_filesys_spec (c == 'M', arg); break;
    case 'W': parse_hardfile_spec (arg); break;
    case 'S': parse_sound_spec (p, arg); break;
    case 'R': p->gfx_framerate = atoi (arg); break;
    case 'x': p->no_xhair = 1; break;
    case 'i': p->illegal_mem = 1; break;
    case 'J': parse_joy_spec (p, arg); break;

    case 't': p->test_drawing_speed = 1; break;
#if defined USE_X11_GFX
    case 'L': p->x11_use_low_bandwidth = 1; break;
    case 'T': p->x11_use_mitshm = 1; break;
#elif defined USE_AMIGA_GFX
    case 'T': p->amiga_use_grey = 1; break;
    case 'x': p->amiga_use_dither = 0; break;
#elif defined USE_CURSES_GFX
    case 'x': p->curses_reverse_video = 1; break;
#endif
    case 'w': p->m68k_speed = atoi (arg); break;

	/* case 'g': p->use_gfxlib = 1; break; */
    case 'G': p->start_gui = 0; break;
#ifdef DEBUGGER
    case 'D': p->start_debugger = 1; break;
#endif

    case 'n':
		if (strchr (arg, 'i') != 0)
		    p->immediate_blits = 1;
		break;

    case 'v':
		set_chipset_mask (p, atoi (arg));
		break;

    case 'C':
		parse_cpu_specs (p, arg);
		break;

    case 'Z':
		p->z3fastmem_size = atoi (arg) * 0x100000;
		break;

    case 'U':
		p->gfxmem_size = atoi (arg) * 0x100000;
		break;

    case 'F':
		p->fastmem_size = atoi (arg) * 0x100000;
		break;

    case 'b':
		p->bogomem_size = atoi (arg) * 0x40000;
		break;

    case 'c':
	p->chipmem_size = atoi (arg) * 0x80000;
	break;

    case 'l':
	if (0 == strcasecmp(arg, "de"))
	    p->keyboard_lang = KBD_LANG_DE;
	else if (0 == strcasecmp(arg, "dk"))
	    p->keyboard_lang = KBD_LANG_DK;
	else if (0 == strcasecmp(arg, "us"))
	    p->keyboard_lang = KBD_LANG_US;
	else if (0 == strcasecmp(arg, "se"))
	    p->keyboard_lang = KBD_LANG_SE;
	else if (0 == strcasecmp(arg, "fr"))
	    p->keyboard_lang = KBD_LANG_FR;
	else if (0 == strcasecmp(arg, "it"))
	    p->keyboard_lang = KBD_LANG_IT;
	else if (0 == strcasecmp(arg, "es"))
	    p->keyboard_lang = KBD_LANG_ES;
	break;

    case 'O': parse_gfx_specs (p, arg); break;
    case 'd':
	if (strchr (arg, 'S') != NULL || strchr (arg, 's')) {
	    write_log ("  Serial on demand.\n");
	    p->serial_demand = 1;
	}
	if (strchr (arg, 'P') != NULL || strchr (arg, 'p')) {
	    write_log ("  Parallel on demand.\n");
	    p->parallel_demand = 1;
	}

	break;

    case 'H':
#ifndef USE_AMIGA_GFX
	p->color_mode = atoi (arg);
	if (p->color_mode < 0) {
	    write_log ("Bad color mode selected. Using default.\n");
	    p->color_mode = 0;
	}
#else
	p->amiga_screen_type = atoi (arg);
	if (p->amiga_screen_type < 0 || p->amiga_screen_type > 2) {
	    write_log ("Bad screen-type selected. Defaulting to public screen.\n");
	    p->amiga_screen_type = 2;
	}
#endif
	break;
    default:
	write_log ("Unknown option `-%c'!\n", c);
	break;
    }
    return !! strchr (arg_required, c);
}

void cfgfile_addcfgparam (char *line)
{
    struct strlist *u;
    char line1b[CONFIG_BLEN], line2b[CONFIG_BLEN];

    if (!line) {
	struct strlist **ps = &temp_lines;
	while (*ps) {
	    struct strlist *s = *ps;
	    *ps = s->next;
			xfree (s->value);
			xfree (s->option);
			xfree (s);
	}
	temp_lines = 0;
	return;
    }
    if (!cfgfile_separate_line (line, line1b, line2b))
	return;
	u = xcalloc (struct strlist, 1);
    u->option = my_strdup(line1b);
    u->value = my_strdup(line2b);
    u->next = temp_lines;
    temp_lines = u;
}

unsigned int cmdlineparser (const char *s, char *outp[], unsigned int max)
{
    int j;
    unsigned int cnt = 0;
    int slash = 0;
    int quote = 0;
    char tmp1[MAX_DPATH];
    const char *prev;
    int doout;

    doout = 0;
    prev = s;
    j = 0;
    while (cnt < max) {
	char c = *s++;
	if (!c)
	    break;
	if (c < 32)
	    continue;
	if (c == '\\')
	    slash = 1;
	if (!slash && c == '"') {
	    if (quote) {
		quote = 0;
		doout = 1;
	    } else {
		quote = 1;
		j = -1;
	    }
	}
	if (!quote && c == ' ')
	    doout = 1;
	if (!doout) {
	    if (j >= 0) {
		tmp1[j] = c;
		tmp1[j + 1] = 0;
	    }
	    j++;
	}
	if (doout) {
	    outp[cnt++] = my_strdup (tmp1);
	    tmp1[0] = 0;
	    doout = 0;
	    j = 0;
	}
	slash = 0;
    }
    if (j > 0 && cnt < max)
	outp[cnt++] = my_strdup (tmp1);

    return cnt;
}

#define UAELIB_MAX_PARSE 100

uae_u32 cfgfile_modify (uae_u32 index, char *parms, uae_u32 size, char *out, uae_u32 outsize)
{
    char *p;
    char *argc[UAELIB_MAX_PARSE];
    unsigned int argv, i;
    uae_u32 err;
    uae_u8 zero = 0;
    static FILE *configstore = 0;
    static char *configsearch;
    static int configsearchfound;

	config_changed = 1;
    err = 0;
    argv = 0;
    p = 0;
    if (index != 0xffffffff) {
	if (!configstore) {
	    err = 20;
	    goto end;
	}
	if (configsearch) {
	    char tmp[CONFIG_BLEN];
	    unsigned int j = 0;
	    char *in = configsearch;
	    unsigned int inlen = strlen (configsearch);
	    int joker = 0;

	    if (in[inlen - 1] == '*') {
		joker = 1;
		inlen--;
	    }

	    for (;;) {
		uae_u8 b = 0;

		if (fread (&b, 1, 1, configstore) != 1) {
		    err = 10;
		    if (configsearch)
			err = 5;
		    if (configsearchfound)
			err = 0;
		    goto end;
		}
				if (j >= sizeof (tmp) / sizeof (char) - 1)
					j = sizeof (tmp) / sizeof (char) - 1;
		if (b == 0) {
		    err = 10;
		    if (configsearch)
			err = 5;
		    if (configsearchfound)
			err = 0;
		    goto end;
		}
		if (b == '\n') {
		    if (configsearch && !strncmp (tmp, in, inlen) &&
			((inlen > 0 && strlen (tmp) > inlen && tmp[inlen] == '=') || (joker))) {
			char *p;
			if (joker)
			    p = tmp - 1;
			else
			    p = strchr (tmp, '=');
			if (p) {
			    for (i = 0; i < outsize - 1; i++) {
				uae_u8 b = *++p;
				out[i] = b;
				out[i + 1] = 0;
				if (!b)
				    break;
			    }
			}
			err = 0xffffffff;
			configsearchfound++;
			goto end;
		    }
		    index--;
		    j = 0;
		} else {
		    tmp[j++] = b;
		    tmp[j] = 0;
		}
	    }
	}
	err = 0xffffffff;
	for (i = 0; i < outsize - 1; i++) {
	    uae_u8 b = 0;
	    if (fread (&b, 1, 1, configstore) != 1)
		err = 0;
	    if (b == 0)
		err = 0;
	    if (b == '\n')
		b = 0;
	    out[i] = b;
	    out[i + 1] = 0;
	    if (!b)
		break;
	}
	goto end;
    }

    if (size > 10000)
	return 10;
    argv = cmdlineparser (parms, argc, UAELIB_MAX_PARSE);

    if (argv <= 1 && index == 0xffffffff) {
	if (configstore) {
	    fclose (configstore);
	    configstore = 0;
	}
	free (configsearch);

	configstore = fopen ("configstore", "w+");
	configsearch = NULL;
	if (argv > 0 && strlen (argc[0]) > 0)
	    configsearch = my_strdup (argc[0]);
	if (!configstore) {
	    err = 20;
	    goto end;
	}
	fseek (configstore, 0, SEEK_SET);
	cfgfile_save_options (configstore, &currprefs, 0);
	fwrite (&zero, 1, 1, configstore);
	fseek (configstore, 0, SEEK_SET);
	err = 0xffffffff;
	configsearchfound = 0;
	goto end;
    }

    for (i = 0; i < argv; i++) {
	if (i + 2 <= argv) {
	    if (!inputdevice_uaelib (argc[i], argc[i + 1])) {
		if (!cfgfile_parse_option (&changed_prefs, argc[i], argc[i + 1], 0)) {
		    err = 5;
		    break;
		}
	    }
			set_special (SPCFLAG_BRK);
	    i++;
	}
    }
end:
    for (i = 0; i < argv; i++)
	free (argc[i]);
    free (p);
    return err;
}

uae_u32 cfgfile_uaelib_modify (uae_u32 index, uae_u32 parms, uae_u32 size, uae_u32 out, uae_u32 outsize)
{
    char *p, *parms_p = NULL, *out_p = NULL;
    unsigned int i;
    int ret;

	if (out)
	    put_byte (out, 0);
	if (size == 0) {
		while (get_byte (parms + size) != 0)
			size++;
	}
    parms_p = xmalloc (uae_char, size + 1);
    if (!parms_p) {
		ret = 10;
		goto end;
    }
	if (out) {
		out_p = xmalloc (char, outsize + 1);
	    if (!out_p) {
			ret = 10;
			goto end;
		}
		out_p[0] = 0;
    }
    p = parms_p;
    for (i = 0; i < size; i++) {
	p[i] = get_byte (parms + i);
	if (p[i] == 10 || p[i] == 13 || p[i] == 0)
	    break;
    }
    p[i] = 0;
    out_p[0] = 0;
    ret = cfgfile_modify (index, parms_p, size, out_p, outsize);
	if (out) {
	    p = out_p;
	    for (i = 0; i < outsize - 1; i++) {
			uae_u8 b = *p++;
			put_byte (out + i, b);
			put_byte (out + i + 1, 0);
			if (!b)
			    break;
	    }
	}
end:
	xfree (out_p);
	xfree (parms_p);
    return ret;
}

uae_u32 cfgfile_uaelib (int mode, uae_u32 name, uae_u32 dst, uae_u32 maxlen)
{
    char tmp[CONFIG_BLEN];
    unsigned int i;
    struct strlist *sl;

    if (mode)
	return 0;

    for (i = 0; i < sizeof(tmp); i++) {
	tmp[i] = get_byte (name + i);
	if (tmp[i] == 0)
	    break;
    }
    tmp[sizeof(tmp) - 1] = 0;
    if (tmp[0] == 0)
	return 0;
    for (sl = currprefs.all_lines; sl; sl = sl->next) {
	if (!strcasecmp (sl->option, tmp))
	    break;
    }

    if (sl) {
	for (i = 0; i < maxlen; i++) {
	    put_byte (dst + i, sl->value[i]);
	    if (sl->value[i] == 0)
		break;
	}
	return dst;
    }
    return 0;
}

uae_u8 *restore_configuration (uae_u8 *src)
{
	char *s = ((char*)src);
	//write_log (s);
	xfree (s);
	src += strlen ((char*)src) + 1;
	return src;
}

uae_u8 *save_configuration (int *len)
{
	int tmpsize = 30000;
	uae_u8 *dstbak, *dst, *p;
	int index = -1;

	dstbak = dst = xmalloc (uae_u8, tmpsize);
	p = dst;
	for (;;) {
		char tmpout[256];
		int ret;
		tmpout[0] = 0;
		ret = cfgfile_modify (index, "*", 1, tmpout, sizeof (tmpout) / sizeof (char));
		index++;
		if (_tcslen (tmpout) > 0) {
			char *out;
			if (!_tcsncmp (tmpout, "input.", 6))
				continue;
			out = (tmpout);
			strcpy ((char*)p, out);
			xfree (out);
			strcat ((char*)p, "\n");
			p += strlen ((char*)p);
			if (p - dstbak >= tmpsize - sizeof (tmpout))
				break;
		}
		if (ret >= 0)
			break;
	}
	*len = p - dstbak + 1;
	return dstbak;
}

static void default_prefs_mini (struct uae_prefs *p, int type)
{
    strcpy (p->description, "UAE default A500 configuration");

    p->nr_floppies = 1;
    p->dfxtype[0] = DRV_35_DD;
    p->dfxtype[1] = DRV_NONE;
    p->cpu_model = 68000;
    p->address_space_24 = 1;
    p->chipmem_size = 0x00080000;
    p->bogomem_size = 0x00080000;
}

void default_prefs (struct uae_prefs *p, int type)
{
    int i;
    int roms[] = { 6, 7, 8, 9, 10, 14, 5, 4, 3, 2, 1, -1 };
    uae_u8 zero = 0;
    struct zfile *f;

    memset (p, 0, sizeof (*p));
    strcpy (p->description, "UAE default configuration");
	p->config_hardware_path[0] = 0;
	p->config_host_path[0] = 0;

    p->gfx_scandoubler = 0;
    p->start_gui = 1;
#ifdef DEBUGGER
    p->start_debugger = 0;
#endif

    p->all_lines = 0;
    /* Note to porters: please don't change any of these options! UAE is supposed
     * to behave identically on all platforms if possible.
     * (TW says: maybe it is time to update default config..) */
    p->illegal_mem = 0;
    p->no_xhair = 0;
    p->use_serial = 0;
    p->serial_demand = 0;
    p->serial_hwctsrts = 1;
    p->parallel_demand = 0;
    p->parallel_matrix_emulation = 0;
    p->parallel_postscript_emulation = 0;
    p->parallel_postscript_detection = 0;
    p->parallel_autoflush_time = 5;
    p->ghostscript_parameters[0] = 0;
    p->uae_hide = 0;

	memset (&p->jports[0], 0, sizeof (struct jport));
	memset (&p->jports[1], 0, sizeof (struct jport));
	memset (&p->jports[2], 0, sizeof (struct jport));
	memset (&p->jports[3], 0, sizeof (struct jport));
	p->jports[0].id = JSEM_MICE;
	p->jports[1].id = JSEM_KBDLAYOUT;
	p->jports[2].id = -1;
	p->jports[3].id = -1;
    p->keyboard_lang = KBD_LANG_US;

    p->produce_sound = 3;
    p->sound_stereo = SND_STEREO;
    p->sound_stereo_separation = 7;
    p->sound_mixed_stereo_delay = 0;
    p->sound_freq = DEFAULT_SOUND_FREQ;
    p->sound_maxbsiz = DEFAULT_SOUND_MAXB;
    p->sound_latency = 100;
    p->sound_interpol = 1;
    p->sound_filter = FILTER_SOUND_EMUL;
    p->sound_filter_type = 0;
    p->sound_auto = 1;

#ifdef JIT
# ifdef NATMEM_OFFSET
    p->comptrustbyte = 0;
    p->comptrustword = 0;
    p->comptrustlong = 0;
    p->comptrustnaddr= 0;
# else
    p->comptrustbyte = 1;
    p->comptrustword = 1;
    p->comptrustlong = 1;
    p->comptrustnaddr= 1;
# endif
    p->compnf = 1;
    p->comp_hardflush = 0;
    p->comp_constjump = 1;
    p->comp_oldsegv = 0;
    p->compfpu = 1;
    p->fpu_strict = 0;
    p->cachesize = 0;
    p->avoid_cmov = 0;
    p->avoid_dga = 0;
    p->avoid_vid = 0;
    p->comp_midopt = 0;
    p->comp_lowopt = 0;
    p->override_dga_address = 0;

    for (i = 0;i < 10; i++)
		p->optcount[i] = -1;
    p->optcount[0] = 4;	/* How often a block has to be executed before it is translated */
    p->optcount[1] = 0;	/* How often to use the naive translation */
    p->optcount[2] = 0;
    p->optcount[3] = 0;
    p->optcount[4] = 0;
    p->optcount[5] = 0;
#endif
    p->gfx_framerate = 1;
    p->gfx_autoframerate = 50;
    p->gfx_size_fs.width = 800;
    p->gfx_size_fs.height = 600;
    p->gfx_size_win.width = 720;
    p->gfx_size_win.height = 568;
    p->gfx_width_fs = 800;
    p->gfx_height_fs = 600;
    p->gfx_width_win = 720;
    p->gfx_height_win = 568;
    for (i = 0; i < 4; i++) {
		p->gfx_size_fs_xtra[i].width = 0;
		p->gfx_size_fs_xtra[i].height = 0;
		p->gfx_size_win_xtra[i].width = 0;
		p->gfx_size_win_xtra[i].height = 0;
    }
    p->gfx_resolution = 1;
    p->gfx_linedbl = 1;
    p->gfx_afullscreen = 0;
    p->gfx_pfullscreen = 0;
    p->gfx_xcenter = 0; p->gfx_ycenter = 0;
    p->gfx_xcenter_pos = -1; p->gfx_ycenter_pos = -1;
    p->gfx_xcenter_size = -1; p->gfx_ycenter_size = -1;
    p->gfx_max_horizontal = RES_HIRES;
    p->gfx_max_vertical = 1;
    p->color_mode = 2;
    p->gfx_blackerthanblack = 0;
    p->gfx_backbuffers = 2;

#ifdef USE_X11_GFX
	p->x11_use_low_bandwidth = 0;
	p->x11_use_mitshm = 0;
	p->x11_hide_cursor = 1;
#endif
#ifdef SVGA
	p->svga_no_linear = 0;
#endif
#ifdef NCURSES
	p->curses_reverse_video = 0;
#endif
    machdep_default_options (p);
	target_default_options (p);
    gfx_default_options (p);
    audio_default_options (p);

    p->immediate_blits = 0;
    p->collision_level = 2;
    p->leds_on_screen = 0;
    p->keyboard_leds_in_use = 0;
    p->keyboard_leds[0] = p->keyboard_leds[1] = p->keyboard_leds[2] = 0;
    p->scsi = 0;
    p->uaeserial = 0;
    p->cpu_idle = 0;
    p->turbo_emulation = 0;
    p->headless = 0;
    p->catweasel = 0;
    p->tod_hack = 0;
    p->maprom = 0;
    p->filesys_no_uaefsdb = 0;
    p->filesys_custom_uaefsdb = 1;
    p->picasso96_nocustom = 1;
    p->cart_internal = 1;
    p->sana2 = 0;

    p->cs_compatible = 1;
    p->cs_rtc = 2;
    p->cs_df0idhw = 1;
    p->cs_a1000ram = 0;
    p->cs_fatgaryrev = -1;
    p->cs_ramseyrev = -1;
    p->cs_agnusrev = -1;
    p->cs_deniserev = -1;
    p->cs_mbdmac = 0;
    p->cs_a2091 = 0;
    p->cs_a4091 = 0;
    p->cs_cd32c2p = p->cs_cd32cd = p->cs_cd32nvram = 0;
    p->cs_cdtvcd = p->cs_cdtvram = p->cs_cdtvcard = 0;
    p->cs_pcmcia = 0;
    p->cs_ksmirror_e0 = 1;
    p->cs_ksmirror_a8 = 0;
    p->cs_ciaoverlay = 1;
    p->cs_ciaatod = 0;
    p->cs_df0idhw = 1;
    p->cs_slowmemisfast = 0;
    p->cs_resetwarning = 1;

#ifdef GFXFILTER
    p->gfx_filter = 0;
    p->gfx_filtershader[0] = 0;
	p->gfx_filtermask[0] = 0;
    p->gfx_filter_horiz_zoom_mult = 0;
    p->gfx_filter_vert_zoom_mult = 0;
	p->gfx_filter_bilinear = 0;
    p->gfx_filter_filtermode = 0;
    p->gfx_filter_scanlineratio = (1 << 4) | 1;
    p->gfx_filter_keep_aspect = 0;
    p->gfx_filter_autoscale = 0;
#endif

    p->df[0][0] = '\0';
    p->df[1][0] = '\0';
    p->df[2][0] = '\0';
    p->df[3][0] = '\0';

    strcpy (p->romfile, "kick.rom");
    strcpy (p->keyfile, "");
    strcpy (p->romextfile, "");
    strcpy (p->flashfile, "");
#ifdef ACTION_REPLAY
    strcpy (p->cartfile, "");
#endif

    prefs_set_attr ("rom_path",       strdup_path_expand (TARGET_ROM_PATH));
    prefs_set_attr ("floppy_path",    strdup_path_expand (TARGET_FLOPPY_PATH));
    prefs_set_attr ("hardfile_path",  strdup_path_expand (TARGET_HARDFILE_PATH));
#ifdef SAVESTATE
    prefs_set_attr ("savestate_path", strdup_path_expand (TARGET_SAVESTATE_PATH));
#endif

	p->prtname[0] = 0;
	p->sername[0] = 0;

    p->fpu_model = 0;
    p->cpu_model = 68000;
	p->cpu_clock_multiplier = 0;
	p->cpu_frequency = 0;
    p->mmu_model = 0;
    p->cpu060_revision = 6;
    p->fpu_revision = -1;
    p->m68k_speed = 0;
    p->cpu_compatible = 1;
    p->address_space_24 = 1;
    p->cpu_cycle_exact = 0;
    p->blitter_cycle_exact = 0;
    p->chipset_mask = CSMASK_ECS_AGNUS;
    p->genlock = 0;
    p->ntscmode = 0;

    p->fastmem_size = 0x00000000;
	p->mbresmem_low_size = 0x00000000;
	p->mbresmem_high_size = 0x00000000;
    p->z3fastmem_size = 0x00000000;
    p->z3fastmem2_size = 0x00000000;
    p->z3fastmem_start = 0x10000000;
    p->chipmem_size = 0x00080000;
    p->bogomem_size = 0x00080000;
    p->gfxmem_size = 0x00000000;
    p->custom_memory_addrs[0] = 0;
    p->custom_memory_sizes[0] = 0;
    p->custom_memory_addrs[1] = 0;
    p->custom_memory_sizes[1] = 0;

    p->nr_floppies = 2;
    p->dfxtype[0] = DRV_35_DD;
    p->dfxtype[1] = DRV_35_DD;
    p->dfxtype[2] = DRV_NONE;
    p->dfxtype[3] = DRV_NONE;
    p->floppy_speed = 100;
    p->floppy_write_length = 0;
#ifdef DRIVESOUND
    p->dfxclickvolume = 33;
#endif

#ifdef SAVESTATE
    p->statecapturebuffersize = 20 * 1024 * 1024;
    p->statecapturerate = 5 * 50;
    p->statecapture = 0;
#endif

#ifdef UAE_MINI
    default_prefs_mini (p, 0);
#endif

	p->input_tablet = TABLET_OFF;
	p->input_magic_mouse = 0;
	p->input_magic_mouse_cursor = 0;

    inputdevice_default_prefs (p);
}

static void buildin_default_prefs_68020 (struct uae_prefs *p)
{
	p->cpu_model = 68020;
	p->address_space_24 = 1;
	p->cpu_compatible = 1;
	p->chipset_mask = CSMASK_ECS_AGNUS | CSMASK_ECS_DENISE | CSMASK_AGA;
	p->chipmem_size = 0x200000;
	p->bogomem_size = 0;
	p->m68k_speed = -1;
}

static void buildin_default_host_prefs (struct uae_prefs *p)
{
#if 0
	p->sound_filter = FILTER_SOUND_OFF;
	p->sound_stereo = SND_STEREO;
	p->sound_stereo_separation = 7;
	p->sound_mixed_stereo = 0;
#endif
}

static void buildin_default_prefs (struct uae_prefs *p)
{
	buildin_default_host_prefs (p);

	p->dfxtype[0] = DRV_35_DD;
	if (p->nr_floppies != 1 && p->nr_floppies != 2)
		p->nr_floppies = 2;
	p->dfxtype[1] = p->nr_floppies >= 2 ? DRV_35_DD : DRV_NONE;
	p->dfxtype[2] = DRV_NONE;
	p->dfxtype[3] = DRV_NONE;
	p->floppy_speed = 100;

	p->fpu_model = 0;
	p->cpu_model = 68000;
	p->cpu_clock_multiplier = 0;
	p->cpu_frequency = 0;
	p->cpu060_revision = 1;
	p->fpu_revision = -1;
	p->m68k_speed = 0;
	p->cpu_compatible = 1;
	p->address_space_24 = 1;
	p->cpu_cycle_exact = 0;
	p->blitter_cycle_exact = 0;
	p->chipset_mask = CSMASK_ECS_AGNUS;
	p->immediate_blits = 0;
	p->collision_level = 2;
	p->produce_sound = 3;
	p->scsi = 0;
	p->uaeserial = 0;
	p->cpu_idle = 0;
	p->turbo_emulation = 0;
	p->catweasel = 0;
	p->tod_hack = 0;
	p->maprom = 0;
#ifdef JIT
	p->cachesize = 0;
#endif
	p->socket_emu = 0;

	p->chipmem_size = 0x00080000;
	p->bogomem_size = 0x00080000;
	p->fastmem_size = 0x00000000;
	p->mbresmem_low_size = 0x00000000;
	p->mbresmem_high_size = 0x00000000;
	p->z3fastmem_size = 0x00000000;
	p->z3fastmem2_size = 0x00000000;
	p->gfxmem_size = 0x00000000;

	p->cs_rtc = 0;
	p->cs_a1000ram = 0;
	p->cs_fatgaryrev = -1;
	p->cs_ramseyrev = -1;
	p->cs_agnusrev = -1;
	p->cs_deniserev = -1;
	p->cs_mbdmac = 0;
	p->cs_a2091 = 0;
	p->cs_a4091 = 0;
	p->cs_cd32c2p = p->cs_cd32cd = p->cs_cd32nvram = 0;
	p->cs_cdtvcd = p->cs_cdtvram = p->cs_cdtvcard = 0;
	p->cs_ide = 0;
	p->cs_pcmcia = 0;
	p->cs_ksmirror_e0 = 1;
	p->cs_ksmirror_a8 = 0;
	p->cs_ciaoverlay = 1;
	p->cs_ciaatod = 0;
	p->cs_df0idhw = 1;
	p->cs_resetwarning = 0;

	_tcscpy (p->romfile, "");
	_tcscpy (p->romextfile, "");
	_tcscpy (p->flashfile, "");
	_tcscpy (p->cartfile, "");
	_tcscpy (p->amaxromfile, "");
	p->prtname[0] = 0;
	p->sername[0] = 0;

	p->mountitems = 0;

	target_default_options (p);
}

static void set_68020_compa (struct uae_prefs *p, int compa, int cd32)
{
	if (compa == 0) {
		p->blitter_cycle_exact = 1;
		p->m68k_speed = 0;
#ifdef JIT
		if (p->cpu_model == 68020 && p->cachesize == 0) {
			p->cpu_cycle_exact = 1;
			p->cpu_clock_multiplier = 4 << 8;
		}
#endif
	}
	if (compa > 1) {
		p->cpu_compatible = 0;
		p->address_space_24 = 0;
#ifdef JIT
		p->cachesize = 8192;
#endif
	}
}

/* 0: cycle-exact
* 1: more compatible
* 2: no more compatible, no 100% sound
* 3: no more compatible, immediate blits, no 100% sound
*/

static void set_68000_compa (struct uae_prefs *p, int compa)
{
	p->cpu_clock_multiplier = 2 << 8;
	switch (compa)
	{
	case 0:
		p->cpu_cycle_exact = p->blitter_cycle_exact = 1;
		break;
	case 1:
		break;
	case 2:
		p->cpu_compatible = 0;
		break;
	case 3:
		p->immediate_blits = 1;
		p->produce_sound = 2;
		p->cpu_compatible = 0;
		break;
	}
}

static int bip_a3000 (struct uae_prefs *p, int config, int compa, int romcheck)
{
	int roms[2];

	if (config == 2)
		roms[0] = 61;
	else if (config == 1)
		roms[0] = 71;
	else
		roms[0] = 59;
	roms[1] = -1;
	p->immediate_blits = 1;
	p->bogomem_size = 0;
	p->chipmem_size = 0x200000;
	p->cpu_model = 68030;
	p->fpu_model = 68882;
	p->chipset_mask = CSMASK_ECS_AGNUS | CSMASK_ECS_DENISE;
	p->cpu_compatible = p->address_space_24 = 0;
	p->m68k_speed = -1;
	p->immediate_blits = 0;
	p->produce_sound = 2;
#ifdef JIT
	p->cachesize = 8192;
#endif
	p->dfxtype[0] = DRV_35_HD;
	p->floppy_speed = 0;
	p->cpu_idle = 150;
	p->cs_compatible = CP_A3000;
	p->mbresmem_low_size = 8 * 1024 * 1024;
	built_in_chipset_prefs (p);
	p->cs_ciaatod = p->ntscmode ? 2 : 1;
	return configure_rom (p, roms, romcheck);
}
static int bip_a4000 (struct uae_prefs *p, int config, int compa, int romcheck)
{
	int roms[8];

	roms[0] = 16;
	roms[1] = 31;
	roms[2] = 13;
	roms[3] = 12;
	roms[4] = -1;
	p->immediate_blits = 1;
	p->bogomem_size = 0;
	p->chipmem_size = 0x200000;
	p->mbresmem_low_size = 8 * 1024 * 1024;
	p->cpu_model = 68030;
	p->fpu_model = 68882;
	if (config > 0)
		p->cpu_model = p->fpu_model = 68040;
	p->chipset_mask = CSMASK_AGA | CSMASK_ECS_AGNUS | CSMASK_ECS_DENISE;
	p->cpu_compatible = p->address_space_24 = 0;
	p->m68k_speed = -1;
	p->immediate_blits = 0;
	p->produce_sound = 2;
#ifdef JIT
	p->cachesize = 8192;
#endif
	p->dfxtype[0] = DRV_35_HD;
	p->dfxtype[1] = DRV_35_HD;
	p->floppy_speed = 0;
	p->cpu_idle = 150;
	p->cs_compatible = CP_A4000;
	built_in_chipset_prefs (p);
	p->cs_ciaatod = p->ntscmode ? 2 : 1;
	return configure_rom (p, roms, romcheck);
}
static int bip_a4000t (struct uae_prefs *p, int config, int compa, int romcheck)
{

	int roms[8];

	roms[0] = 16;
	roms[1] = 31;
	roms[2] = 13;
	roms[3] = -1;
	p->immediate_blits = 1;
	p->bogomem_size = 0;
	p->chipmem_size = 0x200000;
	p->mbresmem_low_size = 8 * 1024 * 1024;
	p->cpu_model = 68030;
	p->fpu_model = 68882;
	if (config > 0)
		p->cpu_model = p->fpu_model = 68040;
	p->chipset_mask = CSMASK_AGA | CSMASK_ECS_AGNUS | CSMASK_ECS_DENISE;
	p->cpu_compatible = p->address_space_24 = 0;
	p->m68k_speed = -1;
	p->immediate_blits = 0;
	p->produce_sound = 2;
#ifdef JIT
	p->cachesize = 8192;
#endif
	p->dfxtype[0] = DRV_35_HD;
	p->dfxtype[1] = DRV_35_HD;
	p->floppy_speed = 0;
	p->cpu_idle = 150;
	p->cs_compatible = CP_A4000T;
	built_in_chipset_prefs (p);
	p->cs_ciaatod = p->ntscmode ? 2 : 1;
	return configure_rom (p, roms, romcheck);
}

static int bip_a1000 (struct uae_prefs *p, int config, int compa, int romcheck)
{
	int roms[4];

	roms[0] = 24;
	roms[1] = 23;
	roms[2] = -1;
	p->chipset_mask = 0;
	p->bogomem_size = 0;
	p->sound_filter = FILTER_SOUND_ON;
	set_68000_compa (p, compa);
	p->dfxtype[1] = DRV_NONE;
	p->cs_compatible = CP_A1000;
	p->cs_slowmemisfast = 1;
	p->cs_dipagnus = 1;
	p->cs_agnusbltbusybug = 1;
	built_in_chipset_prefs (p);
	if (config > 0)
		p->cs_denisenoehb = 1;
	if (config > 1)
		p->chipmem_size = 0x40000;
	return configure_rom (p, roms, romcheck);
}

static int bip_cdtv (struct uae_prefs *p, int config, int compa, int romcheck)
{
	int roms[4];

	roms[0] = 6;
	roms[1] = 32;
	roms[2] = -1;
	if (!configure_rom (p, roms, romcheck))
		return 0;
	roms[0] = 20;
	roms[1] = 21;
	roms[2] = 22;
	roms[3] = -1;
	if (!configure_rom (p, roms, romcheck))
		return 0;
	p->bogomem_size = 0;
	p->chipmem_size = 0x100000;
	p->chipset_mask = CSMASK_ECS_AGNUS;
	p->cs_cdtvcd = p->cs_cdtvram = 1;
	if (config > 0)
		p->cs_cdtvcard = 64;
	p->cs_rtc = 1;
	p->nr_floppies = 0;
	p->dfxtype[0] = DRV_NONE;
	if (config > 0)
		p->dfxtype[0] = DRV_35_DD;
	p->dfxtype[1] = DRV_NONE;
	set_68000_compa (p, compa);
	p->cs_compatible = CP_CDTV;
	built_in_chipset_prefs (p);
	//fetch_datapath (p->flashfile, sizeof (p->flashfile) / sizeof (char));
	_tcscat (p->flashfile, "cdtv.nvr");
	return 1;
}

static int bip_cd32 (struct uae_prefs *p, int config, int compa, int romcheck)
{
	int roms[2];

	buildin_default_prefs_68020 (p);
	roms[0] = 64;
	roms[1] = -1;
	if (!configure_rom (p, roms, 0)) {
		roms[0] = 18;
		roms[1] = -1;
		if (!configure_rom (p, roms, romcheck))
			return 0;
		roms[0] = 19;
		if (!configure_rom (p, roms, romcheck))
			return 0;
	}
	if (config > 0) {
		roms[0] = 23;
		if (!configure_rom (p, roms, romcheck))
			return 0;
	}
	p->cs_cd32c2p = p->cs_cd32cd = p->cs_cd32nvram = 1;
	p->nr_floppies = 0;
	p->dfxtype[0] = DRV_NONE;
	p->dfxtype[1] = DRV_NONE;
	set_68020_compa (p, compa, 1);
	p->cs_compatible = CP_CD32;
	built_in_chipset_prefs (p);
	//fetch_datapath (p->flashfile, sizeof (p->flashfile) / sizeof (char));
	_tcscat (p->flashfile, "cd32.nvr");
	return 1;
}

static int bip_a1200 (struct uae_prefs *p, int config, int compa, int romcheck)
{
	int roms[4];

	buildin_default_prefs_68020 (p);
	roms[0] = 11;
	roms[1] = 15;
	roms[2] = 31;
	roms[3] = -1;
	p->cs_rtc = 0;
	if (config == 1) {
		p->fastmem_size = 0x400000;
		p->cs_rtc = 2;
	}
	set_68020_compa (p, compa, 0);
	p->cs_compatible = CP_A1200;
	built_in_chipset_prefs (p);
	return configure_rom (p, roms, romcheck);
}

static int bip_a600 (struct uae_prefs *p, int config, int compa, int romcheck)
{
	int roms[4];

	roms[0] = 10;
	roms[1] = 9;
	roms[2] = 8;
	roms[3] = -1;
	p->bogomem_size = 0;
	p->chipmem_size = 0x100000;
	if (config > 0)
		p->cs_rtc = 1;
	if (config == 1)
		p->chipmem_size = 0x200000;
	if (config == 2)
		p->fastmem_size = 0x400000;
	p->chipset_mask = CSMASK_ECS_AGNUS | CSMASK_ECS_DENISE;
	set_68000_compa (p, compa);
	p->cs_compatible = CP_A600;
	built_in_chipset_prefs (p);
	return configure_rom (p, roms, romcheck);
}

static int bip_a500p (struct uae_prefs *p, int config, int compa, int romcheck)
{
	int roms[2];

	roms[0] = 7;
	roms[1] = -1;
	p->bogomem_size = 0;
	p->chipmem_size = 0x100000;
	if (config > 0)
		p->cs_rtc = 1;
	if (config == 1)
		p->chipmem_size = 0x200000;
	if (config == 2)
		p->fastmem_size = 0x400000;
	p->chipset_mask = CSMASK_ECS_AGNUS | CSMASK_ECS_DENISE;
	set_68000_compa (p, compa);
	p->cs_compatible = CP_A500P;
	built_in_chipset_prefs (p);
	return configure_rom (p, roms, romcheck);
}
static int bip_a500 (struct uae_prefs *p, int config, int compa, int romcheck)
{
	int roms[4];

	roms[0] = roms[1] = roms[2] = roms[3] = -1;
	switch (config)
	{
	case 0: // KS 1.3, OCS Agnus, 0.5M Chip + 0.5M Slow
		roms[0] = 6;
		roms[1] = 32;
		p->chipset_mask = 0;
		break;
	case 1: // KS 1.3, ECS Agnus, 0.5M Chip + 0.5M Slow
		roms[0] = 6;
		roms[1] = 32;
		break;
	case 2: // KS 1.3, ECS Agnus, 1.0M Chip
		roms[0] = 6;
		roms[1] = 32;
		p->bogomem_size = 0;
		p->chipmem_size = 0x100000;
		break;
	case 3: // KS 1.3, OCS Agnus, 0.5M Chip
		roms[0] = 6;
		roms[1] = 32;
		p->bogomem_size = 0;
		p->chipset_mask = 0;
		p->cs_rtc = 0;
		p->dfxtype[1] = DRV_NONE;
		break;
	case 4: // KS 1.2, OCS Agnus, 0.5M Chip
		roms[0] = 5;
		roms[1] = 4;
		roms[2] = 3;
		p->bogomem_size = 0;
		p->chipset_mask = 0;
		p->cs_rtc = 0;
		p->dfxtype[1] = DRV_NONE;
		break;
	case 5: // KS 1.2, OCS Agnus, 0.5M Chip + 0.5M Slow
		roms[0] = 5;
		roms[1] = 4;
		roms[2] = 3;
		p->chipset_mask = 0;
		break;
	}
	set_68000_compa (p, compa);
	p->cs_compatible = CP_A500;
	built_in_chipset_prefs (p);
	return configure_rom (p, roms, romcheck);
}

static int bip_super (struct uae_prefs *p, int config, int compa, int romcheck)
{
	int roms[8];

	roms[0] = 46;
	roms[1] = 16;
	roms[2] = 31;
	roms[3] = 15;
	roms[4] = 14;
	roms[5] = 12;
	roms[6] = 11;
	roms[7] = -1;
	p->bogomem_size = 0;
	p->chipmem_size = 0x400000;
	p->z3fastmem_size = 8 * 1024 * 1024;
	p->gfxmem_size = 8 * 1024 * 1024;
	p->cpu_model = 68040;
	p->fpu_model = 68040;
	p->chipset_mask = CSMASK_AGA | CSMASK_ECS_AGNUS | CSMASK_ECS_DENISE;
	p->cpu_compatible = p->address_space_24 = 0;
	p->m68k_speed = -1;
	p->immediate_blits = 1;
	p->produce_sound = 2;
#ifdef JIT
	p->cachesize = 8192;

#endif	p->dfxtype[0] = DRV_35_HD;
	p->dfxtype[1] = DRV_35_HD;
	p->floppy_speed = 0;
	p->cpu_idle = 150;
	p->scsi = 1;
	p->uaeserial = 1;
	p->socket_emu = 1;
	p->cart_internal = 0;
	p->picasso96_nocustom = 1;
	p->cs_compatible = 1;
	built_in_chipset_prefs (p);
	p->cs_ide = -1;
	p->cs_ciaatod = p->ntscmode ? 2 : 1;
	//_tcscat(p->flashfile, "battclock.nvr");
	return configure_rom (p, roms, romcheck);
}

static int bip_arcadia (struct uae_prefs *p, int config, int compa, int romcheck)
{
	int roms[4], i;
	struct romlist **rl;

	p->bogomem_size = 0;
	p->chipset_mask = 0;
	p->cs_rtc = 0;
	p->nr_floppies = 0;
	p->dfxtype[0] = DRV_NONE;
	p->dfxtype[1] = DRV_NONE;
	set_68000_compa (p, compa);
	p->cs_compatible = CP_A500;
	built_in_chipset_prefs (p);
	//fetch_datapath (p->flashfile, sizeof (p->flashfile) / sizeof (char));
	_tcscat (p->flashfile, "arcadia.nvr");
	roms[0] = 5;
	roms[1] = 4;
	roms[2] = -1;
	if (!configure_rom (p, roms, romcheck))
		return 0;
	roms[0] = 49;
	roms[1] = 50;
	roms[2] = 51;
	roms[3] = -1;
	if (!configure_rom (p, roms, romcheck))
		return 0;
	rl = getarcadiaroms ();
	for (i = 0; rl[i]; i++) {
		if (config-- == 0) {
			roms[0] = rl[i]->rd->id;
			roms[1] = -1;
			configure_rom (p, roms, 0);
			break;
		}
	}
	xfree (rl);
	return 1;
}

int built_in_prefs (struct uae_prefs *p, int model, int config, int compa, int romcheck)
{
	int v = 0, i;

	buildin_default_prefs (p);
	switch (model)
	{
	case 0:
		v = bip_a500 (p, config, compa, romcheck);
		break;
	case 1:
		v = bip_a500p (p, config, compa, romcheck);
		break;
	case 2:
		v = bip_a600 (p, config, compa, romcheck);
		break;
	case 3:
		v = bip_a1000 (p, config, compa, romcheck);
		break;
	case 4:
		v = bip_a1200 (p, config, compa, romcheck);
		break;
	case 5:
		v = bip_a3000 (p, config, compa, romcheck);
		break;
	case 6:
		v = bip_a4000 (p, config, compa, romcheck);
		break;
	case 7:
		v = bip_a4000t (p, config, compa, romcheck);
		break;
	case 8:
		v = bip_cd32 (p, config, compa, romcheck);
		break;
	case 9:
		v = bip_cdtv (p, config, compa, romcheck);
		break;
	case 10:
		v = bip_arcadia (p, config , compa, romcheck);
		break;
	case 11:
		v = bip_super (p, config, compa, romcheck);
		break;
	}
	for (i = 0; i < 4; i++) {
		if (p->dfxtype[i] < 0)
			p->df[i][0] = DRV_35_DD;
	}
	return v;
}

int built_in_chipset_prefs (struct uae_prefs *p)
{
	if (!p->cs_compatible)
		return 1;

	p->cs_a1000ram = 0;
	p->cs_cd32c2p = p->cs_cd32cd = p->cs_cd32nvram = 0;
	p->cs_cdtvcd = p->cs_cdtvram = 0;
	p->cs_fatgaryrev = -1;
	p->cs_ide = 0;
	p->cs_ramseyrev = -1;
	p->cs_deniserev = -1;
	p->cs_agnusrev = -1;
	p->cs_mbdmac = 0;
	p->cs_a2091 = 0;
	p->cs_pcmcia = 0;
	p->cs_ksmirror_e0 = 1;
	p->cs_ciaoverlay = 1;
	p->cs_ciaatod = 0;
	p->cs_df0idhw = 1;
	p->cs_resetwarning = 1;

	switch (p->cs_compatible)
	{
	case CP_GENERIC: // generic
		p->cs_rtc = 2;
		p->cs_fatgaryrev = 0;
		p->cs_ide = -1;
		p->cs_mbdmac = 1;
		p->cs_ramseyrev = 0x0f;
		break;
	case CP_CDTV: // CDTV
		p->cs_rtc = 1;
		p->cs_cdtvcd = p->cs_cdtvram = 1;
		p->cs_df0idhw = 1;
		p->cs_ksmirror_e0 = 0;
		break;
	case CP_CD32: // CD32
		p->cs_cd32c2p = p->cs_cd32cd = p->cs_cd32nvram = 1;
		p->cs_ksmirror_e0 = 0;
		p->cs_ksmirror_a8 = 1;
		p->cs_ciaoverlay = 0;
		p->cs_resetwarning = 0;
		break;
	case CP_A500: // A500
		p->cs_df0idhw = 0;
		p->cs_resetwarning = 0;
		if (p->bogomem_size || p->chipmem_size > 1 || p->fastmem_size)
			p->cs_rtc = 1;
		break;
	case CP_A500P: // A500+
		p->cs_rtc = 1;
		p->cs_resetwarning = 0;
		break;
	case CP_A600: // A600
		p->cs_rtc = 1;
		p->cs_ide = IDE_A600A1200;
		p->cs_pcmcia = 1;
		p->cs_ksmirror_a8 = 1;
		p->cs_ciaoverlay = 0;
		p->cs_resetwarning = 0;
		break;
	case CP_A1000: // A1000
		p->cs_a1000ram = 1;
		p->cs_ciaatod = p->ntscmode ? 2 : 1;
		p->cs_ksmirror_e0 = 0;
		p->cs_rtc = 0;
		p->cs_agnusbltbusybug = 1;
		p->cs_dipagnus = 1;
		break;
	case CP_A1200: // A1200
		p->cs_ide = IDE_A600A1200;
		p->cs_pcmcia = 1;
		p->cs_ksmirror_a8 = 1;
		p->cs_ciaoverlay = 0;
		break;
	case CP_A2000: // A2000
		p->cs_rtc = 1;
		p->cs_ciaatod = p->ntscmode ? 2 : 1;
		break;
	case CP_A3000: // A3000
		p->cs_rtc = 2;
		p->cs_fatgaryrev = 0;
		p->cs_ramseyrev = 0x0d;
		p->cs_mbdmac = 1;
		p->cs_ciaatod = p->ntscmode ? 2 : 1;
		break;
	case CP_A3000T: // A3000T
		p->cs_rtc = 2;
		p->cs_fatgaryrev = 0;
		p->cs_ramseyrev = 0x0d;
		p->cs_mbdmac = 1;
		p->cs_ciaatod = p->ntscmode ? 2 : 1;
		break;
	case CP_A4000: // A4000
		p->cs_rtc = 2;
		p->cs_fatgaryrev = 0;
		p->cs_ramseyrev = 0x0f;
		p->cs_ide = IDE_A4000;
		p->cs_mbdmac = 0;
		p->cs_ksmirror_a8 = 1;
		p->cs_ciaoverlay = 0;
		break;
	case CP_A4000T: // A4000T
		p->cs_rtc = 2;
		p->cs_fatgaryrev = 0;
		p->cs_ramseyrev = 0x0f;
		p->cs_ide = IDE_A4000;
		p->cs_mbdmac = 2;
		p->cs_ksmirror_a8 = 1;
		p->cs_ciaoverlay = 0;
		break;
	}
	return 1;
}

void config_check_vsync (void)
{
	static int cnt = 0;
	if (cnt == 0) {
		/* resolution_check_change (); */
		DISK_check_change ();
		cnt = 5;
	}
	cnt--;
	if (config_changed) {
		if (config_changed == 1)
			write_log ("* configuration check trigger\n");
		config_changed++;
		if (config_changed > 10)
			config_changed = 0;
	}
}

