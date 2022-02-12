 /*
  * UAE - The Un*x Amiga Emulator
  *
  *  Serial Line Emulation
  *
  * (c) 1996, 1997 Stefan Reinauer <stepan@linux.de>
  * (c) 1997 Christian Schmitt <schmitt@freiburg.linux.de>
  *
  */

#include "sysconfig.h"
#include "sysdeps.h"

#include "options.h"
#include "uae.h"
#include "memory.h"
#include "custom.h"
#include "newcpu.h"
#include "cia.h"
<<<<<<< HEAD
#include "serial.h"

#ifdef SERIAL_PORT
# ifndef WIN32
#  include "osdep/serial.h"
# else
#  include "od-win32/parser.h"
# endif
#endif

#define SERIALDEBUG	0 /* 0, 1, 2 3 */
#define SERIALHSDEBUG	0
#define MODEMTEST	0 /* 0 or 1 */

static int data_in_serdat;	/* new data written to SERDAT */
static int data_in_serdatr;	/* new data received */
static int data_in_sershift;	/* data transferred from SERDAT to shift register */
static uae_u16 serdatshift;	/* serial shift register */
static int ovrun;
static int dtr;
static int serial_period_hsyncs, serial_period_hsync_counter;
static int ninebit;
int serdev;

void serial_open(void);
void serial_close(void);

uae_u16 serper, serdat, serdatr;

static const int allowed_baudrates[] = {
    0, 110, 300, 600, 1200, 2400, 4800, 9600, 14400,
    19200, 31400, 38400, 57600, 115200, 128000, 256000, -1
};

void SERPER (uae_u16 w)
{
    int baud = 0, i, per;
    static int warned;

    if (serper == w)  /* don't set baudrate if it's already ok */
	return;
    ninebit = 0;
    serper = w;

    if (w & 0x8000) {
	if (!warned) {
	    write_log( "SERIAL: program uses 9bit mode PC=%x\n", m68k_getpc (&regs) );
	    warned++;
	}
	ninebit = 1;
    }
    w &= 0x7fff;

    if (w < 13)
	w = 13;

    per = w;
    if (per == 0) per = 1;
    per = 3546895 / (per + 1);
    if (per <= 0) per = 1;
    i = 0;
    while (allowed_baudrates[i] >= 0 && per > allowed_baudrates[i] * 100 / 97)
	i++;
    baud = allowed_baudrates[i];

    serial_period_hsyncs = (((serper & 0x7fff) + 1) * 10) / maxhpos;
    if (serial_period_hsyncs <= 0)
	serial_period_hsyncs = 1;
    serial_period_hsync_counter = 0;

    write_log ("SERIAL: period=%d, baud=%d, hsyncs=%d PC=%x\n", w, baud, serial_period_hsyncs, m68k_getpc (&regs));

    if (ninebit)
	baud *= 2;
    if (currprefs.serial_direct) {
	if (baud != 31400 && baud < 115200)
	    baud = 115200;
	serial_period_hsyncs = 1;
    }
#ifdef SERIAL_PORT
    setbaud (baud);
#endif
}

static char dochar (int v)
{
    v &= 0xff;
    if (v >= 32 && v < 127) return (char)v;
    return '.';
}

static void checkreceive (int mode)
{
#ifdef SERIAL_PORT
    static uae_u32 lastchartime;
    static int ninebitdata;
    struct timeval tv;
    int recdata;

    if (!readseravail ())
	return;

    if (data_in_serdatr) {
	/* probably not needed but there may be programs that expect OVRUNs.. */
	gettimeofday (&tv, NULL);
	if (tv.tv_sec > lastchartime) {
	    ovrun = 1;
	    INTREQ (0x8000 | 0x0800);
	    while (readser (&recdata));
	    write_log ("SERIAL: overrun\n");
	}
	return;
    }

    if (ninebit) {
	for (;;) {
	    if (!readser (&recdata))
		return;
	    if (ninebitdata) {
		serdatr = (ninebitdata & 1) << 8;
		serdatr |= recdata;
		serdatr |= 0x200;
		ninebitdata = 0;
		break;
	    } else {
		ninebitdata = recdata;
		if ((ninebitdata & ~1) != 0xa8) {
		    write_log ("SERIAL: 9-bit serial emulation sync lost, %02.2X != %02.2X\n", ninebitdata & ~1, 0xa8);
		    ninebitdata = 0;
		    return;
		}
		continue;
	    }
	}
    } else {
	if (!readser (&recdata))
	    return;
	serdatr = recdata;
	serdatr |= 0x100;
    }
    gettimeofday (&tv, NULL);
    lastchartime = tv.tv_sec + 5;
    data_in_serdatr = 1;
    serial_check_irq ();
#if SERIALDEBUG > 2
    write_log ("SERIAL: received %02.2X (%c)\n", serdatr & 0xff, dochar (serdatr));
#endif
#endif
}

static void checksend (int mode)
{
    int bufstate;

#ifdef SERIAL_PORT
    bufstate = checkserwrite ();
#else
    bufstate = 1;
#endif

    if (!data_in_serdat && !data_in_sershift)
	return;

    if (data_in_sershift && mode == 0 && bufstate)
	data_in_sershift = 0;

    if (data_in_serdat && !data_in_sershift) {
	data_in_sershift = 1;
	serdatshift = serdat;
#ifdef SERIAL_PORT
	if (ninebit)
	    writeser (((serdatshift >> 8) & 1) | 0xa8);
	writeser (serdatshift);
#endif
	data_in_serdat = 0;
        INTREQ (0x8000 | 0x0001);
#if SERIALDEBUG > 2
	write_log ("SERIAL: send %04.4X (%c)\n", serdatshift, dochar (serdatshift));
#endif
    }
}

void serial_hsynchandler (void)
{
    if (serial_period_hsyncs == 0)
	return;
    serial_period_hsync_counter++;
    if (serial_period_hsyncs == 1 || (serial_period_hsync_counter % (serial_period_hsyncs - 1)) == 0)
	checkreceive (0);
    if ((serial_period_hsync_counter % serial_period_hsyncs) == 0)
	checksend (0);
}

/* Not (fully) implemented yet: (on windows this work)
=======

#undef POSIX_SERIAL
/* Some more or less good way to determine whether we can safely compile in
 * the serial stuff. I'm certain it breaks compilation on some systems. */
#if defined HAVE_SYS_TERMIOS_H && defined HAVE_POSIX_OPT_H && defined HAVE_SYS_IOCTL_H && defined HAVE_TCGETATTR
#define POSIX_SERIAL
#endif

#ifdef POSIX_SERIAL
#include <termios.h>
#include <unistd.h>
#include <sys/ioctl.h>
#endif

#if !defined B300 || !defined B1200 || !defined B2400 || !defined B4800 || !defined B9600
#undef POSIX_SERIAL
#endif
#if !defined B19200 || !defined B57600 || !defined B115200 || !defined B230400
#undef POSIX_SERIAL
#endif

#ifndef O_NONBLOCK
#define O_NONBLOCK O_NDELAY
#endif

#define SERIALDEBUG	1 /* 0, 1, 2 3 */
#define MODEMTEST	0 /* 0 or 1 */

void serial_open(void);
void serial_close(void);
void serial_init (void);
void serial_exit (void);

void serial_dtr_on (void);
void serial_dtr_off (void);

void serial_flush_buffer (void);
static int serial_read (char *buffer);

int serial_readstatus (void);
uae_u16 serial_writestatus (int, int);

uae_u16 SERDATR (void);

int  SERDATS (void);
void  SERPER (uae_u16 w);
void  SERDAT (uae_u16 w);

static char inbuf[1024], outbuf[1024];
static int inptr, inlast, outlast;

int waitqueue=0,
    carrier=0,
    serdev=0,
    dsr=0,
    dtr=0,
    isbaeh=0,
    doreadser=0,
    serstat=-1;

int sd = -1;

#ifdef POSIX_SERIAL
    struct termios tios;
#endif

uae_u16 serper=0,serdat;

void SERPER (uae_u16 w)
{
    int baud=0, pspeed;

    if (!currprefs.use_serial)
		return;

#if defined POSIX_SERIAL
    if (serper == w)  /* don't set baudrate if it's already ok */
		return;
    serper = w;

    if (w&0x8000)
		write_log ("SERPER: 9bit transmission not implemented.\n");

    switch (w & 0x7fff) {
     /* These values should be calculated by the current
      * color clock value (NTSC/PAL). But this solution is
      * easy and it works.
      */

     case 0x2e9b:
     case 0x2e14: baud=300; pspeed=B300; break;
     case 0x170a:
     case 0x0b85: baud=1200; pspeed=B1200; break;
     case 0x05c2:
     case 0x05b9: baud=2400; pspeed=B2400; break;
     case 0x02e9:
     case 0x02e1: baud=4800; pspeed=B4800; break;
     case 0x0174:
     case 0x0170: baud=9600; pspeed=B9600; break;
     case 0x00b9:
     case 0x00b8: baud=19200; pspeed=B19200; break;
     case 0x005c:
     case 0x005d: baud=38400; pspeed=B38400; break;
     case 0x003d: baud=57600; pspeed=B57600; break;
     case 0x001e: baud=115200; pspeed=B115200; break;
     case 0x000f: baud=230400; pspeed=B230400; break;
     default:
	write_log ("SERPER: unsupported baudrate (0x%04x) %d\n",w&0x7fff,
		 (unsigned int)(3579546.471/(double)((w&0x7fff)+1)));  return;
    }

    /* Only access hardware when we own it */
    if (serdev == 1) {
	if (tcgetattr (sd, &tios) < 0) {
	    write_log ("SERPER: TCGETATTR failed\n");
	    return;
	}

	if (cfsetispeed (&tios, pspeed) < 0) {    /* set serial input speed */
	    write_log ("SERPER: CFSETISPEED (%d bps) failed\n", baud);
	    return;
	}
	if (cfsetospeed (&tios, pspeed) < 0) {    /* set serial output speed */
	    write_log ("SERPER: CFSETOSPEED (%d bps) failed\n", baud);
	    return;
	}

	if (tcsetattr (sd, TCSADRAIN, &tios) < 0) {
	    write_log ("SERPER: TCSETATTR failed\n");
	    return;
	}
    }
#endif

#if SERIALDEBUG > 0
    if (serdev == 1)
		write_log ("SERPER: baudrate set to %d bit/sec\n", baud);
#endif
}

/* Not (fully) implemented yet:
>>>>>>> p-uae/v2.1.0
 *
 *  -  Something's wrong with the Interrupts.
 *     (NComm works, TERM does not. TERM switches to a
 *     blind mode after a connect and wait's for the end
 *     of an asynchronous read before switching blind
 *     mode off again. It never gets there on UAE :-< )
 *
<<<<<<< HEAD
=======
 *  -  RTS/CTS handshake, this is not really neccessary,
 *     because you can use RTS/CTS "outside" without
 *     passing it through to the emulated Amiga
 *
 *  -  ADCON-Register ($9e write, $10 read) Bit 11 (UARTBRK)
 *     (see "Amiga Intern", pg 246)
>>>>>>> p-uae/v2.1.0
 */

void SERDAT (uae_u16 w)
{
<<<<<<< HEAD
    serdat = w;

    if (!(w & 0x3ff)) {
#if SERIALDEBUG > 1
	write_log ("SERIAL: zero serial word written?! PC=%x\n", m68k_getpc (&regs));
#endif
	return;
    }

#if SERIALDEBUG > 1
    if (data_in_serdat) {
	write_log ("SERIAL: program wrote to SERDAT but old byte wasn't fetched yet\n");
    }
#endif

    data_in_serdat = 1;
    if (!data_in_sershift)
	checksend (1);

#if SERIALDEBUG > 2
    write_log ("SERIAL: wrote 0x%04x (%c) PC=%x\n", w, dochar (w), m68k_getpc (&regs));
#endif

=======
    unsigned char z;

    if (!currprefs.use_serial)
		return;

    z = (unsigned char)(w&0xff);

    if (currprefs.serial_demand && !dtr) {
		if (!isbaeh) {
		    write_log ("SERDAT: Baeh.. Your software needs SERIAL_ALWAYS to work properly.\n");
		    isbaeh=1;
		}
		return;
    } else {
		outbuf[outlast++] = z;
		if (outlast == sizeof outbuf)
		    serial_flush_buffer();
    }

#if SERIALDEBUG > 2
    write_log ("SERDAT: wrote 0x%04x\n", w);
#endif

    serdat|=0x2000; /* Set TBE in the SERDATR ... */
    intreq|=1;      /* ... and in INTREQ register */
>>>>>>> p-uae/v2.1.0
    return;
}

uae_u16 SERDATR (void)
{
<<<<<<< HEAD
    serdatr &= 0x03ff;
    if (!data_in_serdat)
	serdatr |= 0x2000;
    if (!data_in_sershift)
	serdatr |= 0x1000;
    if (data_in_serdatr)
	serdatr |= 0x4000;
    if (ovrun)
	serdatr |= 0x8000;
#if SERIALDEBUG > 2
    write_log( "SERIAL: read 0x%04.4x (%c) %x\n", serdatr, dochar (serdatr), m68k_getpc (&regs));
#endif
    ovrun = 0;
    data_in_serdatr = 0;
    return serdatr;
}

void serial_check_irq (void)
{
    if (data_in_serdatr)
	INTREQ_0 (0x8000 | 0x0800);
=======
    if (!currprefs.use_serial)
		return 0;
#if SERIALDEBUG > 2
    write_log ("SERDATR: read 0x%04x\n", serdat);
#endif
    waitqueue = 0;
    return serdat;
}

int SERDATS (void)
{
    unsigned char z;

    if (!serdev)           /* || (serdat&0x4000)) */
		return 0;

    if (waitqueue == 1) {
		intreq |= 0x0800;
		return 1;
    }

    if ((serial_read ((char *)&z)) == 1) {
		waitqueue = 1;
		serdat = 0x4100; /* RBF and STP set! */
		serdat |= ((unsigned int)z) & 0xff;
		intreq |= 0x0800; /* Set RBF flag (Receive Buffer full) */

#if SERIALDEBUG > 1
		write_log ("SERDATS: received 0x%02x --> serdat==0x%04x\n",
			(unsigned int)z, (unsigned int)serdat);
#endif
		return 1;
    }
    return 0;
>>>>>>> p-uae/v2.1.0
}

void serial_dtr_on(void)
{
<<<<<<< HEAD
#if SERIALHSDEBUG > 0
    write_log ("SERIAL: DTR on\n");
#endif
    dtr = 1;
    if (currprefs.serial_demand)
	serial_open ();
#ifdef SERIAL_PORT
    setserstat (TIOCM_DTR, dtr);
#endif
=======
#if SERIALDEBUG > 0
    write_log ("DTR on.\n");
#endif
    dtr = 1;

    if (currprefs.serial_demand)
		serial_open ();
>>>>>>> p-uae/v2.1.0
}

void serial_dtr_off(void)
{
<<<<<<< HEAD
#if SERIALHSDEBUG > 0
    write_log ("SERIAL: DTR off\n");
#endif
    dtr = 0;
#ifdef SERIAL_PORT
    if (currprefs.serial_demand)
	serial_close ();
    setserstat(TIOCM_DTR, dtr);
#endif
=======
#if SERIALDEBUG > 0
    write_log ("DTR off.\n");
#endif
    dtr = 0;
    if (currprefs.serial_demand)
		serial_close ();
}

static int serial_read (char *buffer)
{
    if (inptr < inlast) {
		*buffer = inbuf[inptr++];
		return 1;
    }

    if (serdev == 1) {
		inlast = read (sd, inbuf, sizeof inbuf);
		inptr = 0;
		if (inptr < inlast) {
		    *buffer = inbuf[inptr++];
		    return 1;
		}
    }

    return 0;
>>>>>>> p-uae/v2.1.0
}

void serial_flush_buffer (void)
{
<<<<<<< HEAD
}

static uae_u8 oldserbits;

static void serial_status_debug (const char *s)
{
#if SERIALHSDEBUG > 1
    write_log ("%s: DTR=%d RTS=%d CD=%d CTS=%d DSR=%d\n", s,
	(oldserbits & 0x80) ? 0 : 1, (oldserbits & 0x40) ? 0 : 1,
	(oldserbits & 0x20) ? 0 : 1, (oldserbits & 0x10) ? 0 : 1, (oldserbits & 0x08) ? 0 : 1);
#endif
}

uae_u8 serial_readstatus (uae_u8 dir)
{
    int status = 0;
    uae_u8 serbits = oldserbits;

#ifdef SERIAL_PORT
    getserstat (&status);

    if (!(status & TIOCM_CAR)) {
	if (!(serbits & 0x20)) {
	    serbits |= 0x20;
#if SERIALHSDEBUG > 0
	    write_log ("SERIAL: CD off\n");
#endif
	}
    } else {
	if (serbits & 0x20) {
	    serbits &= ~0x20;
#if SERIALHSDEBUG > 0
	    write_log ("SERIAL: CD on\n");
#endif
	}
    }

    if (!(status & TIOCM_DSR)) {
	if (!(serbits & 0x08)) {
	    serbits |= 0x08;
#if SERIALHSDEBUG > 0
	    write_log ("SERIAL: DSR off\n");
#endif
	}
    } else {
	if (serbits & 0x08) {
	    serbits &= ~0x08;
#if SERIALHSDEBUG > 0
	    write_log ("SERIAL: DSR on\n");
#endif
	}
    }

    if (!(status & TIOCM_CTS)) {
	if (!(serbits & 0x10)) {
	    serbits |= 0x10;
#if SERIALHSDEBUG > 0
	    write_log ("SERIAL: CTS off\n");
#endif
	}
    } else {
	if (serbits & 0x10) {
	    serbits &= ~0x10;
#if SERIALHSDEBUG > 0
	    write_log ("SERIAL: CTS on\n");
#endif
	}
    }

    serbits &= 0x08 | 0x10 | 0x20;
    oldserbits &= ~(0x08 | 0x10 | 0x20);
    oldserbits |= serbits;
#endif

    serial_status_debug ("read");

    return oldserbits;
}

uae_u8 serial_writestatus (uae_u8 newstate, uae_u8 dir)
{
#ifdef SERIAL_PORT
    if (((oldserbits ^ newstate) & 0x80) && (dir & 0x80)) {
	if (newstate & 0x80)
	    serial_dtr_off();
	else
	    serial_dtr_on();
    }

    if (!currprefs.serial_hwctsrts && (dir & 0x40)) {
	if ((oldserbits ^ newstate) & 0x40) {
	    if (newstate & 0x40) {
		setserstat (TIOCM_RTS, 0);
#if SERIALHSDEBUG > 0
		write_log ("SERIAL: RTS cleared\n");
#endif
	    } else {
	        setserstat (TIOCM_RTS, 1);
#if SERIALHSDEBUG > 0
		write_log ("SERIAL: RTS set\n");
#endif
	    }
	}
    }

#if 0 /* CIA io-pins can be read even when set to output.. */
    if ((newstate & 0x20) != (oldserbits & 0x20) && (dir & 0x20))
        write_log ("SERIAL: warning, program tries to use CD as an output!\n");
    if ((newstate & 0x10) != (oldserbits & 0x10) && (dir & 0x10))
        write_log ("SERIAL: warning, program tries to use CTS as an output!\n");
    if ((newstate & 0x08) != (oldserbits & 0x08) && (dir & 0x08))
        write_log ("SERIAL: warning, program tries to use DSR as an output!\n");
#endif

    if (((newstate ^ oldserbits) & 0x40) && !(dir & 0x40))
	write_log ("SERIAL: warning, program tries to use RTS as an input!\n");
    if (((newstate ^ oldserbits) & 0x80) && !(dir & 0x80))
	write_log ("SERIAL: warning, program tries to use DTR as an input!\n");

#endif

    oldserbits &= ~(0x80 | 0x40);
    newstate &= 0x80 | 0x40;
    oldserbits |= newstate;
    serial_status_debug ("write");

    return oldserbits;
=======
    if (serdev == 1) {
		if (outlast) {
		    if (sd != 0) {
				write (sd, outbuf, outlast);
		    }
		}
		outlast = 0;
    } else {
		outlast = 0;
		inptr = 0;
		inlast = 0;
    }
}

int serial_readstatus(void)
{
    int status = 0;

#ifdef POSIX_SERIAL
    ioctl (sd, TIOCMGET, &status);

    if (status & TIOCM_CAR) {
		if (!carrier) {
		    ciabpra |= 0x20; /* Push up Carrier Detect line */
		    carrier = 1;
#if SERIALDEBUG > 0
		    write_log ("Carrier detect.\n");
#endif
		}
    } else {
		if (carrier) {
		    ciabpra &= ~0x20;
		    carrier = 0;
#if SERIALDEBUG > 0
		    write_log ("Carrier lost.\n");
#endif
		}
    }

    if (status & TIOCM_DSR) {
		if (!dsr) {
		    ciabpra |= 0x08; /* DSR ON */
		    dsr = 1;
		}
    } else {
		if (dsr) {
		    ciabpra &= ~0x08;
		    dsr = 0;
		}
    }
#endif
    return status;
    }

uae_u16 serial_writestatus (int old, int nw)
{
    if ((old & 0x80) == 0x80 && (nw & 0x80) == 0x00)
	    serial_dtr_on();
    if ((old & 0x80) == 0x00 && (nw & 0x80) == 0x80)
		serial_dtr_off();

    if ((old & 0x40) != (nw & 0x40))
		write_log ("RTS %s.\n", ((nw & 0x40) == 0x40) ? "set" : "cleared");

    if ((old & 0x10) != (nw & 0x10))
		write_log ("CTS %s.\n", ((nw & 0x10) == 0x10) ? "set" : "cleared");

    return nw; /* This value could also be changed here */
>>>>>>> p-uae/v2.1.0
}

void serial_open(void)
{
<<<<<<< HEAD
#ifdef SERIAL_PORT
    if (serdev)
	return;

    if (!openser (currprefs.sername)) {
	write_log ("SERIAL: Could not open device %s\n", currprefs.sername);
	return;
    }
    serdev = 1;
=======
    if (serdev == 1)
		return;

    if ((sd = open (currprefs.sername, O_RDWR|O_NONBLOCK|O_BINARY, 0)) < 0) {
		write_log ("Error: Could not open Device %s\n", currprefs.sername);
		return;
    }

    serdev = 1;

#ifdef POSIX_SERIAL
    if (tcgetattr (sd, &tios) < 0) {		/* Initialize Serial tty */
		write_log ("Serial: TCGETATTR failed\n");
		return;
    }
    cfmakeraw (&tios);

#ifndef MODEMTEST
    tios.c_cflag &= ~CRTSCTS; /* Disable RTS/CTS */
#else
    tios.c_cflag |= CRTSCTS; /* Enabled for testing modems */
#endif

    if (tcsetattr (sd, TCSADRAIN, &tios) < 0) {
		write_log ("Serial: TCSETATTR failed\n");
		return;
    }
>>>>>>> p-uae/v2.1.0
#endif
}

void serial_close (void)
{
<<<<<<< HEAD
#ifdef SERIAL_PORT
    closeser ();
    serdev = 0;
#endif
=======
    if (sd >= 0)
		close (sd);
    serdev = 0;
>>>>>>> p-uae/v2.1.0
}

void serial_init (void)
{
#ifdef SERIAL_PORT
    if (!currprefs.use_serial)
<<<<<<< HEAD
	return;

    if (!currprefs.serial_demand)
	serial_open ();

#endif
=======
		return;

    if (!currprefs.serial_demand)
		serial_open ();
#endif

    serdat = 0x2000;
    return;
>>>>>>> p-uae/v2.1.0
}

void serial_exit (void)
{
#ifdef SERIAL_PORT
    serial_close ();	/* serial_close can always be called because it	*/
#endif
    dtr = 0;		/* just closes *opened* filehandles which is ok	*/
<<<<<<< HEAD
    oldserbits = 0;	/* when exiting.				*/
}

void serial_uartbreak (int v)
{
#ifdef SERIAL_PORT
    serialuartbreak (v);
#endif
=======
    return;		/* when exiting.				*/
>>>>>>> p-uae/v2.1.0
}
