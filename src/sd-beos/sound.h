 /*
  * UAE - The Un*x Amiga Emulator
  *
  * Support for BeOS sound
  *
  * Copyright 1996, 1997 Christian Bauer
<<<<<<< HEAD
  * Copyright 2003-2004 Richard Drummond
  */

extern uae_u16 *sndbuffer;
extern uae_u16 *sndbufpt;
extern int sndbufsize;
=======
  * Copyright 2003-2007 Richard Drummond
  */

#define SOUNDSTUFF 1

extern uae_u16 *paula_sndbuffer;
extern uae_u16 *paula_sndbufpt;
extern int paula_sndbufsize;
>>>>>>> p-uae/v2.1.0
extern void finish_sound_buffer (void);

STATIC_INLINE void check_sound_buffers (void)
{
<<<<<<< HEAD
    if ((char *)sndbufpt - (char *)sndbuffer >= sndbufsize) {
    	finish_sound_buffer ();
=======
    if ((char *)paula_sndbufpt - (char *)paula_sndbuffer >= paula_sndbufsize) {
	finish_sound_buffer ();
>>>>>>> p-uae/v2.1.0
    }
}

#define AUDIO_NAME "beos"

<<<<<<< HEAD
#define PUT_SOUND_BYTE(b) do { *(uae_u8 *)sndbufpt = b; sndbufpt = (uae_u16 *)(((uae_u8 *)sndbufpt) + 1); } while (0)
#define PUT_SOUND_WORD(b) do { *(uae_u16 *)sndbufpt = b; sndbufpt = (uae_u16 *)(((uae_u8 *)sndbufpt) + 2); } while (0)
=======
#define PUT_SOUND_BYTE(b) do { *(uae_u8 *)paula_sndbufpt = b; paula_sndbufpt = (uae_u16 *)(((uae_u8 *)paula_sndbufpt) + 1); } while (0)
#define PUT_SOUND_WORD(b) do { *(uae_u16 *)paula_sndbufpt = b; paula_sndbufpt = (uae_u16 *)(((uae_u8 *)paula_sndbufpt) + 2); } while (0)
>>>>>>> p-uae/v2.1.0
#define PUT_SOUND_BYTE_LEFT(b) PUT_SOUND_BYTE(b)
#define PUT_SOUND_WORD_LEFT(b) PUT_SOUND_WORD(b)
#define PUT_SOUND_BYTE_RIGHT(b) PUT_SOUND_BYTE(b)
#define PUT_SOUND_WORD_RIGHT(b) PUT_SOUND_WORD(b)
#define PUT_SOUND_WORD_MONO(b) PUT_SOUND_WORD_LEFT(b)
#define SOUND16_BASE_VAL 0
<<<<<<< HEAD
#define SOUND8_BASE_VAL 128

#define DEFAULT_SOUND_MAXB 8192
#define DEFAULT_SOUND_MINB 8192
#define DEFAULT_SOUND_BITS 16
#define DEFAULT_SOUND_FREQ 44100
#define DEFAULT_SOUND_LATENCY 50
#define HAVE_STEREO_SUPPORT
#define HAVE_8BIT_AUDIO_SUPPORT
=======

#define DEFAULT_SOUND_FREQ 44100
#define DEFAULT_SOUND_LATENCY 100
#define HAVE_STEREO_SUPPORT
>>>>>>> p-uae/v2.1.0
