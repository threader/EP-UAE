 /*
  * UAE - The Un*x Amiga Emulator
  *
  * Save/restore emulator state
  *
  * (c) 1999-2001 Toni Wilen
  */


/* functions to save byte,word or long word
 * independent of CPU's endianess */

extern void save_u64_func (uae_u8 **, uae_u64);
extern void save_u32_func (uae_u8 **, uae_u32);
extern void save_u16_func (uae_u8 **, uae_u16);
extern void save_u8_func (uae_u8 **, uae_u8);

extern uae_u64 restore_u64_func (const uae_u8 **);
extern uae_u32 restore_u32_func (const uae_u8 **);
extern uae_u16 restore_u16_func (const uae_u8 **);
extern uae_u8  restore_u8_func  (const uae_u8 **);

extern void save_string_func (uae_u8 **, const char *);
extern char *restore_string_func (const uae_u8 **);

#define save_u64(x) save_u64_func (&dst, (x))
#define save_u32(x) save_u32_func (&dst, (x))
#define save_u16(x) save_u16_func (&dst, (x))
#define save_u8(x)  save_u8_func (&dst, (x))

#define restore_u64() restore_u64_func (&src)
#define restore_u32() restore_u32_func (&src)
#define restore_u16() restore_u16_func (&src)
#define restore_u8()  restore_u8_func (&src)

#define save_string(x) save_string_func (&dst, (x))
#define restore_string() restore_string_func (&src)

#define STATE_SAVE 1
#define STATE_RESTORE 2
#define STATE_DOSAVE 4
#define STATE_DORESTORE 8
#define STATE_REWIND 16
#define STATE_DOREWIND 32

#ifdef SAVESTATE
/* save, restore and initialize routines for Amiga's subsystems */

extern const uae_u8 *restore_cpu (const uae_u8 *src);
extern       void    restore_cpu_finish (void);
extern       uae_u8 *save_cpu    (uae_u32 *len, uae_u8 *dstpr);

extern uae_u8 *restore_mmu (uae_u8 *);
extern uae_u8 *save_mmu (int *, uae_u8 *);

extern const uae_u8 *restore_fpu (const uae_u8 *src);
extern       uae_u8 *save_fpu    (uae_u32 *len, uae_u8 *dsptr);

extern const uae_u8 *restore_disk (unsigned int num, const uae_u8 *src);
extern       uae_u8 *save_disk    (unsigned int num, uae_u32 *len, uae_u8 *dsptr);

extern const uae_u8 *restore_floppy (const uae_u8 *src);
extern       uae_u8 *save_floppy    (uae_u32 *len, uae_u8 *dstptr);

extern void DISK_save_custom     (uae_u32 *pdskpt, uae_u16 *pdsklen, uae_u16 *pdsksync, uae_u16 *pdskbytr);
extern void DISK_restore_custom  (uae_u32 pdskpt, uae_u16 pdsklength, uae_u16 pdskbytr);

extern const uae_u8 *restore_custom (const uae_u8 *src);
extern       uae_u8 *save_custom    (uae_u32 *len, uae_u8 *dstptr, int full);

extern const uae_u8 *restore_custom_sprite (unsigned int num, const uae_u8 *src);
extern       uae_u8 *save_custom_sprite    (unsigned int num, uae_u32 *len, uae_u8 *dstptr);

extern const uae_u8 *restore_custom_agacolors (const uae_u8 *src);
extern       uae_u8 *save_custom_agacolors    (uae_u32 *len, uae_u8 *dstptr);

extern const uae_u8 *restore_blitter (const uae_u8 *src);
extern       uae_u8 *save_blitter    (uae_u32 *len, uae_u8 *dstptr);
extern       void    restore_blitter_finish (void);

extern const uae_u8 *restore_audio (unsigned int channel, const uae_u8 *src);
extern       uae_u8 *save_audio    (unsigned int channel, uae_u32 *len, uae_u8 *dstptr);

extern const uae_u8 *restore_cia (unsigned int num, const uae_u8 *src);
extern       uae_u8 *save_cia    (unsigned int num, uae_u32 *len, uae_u8 *dstptr);

extern const uae_u8 *restore_expansion (const uae_u8 *src);
extern       uae_u8 *save_expansion    (uae_u32 *len, uae_u8 *dstptr);

extern const uae_u8 *restore_p96 (uae_u8 *);
extern uae_u8 *save_p96 (int *, uae_u8 *);
extern void restore_p96_finish (void);

extern const uae_u8 *restore_keyboard (const uae_u8 *src);
extern       uae_u8 *save_keyboard    (uae_u32 *len);

extern const uae_u8 *restore_akiko (const uae_u8 *src);
extern uae_u8 *save_akiko (int *len, uae_u8*);
extern void restore_akiko_finish (void);

extern uae_u8 *restore_cdtv (uae_u8 *src);
extern uae_u8 *save_cdtv (int *len, uae_u8*);
extern void restore_cdtv_finish (void);

extern uae_u8 *restore_cdtv_dmac (uae_u8 *src);
extern uae_u8 *save_cdtv_dmac (int *len, uae_u8*);
extern uae_u8 *restore_scsi_dmac (int wdtype, uae_u8 *src);
extern uae_u8 *save_scsi_dmac (int wdtype, int *len, uae_u8*);

extern uae_u8 *save_scsi_device (int wdtype, int num, int *len, uae_u8 *dstptr);
extern uae_u8 *restore_scsi_device (int wdtype, uae_u8 *src);

extern uae_u8 *save_scsidev (int num, int *len, uae_u8 *dstptr);
extern uae_u8 *restore_scsidev (uae_u8 *src);

extern uae_u8 *restore_filesys (struct uaedev_mount_info *mountinfo, uae_u8 *src);
extern uae_u8 *save_filesys (int num, int *len);
extern uae_u8 *restore_filesys_common (uae_u8 *src);
extern uae_u8 *save_filesys_common (int *len);
extern int save_filesys_cando(void);

extern uae_u8 *restore_gayle(uae_u8 *src);
extern uae_u8 *save_gayle (int *len);
extern uae_u8 *restore_ide (uae_u8 *src);
extern uae_u8 *save_ide (int num, int *len);

extern uae_u8 *save_cd (int num, int *len);
extern uae_u8 *restore_cd (int, uae_u8 *src);
extern void restore_cd_finish (void);

extern uae_u8 *save_configuration (int *len);
extern uae_u8 *restore_configuration (uae_u8 *src);
extern uae_u8 *save_log (int, int *len);
extern uae_u8 *restore_log (uae_u8 *src);

extern void restore_cram (uae_u32, size_t);
extern void restore_bram (uae_u32, size_t);
extern void restore_fram (uae_u32, size_t);
extern void restore_zram (uae_u32, size_t, int);
extern void restore_pram (uae_u32, size_t);
extern void restore_ram  (size_t, uae_u8*);
extern void restore_a3000lram (uae_u32, size_t);
extern void restore_a3000hram (uae_u32, size_t);

extern uae_u8 *save_cram (uae_u32 *);
extern uae_u8 *save_bram (uae_u32 *);
extern uae_u8 *save_fram (uae_u32 *);
extern uae_u8 *save_zram (uae_u32 *, int);
extern uae_u8 *save_bootrom (uae_u32 *);
extern uae_u8 *save_pram (uae_u32 *);
extern uae_u8 *save_a3000lram (uae_u32 *);
extern uae_u8 *save_a3000hram (uae_u32 *);

extern const uae_u8 *restore_rom (const uae_u8 *src);
extern       uae_u8 *save_rom    (int first, uae_u32 *len, uae_u8 *dstptr);

extern void restore_bootrom (uae_u32, size_t);

extern const uae_u8 *restore_action_replay (const uae_u8 *);
extern       uae_u8 *save_action_replay    (uae_u32 *, uae_u8 *);
extern uae_u8 *restore_hrtmon (uae_u8 *);
extern uae_u8 *save_hrtmon (int *, uae_u8 *);

extern void savestate_initsave (const char *filename, int docompress, int nodialogs);
extern int save_state (const char *filename, const char *description);

extern void restore_state (const char *filename);
extern void savestate_restore_finish (void);

extern void custom_save_state (void);
extern void custom_prepare_savestate (void);

#define STATE_SAVE 1
#define STATE_RESTORE 2
#define STATE_DOSAVE 4
#define STATE_DORESTORE 8
#define STATE_REWIND 16
#define STATE_DOREWIND 32

extern int savestate_state;
extern char savestate_fname[MAX_DPATH];
extern struct zfile *savestate_file;

extern void savestate_quick (int slot, int save);

extern void savestate_capture (int);
extern void savestate_free (void);
extern void savestate_init (void);
extern void savestate_rewind (void);
extern int savestate_dorewind (int);
extern void savestate_listrewind (void);

#else

#define savestate_state 0

#endif

