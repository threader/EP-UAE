 /*
  * UAE - The Un*x Amiga Emulator
  *
  * Sound emulation stuff
  *
  * Copyright 1995, 1996, 1997 Bernd Schmidt
  */

#define PERIOD_MAX ULONG_MAX

extern void aud0_handler (void);
extern void aud1_handler (void);
extern void aud2_handler (void);
extern void aud3_handler (void);

extern void AUDxDAT (unsigned int nr, uae_u16 value);
extern void AUDxVOL (unsigned int nr, uae_u16 value);
extern void AUDxPER (unsigned int nr, uae_u16 value);
extern void AUDxLCH (unsigned int nr, uae_u16 value);
extern void AUDxLCL (unsigned int nr, uae_u16 value);
extern void AUDxLEN (unsigned int nr, uae_u16 value);

extern int init_audio (void);
extern void ahi_install (void);
extern void audio_reset (void);
extern void update_audio (void);
extern void schedule_audio (void);
extern void audio_evhandler (void);
extern void audio_hsync (unsigned int);
extern void audio_update_adkmasks (void);
extern void audio_update_irq (uae_u16);
extern void update_sound (unsigned int freq, int longframe, int linetoggle);
extern void led_filter_audio (void);
extern void set_audio (void);
extern int audio_activate (void);
extern void audio_vsync (void);

extern int  audio_setup (void);
extern int  audio_init (void);
extern void audio_close (void);
extern void audio_reset (void);
extern void audio_pause (void);
extern void audio_resume (void);
extern void audio_volume (int);

void switch_audio_interpol (void);

extern int sound_available;

extern void audio_sampleripper(int);
extern int sampleripper_enabled;
//extern void write_wavheader (FILE *wavfile, uae_u32 size, uae_u32 freq);

enum {
    SND_MONO, SND_STEREO, SND_4CH_CLONEDSTEREO, SND_4CH, SND_6CH_CLONEDSTEREO, SND_6CH, SND_NONE };
STATIC_INLINE int get_audio_stereomode (int channels)
{
    switch (channels)
    {
	case 1:
		return SND_MONO;
	case 2:
		return SND_STEREO;
	case 4:
		return SND_4CH;
	case 6:
		return SND_6CH;
    }
    return SND_STEREO;
}
STATIC_INLINE int get_audio_nativechannels (int stereomode)
{
    int ch[] = { 1, 2, 4, 4, 6, 6, 0 };
    return ch[stereomode];
}
STATIC_INLINE int get_audio_amigachannels (int stereomode)
{
    int ch[] = { 1, 2, 2, 4, 2, 4, 0 };
    return ch[stereomode];
}
STATIC_INLINE int get_audio_ismono (int stereomode)
{
    if (stereomode == 0)
		return 1;
    return 0;
}

#define SOUND_MAX_DELAY_BUFFER 1024
#define SOUND_MAX_LOG_DELAY 10
#define MIXED_STEREO_MAX 16
#define MIXED_STEREO_SCALE 32
