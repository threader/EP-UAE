 /*
  * UAE - The Un*x Amiga Emulator
  *
  * Support for SDL sound
  *
  * Copyright 1997 Bernd Schmidt
  * Copyright 2003-2006 Richard Drummond
<<<<<<< HEAD
=======
  * Copyright 2009-2010 Mustafa TUFAN
>>>>>>> p-uae/v2.1.0
  */

#include "sysconfig.h"
#include "sysdeps.h"

#include "options.h"
<<<<<<< HEAD
=======
#include "audio.h"
#include "memory.h"
#include "events.h"
#include "custom.h"
#include "gui.h"
>>>>>>> p-uae/v2.1.0
#include "gensound.h"
#include "sounddep/sound.h"
#include "threaddep/thread.h"
#include <SDL_audio.h>

<<<<<<< HEAD
static int have_sound = 0;

uae_u16 sndbuffer[44100];
uae_u16 *sndbufpt;
int sndbufsize;
=======
int have_sound = 0;
static int statuscnt;

uae_u16 paula_sndbuffer[44100];
uae_u16 *paula_sndbufpt;
int paula_sndbufsize;
>>>>>>> p-uae/v2.1.0
static SDL_AudioSpec spec;

static smp_comm_pipe to_sound_pipe;
static uae_sem_t data_available_sem, callback_done_sem, sound_init_sem;

static int in_callback, closing_sound;

static void clearbuffer (void)
{
<<<<<<< HEAD
    memset (sndbuffer, (spec.format == AUDIO_U8) ? SOUND8_BASE_VAL : SOUND16_BASE_VAL, sizeof (sndbuffer));
=======
    memset (paula_sndbuffer, 0, sizeof (paula_sndbuffer));
>>>>>>> p-uae/v2.1.0
}

/* This shouldn't be necessary . . . */
static void dummy_callback (void *userdata, Uint8 *stream, int len)
{
  return;
}

static void sound_callback (void *userdata, Uint8 *stream, int len)
{
    if (closing_sound)
<<<<<<< HEAD
	return;
    in_callback = 1;
    /* Wait for data to finish.  */
    uae_sem_wait (&data_available_sem);
    if (! closing_sound) {
	memcpy (stream, sndbuffer, sndbufsize);
	/* Notify writer that we're done.  */
	uae_sem_post (&callback_done_sem);
    }
=======
		return;
    in_callback = 1;

    /* Wait for data to finish.  */
    uae_sem_wait (&data_available_sem);

    if (!closing_sound) {
		memcpy (stream, paula_sndbuffer, paula_sndbufsize);

		/* Notify writer that we're done.  */
		uae_sem_post (&callback_done_sem);
    }

>>>>>>> p-uae/v2.1.0
    in_callback = 0;
}

void finish_sound_buffer (void)
{
<<<<<<< HEAD
=======
	if (currprefs.turbo_emulation)
		return;
#ifdef DRIVESOUND
    driveclick_mix ((uae_s16*)paula_sndbuffer, paula_sndbufsize / 2);
#endif
	if (!have_sound)
		return;
	if (statuscnt > 0) {
		statuscnt--;
		if (statuscnt == 0)
			gui_data.sndbuf_status = 0;
	}
	if (gui_data.sndbuf_status == 3)
		gui_data.sndbuf_status = 0;
>>>>>>> p-uae/v2.1.0
    uae_sem_post (&data_available_sem);
    uae_sem_wait (&callback_done_sem);
}

/* Try to determine whether sound is available. */
int setup_sound (void)
{
    int success = 0;

    if (SDL_InitSubSystem (SDL_INIT_AUDIO) == 0) {
<<<<<<< HEAD
	spec.freq = currprefs.sound_freq;
	spec.format = currprefs.sound_bits == 8 ? AUDIO_U8 : AUDIO_S16SYS;
	spec.channels = currprefs.sound_stereo ? 2 : 1;
	spec.callback = dummy_callback;
	spec.samples  = spec.freq * currprefs.sound_latency / 1000;
	spec.callback = sound_callback;
	spec.userdata = 0;

	if (SDL_OpenAudio (&spec, 0) < 0) {
	    write_log ("Couldn't open audio: %s\n", SDL_GetError());
	    SDL_QuitSubSystem (SDL_INIT_AUDIO);
	} else {
	    success = 1;
	    SDL_CloseAudio ();
	}
=======
		spec.freq = currprefs.sound_freq;
		spec.format = AUDIO_S16SYS;
		spec.channels = currprefs.sound_stereo ? 2 : 1;
		spec.callback = dummy_callback;
		spec.samples  = spec.freq * currprefs.sound_latency / 1000;
		spec.callback = sound_callback;
		spec.userdata = 0;

		if (SDL_OpenAudio (&spec, 0) < 0) {
		    write_log ("SDL: Couldn't open audio: %s\n", SDL_GetError());
		    SDL_QuitSubSystem (SDL_INIT_AUDIO);
		} else {
		    success = 1;
		    SDL_CloseAudio ();
		}
>>>>>>> p-uae/v2.1.0
    }

    sound_available = success;

    return sound_available;
}

static int open_sound (void)
{
<<<<<<< HEAD
    spec.freq = currprefs.sound_freq;
    spec.format = currprefs.sound_bits == 8 ? AUDIO_U8 : AUDIO_S16SYS;
=======
	if (!currprefs.produce_sound)
		return 0;
	config_changed = 1;

    spec.freq = currprefs.sound_freq;
    spec.format = AUDIO_S16SYS;
>>>>>>> p-uae/v2.1.0
    spec.channels = currprefs.sound_stereo ? 2 : 1;
    spec.samples  = spec.freq * currprefs.sound_latency / 1000;
    spec.callback = sound_callback;
    spec.userdata = 0;

    clearbuffer();
    if (SDL_OpenAudio (&spec, NULL) < 0) {
<<<<<<< HEAD
	write_log ("Couldn't open audio: %s\n", SDL_GetError());
	return 0;
    }

    if (spec.format == AUDIO_S16SYS) {
	init_sound_table16 ();
	sample_handler = currprefs.sound_stereo ? sample16s_handler : sample16_handler;
    } else {
	init_sound_table8 ();
	sample_handler = currprefs.sound_stereo ? sample8s_handler : sample8_handler;
    }
    have_sound = 1;

    sound_available = 1;
    obtainedfreq = currprefs.sound_freq;
    sndbufsize = spec.samples * currprefs.sound_bits / 8 * spec.channels;
    write_log ("SDL sound driver found and configured for %d bits at %d Hz, buffer is %d ms (%d bytes).\n",
	currprefs.sound_bits, spec.freq, spec.samples * 1000 / spec.freq, sndbufsize);
   sndbufpt = sndbuffer;
=======
		write_log ("SDL: Couldn't open audio: %s\n", SDL_GetError());
		return 0;
    }

    init_sound_table16 ();
    sample_handler = currprefs.sound_stereo ? sample16s_handler : sample16_handler;

    obtainedfreq = currprefs.sound_freq;
    write_log ("SDL: sound driver found and configured at %d Hz, buffer is %d ms (%d bytes).\n", spec.freq, spec.samples * 1000 / spec.freq, paula_sndbufsize);

    have_sound = 1;
    sound_available = 1;
	update_sound (fake_vblank_hz, 1, currprefs.ntscmode);
	paula_sndbufsize = spec.samples * 2 * spec.channels;
	paula_sndbufpt = paula_sndbuffer;
#ifdef DRIVESOUND
	driveclick_init();
#endif
>>>>>>> p-uae/v2.1.0

    return 1;
}

static void *sound_thread (void *dummy)
{
    for (;;) {
<<<<<<< HEAD
	int cmd = read_comm_pipe_int_blocking (&to_sound_pipe);
	int n;

	switch (cmd) {
	case 0:
	    open_sound ();
	    uae_sem_post (&sound_init_sem);
	    break;
	case 1:
	    uae_sem_post (&sound_init_sem);
	    return 0;
	}
    }
=======
		int cmd = read_comm_pipe_int_blocking (&to_sound_pipe);

		switch (cmd) {
			case 0:
			    open_sound ();
			    uae_sem_post (&sound_init_sem);
			    break;
			case 1:
			    uae_sem_post (&sound_init_sem);
			    return 0;
			}
    	}
>>>>>>> p-uae/v2.1.0
}

/* We need a thread for this, since communication between finish_sound_buffer
 * and the callback works through semaphores.  In theory, this is unnecessary,
 * since SDL uses a sound thread internally, and the callback runs in its
 * context.  But we don't want to depend on SDL's internals too much.  */
static void init_sound_thread (void)
{
    uae_thread_id tid;

    init_comm_pipe (&to_sound_pipe, 20, 1);
    uae_sem_init (&data_available_sem, 0, 0);
    uae_sem_init (&callback_done_sem, 0, 0);
    uae_sem_init (&sound_init_sem, 0, 0);
<<<<<<< HEAD
    uae_start_thread (sound_thread, NULL, &tid);
=======
    uae_start_thread ("Sound", sound_thread, NULL, &tid);
>>>>>>> p-uae/v2.1.0
}

void close_sound (void)
{
<<<<<<< HEAD
    if (! have_sound)
	return;
=======
	config_changed = 1;
	gui_data.sndbuf = 0;
	gui_data.sndbuf_status = 3;
    if (!have_sound)
		return;
>>>>>>> p-uae/v2.1.0

    SDL_PauseAudio (1);
    clearbuffer();
    if (in_callback) {
<<<<<<< HEAD
	closing_sound = 1;
	uae_sem_post (&data_available_sem);
=======
		closing_sound = 1;
		uae_sem_post (&data_available_sem);
>>>>>>> p-uae/v2.1.0
    }
    write_comm_pipe_int (&to_sound_pipe, 1, 1);
    uae_sem_wait (&sound_init_sem);
    SDL_CloseAudio ();
    uae_sem_destroy (&data_available_sem);
    uae_sem_destroy (&sound_init_sem);
    uae_sem_destroy (&callback_done_sem);
    have_sound = 0;
}

int init_sound (void)
{
<<<<<<< HEAD
=======
	gui_data.sndbuf_status = 3;
	gui_data.sndbuf = 0;
	if (!sound_available)
		return 0;
	if (currprefs.produce_sound <= 1)
		return 0;
	if (have_sound)
		return 1;

>>>>>>> p-uae/v2.1.0
    in_callback = 0;
    closing_sound = 0;

    init_sound_thread ();
    write_comm_pipe_int (&to_sound_pipe, 0, 1);
    uae_sem_wait (&sound_init_sem);
    SDL_PauseAudio (0);
<<<<<<< HEAD

=======
#ifdef DRIVECLICK
	driveclick_reset ();
#endif
>>>>>>> p-uae/v2.1.0
    return have_sound;
}

void pause_sound (void)
{
<<<<<<< HEAD
=======
	if (!have_sound)
		return;
>>>>>>> p-uae/v2.1.0
    SDL_PauseAudio (1);
}

void resume_sound (void)
{
<<<<<<< HEAD
=======
	if (!have_sound)
		return;
>>>>>>> p-uae/v2.1.0
    clearbuffer();
    SDL_PauseAudio (0);
}

void reset_sound (void)
{
   clearbuffer();
   return;
}

void sound_volume (int dir)
{
}

<<<<<<< HEAD
=======
void restart_sound_buffer (void)
{
}


>>>>>>> p-uae/v2.1.0
/*
 * Handle audio specific cfgfile options
 */
void audio_default_options (struct uae_prefs *p)
{
}

void audio_save_options (FILE *f, const struct uae_prefs *p)
{
}

int audio_parse_option (struct uae_prefs *p, const char *option, const char *value)
{
    return 0;
}
