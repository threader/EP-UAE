/*
* UAE - The Un*x Amiga Emulator
*
* Debugger
*
* (c) 1995 Bernd Schmidt
* (c) 2006 Toni Wilen
*
*/

#include "sysconfig.h"
#include "sysdeps.h"

#include <ctype.h>
#include <signal.h>

#include "options.h"
#include "uae.h"
#include "memory.h"
#include "custom.h"
#include "newcpu.h"
#include "cpu_prefetch.h"
#include "debug.h"
#include "cia.h"
#include "xwin.h"
#include "identify.h"
#include "disk.h"
#include "savestate.h"
#include "autoconf.h"
#include "filesys.h"
#include "akiko.h"
#include "inputdevice.h"
#include "crc32.h"
#include "cpummu.h"
#include "rommgr.h"

int debugger_active;
static uaecptr skipaddr_start, skipaddr_end;
static int skipaddr_doskip;
static uae_u32 skipins;
static int do_skip;
#ifdef SAVESTATE
static int debug_rewind;
#endif
static int memwatch_enabled, memwatch_triggered;
static uae_u16 sr_bpmask, sr_bpvalue;
int debugging;
int exception_debugging;
int debug_copper = 0;
int debug_dma = 0;
int debug_sprite_mask = 0xff;
int debug_illegal = 0;
uae_u64 debug_illegal_mask;

static uaecptr processptr;
static uae_char *processname;

static uaecptr debug_copper_pc;

extern int audio_channel_mask;
extern int inputdevice_logging;

static FILE *logfile;

#ifndef _WIN32
#define console_out               printf
#define console_flush()           fflush( stdout )
#define console_get( input, len ) fgets( input, len, stdin )
#endif


#define MAX_HIST 100

unsigned int firsthist = 0;
unsigned int lasthist = 0;

static struct regstruct history[MAX_HIST];


void deactivate_debugger (void)
{
	debugger_active = 0;
	debugging = 0;
	exception_debugging = 0;
	processptr = 0;
	xfree (processname);
	processname = NULL;
}

void activate_debugger (void)
{
    if (debuggable ()) {
	if (is_fullscreen ()) {
	    toggle_fullscreen ();
	    if (is_fullscreen ()) {
		write_log ("Cannot activate debugger in full-screen mode\n");
		return;
            }
        }

	if (logfile)
	    fclose (logfile);
	logfile = 0;
	do_skip = 0;
	if (debugger_active)
		return;
	debugger_active = 1;
	set_special (&regs, SPCFLAG_BRK);
	debugging = 1;
	mmu_triggered = 0;
	}
}


static const char help[] = {
	"          HELP for UAE Debugger\n"
	"         -----------------------\n\n"
	"  g [<address>]         Start execution at the current address or <address>\n"
	"  c                     Dump state of the CIA, disk drives and custom registers\n"
	"  r                     Dump state of the CPU\n"
	"  r <reg> <value>       Modify CPU registers (Dx,Ax,USP,ISP,VBR,...)\n"
	"  m <address> [<lines>] Memory dump starting at <address>\n"
	"  d <address> [<lines>] Disassembly starting at <address>\n"
	"  t [instructions]      Step one or more instructions\n"
	"  z                     Step through one instruction - useful for JSR, DBRA etc\n"
	"  f                     Step forward until PC in RAM (\"boot block finder\")\n"
	"  f <address>           Add/remove breakpoint\n"
	"  fa <address> [<start>] [<end>]\n"
	"                        Find effective address <address>\n"
	"  fi                    Step forward until PC points to RTS, RTD or RTE\n"
	"  fi <opcode>           Step forward until PC points to <opcode>\n"
	"  fp \"<name>\"/<addr>    Step forward until process <name> or <addr> is active\n"
	"  fl                    List breakpoints\n"
	"  fd                    Remove all breakpoints\n"
	"  fs <val> <mask>       Break when (SR & mask) = val\n"                   
	"  f <addr1> <addr2>     Step forward until <addr1> <= PC <= <addr2>\n"
	"  e                     Dump contents of all custom registers, ea = AGA colors\n"
	"  i [<addr>]            Dump contents of interrupt and trap vectors\n"
	"  il [<mask>]           Exception breakpoint\n"
	"  o <0-2|addr> [<lines>]View memory as Copper instructions\n"
	"  od                    Enable/disable Copper vpos/hpos tracing\n"
	"  ot                    Copper single step trace\n"
	"  ob <addr>             Copper breakpoint\n"
	"  H[H] <cnt>            Show PC history (HH=full CPU info) <cnt> instructions\n"
	"  C <value>             Search for values like energy or lifes in games\n"
	"  Cl                    List currently found trainer addresses\n"
	"  D[idxzs <[max diff]>] Deep trainer. i=new value must be larger, d=smaller,\n"
	"                        x = must be same, z = must be different, s = restart.\n"
	"  W <address> <value>   Write into Amiga memory\n"
	"  w <num> <address> <length> <R/W/I/F/C> [<value>] (read/write/opcode/freeze/mustchange)\n"
	"                        Add/remove memory watchpoints\n"
	"  wd [<0-1>]            Enable illegal access logger. 1 = enable break.\n"
	"  S <file> <addr> <n>   Save a block of Amiga memory\n"
	"  s \"<string>\"/<values> [<addr>] [<length>]\n"
	"                        Search for string/bytes\n"
	"  T or Tt               Show exec tasks and their PCs\n"
	"  Td,Tl,Tr              Show devices, libraries or resources\n"
#ifdef SAVESTATE
	"  b                     Step to previous state capture position\n"
#endif
	"  M<a/b/s> <val>        Enable or disable audio channels, bitplanes or sprites\n"
	"  sp <addr> [<addr2][<size>] Dump sprite information\n"
	"  di <mode> [<track>]   Break on disk access. R=DMA read,W=write,RW=both,P=PIO\n"
	"                        Also enables level 1 disk logging\n"
	"  did <log level>       Enable disk logging\n"
	"  dj [<level bitmask>]  Enable joystick/mouse input debugging\n"
	"  smc [<0-1>]           Enable self-modifying code detector. 1 = enable break.\n"
	"  dm                    Dump current address space map\n"
	"  v <vpos> [<hpos>]     Show DMA data (accurate only in cycle-exact mode)\n"
	"                        v [-1 to -4] = enable visual DMA debugger\n"
	"  ?<value>              Hex/Bin/Dec converter\n"
#ifdef _WIN32
	"  x                     Close debugger.\n"
	"  xx                    Switch between console and GUI debugger.\n"
	"  mg <address>          Memory dump starting at <address> in GUI\n"
	"  dg <address>          Disassembly starting at <address> in GUI\n"
#endif
	"  q                     Quit the emulator. You don't want to use this command.\n\n"
};

void debug_help (void)
{
	console_out (help);
}

static int debug_linecounter;
#define MAX_LINECOUNTER 1000

static void ignore_ws (const char **c)
{
    while (**c && isspace (**c))
	(*c)++;
}

static uae_u32 readint (const char **c);
static uae_u32 readhex (const char **c)
{
    uae_u32 val = 0;
    char nc;

    ignore_ws (c);
    if (**c == '!' || **c == '_') {
	(*c)++;
	return readint (c);
    }
    while (isxdigit (nc = **c)) {
	(*c)++;
	val *= 16;
	nc = toupper (nc);
	if (isdigit (nc)) {
	    val += nc - '0';
	} else {
	    val += nc - 'A' + 10;
	}
    }
    return val;
}

static uae_u32 readint (const char **c)
{
    uae_u32 val = 0;
    char nc;
    int sign = 1;

    ignore_ws (c);
    if (**c == '$') {
	(*c)++;
	return readhex (c);
    }
    if (**c == '0' && toupper ((*c)[1]) == 'X') {
	(*c) += 2;
	return readhex (c);
    }
    if (**c == '-') {
	sign = 1;
	(*c)++;
    }
    while (isdigit (nc = **c)) {
	(*c)++;
	val *= 10;
	val += nc - '0';
    }
    return val * sign;
}

static char next_char (const char **c)
{
    ignore_ws (c);
    return *(*c)++;
}

static char peek_next_char (const char **c)
{
    const char *pc = *c;
    return pc[1];
}

static int more_params (const char **c)
{
    ignore_ws (c);
    return (**c) != 0;
}

static int next_string (const char **c, char *out, unsigned int max, int forceupper)
{
    char *p = out;

    *p = 0;
    while (**c != 0) {
	if (**c == 32) {
	    ignore_ws (c);
	    return strlen (out);
	}
	*p = next_char (c);
	if (forceupper)
	    *p = toupper (*p);
	*++p = 0;
	max--;
	if (max <= 1)
	    break;
    }
    return strlen (out);
}

static uae_u32 lastaddr (void)
{
	if (currprefs.z3fastmem_size)
		return z3fastmem_start + currprefs.z3fastmem_size;
	if (currprefs.mbresmem_high_size)
		return a3000hmem_start + currprefs.mbresmem_high_size;
	if (currprefs.mbresmem_low_size)
		return a3000lmem_start + currprefs.mbresmem_low_size;
	if (currprefs.bogomem_size)
		return bogomem_start + currprefs.bogomem_size;
	if (currprefs.fastmem_size)
		return fastmem_start + currprefs.fastmem_size;
	return currprefs.chipmem_size;
}

static uaecptr nextaddr2 (uaecptr addr, int *next)
{
	uaecptr prev, prevx;
	int size, sizex;

	if (addr >= lastaddr()) {
		*next = -1;
		return 0xffffffff;
	}
	prev = currprefs.z3fastmem_start;
	size = currprefs.z3fastmem_size;

	if (currprefs.mbresmem_high_size) {
		sizex = size;
		prevx = prev;
		size = currprefs.mbresmem_high_size;
		prev = a3000hmem_start;
		if (addr == prev + size) {
			*next = prevx + sizex;
			return prevx;
		}
	}
	if (currprefs.mbresmem_low_size) {
		prevx = prev;
		sizex = size;
		size = currprefs.mbresmem_low_size;
		prev = a3000lmem_start;
		if (addr == prev + size) {
			*next = prevx + sizex;
			return prevx;
		}
	}
	if (currprefs.bogomem_size) {
		sizex = size;
		prevx = prev;
		size = currprefs.bogomem_size;
		prev = bogomem_start;
		if (addr == prev + size) {
			*next = prevx + sizex;
			return prevx;
		}
	}
	if (currprefs.fastmem_size) {
		sizex = size;
		prevx = prev;
		size = currprefs.fastmem_size;
		prev = fastmem_start;
		if (addr == prev + size) {
			*next = prevx + sizex;
			return prevx;
		}
	}
	sizex = size;
	prevx = prev;
	size = currprefs.chipmem_size;
	if (addr == size) {
		*next = prevx + sizex;
		return prevx;
	}
	if (addr == 1)
		*next = size;
	return addr;
}

static uaecptr nextaddr (uaecptr addr, uaecptr *end)
{
	uaecptr paddr = addr;
	int next;
	if (addr == 0xffffffff) {
		if (end)
			*end = currprefs.chipmem_size;
		return 0;
	}
	if (end)
		next = *end;
	addr = nextaddr2(addr + 1, &next);
	if (end)
		*end = next;
#if 0
	if (next && addr != 0xffffffff) {
		uaecptr xa = addr;
		if (xa == 1)
			xa = 0;
		console_out ("%08X -> %08X (%08X)...\n", xa, xa + next - 1, next);
	}
#endif
	return addr;
}

int safe_addr(uaecptr addr, int size)
{
	addrbank *ab = &get_mem_bank (addr);
	if (!ab)
		return 0;
	if (ab->flags & ABFLAG_SAFE)
		return 1;
	if (!ab->check (addr, size))
		return 0;
	if (ab->flags & (ABFLAG_RAM | ABFLAG_ROM | ABFLAG_ROMIN | ABFLAG_SAFE))
		return 1;
	return 0;
}

uaecptr dumpmem2 (uaecptr addr, TCHAR *out, int osize)
{
	int i, cols = 8;
	int nonsafe = 0;

	if (osize <= (9 + cols * 5 + 1 + 2 * cols))
		return addr;
	_stprintf (out, "%08lX ", addr);
	for (i = 0; i < cols; i++) {
		uae_u8 b1, b2;
		b1 = b2 = 0;
		if (safe_addr(addr, 2)) {
			b1 = get_byte (addr + 0);
			b2 = get_byte (addr + 1);
			_stprintf (out + 9 + i * 5, "%02X%02X ", b1, b2);
			out[9 + cols * 5 + 1 + i * 2 + 0] = b1 >= 32 && b1 < 127 ? b1 : '.';
			out[9 + cols * 5 + 1 + i * 2 + 1] = b2 >= 32 && b2 < 127 ? b2 : '.';
		} else {
			nonsafe++;
			_tcscpy (out + 9 + i * 5, "**** ");
			out[9 + cols * 5 + 1 + i * 2 + 0] = '*';
			out[9 + cols * 5 + 1 + i * 2 + 1] = '*';
		}
		addr += 2;
	}
	out[9 + cols * 5] = ' ';
	out[9 + cols * 5 + 1 + 2 * cols] = 0;
	if (nonsafe == cols) {
		addrbank *ab = &get_mem_bank (addr);
		if (ab->name)
			memcpy (out + (9 + 4 + 1) * sizeof (TCHAR), ab->name, _tcslen (ab->name) * sizeof (TCHAR));
	}
	return addr;
}

static void dumpmem (uaecptr addr, uaecptr *nxmem, int lines)
{
	TCHAR line[MAX_LINEWIDTH + 1];
	for (;lines--;) {
		addr = dumpmem2 (addr, line, sizeof(line));
		write_log ("%s\n", line);
	}
	*nxmem = addr;
}

static void dump_custom_regs (void)
{
	unsigned int len;
	unsigned int i, j, end;
	uae_u8 *p1, *p2, *p3, *p4;

	p1 = p2 = save_custom (&len, 0, 1);
	p1 += 4; // skip chipset type
	for (i = 0; i < 4; i++) {
		p4 = p1 + 0xa0 + i * 16;
		p3 = save_audio (i, &len, 0);
		p4[0] = p3[12];
		p4[1] = p3[13];
		p4[2] = p3[14];
		p4[3] = p3[15];
		p4[4] = p3[4];
		p4[5] = p3[5];
		p4[6] = p3[8];
		p4[7] = p3[9];
		p4[8] = 0;
		p4[9] = p3[1];
		p4[10] = p3[10];
		p4[11] = p3[11];
		free (p3);
	}
	end = 0;
	while (custd[end].name)
		end++;
	end++;
	end /= 2;
	for (i = 0; i < end; i++) {
		uae_u16 v1, v2;
		int addr1, addr2;
		j = end + i;
		addr1 = custd[i].adr & 0x1ff;
		addr2 = custd[j].adr & 0x1ff;
		v1 = (p1[addr1 + 0] << 8) | p1[addr1 + 1];
		v2 = (p1[addr2 + 0] << 8) | p1[addr2 + 1];
		console_out ("%03X %s\t%04X\t%03X %s\t%04X\n",
			addr1, custd[i].name, v1,
			addr2, custd[j].name, v2);
	}
	free (p2);
}

static void dump_vectors (uaecptr addr)
{
	unsigned int i = 0, j = 0;

	if (addr == 0xffffffff)
		addr = regs.vbr;

	while (int_labels[i].name || trap_labels[j].name) {
		if (int_labels[i].name) {
			console_out ("$%08X %02d: %12s $%08X  ", int_labels[i].adr + addr, int_labels[i].adr / 4,
				int_labels[i].name, get_long (int_labels[i].adr + addr));
			i++;
		} /* note */
		if (trap_labels[j].name) {
			console_out ("$%08X %02d: %12s $%08X", trap_labels[j].adr + addr, trap_labels[j].adr / 4,
				trap_labels[j].name, get_long (trap_labels[j].adr + addr));
			j++;
		}
		console_out ("\n");
	}
}

static void disassemble_wait (uae_u32 insn)
{
	unsigned int vp, hp, ve, he, bfd, v_mask, h_mask;
	int doout = 0;

	vp = (insn & 0xff000000) >> 24;
	hp = (insn & 0x00fe0000) >> 16;
	ve = (insn & 0x00007f00) >> 8;
	he = (insn & 0x000000fe);
	bfd = (insn & 0x00008000) >> 15;

	/* bit15 can never be masked out*/
	v_mask = vp & (ve | 0x80);
	h_mask = hp & he;
	if (v_mask > 0) {
		doout = 1;
		console_out ("vpos ");
		if (ve != 0x7f) {
			console_out ("& 0x%02x ", ve);
		}
		console_out (">= 0x%02x", v_mask);
	}
	if (he > 0) {
		if (v_mask > 0) {
			console_out (" and");
		}
		console_out (" hpos ");
		if (he != 0xfe) {
			console_out ("& 0x%02x ", he);
		}
		console_out (">= 0x%02x", h_mask);
	} else {
		if (doout)
			console_out (", ");
		console_out (", ignore horizontal");
	}

	console_out ("\n                        \t; VP %02x, VE %02x; HP %02x, HE %02x; BFD %d\n",
		vp, ve, hp, he, bfd);
}

#define NR_COPPER_RECORDS 100000
/* Record copper activity for the debugger.  */
struct cop_rec
{
	unsigned int hpos, vpos;
	uaecptr addr;
};
static struct cop_rec *cop_record[2];
static unsigned int nr_cop_records[2], curr_cop_set;

#define NR_DMA_REC_HPOS 256
#define NR_DMA_REC_VPOS 1000
static struct dma_rec *dma_record[2];
static int dma_record_toggle;

void record_dma_reset (void)
{
	int v, h;
	struct dma_rec *dr, *dr2;

	if (!dma_record[0])
		return;
	dma_record_toggle ^= 1;
	dr = dma_record[dma_record_toggle];
	for (v = 0; v < NR_DMA_REC_VPOS; v++) {
		for (h = 0; h < NR_DMA_REC_HPOS; h++) {
			dr2 = &dr[v * NR_DMA_REC_HPOS + h];
			memset (dr2, 0, sizeof (struct dma_rec));
			dr2->reg = 0xffff;
			dr2->addr = 0xffffffff;
		}
	}
}

void record_copper_reset (void)
{
	/* Start a new set of copper records.  */
	curr_cop_set ^= 1;
	nr_cop_records[curr_cop_set] = 0;
}

STATIC_INLINE uae_u32 ledcolor (uae_u32 c, uae_u32 *rc, uae_u32 *gc, uae_u32 *bc, uae_u32 *a)
{
	uae_u32 v = rc[(c >> 16) & 0xff] | gc[(c >> 8) & 0xff] | bc[(c >> 0) & 0xff];
	if (a)
		v |= a[255 - ((c >> 24) & 0xff)];
	return v;
}

STATIC_INLINE void putpixel (uae_u8 *buf, int bpp, int x, xcolnr c8)
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
			uae_u32 *p = (uae_u32*)buf + x;
			*p = c8;
			break;
		}
	}
}

#define lc(x) ledcolor (x, xredcolors, xgreencolors, xbluecolors, NULL);
void debug_draw_cycles (uae_u8 *buf, int bpp, int line, int width, int height, uae_u32 *xredcolors, uae_u32 *xgreencolors, uae_u32 *xbluescolors)
{
	int y, x, xx, dx, xplus, yplus;
	struct dma_rec *dr;
	int t;
	uae_u32 cc[DMARECORD_MAX];

	if (debug_dma >= 4)
		yplus = 2;
	else
		yplus = 1;
	if (debug_dma >= 3)
		xplus = 2;
	else
		xplus = 1;

	t = dma_record_toggle ^ 1;
	y = line / yplus - 8;

	if (y < 0)
		return;
	if (y > maxvpos)
		return;
	if (y >= height)
		return;

	dx = width - xplus * ((maxhpos + 1) & ~1) - 16;

	cc[0] = lc(0x222222);
	cc[DMARECORD_REFRESH] = lc(0x444444);
	cc[DMARECORD_CPU] = lc(0x888888);
	cc[DMARECORD_COPPER] = lc(0xeeee00);
	cc[DMARECORD_AUDIO] = lc(0xff0000);
	cc[DMARECORD_BLITTER] = lc(0x00ff00);
	cc[DMARECORD_BLITTER_LINE] = lc(0x008800);
	cc[DMARECORD_BITPLANE] = lc(0x0000ff);
	cc[DMARECORD_SPRITE] = lc(0xff00ff);
	cc[DMARECORD_DISK] = lc(0xffffff);

	for (x = 0; x < maxhpos; x++) {
		uae_u32 c = cc[0];
		xx = x * xplus + dx;
		dr = &dma_record[t][y * NR_DMA_REC_HPOS + x];
		if (dr->reg != 0xffff) {
			c = cc[dr->type];	    
		}
		putpixel (buf, bpp, xx, c);
		if (xplus)
			putpixel (buf, bpp, xx + 1, c);
	}
}




void record_dma_event (int evt, int hpos, int vpos)
{
	struct dma_rec *dr;

	if (!dma_record[0])
		return;
	if (hpos >= NR_DMA_REC_HPOS || vpos >= NR_DMA_REC_VPOS)
		return;
	dr = &dma_record[dma_record_toggle][vpos * NR_DMA_REC_HPOS + hpos];
	dr->evt |= evt;
}

struct dma_rec *record_dma (uae_u16 reg, uae_u16 dat, uae_u32 addr, int hpos, int vpos, int type)
{
	struct dma_rec *dr;

	if (!dma_record[0]) {
		dma_record[0] = xmalloc (struct dma_rec, NR_DMA_REC_HPOS * NR_DMA_REC_VPOS);
		dma_record[1] = xmalloc (struct dma_rec, NR_DMA_REC_HPOS * NR_DMA_REC_VPOS);
		dma_record_toggle = 0;
		record_dma_reset ();
	}
	if (hpos >= NR_DMA_REC_HPOS || vpos >= NR_DMA_REC_VPOS)
		return NULL;
	dr = &dma_record[dma_record_toggle][vpos * NR_DMA_REC_HPOS + hpos];
	if (dr->reg != 0xffff) {
		write_log ("DMA conflict: v=%d h=%d OREG=%04X NREG=%04X\n", vpos, hpos, dr->reg, reg);
		return dr;
	}
	dr->reg = reg;
	dr->dat = dat;
	dr->addr = addr;
	dr->type = type;
	return dr;
}

static void decode_dma_record (int hpos, int vpos, int toggle)
{
	struct dma_rec *dr;
	int h, i, maxh;

	if (!dma_record[0])
		return;
	dr = &dma_record[dma_record_toggle ^ toggle][vpos * NR_DMA_REC_HPOS];
	write_log ("Line: %02X %3d HPOS %02X %3d:\n", vpos, vpos, hpos, hpos);
	h = hpos;
	dr += hpos;
	maxh = hpos + 80;
	if (maxh > maxhpos)
		maxh = maxhpos;
	while (h < maxh) {
		int col = 9;
		int cols = 8;
		char l1[81];
		char l2[81];
		char l3[81];
		char l4[81];
		for (i = 0; i < cols && h < maxh; i++, h++, dr++) {
			int cl = i * col, cl2;
			int r = dr->reg;
			char *sr;

			sr = "    ";
			if (dr->type == DMARECORD_COPPER)
				sr = "COP ";
			else if (dr->type == DMARECORD_BLITTER)
				sr = "BLT ";
			else if (dr->type == DMARECORD_REFRESH)
				sr = "RFS ";
			else if (dr->type == DMARECORD_AUDIO)
				sr = "AUD ";
			else if (dr->type == DMARECORD_DISK)
				sr = "DSK ";
			else if (dr->type == DMARECORD_SPRITE)
				sr = "SPR ";
			_stprintf (l1 + cl, "[%02X %3d]", h, h);
			_tcscpy (l4 + cl, "        ");
			if (r != 0xffff) {
				if (r & 0x1000) {
					if ((r & 0x0100) == 0x0000)
						_tcscpy (l2 + cl, "  CPU-R  ");
					else if ((r & 0x0100) == 0x0100)
						_tcscpy (l2 + cl, "  CPU-W  ");
					if ((r & 0xff) == 4)
						l2[cl + 7] = 'L';
					if ((r & 0xff) == 2)
						l2[cl + 7] = 'W';
					if ((r & 0xff) == 1)
						l2[cl + 7] = 'B';
				} else {
					_stprintf (l2 + cl, "%4s %03X", sr, r);
				}
				_stprintf (l3 + cl, "    %04X", dr->dat);
				if (dr->addr != 0xffffffff)
					_stprintf (l4 + cl, "%08X", dr->addr & 0x00ffffff);
			} else {
				_tcscpy (l2 + cl, "        ");
				_tcscpy (l3 + cl, "        ");
			}
			cl2 = cl;
			if (dr->evt & DMA_EVENT_BLITNASTY)
				l3[cl2++] = 'N';
			if (dr->evt & DMA_EVENT_BLITFINISHED)
				l3[cl2++] = 'B';
			if (dr->evt & DMA_EVENT_BLITIRQ)
				l3[cl2++] = 'b';
			if (dr->evt & DMA_EVENT_BPLFETCHUPDATE)
				l3[cl2++] = 'p';
			if (dr->evt & DMA_EVENT_COPPERWAKE)
				l3[cl2++] = 'W';
			if (dr->evt & DMA_EVENT_CPUIRQ)
				l3[cl2++] = 'I';
			if (dr->evt & DMA_EVENT_INTREQ)
				l3[cl2++] = 'i';
			if (i < cols - 1 && h < maxh - 1) {
				l1[cl + col - 1] = 32;
				l2[cl + col - 1] = 32;
				l3[cl + col - 1] = 32;
				l4[cl + col - 1] = 32;
			}
		}
		console_out ("%s\n", l1);
		console_out ("%s\n", l2);
		console_out ("%s\n", l3);
		console_out ("%s\n", l4);
		console_out ("\n");
	}
}

void record_copper (uaecptr addr, unsigned int hpos, unsigned int vpos)
{
	unsigned int t = nr_cop_records[curr_cop_set];
	if (!cop_record[0]) {
		cop_record[0] = xmalloc (struct cop_rec, NR_COPPER_RECORDS);
		cop_record[1] = xmalloc (struct cop_rec, NR_COPPER_RECORDS);
	}
	if (t < NR_COPPER_RECORDS) {
		cop_record[curr_cop_set][t].addr = addr;
		cop_record[curr_cop_set][t].hpos = hpos;
		cop_record[curr_cop_set][t].vpos = vpos;
		nr_cop_records[curr_cop_set] = t + 1;
	}
	if (debug_copper & 2) { /* trace */
		debug_copper &= ~2;
		activate_debugger ();
	}
	if ((debug_copper & 4) && addr >= debug_copper_pc && addr <= debug_copper_pc + 3) {
		debug_copper &= ~4;
		activate_debugger ();
	}
}

static int find_copper_record (uaecptr addr, unsigned int *phpos, unsigned int *pvpos)
{
	unsigned int s = curr_cop_set ^ 1;
	unsigned int t = nr_cop_records[s];
	unsigned int i;
	for (i = 0; i < t; i++) {
		if (cop_record[s][i].addr == addr) {
		    *phpos = cop_record[s][i].hpos;
		    *pvpos = cop_record[s][i].vpos;
		    return 1;
		}
	}
	return 0;
}

/* simple decode copper by Mark Cox */
static void decode_copper_insn (uae_u32 insn, uaecptr addr)
{
    uae_u32 insn_type = insn & 0x00010001;
    unsigned int hpos, vpos;
    char here = ' ';
    char record[] = "          ";
    if (find_copper_record (addr, &hpos, &vpos)) {
	sprintf (record, " [%03x %03x]", vpos, hpos);
    }

    if (get_copper_address(-1) >= addr && get_copper_address(-1) <= addr + 3)
	here = '*';

    console_out ("%c%08x: %04x %04x%s\t; ", here, addr, insn >> 16, insn & 0xFFFF, record);

    switch (insn_type) {
    case 0x00010000: /* WAIT insn */
	console_out ("Wait for ");
	disassemble_wait (insn);

	if (insn == 0xfffffffe)
	    console_out ("                           \t; End of Copperlist\n");

	break;

    case 0x00010001: /* SKIP insn */
	console_out ("Skip if ");
	disassemble_wait (insn);
	break;

    case 0x00000000:
    case 0x00000001: /* MOVE insn */
	{
	    uae_u32 addr = (insn >> 16) & 0x1fe;
	    int i = 0;
	    while (custd[i].name) {
		if (custd[i].adr == addr + 0xdff000)
		    break;
		i++;
	    }
	    if (custd[i].name)
		console_out ("%s := 0x%04x\n", custd[i].name, insn & 0xffff);
	    else
		console_out ("%04x := 0x%04x\n", addr, insn & 0xffff);
	}
	break;

    default:
	abort ();
    }

}

static uaecptr decode_copperlist (uaecptr address, unsigned int nolines)
{
    uae_u32 insn;
    while (nolines-- > 0) {
	insn = get_long (address);
	decode_copper_insn (insn, address);
	address += 4;
    }
    return address;
    /* You may wonder why I don't stop this at the end of the copperlist?
     * Well, often nice things are hidden at the end and it is debatable the actual
     * values that mean the end of the copperlist */
}

static int copper_debugger (const char **c)
{
    static uaecptr nxcopper;
    uae_u32 maddr;
    unsigned int lines;

    if (**c == 'd') {
	next_char (c);
	if (debug_copper)
	    debug_copper = 0;
	else
	    debug_copper = 1;
	console_out ("Copper debugger %s.\n", debug_copper ? "enabled" : "disabled");
    } else if (**c == 't') {
	debug_copper = 1|2;
	return 1;
    } else if (**c == 'b') {
	(*c)++;
	debug_copper = 1|4;
	if (more_params (c)) {
	    debug_copper_pc = readhex (c);
	    console_out ("Copper breakpoint @0x%8.8x\n", debug_copper_pc);
	} else {
	    debug_copper &= ~4;
	}
    } else {
	if (more_params (c)) {
	    maddr = readhex (c);
	    if (maddr == 1 || maddr == 2)
		maddr = get_copper_address (maddr);
	    else if (maddr == 0)
		maddr = get_copper_address (-1);
	} else
	    maddr = nxcopper;

	if (more_params (c))
	    lines = readhex (c);
	else
	    lines = 20;

	nxcopper = decode_copperlist (maddr, lines);
    }
    return 0;
}

/* cheat-search by Holger Jakob */
static void cheatsearch (const char **c)
{
    uae_u8 *p = get_real_address (0);
    static uae_u32 *vlist = NULL;
    uae_u32 ptr;
    uae_u32 val = 0;
    uae_u32 type = 0; /* not yet */
    uae_u32 count = 0;
    uae_u32 fcount = 0;
    uae_u32 full = 0;

    ignore_ws (c);
    val = readhex (c);
    if (vlist == NULL) {
	vlist = malloc (256 * 4);
	if (vlist != 0) {
	    for (count = 0; count < 255; count++)
		vlist[count] = 0;
	    count = 0;
	    for (ptr = 0; ptr < allocated_chipmem - 40; ptr += 2, p += 2) {
		if (ptr >= 0x438 && p[3] == (val & 0xff)
		    && p[2] == (val >> 8 & 0xff)
		    && p[1] == (val >> 16 & 0xff)
		    && p[0] == (val >> 24 & 0xff))
		{
		    if (count < 255) {
			vlist[count++]=ptr;
			console_out ("%08x: %x%x%x%x\n",ptr,p[0],p[1],p[2],p[3]);
		    } else
			full = 1;
		}
	    }
	    console_out ("Found %d possible addresses with %d\n",count,val);
	    console_out ("Now continue with 'g' and use 'C' with a different value\n");
	}
    } else {
	for (count = 0; count<255; count++) {
	    if (p[vlist[count]+3] == (val & 0xff)
		&& p[vlist[count]+2] == (val>>8 & 0xff)
		&& p[vlist[count]+1] == (val>>16 & 0xff)
		&& p[vlist[count]] == (val>>24 & 0xff))
	    {
		fcount++;
		console_out ("%08x: %x%x%x%x\n", vlist[count], p[vlist[count]],
			p[vlist[count]+1], p[vlist[count]+2], p[vlist[count]+3]);
	    }
	}
	console_out ("%d hits of %d found\n",fcount,val);
	free (vlist);
	vlist = NULL;
    }
}

struct breakpoint_node bpnodes[BREAKPOINT_TOTAL];
static addrbank **debug_mem_banks;
static addrbank *debug_mem_area;
struct memwatch_node mwnodes[MEMWATCH_TOTAL];
static struct memwatch_node mwhit;

static uae_u8 *illgdebug, *illghdebug;
static int illgdebug_break;
extern int cdtv_enabled, cd32_enabled;

static void illg_free (void)
{
	xfree (illgdebug);
	illgdebug = NULL;
	xfree (illghdebug);
	illghdebug = NULL;
}

static void illg_init (void)
{
    unsigned int i;
	uae_u8 c = 3;
	uaecptr addr, end;


    free (illgdebug);
	illgdebug = xmalloc (uae_u8, 0x01000000);
	illghdebug = xmalloc (uae_u8, 65536);
	if (!illgdebug || !illghdebug) {
		illg_free();
		return;
	}
	addr = 0xffffffff;
	while ((addr = nextaddr (addr, &end)) != 0xffffffff)  {
		if (end < 0x01000000) {
			memset (illgdebug + addr, c, end - addr);
		} else {
			uae_u32 s = addr >> 16;
			uae_u32 e = end >> 16;
			memset (illghdebug + s, c, e - s);
		}
		addr = end - 1;
	}
	if (currprefs.gfxmem_size)
		memset (illghdebug + (p96ram_start >> 16), 3, currprefs.gfxmem_size >> 16);

	i = 0;
	while (custd[i].name) {
		int rw = custd[i].rw;
		illgdebug[custd[i].adr] = rw;
		illgdebug[custd[i].adr + 1] = rw;
		i++;
	}
	for (i = 0; i < 16; i++) { /* CIAs */
		if (i == 11)
			continue;
		illgdebug[0xbfe001 + i * 0x100] = c;
		illgdebug[0xbfd000 + i * 0x100] = c;
	}
	memset (illgdebug + 0xf80000, 1, 512 * 1024); /* KS ROM */
	memset (illgdebug + 0xdc0000, c, 0x3f); /* clock */
#ifdef CDTV
	if (currprefs.cs_cdtvram) {
		memset (illgdebug + 0xdc8000, c, 4096); /* CDTV batt RAM */
		memset (illgdebug + 0xf00000, 1, 256 * 1024); /* CDTV ext ROM */
	}
#endif
#ifdef CD32
	if (currprefs.cs_cd32cd) {
		memset (illgdebug + AKIKO_BASE, c, AKIKO_BASE_END - AKIKO_BASE);
		memset (illgdebug + 0xe00000, 1, 512 * 1024); /* CD32 ext ROM */
	}
#endif
    if (cloanto_rom)
	memset (illgdebug + 0xe00000, 1, 512 * 1024);
#ifdef FILESYS
	if (uae_boot_rom) /* filesys "rom" */
		memset (illgdebug + rtarea_base, 1, 0x10000);
#endif
}

/* add special custom register check here */
static void illg_debug_check (uaecptr addr, int rwi, int size, uae_u32 val)
{
	return;
}

static void illg_debug_do (uaecptr addr, int rwi, int size, uae_u32 val)
{
	uae_u8 mask;
    uae_u32 pc = m68k_getpc (&regs);
	int i;

	for (i = size - 1; i >= 0; i--) {
		uae_u8 v = val >> (i * 8);
		uae_u32 ad = addr + i;
		if (ad >= 0x01000000)
			mask = illghdebug[ad >> 16];
		else
			mask = illgdebug[ad];
		if ((mask & 3) == 3)
			return;
		if (mask & 0x80) {
			illg_debug_check (ad, rwi, size, val);
		} else if ((mask & 3) == 0) {
			if (rwi & 2)
				console_out ("W: %08X=%02X PC=%08X\n", ad, v, pc);
			else if (rwi & 1)
				console_out ("R: %08X    PC=%08X\n", ad, pc);
			if (illgdebug_break)
				activate_debugger ();
		} else if (!(mask & 1) && (rwi & 1)) {
			console_out ("RO: %08X=%02X PC=%08X\n", ad, v, pc);
			if (illgdebug_break)
				activate_debugger ();
		} else if (!(mask & 2) && (rwi & 2)) {
			console_out ("WO: %08X    PC=%08X\n", ad, pc);
			if (illgdebug_break)
				activate_debugger ();
		}
	}
}

STATIC_INLINE uae_u8 debug_mem_off (uaecptr addr)
{
    return (munge24 (addr) >> 16) & 0xff;
}

struct smc_item {
	uae_u32 addr;
	uae_u8 cnt;
};

static int smc_size, smc_mode;
static struct smc_item *smc_table;

static void smc_free (void)
{
	if (smc_table)
		console_out ("SMCD disabled\n");
	xfree(smc_table);
	smc_mode = 0;
	smc_table = NULL;
}

static void initialize_memwatch (int mode);
static void smc_detect_init (TCHAR **c)
{
	int v, i;

	ignore_ws (c);
	v = readint (c);
	smc_free ();
	smc_size = 1 << 24;
	if (currprefs.z3fastmem_size)
		smc_size = currprefs.z3fastmem_start + currprefs.z3fastmem_size;
	smc_size += 4;
	smc_table = xmalloc (struct smc_item, smc_size);
	if (!smc_table)
		return;
	for (i = 0; i < smc_size; i++) {
		smc_table[i].addr = 0xffffffff;
		smc_table[i].cnt = 0;
	}
	if (!memwatch_enabled)
		initialize_memwatch (0);
	if (v)
		smc_mode = 1;
	console_out ("SMCD enabled. Break=%d\n", smc_mode);
}

#define SMC_MAXHITS 8
static void smc_detector (uaecptr addr, int rwi, int size, uae_u32 *valp)
{
	int i, hitcnt;
	uaecptr hitaddr, hitpc;

	if (!smc_table)
		return;
	if (addr >= smc_size)
		return;
	if (rwi == 2) {
		for (i = 0; i < size; i++) {
			if (smc_table[addr + i].cnt < SMC_MAXHITS) {
				smc_table[addr + i].addr = m68k_getpc (&regs);
			}
		}
		return;
	}
	hitpc = smc_table[addr].addr;
	if (hitpc == 0xffffffff)
		return;
	hitaddr = addr;
	hitcnt = 0;
	while (addr < smc_size && smc_table[addr].addr != 0xffffffff) {
		smc_table[addr++].addr = 0xffffffff;
		hitcnt++;
	}
	if ((hitpc & 0xFFF80000) == 0xF80000)
		return;
	if (currprefs.cpu_model == 68000 && currprefs.cpu_compatible) {
		/* ignore single-word unconditional jump instructions
		* (instruction prefetch from PC+2 can cause false positives) */
		if (regs.irc == 0x4e75 || regs.irc == 4e74 || regs.irc == 0x4e72 || regs.irc == 0x4e77)
			return; /* RTS, RTD, RTE, RTR */
		if ((regs.irc & 0xff00) == 0x6000 && (regs.irc & 0x00ff) != 0 && (regs.irc & 0x00ff) != 0xff)
			return; /* BRA.B */
	}
	if (hitcnt < 100) {
		smc_table[hitaddr].cnt++;
		console_out ("SMC at %08X - %08X (%d) from %08X\n",
			hitaddr, hitaddr + hitcnt, hitcnt, hitpc);
		if (smc_mode)
			activate_debugger ();
		if (smc_table[hitaddr].cnt >= SMC_MAXHITS)
			console_out ("* hit count >= %d, future hits ignored\n", SMC_MAXHITS);
	}
}

static int memwatch_func (uaecptr addr, int rwi, int size, uae_u32 *valp)
{
	int i, brk;
	uae_u32 val = *valp;

	if (illgdebug)
		illg_debug_do (addr, rwi, size, val);
	addr = munge24 (addr);
	if (smc_table && (rwi >= 2))
		smc_detector (addr, rwi, size, valp);
	for (i = 0; i < MEMWATCH_TOTAL; i++) {
		struct memwatch_node *m = &mwnodes[i];
		uaecptr addr2 = m->addr;
		uaecptr addr3 = addr2 + m->size;
		int rwi2 = m->rwi;
		uae_u32 oldval = 0;
		int isoldval = 0;

		brk = 0;
		if (m->size == 0)
			continue;
		if (!(rwi & rwi2))
			continue;
		if (addr >= addr2 && addr < addr3)
			brk = 1;
		if (!brk && size == 2 && (addr + 1 >= addr2 && addr + 1 < addr3))
			brk = 1;
		if (!brk && size == 4 && ((addr + 2 >= addr2 && addr + 2 < addr3) || (addr + 3 >= addr2 && addr + 3 < addr3)))
			brk = 1;

		if (!brk)
			continue;
		if (mem_banks[addr >> 16]->check (addr, size)) {
			uae_u8 *p = mem_banks[addr >> 16]->xlateaddr (addr);
			if (size == 1)
				oldval = p[0];
			else if (size == 2)
				oldval = (p[0] << 8) | p[1];
			else
				oldval = (p[0] << 24) | (p[1] << 16) | (p[2] << 8) | (p[3] << 0);
			isoldval = 1;
		}

		if (!m->frozen && m->val_enabled) {
			int trigger = 0;
			uae_u32 mask = (1 << (m->size * 8)) - 1;
			int scnt = size;
			for (;;) {
				if (((m->val & mask) & m->valmask) == ((val & mask) & m->valmask))
					trigger = 1;
				if (mask & 0x80000000)
					break;
				if (m->size == 1) {
					mask <<= 8;
					scnt--;
				} else if (m->size == 2) {
					mask <<= 16;
					scnt -= 2;
				}
				if (scnt <= 0)
					break;
			}
			if (!trigger)
				continue;
		}

		if (m->mustchange && rwi == 2 && isoldval) {
			if (oldval == *valp)
				continue;
		}

		if (m->modval_written) {
			if (!rwi) {
				brk = 0;
			} else if (m->modval_written == 1) {
				m->modval_written = 2;
				m->modval = val;
				brk = 0;
			} else if (m->modval == val) {
				brk = 0;
			}
		}
		if (m->frozen) {
			if (m->val_enabled) {
				int shift = addr - m->addr;
				int max = 0;
				if (m->val > 256)
					max = 1;
				if (m->val > 65536)
					max = 3;
				shift &= max;
				*valp = m->val >> ((max - shift) * 8);
			}
			return 0;
		}
		//	if (!notinrom ())
		//	    return 1;
		mwhit.pc = M68K_GETPC;
		mwhit.addr = addr;
		mwhit.rwi = rwi;
		mwhit.size = size;
		mwhit.val = 0;
		if (mwhit.rwi & 2)
			mwhit.val = val;
		memwatch_triggered = i + 1;
		debugging = 1;
		set_special (&regs, SPCFLAG_BRK);
		return 1;
	}
	return 1;
}

static int mmu_hit (uaecptr addr, int size, int rwi, uae_u32 *v);

static uae_u32 REGPARAM2 mmu_lget (uaecptr addr)
{
	uae_u8 off = debug_mem_off (addr);
	uae_u32 v = 0;
	if (!mmu_hit (addr, 4, 0, &v))
		v = debug_mem_banks[off]->lget (addr);
	return v;
}
static uae_u32 REGPARAM2 mmu_wget (uaecptr addr)
{
	uae_u8 off = debug_mem_off (addr);
	uae_u32 v = 0;
	if (!mmu_hit (addr, 2, 0, &v))
		v = debug_mem_banks[off]->wget (addr);
	return v;
}
static uae_u32 REGPARAM2 mmu_bget (uaecptr addr)
{
	int off = debug_mem_off (addr);
	uae_u32 v = 0;
	if (!mmu_hit(addr, 1, 0, &v))
		v = debug_mem_banks[off]->bget (addr);
	return v;
}
static void REGPARAM2 mmu_lput (uaecptr addr, uae_u32 v)
{
	int off = debug_mem_off (addr);
	if (!mmu_hit (addr, 4, 1, &v))
		debug_mem_banks[off]->lput (addr, v);
}
static void REGPARAM2 mmu_wput (uaecptr addr, uae_u32 v)
{
	int off = debug_mem_off (addr);
	if (!mmu_hit (addr, 2, 1, &v))
		debug_mem_banks[off]->wput (addr, v);
}
static void REGPARAM2 mmu_bput (uaecptr addr, uae_u32 v)
{
	int off = debug_mem_off (addr);
	if (!mmu_hit (addr, 1, 1, &v))
		debug_mem_banks[off]->bput (addr, v);
}

static uae_u32 REGPARAM2 debug_lget (uaecptr addr)
{
	int off = debug_mem_off (addr);
	uae_u32 v;
	v = debug_mem_banks[off]->lget (addr);
	memwatch_func (addr, 1, 4, &v);
	return v;
}
static uae_u32 REGPARAM2 mmu_lgeti (uaecptr addr)
{
	int off = debug_mem_off (addr);
	uae_u32 v = 0;
	if (!mmu_hit (addr, 4, 4, &v))
		v = debug_mem_banks[off]->lgeti (addr);
	return v;
}
static uae_u32 REGPARAM2 mmu_wgeti (uaecptr addr)
{
	int off = debug_mem_off (addr);
	uae_u32 v = 0;
	if (!mmu_hit (addr, 2, 4, &v))
		v = debug_mem_banks[off]->wgeti (addr);
	return v;
}

static uae_u32 REGPARAM2 debug_wget (uaecptr addr)
{
	int off = debug_mem_off (addr);
	uae_u32 v;
	v = debug_mem_banks[off]->wget (addr);
	memwatch_func (addr, 1, 2, &v);
	return v;
}
static uae_u32 REGPARAM2 debug_bget (uaecptr addr)
{
	int off = debug_mem_off (addr);
	uae_u32 v;
	v = debug_mem_banks[off]->bget (addr);
	memwatch_func (addr, 1, 1, &v);
	return v;
}
static uae_u32 REGPARAM2 debug_lgeti (uaecptr addr)
{
	int off = debug_mem_off (addr);
	uae_u32 v;
	v = debug_mem_banks[off]->lgeti (addr);
	memwatch_func (addr, 4, 4, &v);
	return v;
}
static uae_u32 REGPARAM2 debug_wgeti (uaecptr addr)
{
	int off = debug_mem_off (addr);
	uae_u32 v;
	v = debug_mem_banks[off]->wgeti (addr);
	memwatch_func (addr, 4, 2, &v);
	return v;
}
static void REGPARAM2 debug_lput (uaecptr addr, uae_u32 v)
{
	int off = debug_mem_off (addr);
	if (memwatch_func (addr, 2, 4, &v))
		debug_mem_banks[off]->lput (addr, v);
}
static void REGPARAM2 debug_wput (uaecptr addr, uae_u32 v)
{
	int off = debug_mem_off (addr);
	if (memwatch_func (addr, 2, 2, &v))
		debug_mem_banks[off]->wput (addr, v);
}
static void REGPARAM2 debug_bput (uaecptr addr, uae_u32 v)
{
	int off = debug_mem_off (addr);
	if (memwatch_func (addr, 2, 1, &v))
		debug_mem_banks[off]->bput (addr, v);
}
static int REGPARAM2 debug_check (uaecptr addr, uae_u32 size)
{
	return debug_mem_banks[munge24 (addr) >> 16]->check (addr, size);
}
static uae_u8 *REGPARAM2 debug_xlate (uaecptr addr)
{
	return debug_mem_banks[munge24 (addr) >> 16]->xlateaddr (addr);
}

void debug_wputpeekdma (uaecptr addr, uae_u32 v)
{
	if (!memwatch_enabled)
		return;
	memwatch_func (addr, 2, 2, &v);
}
void debug_wgetpeekdma (uaecptr addr, uae_u32 v)
{
	uae_u32 vv = v;
	if (!memwatch_enabled)
		return;
	memwatch_func (addr, 1, 2, &vv);
}

void debug_putlpeek (uaecptr addr, uae_u32 v)
{
	if (!memwatch_enabled)
		return;
	memwatch_func (addr, 2, 4, &v);
}
void debug_wputpeek (uaecptr addr, uae_u32 v)
{
	if (!memwatch_enabled)
		return;
	memwatch_func (addr, 2, 2, &v);
}
void debug_bputpeek (uaecptr addr, uae_u32 v)
{
	if (!memwatch_enabled)
		return;
	memwatch_func (addr, 2, 1, &v);
}
void debug_bgetpeek (uaecptr addr, uae_u32 v)
{
	uae_u32 vv = v;
	if (!memwatch_enabled)
		return;
	memwatch_func (addr, 1, 1, &vv);
}
void debug_wgetpeek (uaecptr addr, uae_u32 v)
{
	uae_u32 vv = v;
	if (!memwatch_enabled)
		return;
	memwatch_func (addr, 1, 2, &vv);
}
void debug_lgetpeek (uaecptr addr, uae_u32 v)
{
	uae_u32 vv = v;
	if (!memwatch_enabled)
		return;
	memwatch_func (addr, 1, 4, &vv);
}

struct membank_store
{
	addrbank *addr;
	addrbank store;
};

static struct membank_store *membank_stores;

static void deinitialize_memwatch (void)
{
    unsigned int i;

    if (!memwatch_enabled)
	return;
    for (i = 0; i < 256; i++) {
	addrbank *a1 = debug_mem_banks[i];
	addrbank *a2 = mem_banks[i];
	memcpy (a2, a1, sizeof (addrbank));
	free (a1);
    }
    free (debug_mem_banks);
    debug_mem_banks = 0;
    memwatch_enabled = 0;
    free (illgdebug);
    illgdebug = 0;
}

static void initialize_memwatch (int mode)
{
    unsigned int i;
	int j, as;
	addrbank *a1, *a2, *oa;

	deinitialize_memwatch ();
	as = currprefs.address_space_24 ? 256 : 65536;
	debug_mem_banks = xmalloc (addrbank*, as);
	debug_mem_area = xmalloc (addrbank, as);
	membank_stores = xcalloc (struct membank_store, 32);
	oa = NULL;
	for (i = 0; i < as; i++) {
		a1 = debug_mem_banks[i] = debug_mem_area + i;
		a2 = mem_banks[i];
		if (a2 != oa) {
			for (j = 0; membank_stores[j].addr; j++) {
				if (membank_stores[j].addr == a2)
					break;
			}
			if (membank_stores[j].addr == NULL) {
				membank_stores[j].addr = a2;
				memcpy (&membank_stores[j].store, a2, sizeof (addrbank));
			}
		}
		memcpy (a1, a2, sizeof (addrbank));
	}
	for (i = 0; i < as; i++) {
		a2 = mem_banks[i];
		a2->bget = mode ? mmu_bget : debug_bget;
		a2->wget = mode ? mmu_wget : debug_wget;
		a2->lget = mode ? mmu_lget : debug_lget;
		a2->bput = mode ? mmu_bput : debug_bput;
		a2->wput = mode ? mmu_wput : debug_wput;
		a2->lput = mode ? mmu_lput : debug_lput;
		a2->check = debug_check;
		a2->xlateaddr = debug_xlate;
		a2->wgeti = mode ? mmu_wgeti : debug_wgeti;
		a2->lgeti = mode ? mmu_lgeti : debug_lgeti;
	}
	if (mode)
		mmu_enabled = 1;
	else
		memwatch_enabled = 1;
}

int debug_bankchange (int mode)
{
/*
	if (mode == -1) {
		int v = deinitialize_memwatch ();
		if (v < 0)
			return -2;
		return v;
	}
	if (mode >= 0) {
		initialize_memwatch (mode);
		memwatch_setup ();
	}
	return -1;
*/
}

static void memwatch_dump (int num)
{
    unsigned int i;
    const struct memwatch_node *mwn;
    for (i = 0; i < MEMWATCH_TOTAL; i++) {
	if ((num >= 0 && num == (int)i) || (num < 0)) {
	    mwn = &mwnodes[i];
	    if (mwn->size == 0)
		continue;
	    console_out ("%d: %8.8X - %8.8X (%d) %s",
		i, mwn->addr, mwn->addr + (mwn->size - 1), mwn->size,
		mwn->rwi == 0 ? "R" : (mwn->rwi == 1 ? "W" : "RW"));
	    if (mwn->val_enabled)
		console_out (" =%X", mwn->val);
	    if (mwn->modval_written)
		console_out (" =M");
	    console_out ("\n");
	}
    }
}

static void memwatch (const char **c)
{
    int num;
    struct memwatch_node *mwn;
    char nc;

    if (!memwatch_enabled) {
	initialize_memwatch (0);
	console_out ("Memwatch breakpoints enabled\n");
    }

    ignore_ws (c);
    if (!more_params (c)) {
	memwatch_dump (-1);
	return;
    }
    nc = next_char (c);
    if (nc == '-') {
	deinitialize_memwatch ();
	console_out ("Memwatch breakpoints disabled\n");
	return;
    }
    if (nc == 'd') {
	if (illgdebug) {
	    ignore_ws (c);
	    if (more_params (c)) {
		uae_u32 addr = readhex (c);
		uae_u32 len = 1;
		if (more_params (c))
		    len = readhex (c);
		write_log ("cleared logging addresses %8.8X - %8.8X\n", addr, addr + len);
		while (len > 0) {
		    addr &= 0xffffff;
		    illgdebug[addr] = 0;
		    addr++;
		    len--;
		}
	    }
	} else {
	    illg_init ();
	    console_out ("Illegal memory access logging enabled\n");
	    ignore_ws (c);
	    illgdebug_break = 0;
	    if (more_params (c))
		illgdebug_break = 1;
	}
	return;
    }
    num = nc - '0';
    if (num < 0 || num >= MEMWATCH_TOTAL)
	return;
    mwn = &mwnodes[num];
    mwn->size = 0;
    ignore_ws (c);
    if (!more_params (c)) {
	console_out ("Memwatch %d removed\n", num);
	return;
    }
    mwn->addr = readhex (c);
    mwn->size = 1;
	mwn->rwi = 7;
    mwn->val_enabled = 0;
	mwn->valmask = 0xffffffff;
	mwn->frozen = 0;
    mwn->modval_written = 0;
    ignore_ws (c);
    if (more_params (c)) {
	mwn->size = readhex (c);
	ignore_ws (c);
	if (more_params (c)) {
	    char nc = toupper (next_char (c));
	    if (nc == 'W')
		mwn->rwi = 1;
	    else if (nc == 'R' && toupper (**c) != 'W')
		mwn->rwi = 0;
	    else if (nc == 'R' && toupper (**c) == 'W')
		next_char (c);
	    ignore_ws (c);
	    if (more_params (c)) {
				if (toupper (**c) == 'M') {
				    mwn->modval_written = 1;
				} else if (toupper (**c) == 'C') {
					mwn->mustchange = 1;
		} else {
		    mwn->val = readhex (c);
		    mwn->val_enabled = 1;
		}
	    }
	}
    }
	if (mwn->frozen && mwn->rwi == 0)
		mwn->rwi = 3;
    memwatch_dump (num);
}

static void writeintomem (const char **c)
{
    uae_u32 addr = 0;
    uae_u32 val = 0;
    char cc;

    ignore_ws (c);
    addr = readhex (c);
    ignore_ws (c);
    val = readhex (c);
    if (val > 0xffff) {
	put_long (addr, val);
	cc = 'L';
    } else if (val > 0xff) {
	put_word (addr, val);
	cc = 'W';
    } else {
	put_byte (addr, val);
	cc = 'B';
    }
	console_out ("Wrote %X (%u) at %08X.%c\n", val, val, addr, cc);
}

static uae_u8 *dump_xlate (uae_u32 addr)
{
	if (!mem_banks[addr >> 16]->check (addr, 1))
		return NULL;
	return mem_banks[addr >> 16]->xlateaddr (addr);
}

static void memory_map_dump_2 (int log)
{
	int i, j, max, im;
	addrbank *a1 = mem_banks[0];
	char txt[256];

	im = currprefs.illegal_mem;
	currprefs.illegal_mem = 0;
	max = currprefs.address_space_24 ? 256 : 65536;
	j = 0;
	for (i = 0; i < max + 1; i++) {
		addrbank *a2 = NULL;
		if (i < max)
			a2 = mem_banks[i];
		if (a1 != a2) {
			int k, mirrored, size, size_out;
			char size_ext;
			uae_u8 *caddr;
			char *name;
			char tmp[MAX_DPATH];

			name = a1->name;
			if (name == NULL)
				name = "<none>";

			k = j;
			caddr = dump_xlate (k << 16);
			mirrored = caddr ? 1 : 0;
			k++;
			while (k < i && caddr) {
				if (dump_xlate (k << 16) == caddr)
					mirrored++;
				k++;
			}
			size = (i - j) << (16 - 10);
			size_out = size;
			size_ext = 'K';
			if (j >= 256) {
				size_out /= 1024;
				size_ext = 'M';
			}
			_stprintf (txt, "%08X %7d%c/%d = %7d%c %s", j << 16, size_out, size_ext,
				mirrored, mirrored ? size_out / mirrored : size_out, size_ext, name);

			tmp[0] = 0;
			if (a1->flags == ABFLAG_ROM && mirrored) {
				char *p = txt + _tcslen (txt);
				uae_u32 crc = get_crc32 (a1->xlateaddr(j << 16), (size * 1024) / mirrored);
				struct romdata *rd = getromdatabycrc (crc);
				_stprintf (p, " (%08X)", crc);
				if (rd) {
					tmp[0] = '=';
					getromname (rd, tmp + 1);
					_tcscat (tmp, "\n");
				}
			}
			_tcscat (txt, "\n");
			if (log)
				write_log (txt);
			else
				console_out (txt);
			if (tmp[0]) {
				if (log)
					write_log (tmp);
				else
					console_out (tmp);
			}
			j = i;
			a1 = a2;
		}
	}
	currprefs.illegal_mem = im;
}
void memory_map_dump (void)
{
	memory_map_dump_2 (1);
}

STATIC_INLINE uaecptr BPTR2APTR (uaecptr addr)
{
	return addr << 2;
}

static void print_task_info (uaecptr node)
{
	char *s;
	int process = get_byte (node + 8) == 13 ? 1 : 0;

	console_out ("%08X: ", node);
	console_out (process ? " PROCESS '%s'" : " TASK    '%s'\n", ((char*)get_real_address (get_long (node + 10))));
	if (process) {
		uaecptr cli = BPTR2APTR (get_long (node + 172));
		int tasknum = get_long (node + 140);
		if (cli && tasknum) {
			uae_u8 *command_bstr = get_real_address (BPTR2APTR (get_long (cli + 16)));
			console_out (" [%d, '%s']\n", tasknum, command_bstr);
			xfree (command_bstr);
		} else {
			console_out ("\n");
		}
	}
}

static void show_exec_tasks (void)
{
	uaecptr execbase = get_long (4);
	uaecptr taskready = get_long (execbase + 406);
	uaecptr taskwait = get_long (execbase + 420);
	uaecptr node, end;
	console_out ("Execbase at 0x%08X\n", execbase);
	console_out ("Current:\n");
	node = get_long (execbase + 276);
	print_task_info (node);
	console_out ("Ready:\n");
	node = get_long (taskready);
	end = get_long (taskready + 4);
	while (node) {
		print_task_info (node);
		node = get_long (node);
	}
	console_out ("Waiting:\n");
	node = get_long (taskwait);
	end = get_long (taskwait + 4);
	while (node) {
		print_task_info (node);
		node = get_long (node);
	}
}

static void show_exec_lists (char t)
{
	uaecptr execbase = get_long (4);
	uaecptr list = 0, node;

	switch (toupper (t))
	{
	case 'R':
		list = execbase + 336;
		break;
	case 'D':
		list = execbase + 350;
		break;
	case 'L':
		list = execbase + 378;
		break;
	}
	if (list == 0)
		return;
	node = get_long (list);
	while (get_long (node)) {
		console_out ("%08x %s\n", node, ((char*)get_real_address (get_long (node + 10))));
		node = get_long (node);
	}
}

#if 0
static int trace_same_insn_count;
static uae_u8 trace_insn_copy[10];
static struct regstruct trace_prev_regs;
#endif
static uaecptr nextpc;

int instruction_breakpoint (const char **c)
{
	struct breakpoint_node *bpn;
	int i;

	if (more_params (c)) {
		char nc = toupper ((*c)[0]);
		if (nc == 'S') {
			next_char (c);
			sr_bpvalue = sr_bpmask = 0;
			if (more_params (c)) {
				sr_bpmask = 0xffff;
				sr_bpvalue = readhex (c);
				if (more_params (c))
					sr_bpmask = readhex (c);
			}
			console_out ("SR breakpoint, value=%04X, mask=%04X\n", sr_bpvalue, sr_bpmask);
			return 0;
		} else if (nc == 'I') {
			next_char (c);
			if (more_params (c))
				skipins = readhex (c);
			else
				skipins = 0x10000;
			do_skip = 1;
			skipaddr_doskip = 1;
			return 1;
		} else if (nc == 'D' && (*c)[1] == 0) {
			for (i = 0; i < BREAKPOINT_TOTAL; i++)
				bpnodes[i].enabled = 0;
			console_out ("All breakpoints removed\n");
			return 0;
		} else if (nc == 'L') {
			int got = 0;
			for (i = 0; i < BREAKPOINT_TOTAL; i++) {
				bpn = &bpnodes[i];
				if (!bpn->enabled)
					continue;
				console_out ("%8X ", bpn->addr);
				got = 1;
			}
			if (!got)
				console_out ("No breakpoints\n");
			else
				console_out ("\n");
			return 0;
		}
		skipaddr_doskip = 1;
		skipaddr_start = readhex (c);
		if (more_params (c)) {
			skipaddr_end = readhex (c);
		} else {
			for (i = 0; i < BREAKPOINT_TOTAL; i++) {
				bpn = &bpnodes[i];
				if (bpn->enabled && bpn->addr == skipaddr_start) {
					bpn->enabled = 0;
					console_out ("Breakpoint removed\n");
					skipaddr_start = 0xffffffff;
					skipaddr_doskip = 0;
					return 0;
				}
			}
			for (i = 0; i < BREAKPOINT_TOTAL; i++) {
				bpn = &bpnodes[i];
				if (bpn->enabled)
					continue;
				bpn->addr = skipaddr_start;
				bpn->enabled = 1;
				console_out ("Breakpoint added\n");
				skipaddr_start = 0xffffffff;
				skipaddr_doskip = 0;
				break;
			}
			return 0;
		}
	}
#if 0
	if (skipaddr_start == 0xC0DEDBAD) {
		trace_same_insn_count = 0;
		logfile = fopen ("uae.trace", "w");
		memcpy (trace_insn_copy, regs.pc_p, 10);
		memcpy (&trace_prev_regs, &regs, sizeof regs);
	}
#endif
	do_skip = 1;
	skipaddr_doskip = -1;
	return 1;
}

static void savemem (const char **cc)
{
    uae_u8 b;
    uae_u32 src, src2, len, len2;
    const char *start;
    char *name = 0;
    int name_len;

    FILE *fp;

    if (!more_params (cc))
	goto S_argh;

    start = *cc;
    while (**cc != '\0' && !isspace (**cc))
		(*cc)++;
    if (!isspace (**cc))
		goto S_argh;

    name_len = *cc - start;
    name = malloc (name_len + 1);
    if (!name)
	goto S_argh;
    strncpy (name, start, name_len);
    name[name_len] = '\0';

	//**cc = '\0';
    (*cc)++;
    if (!more_params (cc))
		goto S_argh;
    src2 = src = readhex (cc);
    if (!more_params (cc))
		goto S_argh;
    len2 = len = readhex (cc);
	fp = fopen (name, "wb");
    if (fp == NULL) {
		console_out ("Couldn't open file '%s'\n", name);
		return;
    }
    while (len > 0) {
		b = get_byte (src);
		src++;
		len--;
		if (fwrite (&b, 1, 1, fp) != 1) {
			console_out ("Error writing file\n");
		    break;
		}
    }
    fclose (fp);
    if (len == 0)
		console_out ("Wrote %08X - %08X (%d bytes) to '%s'\n",
	    src2, src2 + len2 - 1, len2, name);
    free (name);
    return;
S_argh:
    if (name)
	free (name);
    console_out ("S-command needs more arguments!\n");
}

static void searchmem (const char **cc)
{
    int i, sslen, got, val, stringmode;
    uae_u8 ss[256];
    uae_u32 addr, endaddr;
    char nc;

	got = 0;
	sslen = 0;
	stringmode = 0;
	ignore_ws (cc);
	if (**cc == '"') {
		stringmode = 1;
		(*cc)++;
		while (**cc != '"' && **cc != 0) {
			ss[sslen++] = tolower (**cc);
			(*cc)++;
		}
		if (**cc != 0)
			(*cc)++;
	} else {
		for (;;) {
			if (**cc == 32 || **cc == 0)
				break;
	    nc = toupper (next_char (cc));
			if (isspace (nc))
				break;
			if (isdigit(nc))
				val = nc - '0';
			else
				val = nc - 'A' + 10;
			if (val < 0 || val > 15)
				return;
			val *= 16;
			if (**cc == 32 || **cc == 0)
				break;
	    nc = toupper (next_char (cc));
			if (isspace (nc))
				break;
			if (isdigit(nc))
				val += nc - '0';
			else
				val += nc - 'A' + 10;
			if (val < 0 || val > 255)
				return;
			ss[sslen++] = (uae_u8)val;
		}
	}
	if (sslen == 0)
		return;
	ignore_ws (cc);
	addr = 0;
	endaddr = lastaddr ();
	if (more_params (cc)) {
		addr = readhex (cc);
		if (more_params (cc))
			endaddr = readhex (cc);
	}
    console_out ("Searching from %08x to %08x..\n", addr, endaddr);
	while ((addr = nextaddr (addr, NULL)) != 0xffffffff) {
		if (addr == endaddr)
			break;
		for (i = 0; i < sslen; i++) {
	    uae_u8 b = get_byte (addr + i);
	    if (stringmode) {
		if (tolower (b) != ss[i])
		    break;
	    } else {
		if (b != ss[i])
		    break;
	    }
	}
	if (i == sslen) {
	    got++;
	    console_out (" %08x", addr);
	    if (got > 100) {
		console_out ("\nMore than 100 results, aborting..");
		break;
	    }
	}
    }
    if (!got)
		console_out ("nothing found");
	console_out ("\n");
}

#ifdef SAVESTATE
static int staterecorder (const char **cc)
{
    char nc;

	if (!more_params (cc)) {
		if (savestate_dorewind (1)) {
			debug_rewind = 1;
			return 1;
		}
		return 0;
	}
	nc = next_char (cc);
	if (nc == 'l') {
		savestate_listrewind ();
		return 0;
	}
	return 0;
}
#endif

static int debugtest_modes[DEBUGTEST_MAX];
static const TCHAR *debugtest_names[] = {
	"Blitter", "Keyboard", "Floppy"
};

static void disk_debug (const char **inptr)
{
    char parm[10];
    unsigned int i;

	if (**inptr == 'd') {
		(*inptr)++;
		ignore_ws (inptr);
		disk_debug_logging = readint (inptr);
		console_out ("Disk logging level %d\n", disk_debug_logging);
		return;
	}
	disk_debug_mode = 0;
	disk_debug_track = -1;
	ignore_ws (inptr);
	if (!next_string (inptr, parm, sizeof (parm) / sizeof (TCHAR), 1))
		goto end;
	for (i = 0; i < strlen (parm); i++) {
		if (parm[i] == 'R')
			disk_debug_mode |= DISK_DEBUG_DMA_READ;
		if (parm[i] == 'W')
			disk_debug_mode |= DISK_DEBUG_DMA_WRITE;
		if (parm[i] == 'P')
			disk_debug_mode |= DISK_DEBUG_PIO;
	}
	if (more_params(inptr))
		disk_debug_track = readint(inptr);
	if (disk_debug_track < 0 || disk_debug_track > 2 * 83)
		disk_debug_track = -1;
	if (disk_debug_logging == 0)
		disk_debug_logging = 1;
end:
	console_out ("Disk breakpoint mode %c%c%c track %d\n",
		disk_debug_mode & DISK_DEBUG_DMA_READ ? 'R' : '-',
		disk_debug_mode & DISK_DEBUG_DMA_WRITE ? 'W' : '-',
		disk_debug_mode & DISK_DEBUG_PIO ? 'P' : '-',
		disk_debug_track);
}

static void find_ea (const char **inptr)
{
	uae_u32 ea, sea, dea;
	uaecptr addr, end;
	int hits = 0;

	addr = 0;
	end = lastaddr();
	ea = readhex (inptr);
	if (more_params(inptr)) {
		addr = readhex (inptr);
		if (more_params(inptr))
			end = readhex (inptr);
	}
	console_out ("Searching from %08X to %08X\n", addr, end);
	while((addr = nextaddr(addr, &end)) != 0xffffffff) {
		if ((addr & 1) == 0 && addr + 6 <= end) {
			sea = 0xffffffff;
			dea = 0xffffffff;
			m68k_disasm_ea (NULL, addr, NULL, 1, &sea, &dea);
			if (ea == sea || ea == dea) {
				m68k_disasm (stdout, addr, NULL, 1);
				hits++;
				if (hits > 100) {
					write_log ("Too many hits. End addr = %08X\n", addr);
					break;
				}
			}
		}
	}
}

static void m68k_modify (const char **inptr)
{
    uae_u32 v;
    char parm[10];
    unsigned char c1, c2;
	int i;

	if (!next_string (inptr, parm, sizeof (parm) / sizeof (TCHAR), 1))
		return;
    c1 = toupper (parm[0]);
    c2 = toupper (parm[1]);
    if (c1 == 'A' || c1 == 'D' || c1 == 'P') {
	if (!isdigit (c2))
	    return;
	c2 -= '0';
    }
    v = readhex (inptr);
    if (c1 == 'A')
	regs.regs[8 + c2] = v;
    else if (c1 == 'D')
	regs.regs[c2] = v;
    else if (c1 == 'P' && c2 == 0)
	regs.irc = v;
    else if (c1 == 'P' && c2 == 1)
	regs.ir = v;
    else if (!strcmp (parm, "VBR"))
	regs.vbr = v;
    else if (!strcmp (parm, "USP")) {
	regs.usp = v;
	MakeFromSR (&regs);
    } else if (!strcmp (parm, "ISP")) {
	regs.isp = v;
	MakeFromSR (&regs);
    } else if (!strcmp (parm, "MSP")) {
	regs.msp = v;
	MakeFromSR (&regs);
    } else if (!strcmp (parm, "SR")) {
	regs.sr = v;
	MakeFromSR (&regs);
    } else if (!strcmp (parm, "CCR")) {
	regs.sr = (regs.sr & ~15) | (v & 15);
	MakeFromSR (&regs);
    }
}

static void debug_1 (void)
{
	char input[MAX_LINEWIDTH];
    uaecptr nxdis, nxmem, addr;

    m68k_dumpstate (stdout, &nextpc);
    nxdis = nextpc; nxmem = 0;
	debugger_active = 1;

    for (;;) {
		char cmd;
		const char *inptr;
		int v;

		if (!debugger_active)
			return;
		//update_debug_info ();
		console_out (">");
		console_flush ();
		debug_linecounter = 0;
		v = console_get (input, MAX_LINEWIDTH);
		if (v < 0)
			return;
		if (v == 0)
			continue;

		inptr = input;
		cmd = next_char (&inptr);
		switch (cmd)
		{
		case 'c': dumpcia (); dumpdisk (); dumpcustom (); break;
		case 'i':
		{
			if (*inptr == 'l') {
				next_char (&inptr);
				if (more_params (&inptr)) {
					debug_illegal_mask = readhex (&inptr);
				} else {
					debug_illegal_mask = debug_illegal ? 0 : -1;
					debug_illegal_mask &= ~((uae_u64)255 << 24); // mask interrupts
				}
				write_log ("Exception breakpoint mask: %0I64X\n", debug_illegal_mask);
				debug_illegal = debug_illegal_mask ? 1 : 0;
			} else {
				addr = 0xffffffff;
				if (more_params (&inptr))
					addr = readhex (&inptr);
				dump_vectors (addr);
			}
			break;
		}
	case 'e': dump_custom_regs (); break;
	case 'r': if (more_params (&inptr))
		      m68k_modify (&inptr);
		  else
		      m68k_dumpstate (stdout, &nextpc);
	break;
	case 'C': cheatsearch (&inptr); break;
	case 'W': writeintomem (&inptr); break;
	case 'w': memwatch (&inptr); break;
	case 'S': savemem (&inptr); break;
	case 's':
	    if (*inptr == 'c') {
//		screenshot (1, 1);
	    } else if (*inptr == 'm') {
		next_char (&inptr);
		if (more_params (&inptr))
		    debug_sprite_mask = readint (&inptr);
		console_out ("sprite mask: %2.2X\n", debug_sprite_mask);
	    } else {
		searchmem (&inptr);
	    }
	break;
	case 'd':
	{
	    if (*inptr == 'i') {
		next_char (&inptr);
		disk_debug (&inptr);
	    } else {
		uae_u32 daddr;
		int count;
		if (more_params (&inptr))
		    daddr = readhex (&inptr);
		else
		    daddr = nxdis;
		if (more_params (&inptr))
		    count = readhex (&inptr);
		else
		    count = 10;
		m68k_disasm (stdout, daddr, &nxdis, count);
	    }
	}
	break;
	case 'T': show_exec_tasks (); break;
	case 't':
	    if (more_params (&inptr))
		skipaddr_doskip = readint (&inptr);
	    if (skipaddr_doskip <= 0 || skipaddr_doskip > 10000)
		skipaddr_doskip = 1;
	    set_special (&regs, SPCFLAG_BRK);
	    exception_debugging = 1;
	    return;
	case 'z':
	    skipaddr_start = nextpc;
	    skipaddr_doskip = 1;
	    do_skip = 1;
	    exception_debugging = 1;
	    return;

	case 'f':
	    if (instruction_breakpoint (&inptr))
		return;
	    break;

	case 'q': uae_quit();
	    debugger_active = 0;
	    debugging = 0;
	    return;

	case 'g':
	    if (more_params (&inptr)) {
		m68k_setpc (&regs, readhex (&inptr));
		fill_prefetch_slow (&regs);
	    }
	    debugger_active = 0;
	    debugging = 0;
	    exception_debugging = 0;
	    return;

	case 'H':
	{
	    unsigned int count, temp, badly;
	    uae_u32 oldpc = m68k_getpc (&regs);
	    struct regstruct save_regs = regs;

	    badly = 0;
	    if (inptr[0] == 'H') {
		badly = 1;
		inptr++;
	    }

	    if (more_params (&inptr))
		count = readhex (&inptr);
	    else
		count = 10;
	    temp = lasthist;
	    while (count-- > 0 && temp != firsthist) {
		if (temp == 0)
		    temp = MAX_HIST - 1;
		else
		    temp--;
	    }
	    while (temp != lasthist) {
		regs = history[temp];
		m68k_setpc (&regs, history[temp].pc);
		if (badly) {
		    m68k_dumpstate (stdout, NULL);
		} else {
		    m68k_disasm (stdout, history[temp].pc, NULL, 1);
		}
		if (++temp == MAX_HIST)
		    temp = 0;
	    }
	    regs = save_regs;
	    m68k_setpc (&regs, oldpc);
	}
	break;
	case 'm':
	{
	    uae_u32 maddr; int lines;
	    if (more_params (&inptr))
		maddr = readhex (&inptr);
	    else
		maddr = nxmem;
	    if (more_params (&inptr))
		lines = readhex (&inptr);
	    else
		lines = 20;
	    dumpmem (maddr, &nxmem, lines);
	}
	break;
	case 'o':
	{
	    if (copper_debugger(&inptr)) {
		debugger_active = 0;
		debugging = 0;
		return;
	    }
	    break;
	}
	case 'O':
	    break;
#ifdef SAVESTATE
	case 'b':
	    if (staterecorder (&inptr))
		return;
	    break;
#endif
	case 'a':
	    if (more_params (&inptr)) {
		char nc = next_char (&inptr);
		if (nc == 'm')
		    audio_channel_mask = readint (&inptr);
	    }
	    break;
	case 'h':
	case '?':
	    debug_help ();
	break;
	}
    }
}

static void addhistory (void)
{
    history[lasthist] = regs;
    history[lasthist].pc = m68k_getpc (&regs);
    if (++lasthist == MAX_HIST)
	lasthist = 0;
    if (lasthist == firsthist) {
	if (++firsthist == MAX_HIST) firsthist = 0;
    }
}

void debug (void)
{
    int i;

#ifdef SAVESTATE
    if (savestate_state)
	return;
#endif

    bogusframe = 1;
    addhistory ();

#if 0
    if (do_skip && skipaddr_start == 0xC0DEDBAD) {
	if (trace_same_insn_count > 0) {
	    if (memcmp (trace_insn_copy, regs.pc_p, 10) == 0
		&& memcmp (trace_prev_regs.regs, regs.regs, sizeof regs.regs) == 0)
	    {
		trace_same_insn_count++;
		return;
	    }
	}
	if (trace_same_insn_count > 1)
	    fprintf (logfile, "[ repeated %d times ]\n", trace_same_insn_count);
	m68k_dumpstate (logfile, &nextpc);
	trace_same_insn_count = 1;
	memcpy (trace_insn_copy, regs.pc_p, 10);
	memcpy (&trace_prev_regs, &regs, sizeof regs);
    }
#endif

    if (!memwatch_triggered) {
	if (do_skip) {
	    uae_u32 pc = munge24 (m68k_getpc (&regs));
	    uae_u16 opcode = (currprefs.cpu_compatible || currprefs.cpu_cycle_exact) ? regs.ir : get_word (pc);
	    int bp = 0;

	    for (i = 0; i < BREAKPOINT_TOTAL; i++) {
		if (!bpnodes[i].enabled)
		    continue;
		if (bpnodes[i].addr == pc) {
		    bp = 1;
		    console_out ("Breakpoint at %8.8X\n", pc);
		    break;
		}
	    }
	    if (skipaddr_doskip) {
		if (skipaddr_start == pc)
		    bp = 1;
		if (skipins != 0xffffffff) {
		    if (skipins == 0x10000) {
			if (opcode == 0x4e75 || opcode == 0x4e73 || opcode == 0x4e77)
			    bp = 1;
		    } else if (opcode == skipins)
			bp = 1;
		} else if (skipaddr_start == 0xffffffff && skipaddr_doskip < 0) {
		    if ((pc < 0xe00000 || pc >= 0x1000000) && opcode != 0x4ef9)
			bp = 1;
		} else if (skipaddr_start == 0xffffffff && skipaddr_doskip > 0) {
			bp = 1;
		} else if (skipaddr_end != 0xffffffff) {
		    if (pc >= skipaddr_start && pc < skipaddr_end)
			bp = 1;
		}
	    }
	    if (!bp) {
		set_special (&regs, SPCFLAG_BRK);
		return;
	    }
	}
    } else {
	write_log ("Memwatch %d: break at %8.8X.%c %c %8.8X\n", memwatch_triggered - 1, mwhit.addr,
	    mwhit.size == 1 ? 'B' : (mwhit.size == 2 ? 'W' : 'L'), mwhit.rwi ? 'W' : 'R', mwhit.val);
	memwatch_triggered = 0;
    }
    if (skipaddr_doskip > 0) {
	skipaddr_doskip--;
	if (skipaddr_doskip > 0) {
	    set_special (&regs, SPCFLAG_BRK);
	    return;
	}
    }

    inputdevice_unacquire ();
    audio_pause ();
    do_skip = 0;
    skipaddr_start = 0xffffffff;
    skipaddr_end = 0xffffffff;
    skipins = 0xffffffff;
    skipaddr_doskip = 0;
    exception_debugging = 0;

#ifdef SAVESTATE
    debug_rewind = 0;
	processptr = 0;
#if 0
    if (!currprefs.statecapture) {
	changed_prefs.statecapture = currprefs.statecapture = 1;
	savestate_init ();
    }
#endif
#endif

    debug_1 ();

#ifdef SAVESTATE
    if (!debug_rewind
#ifdef JIT
	&& !currprefs.cachesize
#endif
#ifdef FILESYS
	&& nr_units (currprefs.mountinfo) == 0
#endif
	) {
	savestate_capture (1);
    }
#endif

	for (i = 0; i < BREAKPOINT_TOTAL; i++) {
		if (bpnodes[i].enabled)
			do_skip = 1;
	}
	if (sr_bpmask || sr_bpvalue)
		do_skip = 1;
	if (do_skip) {
		set_special (&regs, SPCFLAG_BRK);
		m68k_resumestopped ();
		debugging = 1;
	}
	audio_resume ();
	inputdevice_acquire (1);
}

const TCHAR *debuginfo (int mode)
{
	static TCHAR txt[100];
	uae_u32 pc = M68K_GETPC;
	_stprintf (txt, "PC=%08X INS=%04X %04X %04X",
		pc, get_word (pc), get_word (pc + 2), get_word (pc + 4));
	return txt;
}

static int mmu_logging;

#define MMU_PAGE_SHIFT 16

struct mmudata {
	uae_u32 flags;
	uae_u32 addr;
	uae_u32 len;
	uae_u32 remap;
	uae_u32 p_addr;
};

static struct mmudata *mmubanks;
static uae_u32 mmu_struct, mmu_callback, mmu_regs;
static uae_u32 mmu_fault_bank_addr, mmu_fault_addr;
static int mmu_fault_size, mmu_fault_rw;
static int mmu_slots;
static struct regstruct mmur;

struct mmunode {
	struct mmudata *mmubank;
	struct mmunode *next;
};
static struct mmunode **mmunl;
//extern regstruct mmu_backup_regs;

#define MMU_READ_U (1 << 0)
#define MMU_WRITE_U (1 << 1)
#define MMU_READ_S (1 << 2)
#define MMU_WRITE_S (1 << 3)
#define MMU_READI_U (1 << 4)
#define MMU_READI_S (1 << 5)

#define MMU_MAP_READ_U (1 << 8)
#define MMU_MAP_WRITE_U (1 << 9)
#define MMU_MAP_READ_S (1 << 10)
#define MMU_MAP_WRITE_S (1 << 11)
#define MMU_MAP_READI_U (1 << 12)
#define MMU_MAP_READI_S (1 << 13)

void mmu_do_hit (void)
{
	int i;
	uaecptr p;
	uae_u32 pc;

	mmu_triggered = 0;
	pc = m68k_getpc (&regs);
	p = mmu_regs + 18 * 4;
	put_long (p, pc);
	//regs = mmu_backup_regs;
	regs.intmask = 7;
	regs.t0 = regs.t1 = 0;
	if (!regs.s) {
		regs.usp = m68k_areg (&regs, 7);
		if (currprefs.cpu_model >= 68020)
			m68k_areg (&regs, 7) = regs.m ? regs.msp : regs.isp;
		else
			m68k_areg (&regs, 7) = regs.isp;
		regs.s = 1;
	}
	MakeSR (&regs);
	m68k_setpc (&regs, mmu_callback);
	fill_prefetch_slow (&regs);

	if (currprefs.cpu_model > 68000) {
		for (i = 0 ; i < 9; i++) {
			m68k_areg (&regs, 7) -= 4;
			put_long (m68k_areg (&regs, 7), 0);
		}
		m68k_areg (&regs, 7) -= 4;
		put_long (m68k_areg (&regs, 7), mmu_fault_addr);
		m68k_areg (&regs, 7) -= 2;
		put_word (m68k_areg (&regs, 7), 0); /* WB1S */
		m68k_areg (&regs, 7) -= 2;
		put_word (m68k_areg (&regs, 7), 0); /* WB2S */
		m68k_areg (&regs, 7) -= 2;
		put_word (m68k_areg (&regs, 7), 0); /* WB3S */
		m68k_areg (&regs, 7) -= 2;
		put_word (m68k_areg (&regs, 7),
			(mmu_fault_rw ? 0 : 0x100) | (mmu_fault_size << 5)); /* SSW */
		m68k_areg (&regs, 7) -= 4;
		put_long (m68k_areg (&regs, 7), mmu_fault_bank_addr);
		m68k_areg (&regs, 7) -= 2;
		put_word (m68k_areg (&regs, 7), 0x7002);
	}
	m68k_areg (&regs, 7) -= 4;
	put_long (m68k_areg (&regs, 7), get_long (p - 4));
	m68k_areg (&regs, 7) -= 2;
	put_word (m68k_areg (&regs, 7), mmur.sr);
#ifdef JIT
	set_special(&regs, SPCFLAG_END_COMPILE);
#endif
}

static void mmu_do_hit_pre (struct mmudata *md, uaecptr addr, int size, int rwi, uae_u32 v)
{
	uae_u32 p, pc;
	int i;

	mmur = regs;
	pc = m68k_getpc (&regs);
	if (mmu_logging)
		console_out ("MMU: hit %08X SZ=%d RW=%d V=%08X PC=%08X\n", addr, size, rwi, v, pc);

	p = mmu_regs;
	put_long (p, 0); p += 4;
	for (i = 0; i < 16; i++) {
		put_long (p, regs.regs[i]);
		p += 4;
	}
	put_long (p, pc); p += 4;
	put_long (p, 0); p += 4;
	put_long (p, regs.usp); p += 4;
	put_long (p, regs.isp); p += 4;
	put_long (p, regs.msp); p += 4;
	put_word (p, regs.sr); p += 2;
	put_word (p, (size << 1) | ((rwi & 2) ? 1 : 0)); /* size and rw */ p += 2;
	put_long (p, addr); /* fault address */ p += 4;
	put_long (p, md->p_addr); /* bank address */ p += 4;
	put_long (p, v); p += 4;
	mmu_fault_addr = addr;
	mmu_fault_bank_addr = md->p_addr;
	mmu_fault_size = size;
	mmu_fault_rw = rwi;
	mmu_triggered = 1;
}

static int mmu_hit (uaecptr addr, int size, int rwi, uae_u32 *v)
{
	int s, trig;
	uae_u32 flags;
	struct mmudata *md;
	struct mmunode *mn;

	if (mmu_triggered)
		return 1;

	mn = mmunl[addr >> MMU_PAGE_SHIFT];
	if (mn == NULL)
		return 0;

	s = regs.s;
	while (mn) {
		md = mn->mmubank;
		if (addr >= md->addr && addr < md->addr + md->len) {
			flags = md->flags;
			if (flags & (MMU_MAP_READ_U | MMU_MAP_WRITE_U | MMU_MAP_READ_S | MMU_MAP_WRITE_S | MMU_MAP_READI_U | MMU_MAP_READI_S)) {
				trig = 0;
				if (!s && (flags & MMU_MAP_READ_U) && (rwi & 1))
					trig = 1;
				if (!s && (flags & MMU_MAP_WRITE_U) && (rwi & 2))
					trig = 1;
				if (s && (flags & MMU_MAP_READ_S) && (rwi & 1))
					trig = 1;
				if (s && (flags & MMU_MAP_WRITE_S) && (rwi & 2))
					trig = 1;
				if (!s && (flags & MMU_MAP_READI_U) && (rwi & 4))
					trig = 1;
				if (s && (flags & MMU_MAP_READI_S) && (rwi & 4))
					trig = 1;
				if (trig) {
					uaecptr maddr = md->remap + (addr - md->addr);
					if (maddr == addr) /* infinite mmu hit loop? no thanks.. */
						return 1;
					if (mmu_logging)
						console_out ("MMU: remap %08X -> %08X SZ=%d RW=%d\n", addr, maddr, size, rwi);
					if ((rwi & 2)) {
						switch (size)
						{
						case 4:
							put_long (maddr, *v);
							break;
						case 2:
							put_word (maddr, *v);
							break;
						case 1:
							put_byte (maddr, *v);
							break;
						}
					} else {
						switch (size)
						{
						case 4:
							*v = get_long (maddr);
							break;
						case 2:
							*v = get_word (maddr);
							break;
						case 1:
							*v = get_byte (maddr);
							break;
						}
					}
					return 1;
				}
			}
			if (flags & (MMU_READ_U | MMU_WRITE_U | MMU_READ_S | MMU_WRITE_S | MMU_READI_U | MMU_READI_S)) {
				trig = 0;
				if (!s && (flags & MMU_READ_U) && (rwi & 1))
					trig = 1;
				if (!s && (flags & MMU_WRITE_U) && (rwi & 2))
					trig = 1;
				if (s && (flags & MMU_READ_S) && (rwi & 1))
					trig = 1;
				if (s && (flags & MMU_WRITE_S) && (rwi & 2))
					trig = 1;
				if (!s && (flags & MMU_READI_U) && (rwi & 4))
					trig = 1;
				if (s && (flags & MMU_READI_S) && (rwi & 4))
					trig = 1;
				if (trig) {
					mmu_do_hit_pre (md, addr, size, rwi, *v);
					return 1;
				}
			}
		}
		mn = mn->next;
	}
	return 0;
}

static void mmu_free_node(struct mmunode *mn)
{
	if (!mn)
		return;
	mmu_free_node (mn->next);
	xfree (mn);
}

static void mmu_free(void)
{
	struct mmunode *mn;
	int i;

	for (i = 0; i < mmu_slots; i++) {
		mn = mmunl[i];
		mmu_free_node (mn);
	}
	xfree (mmunl);
	mmunl = NULL;
	xfree (mmubanks);
	mmubanks = NULL;
}

static int getmmubank(struct mmudata *snptr, uaecptr p)
{
	snptr->flags = get_long (p);
	if (snptr->flags == 0xffffffff)
		return 1;
	snptr->addr = get_long (p + 4);
	snptr->len = get_long (p + 8);
	snptr->remap = get_long (p + 12);
	snptr->p_addr = p;
	return 0;
}

int mmu_init(int mode, uaecptr parm, uaecptr parm2)
{
	uaecptr p, p2, banks;
	int size;
	struct mmudata *snptr;
	struct mmunode *mn;
	static int wasjit;
#ifdef JIT
	if (currprefs.cachesize) {
		wasjit = currprefs.cachesize;
		changed_prefs.cachesize = 0;
		console_out ("MMU: JIT disabled\n");
		check_prefs_changed_comp ();
	}
	if (mode == 0) {
		if (mmu_enabled) {
			mmu_free ();
			deinitialize_memwatch ();
			console_out ("MMU: disabled\n");
			changed_prefs.cachesize = wasjit;
		}
		mmu_logging = 0;
		return 1;
	}
#endif

	if (mode == 1) {
		if (!mmu_enabled)
			return 0xffffffff;
		return mmu_struct;
	}

	p = parm;
	mmu_struct = p;
	if (get_long (p) != 1) {
		console_out ("MMU: version mismatch %d <> %d\n", get_long (p), 1);
		return 0;
	}
	p += 4;
	mmu_logging = get_long (p) & 1;
	p += 4;
	mmu_callback = get_long (p);
	p += 4;
	mmu_regs = get_long (p);
	p += 4;

	if (mode == 3) {
		int off;
		uaecptr addr = get_long (parm2 + 4);
		if (!mmu_enabled)
			return 0;
		off = addr >> MMU_PAGE_SHIFT;
		mn = mmunl[off];
		while (mn) {
			if (mn->mmubank->p_addr == parm2) {
				getmmubank(mn->mmubank, parm2);
				if (mmu_logging)
					console_out ("MMU: bank update %08X: %08X - %08X %08X\n",
					mn->mmubank->flags, mn->mmubank->addr, mn->mmubank->len + mn->mmubank->addr,
					mn->mmubank->remap);
			}
			mn = mn->next;
		}
		return 1;
	}

	mmu_slots = 1 << ((currprefs.address_space_24 ? 24 : 32) - MMU_PAGE_SHIFT);
	mmunl = xcalloc (struct mmunode*, mmu_slots);
	size = 1;
	p2 = get_long (p);
	while (get_long (p2) != 0xffffffff) {
		p2 += 16;
		size++;
	}
	p = banks = get_long (p);
	snptr = mmubanks = xmalloc (struct mmudata, size);
	for (;;) {
		int off;
		if (getmmubank(snptr, p))
			break;
		p += 16;
		off = snptr->addr >> MMU_PAGE_SHIFT;
		if (mmunl[off] == NULL) {
			mn = mmunl[off] = xcalloc (struct mmunode, 1);
		} else {
			mn = mmunl[off];
			while (mn->next)
				mn = mn->next;
			mn = mn->next = xcalloc (struct mmunode, 1);
		}
		mn->mmubank = snptr;
		snptr++;
	}

	initialize_memwatch (1);
	console_out ("MMU: enabled, %d banks, CB=%08X S=%08X BNK=%08X SF=%08X, %d*%d\n",
		size - 1, mmu_callback, parm, banks, mmu_regs, mmu_slots, 1 << MMU_PAGE_SHIFT);
	set_special (&regs, SPCFLAG_BRK);
	return 1;
}
