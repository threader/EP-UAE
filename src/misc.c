/*
 * A collection of ugly and random junk brought in from Win32
 * which desparately needs to be tidied up
 */

#include "sysconfig.h"
#include "sysdeps.h"

#include "options.h"
#include "memory.h"
#include "custom.h"
#include "newcpu.h"
#include "events.h"
#include "uae.h"
#include "autoconf.h"
#include "traps.h"
#include "enforcer.h"
#include "picasso96.h"
<<<<<<< HEAD

static uae_u32 REGPARAM2 misc_demux (TrapContext *context)
{
//use the extern int (6 #13)
// d0 0=opensound      d1=unit d2=samplerate d3=blksize ret: sound frequency
// d0 1=closesound     d1=unit
// d0 2=writesamples   d1=unit a0=addr      write blksize samples to card
// d0 3=readsamples    d1=unit a0=addr      read samples from card ret: d0=samples read
      // make sure you have from amigaside blksize*4 mem alloced
      // d0=-1 no data available d0=-2 no recording open
          // d0 > 0 there are more blksize Data in the que
          // do the loop until d0 get 0
          // if d0 is greater than 200 bring a message
          // that show the user that data is lost
          // maximum blocksbuffered are 250 (8,5 sec)
// d0 4=writeinterrupt d1=unit  d0=0 no interrupt happen for this unit
          // d0=-2 no playing open

        //note units for now not support use only unit 0

// d0=10 get clipboard size      d0=size in bytes
// d0=11 get clipboard data      a0=clipboarddata
                                  //Note: a get clipboard size must do before
// d0=12 write clipboard data    a0=clipboarddata
// d0=13 setp96mouserate         d1=hz value
// d0=100 open dll               d1=dll name in windows name conventions
// d0=101 get dll function addr  d1=dllhandle a0 function/var name
// d0=102 exec dllcode           a0=addr of function (see 101)
// d0=103 close dll
// d0=104 screenlost
// d0=105 mem offset
// d0=106 16Bit byteswap
// d0=107 32Bit byteswap
// d0=108 free swap array
// d0=200 ahitweak               d1=offset for dsound position pointer

    int opcode = m68k_dreg (&context->regs, 0);

    switch (opcode) {
        int i, slen, t, todo, byte1, byte2;
        uae_u32 src, num_vars;
        static int cap_pos, clipsize;
#if 0
        LPTSTR p, p2, pos1, pos2;
        static  LPTSTR clipdat;
#endif
        int cur_pos;

/*
 * AHI emulation support
 */
#ifdef AHI
	case 0:
	    cap_pos = 0;
	    sound_freq_ahi = m68k_dreg (&context->regs, 2);
	    amigablksize = m68k_dreg (&context->regs, 3);
	    sound_freq_ahi = ahi_open_sound();
	    uaevar.changenum--;
	    return sound_freq_ahi;
	case 1:
	    ahi_close_sound();
	    sound_freq_ahi = 0;
	    return 0;
	case 2:
	    addr=(char *)m68k_areg (&context->regs, 0);
	    for (i = 0; i < (amigablksize*4); i += 4) {
		ahisndbufpt[0] = get_long((unsigned int)addr + i);
		ahisndbufpt+=1;
		/*ahisndbufpt[0]=chipmem_bget((unsigned int)addr+i+2);
		  ahisndbufpt+=1;
		  ahisndbufpt[0]=chipmem_bget((unsigned int)addr+i+1);
		  ahisndbufpt+=1;
		  ahisndbufpt[0]=chipmem_bget((unsigned int)addr+i);
		  ahisndbufpt+=1;*/
	    }
	    ahi_finish_sound_buffer();
	    return amigablksize;
	case 3:
	    if (norec)
		return -1;
	    if (!ahi_on)
		return -2;
	    i = IDirectSoundCaptureBuffer_GetCurrentPosition (lpDSB2r, &t, &cur_pos);
	    t = amigablksize*4;

	    if (cap_pos <= cur_pos)
		todo = cur_pos - cap_pos;
	    else
		todo = cur_pos + (RECORDBUFFER * t) - cap_pos;
	    if (todo < t) {
	        //if no complete buffer ready exit
		return -1;
	    }
	    i = IDirectSoundCaptureBuffer_Lock (lpDSB2r, cap_pos, t, &pos1, &byte1, &pos2, &byte2, 0);

	    if ((cap_pos + t) < (t * RECORDBUFFER)) {
		cap_pos=cap_pos+t;
	    } else {
		cap_pos = 0;
	    }
	    addr= (char *) m68k_areg (&context->regs, 0);
	    sndbufrecpt= (unsigned int*) pos1;
	    t = t / 4;
	    for (i=0; i < t; i++) {
		put_long ((uae_u32) addr, sndbufrecpt[0]);
		addr += 4;
		sndbufrecpt += 1;
	    }
	    t = t * 4;
	    i = IDirectSoundCaptureBuffer_Unlock (lpDSB2r, pos1, byte1, pos2, byte2);
	    return (todo - t) / t;
	case 4:
	    if (!ahi_on)
		return -2;
	    i = intcount;
	    intcount = 0;
	    return i;
	case 5:
	    if (!ahi_on)
		return 0;
	    ahi_updatesound (1);
	    return 1;
#endif

#if 0
/*
 * Support for clipboard hack
 */
	case 10:
	    i = OpenClipboard (0);
	    clipdat = GetClipboardData (CF_TEXT);
	    if (clipdat) {
		clipsize=strlen(clipdat);
		clipsize++;
		return clipsize;
	    } else {
		return 0;
	    }
	case 11:
	    addr = (char *) m68k_areg (&context->regs, 0);
	    for (i=0; i < clipsize; i++) {
		put_byte ((uae_u32) addr, clipdat[0]);
		addr++;
		clipdat++;
	    }
	    CloseClipboard ();
	    return 0;
	case 12:
	    addr = (char *) m68k_areg (&context->regs, 0);
	    addr = (char *) get_real_address ((uae_u32)addr);
	    i = OpenClipboard (0);
	    EmptyClipboard ();
	    slen = strlen (addr);
	    p = GlobalAlloc (GMEM_DDESHARE, slen + 2);
	    p2 = GlobalLock (p);
	    memcpy (p2, addr, slen);
	    p2[slen] = 0;
	    GlobalUnlock (p);
	    i = (int) SetClipboardData (CF_TEXT, p2);
	    CloseClipboard ();
	    GlobalFree (p);
	    return 0;
#endif

/*
 * Hack for higher P96 mouse draw rate
 */
#ifdef PICASSO96
	case 13: {
	    extern int p96hack_vpos2;
	    extern int hack_vpos;
	    extern int p96refresh_active;
	    extern uae_u16 vtotal;
	    extern unsigned int new_beamcon0;
	    p96hack_vpos2 = 15625 / m68k_dreg (&context->regs, 1);
	    p96refresh_active = 1;
	    if (!picasso_on)
		return 0;
	    vtotal = p96hack_vpos2; // only do below if P96 screen is visible
	    new_beamcon0 |= 0x80;
	    hack_vpos = vtotal;
	    return 0;
	}
#endif

/*
 * Support for enforcer emulation
 */
#ifdef ENFORCER
	case 20:
	    return enforcer_enable ();

	case 21:
	    return enforcer_disable ();
#endif

#if 0
	case 25:
	    flushprinter ();
	    return 0;
#endif


#if 0
	case 100: {	// open dll
	    char *dllname;
	    uae_u32 result;
	    dllname = (char *) m68k_areg (&context->regs, 0);
	    dllname = (char *) get_real_address ((uae_u32)dllname);
	    result = (uae_u32) LoadLibrary (dllname);
	    write_log ("%s windows dll/alib loaded at %d (0 mean failure)\n", dllname, result);
	    return result;
	}
	case 101: {	//get dll label
	    HMODULE m;
	    char *funcname;
	    m = (HMODULE) m68k_dreg (&context->regs, 1);
	    funcname = (char *) m68k_areg (&context->regs, 0);
	    funcname = (char *) get_real_address ((uae_u32)funcname);
	    return (uae_u32) GetProcAddress (m, funcname);
	}
	case 102:	//execute native code
	    return emulib_ExecuteNativeCode2 ();

	case 103: {	//close dll
	    HMODULE libaddr;
	    libaddr = (HMODULE) m68k_dreg (&context->regs, 1);
	    FreeLibrary (libaddr);
	    return 0;
	}
	case 104: {	//screenlost
	    static int oldnum=0;
	    if (uaevar.changenum == oldnum)
		return 0;
	    oldnum = uaevar.changenum;
	    return 1;
	}
        case 105:	//returns memory offset
	    return (uae_u32) get_real_address (0);
	case 106:	//byteswap 16bit vars
			//a0 = start address
			//d1 = number of 16bit vars
			//returns address of new array
	    src = m68k_areg (&context->regs, 0);
	    num_vars = m68k_dreg (&context->regs, 1);

	    if (bswap_buffer_size < num_vars * 2) {
		bswap_buffer_size = (num_vars + 1024) * 2;
		free (bswap_buffer);
		bswap_buffer = (void*) malloc (bswap_buffer_size);
	    }
	    __asm {
			mov esi, dword ptr [src]
			mov edi, dword ptr [bswap_buffer]
			mov ecx, num_vars

			mov ebx, ecx
			and ecx, 3
			je BSWAP_WORD_4X

		BSWAP_WORD_LOOP:
			mov ax, [esi]
			mov ax, [esi]
			mov dl, al
			mov al, ah
			mov ah, dl
			mov [edi], ax
			add esi, 2
			add edi, 2
			loopne BSWAP_WORD_LOOP

		BSWAP_WORD_4X:
			mov ecx, ebx
			shr ecx, 2
			je BSWAP_WORD_END
		BSWAP_WORD_4X_LOOP:
			mov ax, [esi]
			mov dl, al
			mov al, ah
			mov ah, dl
			mov [edi], ax
			mov ax, [esi+2]
			mov dl, al
			mov al, ah
			mov ah, dl
			mov [edi+2], ax
			mov ax, [esi+4]
			mov dl, al
			mov al, ah
			mov ah, dl
			mov [edi+4], ax
			mov ax, [esi+6]
			mov dl, al
			mov al, ah
			mov ah, dl
			mov [edi+6], ax
			add esi, 8
			add edi, 8
			loopne BSWAP_WORD_4X_LOOP

		BSWAP_WORD_END:
	    }
	    return (uae_u32) bswap_buffer;
        case 107:	//byteswap 32bit vars - see case 106
			//a0 = start address
			//d1 = number of 32bit vars
			//returns address of new array
	    src = m68k_areg (&context->regs, 0);
	    num_vars = m68k_dreg (&context->regs, 1);
	    if (bswap_buffer_size < num_vars * 4) {
		bswap_buffer_size = (num_vars + 16384) * 4;
		free (bswap_buffer);
		bswap_buffer = (void*) malloc (bswap_buffer_size);
	    }
	    __asm {
			mov esi, dword ptr [src]
			mov edi, dword ptr [bswap_buffer]
			mov ecx, num_vars

			mov ebx, ecx
			and ecx, 3
			je BSWAP_DWORD_4X

		BSWAP_DWORD_LOOP:
			mov eax, [esi]
			bswap eax
			mov [edi], eax
			add esi, 4
			add edi, 4
			loopne BSWAP_DWORD_LOOP

		BSWAP_DWORD_4X:
			mov ecx, ebx
			shr ecx, 2
			je BSWAP_DWORD_END
			BSWAP_DWORD_4X_LOOP:
			mov eax, [esi]
			bswap eax
			mov [edi], eax
			mov eax, [esi+4]
			bswap eax
			mov [edi+4], eax
			mov eax, [esi+8]
			bswap eax
			mov [edi+8], eax
			mov [edi+8], eax
			mov eax, [esi+12]
			bswap eax
			mov [edi+12], eax
			add esi, 16
			add edi, 16
		loopne BSWAP_DWORD_4X_LOOP

		BSWAP_DWORD_END:
	    }
	    return (uae_u32) bswap_buffer;
	case 108:	//frees swap array
	    bswap_buffer_size = 0;
	    free (bswap_buffer);
	    bswap_buffer = NULL;
	    return 0;
	case 200:
	    ahitweak = m68k_dreg (&context->regs, 1);
	    return 1;
#endif
	default:
	    return 0x12345678;	// Code for not supported function
    }
}


void misc_hsync_stuff (void)
{
    static int misc_demux_installed;

#ifdef AHI
    {
	static int count;
	if (ahi_on) {
	    count++;
	    //15625/count freebuffer check
	    if (count > 20) {
		ahi_updatesound (1);
		count = 0;
	    }
	}
    }
#endif

    if (!misc_demux_installed) {
	uaecptr a = here ();
	org (RTAREA_BASE + 0xFFC0);
	calltrap (deftrap (misc_demux));
	dw (0x4e75);// rts
	org (a);
	misc_demux_installed = 1;
    }
=======
#include "driveclick.h"

#define TRUE 1
#define FALSE 0

static int logging_started;
#define LOG_BOOT "puaebootlog.txt"
#define LOG_NORMAL "puaelog.txt"

static int tablet;
static int axmax, aymax, azmax;
static int xmax, ymax, zmax;
static int xres, yres;
static int maxpres;
static TCHAR *tabletname;
static int tablet_x, tablet_y, tablet_z, tablet_pressure, tablet_buttons, tablet_proximity;
static int tablet_ax, tablet_ay, tablet_az, tablet_flags;

unsigned int log_scsi = 1;
int log_net, uaelib_debug;
unsigned int flashscreen;

struct winuae_currentmode {
        unsigned int flags;
        int native_width, native_height, native_depth, pitch;
        int current_width, current_height, current_depth;
        int amiga_width, amiga_height;
        int frequency;
        int initdone;
        int fullfill;
        int vsync;
};

static struct winuae_currentmode currentmodestruct;
static struct winuae_currentmode *currentmode = &currentmodestruct;

static int serial_period_hsyncs, serial_period_hsync_counter;
static int data_in_serdatr; /* new data received */

// --- dinput.cpp START ---
int rawkeyboard = -1;
int no_rawinput;
// --- dinput.cpp END -----

static uae_u32 REGPARAM2 gfxmem_lgetx (uaecptr addr)
{
	uae_u32 *m;
	addr -= gfxmem_start & gfxmem_mask;
	addr &= gfxmem_mask;
	m = (uae_u32 *)(gfxmemory + addr);
	return do_get_mem_long (m);
}

static uae_u32 REGPARAM2 gfxmem_wgetx (uaecptr addr)
{
	uae_u16 *m;
	addr -= gfxmem_start & gfxmem_mask;
	addr &= gfxmem_mask;
	m = (uae_u16 *)(gfxmemory + addr);
	return do_get_mem_word (m);
}

static uae_u32 REGPARAM2 gfxmem_bgetx (uaecptr addr)
{
	addr -= gfxmem_start & gfxmem_mask;
	addr &= gfxmem_mask;
	return gfxmemory[addr];
}

static void REGPARAM2 gfxmem_lputx (uaecptr addr, uae_u32 l)
{
	uae_u32 *m;
	addr -= gfxmem_start & gfxmem_mask;
	addr &= gfxmem_mask;
	m = (uae_u32 *)(gfxmemory + addr);
	do_put_mem_long (m, l);
}

static void REGPARAM2 gfxmem_wputx (uaecptr addr, uae_u32 w)
{
	uae_u16 *m;
	addr -= gfxmem_start & gfxmem_mask;
	addr &= gfxmem_mask;
	m = (uae_u16 *)(gfxmemory + addr);
	do_put_mem_word (m, (uae_u16)w);
}

static void REGPARAM2 gfxmem_bputx (uaecptr addr, uae_u32 b)
{
	addr -= gfxmem_start & gfxmem_mask;
	addr &= gfxmem_mask;
	gfxmemory[addr] = b;
}

static int REGPARAM2 gfxmem_check (uaecptr addr, uae_u32 size)
{
	addr -= gfxmem_start & gfxmem_mask;
	addr &= gfxmem_mask;
	return (addr + size) < allocated_gfxmem;
}

static uae_u8 *REGPARAM2 gfxmem_xlate (uaecptr addr)
{
	addr -= gfxmem_start & gfxmem_mask;
	addr &= gfxmem_mask;
	return gfxmemory + addr;
}


addrbank gfxmem_bankx = {
	gfxmem_lgetx, gfxmem_wgetx, gfxmem_bgetx,
	gfxmem_lputx, gfxmem_wputx, gfxmem_bputx,
	gfxmem_xlate, gfxmem_check, NULL, "RTG RAM",
	dummy_lgeti, dummy_wgeti, ABFLAG_RAM
};

void getgfxoffset (int *dxp, int *dyp, int *mxp, int *myp)
{
*dxp = 0;
*dyp = 0;
*mxp = 0;
*myp = 0;
/*
        int dx, dy;

        getfilteroffset (&dx, &dy, mxp, myp);
        *dxp = dx;
        *dyp = dy;
        if (picasso_on) {
                dx = picasso_offset_x;
                dy = picasso_offset_y;
                *mxp = picasso_offset_mx;
                *myp = picasso_offset_my;
        }
        *dxp = dx;
        *dyp = dy;
        if (currentmode->flags & DM_W_FULLSCREEN) {
                if (scalepicasso && screen_is_picasso)
                        return;
                if (usedfilter && !screen_is_picasso)
                        return;
                if (currentmode->fullfill && (currentmode->current_width > currentmode->native_width || currentmode->current_height > currentmode->native_height))
                        return;
                dx += (currentmode->native_width - currentmode->current_width) / 2;
                dy += (currentmode->native_height - currentmode->current_height) / 2;
        }
        *dxp = dx;
        *dyp = dy;
*/
}


int is_tablet (void)
{
        return tablet ? 1 : 0;
}

int vsync_switchmode (int hz, int oldhz)
{
        static int tempvsync;
        int w = currentmode->native_width;
        int h = currentmode->native_height;
        int d = currentmode->native_depth / 8;
//        struct MultiDisplay *md = getdisplay (&currprefs);
        struct PicassoResolution *found;

        int newh, i, cnt;
        int dbl = getvsyncrate (currprefs.chipset_refreshrate) != currprefs.chipset_refreshrate ? 2 : 1;

        if (hz < 0)
                return tempvsync;

        newh = h * oldhz / hz;
        hz = hz * dbl;

        found = NULL;
/*        for (i = 0; md->DisplayModes[i].depth >= 0 && !found; i++) {
                struct PicassoResolution *r = &md->DisplayModes[i];
                if (r->res.width == w && r->res.height == h && r->depth == d) {
                        int j;
                        for (j = 0; r->refresh[j] > 0; j++) {
                                if (r->refresh[j] == oldhz) {
                                        found = r;
                                        break;
                                }
                        }
                }
        }*/
        if (found == NULL) {
                write_log ("refresh rate changed to %d but original rate was not found\n", hz);
                return 0;
        }

        found = NULL;
/*        for (cnt = 0; cnt <= abs (newh - h) + 1 && !found; cnt++) {
                for (i = 0; md->DisplayModes[i].depth >= 0 && !found; i++) {
                        struct PicassoResolution *r = &md->DisplayModes[i];
                        if (r->res.width == w && (r->res.height == newh + cnt || r->res.height == newh - cnt) && r->depth == d) {
                                int j;
                                for (j = 0; r->refresh[j] > 0; j++) {
                                        if (r->refresh[j] == hz) {
                                                found = r;
                                                break;
                                        }
                                }
                        }
                }
        }*/
        if (!found) {
                tempvsync = currprefs.gfx_avsync;
                changed_prefs.gfx_avsync = 0;
                write_log ("refresh rate changed to %d but no matching screenmode found, vsync disabled\n", hz);
        } else {
                newh = found->res.height;
                changed_prefs.gfx_size_fs.height = newh;
                changed_prefs.gfx_refreshrate = hz;
                write_log ("refresh rate changed to %d, new screenmode %dx%d\n", hz, w, newh);
        }
/*
        reopen (1);
*/
        return 0;
}

void serial_check_irq (void)
{
        if (data_in_serdatr)
                INTREQ_0 (0x8000 | 0x0800);
}

void serial_uartbreak (int v)
{
#ifdef SERIAL_PORT
        serialuartbreak (v);
#endif
}

void serial_hsynchandler (void)
{
#ifdef AHI
        extern void hsyncstuff(void);
        hsyncstuff();
#endif
/*
        if (serial_period_hsyncs == 0)
                return;
        serial_period_hsync_counter++;
        if (serial_period_hsyncs == 1 || (serial_period_hsync_counter % (serial_period_hsyncs - 1)) == 0) {
                checkreceive_serial (0);
                checkreceive_enet (0);
        }
        if ((serial_period_hsync_counter % serial_period_hsyncs) == 0)
                checksend (0);
*/
}

/*
static int drvsampleres[] = {
        IDR_DRIVE_CLICK_A500_1, DS_CLICK,
        IDR_DRIVE_SPIN_A500_1, DS_SPIN,
        IDR_DRIVE_SPINND_A500_1, DS_SPINND,
        IDR_DRIVE_STARTUP_A500_1, DS_START,
        IDR_DRIVE_SNATCH_A500_1, DS_SNATCH,
        -1
};
*/
int driveclick_loadresource (struct drvsample *sp, int drivetype)
{
/*
        int i, ok;

        ok = 1;
        for (i = 0; drvsampleres[i] >= 0; i += 2) {
                struct drvsample *s = sp + drvsampleres[i + 1];
                HRSRC res = FindResource (NULL, MAKEINTRESOURCE (drvsampleres[i + 0]), "WAVE");
                if (res != 0) {
                        HANDLE h = LoadResource (NULL, res);
                        int len = SizeofResource (NULL, res);
                        uae_u8 *p = LockResource (h);
                        s->p = decodewav (p, &len);
                        s->len = len;
                } else {
                        ok = 0;
                }
        }
        return ok;
*/
}

void driveclick_fdrawcmd_close(int drive)
{
/*
        if (h[drive] != INVALID_HANDLE_VALUE)
                CloseHandle(h[drive]);
        h[drive] = INVALID_HANDLE_VALUE;
        motors[drive] = 0;
*/
}

static int driveclick_fdrawcmd_open_2(int drive)
{
/*
        TCHAR s[32];

        driveclick_fdrawcmd_close(drive);
        _stprintf (s, "\\\\.\\fdraw%d", drive);
        h[drive] = CreateFile(s, GENERIC_READ | GENERIC_WRITE, 0, NULL, OPEN_EXISTING, 0, NULL);
        if (h[drive] == INVALID_HANDLE_VALUE)
                return 0;
        return 1;
*/
}

int driveclick_fdrawcmd_open(int drive)
{
/*
        if (!driveclick_fdrawcmd_open_2(drive))
                return 0;
        driveclick_fdrawcmd_init(drive);
        return 1;
*/
}

void driveclick_fdrawcmd_detect(void)
{
/*
        static int detected;
        if (detected)
                return;
        detected = 1;
        if (driveclick_fdrawcmd_open_2(0))
                driveclick_pcdrivemask |= 1;
        driveclick_fdrawcmd_close(0);
        if (driveclick_fdrawcmd_open_2(1))
                driveclick_pcdrivemask |= 2;
        driveclick_fdrawcmd_close(1);
*/
}

void driveclick_fdrawcmd_seek(int drive, int cyl)
{
//        write_comm_pipe_int (dc_pipe, (drive << 8) | cyl, 1);
}
void driveclick_fdrawcmd_motor (int drive, int running)
{
//        write_comm_pipe_int (dc_pipe, 0x8000 | (drive << 8) | (running ? 1 : 0), 1);
}

void driveclick_fdrawcmd_vsync(void)
{
/*
        int i;
        for (i = 0; i < 2; i++) {
                if (motors[i] > 0) {
                        motors[i]--;
                        if (motors[i] == 0)
                                CmdMotor(h[i], 0);
                }
        }
*/
}

static int driveclick_fdrawcmd_init(int drive)
{
/*
        static int thread_ok;

        if (h[drive] == INVALID_HANDLE_VALUE)
                return 0;
        motors[drive] = 0;
        SetDataRate(h[drive], 3);
        CmdSpecify(h[drive], 0xd, 0xf, 0x1, 0);
        SetMotorDelay(h[drive], 0);
        CmdMotor(h[drive], 0);
        if (thread_ok)
                return 1;
        thread_ok = 1;
        init_comm_pipe (dc_pipe, DC_PIPE_SIZE, 3);
        uae_start_thread ("DriveClick", driveclick_thread, NULL, NULL);
        return 1;
*/
}

uae_u32 emulib_target_getcpurate (uae_u32 v, uae_u32 *low)
{
/*
        *low = 0;
        if (v == 1) {
                LARGE_INTEGER pf;
                pf.QuadPart = 0;
                QueryPerformanceFrequency (&pf);
                *low = pf.LowPart;
                return pf.HighPart;
        } else if (v == 2) {
                LARGE_INTEGER pf;
                pf.QuadPart = 0;
                QueryPerformanceCounter (&pf);
                *low = pf.LowPart;
                return pf.HighPart;
        }
*/
        return 0;
}

int isfat (uae_u8 *p)
{
	int i, b;

	if ((p[0x15] & 0xf0) != 0xf0)
		return 0;
	if (p[0x0b] != 0x00 || p[0x0c] != 0x02)
		return 0;
	b = 0;
	for (i = 0; i < 8; i++) {
		if (p[0x0d] & (1 << i))
			b++;
	}
	if (b != 1)
		return 0;
	if (p[0x0f] != 0)
		return 0;
	if (p[0x0e] > 8 || p[0x0e] == 0)
		return 0;
	if (p[0x10] == 0 || p[0x10] > 8)
		return 0;
	b = (p[0x12] << 8) | p[0x11];
	if (b > 8192 || b <= 0)
		return 0;
	b = p[0x16] | (p[0x17] << 8);
	if (b == 0 || b > 8192)
		return 0;
	return 1;
}

void setmouseactivexy (int x, int y, int dir)
{
/*        int diff = 8;

        if (isfullscreen () > 0)
                return;
        x += amigawin_rect.left;
        y += amigawin_rect.top;
        if (dir & 1)
                x = amigawin_rect.left - diff;
        if (dir & 2)
                x = amigawin_rect.right + diff;
        if (dir & 4)
                y = amigawin_rect.top - diff;
        if (dir & 8)
                y = amigawin_rect.bottom + diff;
        if (!dir) {
                x += (amigawin_rect.right - amigawin_rect.left) / 2;
                y += (amigawin_rect.bottom - amigawin_rect.top) / 2;
        }
        if (mouseactive) {
                disablecapture ();
                SetCursorPos (x, y);
                if (dir)
                        recapture = 1;
        }*/
}

void setmouseactive (int active)
{
}

char *au_fs_copy (char *dst, int maxlen, const char *src)
{
        int i;

        for (i = 0; src[i] && i < maxlen - 1; i++)
                dst[i] = src[i];
        dst[i] = 0;
        return dst;
}

int my_existsfile (const char *name)
{
		struct stat sonuc;
		if (lstat (name, &sonuc) == -1) {
			return 0;
		} else {
			if (!S_ISDIR(sonuc.st_mode))
				return 1;
		}
        return 0;
}

int my_existsdir (const char *name)
{
		struct stat sonuc;

		if (lstat (name, &sonuc) == -1) {
			return 0;
		} else {
			if (S_ISDIR(sonuc.st_mode))
				return 1;
		}
        return 0;
}

int my_getvolumeinfo (const char *root)
{
		struct stat sonuc;
        int ret = 0;

        if (lstat (root, &sonuc) == -1)
                return -1;
        if (!S_ISDIR(sonuc.st_mode))
                return -1;
//------------
        return ret;
}

// --- clipboard.c --- temporary here ---
static uaecptr clipboard_data;
static int vdelay, signaling, initialized;

void amiga_clipboard_die (void)
{
	signaling = 0;
	write_log ("clipboard not initialized\n");
}

void amiga_clipboard_init (void)
{
	signaling = 0;
	write_log ("clipboard initialized\n");
	initialized = 1;
}

void amiga_clipboard_task_start (uaecptr data)
{
	clipboard_data = data;
	signaling = 1;
	write_log ("clipboard task init: %08x\n", clipboard_data);
}

uae_u32 amiga_clipboard_proc_start (void)
{
	write_log ("clipboard process init: %08x\n", clipboard_data);
	signaling = 1;
	return clipboard_data;
}

void amiga_clipboard_got_data (uaecptr data, uae_u32 size, uae_u32 actual)
{
	uae_u8 *addr;
	if (!initialized) {
		write_log ("clipboard: got_data() before initialized!?\n");
		return;
	}
}


int get_guid_target (uae_u8 *out)
{
	unsigned Data1, Data2, Data3, Data4;

	srand(time(NULL));
	Data1 = rand();
	Data2 = ((rand() & 0x0fff) | 0x4000);
	Data3 = rand() % 0x3fff + 0x8000;
	Data4 = rand();

	out[0] = Data1 >> 24;
	out[1] = Data1 >> 16;
	out[2] = Data1 >>  8;
	out[3] = Data1 >>  0;
	out[4] = Data2 >>  8;
	out[5] = Data2 >>  0;
	out[6] = Data3 >>  8;
	out[7] = Data3 >>  0;
	memcpy (out + 8, Data4, 8);
	return 1;
}

static int testwritewatch (void)
{
}

void machdep_free (void)
{
#ifdef LOGITECHLCD
        lcd_close ();
#endif
}

void target_run (void)
{
        //shellexecute (currprefs.win32_commandpathstart);
}

void fetch_path (char *name, char *out, int size)
{
        int size2 = size;

        _tcscpy (out, start_path_data);
        if (!name)
                return;
}
void fetch_configurationpath (char *out, int size)
{
        fetch_path ("ConfigurationPath", out, size);
}
void fetch_screenshotpath (char *out, int size)
{
        fetch_path ("ScreenshotPath", out, size);
}
void fetch_ripperpath (char *out, int size)
{
        fetch_path ("RipperPath", out, size);
}
void fetch_datapath (char *out, int size)
{
        fetch_path (NULL, out, size);
}

// --- dinput.cpp ---
int input_get_default_keyboard (int i)
{
        if (rawkeyboard > 0) {
                if (i == 0)
                        return 0;
                return 1;
        } else {
                if (i == 0)
                        return 1;
                return 0;
        }
}

// --- unicode.cpp ---
static unsigned int fscodepage;

char *ua_fs (const char *s, int defchar)
{
	return s;
}

char *ua_copy (char *dst, int maxlen, const char *src)
{
        dst[0] = 0;
		strncpy (dst, src, maxlen);
        return dst;
}

// --- win32gui.cpp ---
static int qs_override;

int target_cfgfile_load (struct uae_prefs *p, char *filename, int type, int isdefault)
{
	int v, i, type2;
	int ct, ct2 = 0, size;
	char tmp1[MAX_DPATH], tmp2[MAX_DPATH];
	char fname[MAX_DPATH];

	_tcscpy (fname, filename);
	if (!zfile_exists (fname)) {
		fetch_configurationpath (fname, sizeof (fname) / sizeof (TCHAR));
		if (_tcsncmp (fname, filename, _tcslen (fname)))
			_tcscat (fname, filename);
		else
			_tcscpy (fname, filename);
	}

	if (!isdefault)
		qs_override = 1;
	if (type < 0) {
		type = 0;
		cfgfile_get_description (fname, NULL, NULL, NULL, &type);
	}
	if (type == 0 || type == 1) {
		discard_prefs (p, 0);
	}
	type2 = type;
	if (type == 0) {
		default_prefs (p, type);
	}
		
	//regqueryint (NULL, "ConfigFile_NoAuto", &ct2);
	v = cfgfile_load (p, fname, &type2, ct2, isdefault ? 0 : 1);
	if (!v)
		return v;
	if (type > 0)
		return v;
	for (i = 1; i <= 2; i++) {
		if (type != i) {
			size = sizeof (ct);
			ct = 0;
			//regqueryint (NULL, configreg2[i], &ct);
			if (ct && ((i == 1 && p->config_hardware_path[0] == 0) || (i == 2 && p->config_host_path[0] == 0) || ct2)) {
				size = sizeof (tmp1) / sizeof (TCHAR);
				//regquerystr (NULL, configreg[i], tmp1, &size);
				fetch_path ("ConfigurationPath", tmp2, sizeof (tmp2) / sizeof (TCHAR));
				_tcscat (tmp2, tmp1);
				v = i;
				cfgfile_load (p, tmp2, &v, 1, 0);
			}
		}
	}
	v = 1;
	return v;
}
// --- win32gfx.c
int screen_is_picasso = 0;
struct uae_filter *usedfilter;
uae_u32 redc[3 * 256], grec[3 * 256], bluc[3 * 256];

static int isfullscreen_2 (struct uae_prefs *p)
{
        if (screen_is_picasso)
                return p->gfx_pfullscreen == 1 ? 1 : (p->gfx_pfullscreen == 2 ? -1 : 0);
        else
                return p->gfx_afullscreen == 1 ? 1 : (p->gfx_afullscreen == 2 ? -1 : 0);
}
int isfullscreen (void)
{
        return isfullscreen_2 (&currprefs);
}

// --- win32.c
uae_u8 *save_log (int bootlog, int *len)
{
        FILE *f;
        uae_u8 *dst = NULL;
        int size;

        if (!logging_started)
                return NULL;
        f = _tfopen (bootlog ? LOG_BOOT : LOG_NORMAL, "rb");
        if (!f)
                return NULL;
        fseek (f, 0, SEEK_END);
        size = ftell (f);
        fseek (f, 0, SEEK_SET);
        if (size > 30000)
                size = 30000;
        if (size > 0) {
                dst = xcalloc (uae_u8, size + 1);
                if (dst)
                        fread (dst, 1, size, f);
                fclose (f);
                *len = size + 1;
        }
        return dst;
>>>>>>> p-uae/v2.1.0
}
