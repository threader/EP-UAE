 /*
  * UAE - The Un*x Amiga Emulator
  *
  * Autoconfig device support
  *
  * (c) 1996 Ed Hanway
  */

#define RTAREA_DEFAULT 0xf00000
#define RTAREA_BACKUP  0xef0000

extern uae_u32 addr (int);
extern void db (uae_u8);
extern void dw (uae_u16);
extern void dl (uae_u32);
extern uae_u32 ds_ansi (const uae_char*);
extern uae_u32 ds (const char *);
extern uae_u8 dbg (uaecptr);
extern void calltrap (uae_u32);
extern void org (uae_u32);
extern uae_u32 here (void);
extern uaecptr makedatatable (uaecptr resid, uaecptr resname, uae_u8 type, uae_s8 priority, uae_u16 ver, uae_u16 rev);

extern void align (int);

extern volatile int uae_int_requested;
extern void set_uae_int_flag (void);

#define RTS 0x4e75
#define RTE 0x4e73

extern uaecptr EXPANSION_explibname, EXPANSION_doslibname, EXPANSION_uaeversion;
extern uaecptr EXPANSION_explibbase, EXPANSION_uaedevname, EXPANSION_haveV36;
extern uaecptr EXPANSION_bootcode, EXPANSION_nullfunc;

extern uaecptr ROM_filesys_resname, ROM_filesys_resid;
extern uaecptr ROM_filesys_diagentry;
extern uaecptr ROM_hardfile_resname, ROM_hardfile_resid;
extern uaecptr ROM_hardfile_init;
extern uaecptr filesys_initcode;

extern int nr_directory_units (struct uaedev_mount_info *mountinfo, struct uae_prefs*);
extern uaecptr need_uae_boot_rom (void);

struct mountedinfo
{
    uae_u64 size;
    int ismounted;
    int ismedia;
    int nrcyls;
};
/* note */
extern int add_filesys_unitconfig (struct uae_prefs *p, int index, char *error);
extern int get_filesys_unitconfig (struct uae_prefs *p, int index, struct mountedinfo*);
extern int kill_filesys_unitconfig (struct uae_prefs *p, int nr);
extern int move_filesys_unitconfig (struct uae_prefs *p, int nr, int to);
/* end note */
extern char *validatedevicename (char *s);
extern char *validatevolumename (char *s);

int filesys_insert(struct uaedev_mount_info *mountinfo, int nr, char *volume, const char *rootdir, int readonly, int flags);
int filesys_eject(struct uaedev_mount_info *mountinfo, int nr);
int filesys_media_change (struct uaedev_mount_info *mountinfo, const char *rootdir, int inserted, struct uaedev_config_info *uci);

extern char *filesys_createvolname (const char *volname, const char *rootdir, const char *def);
extern int target_get_volume_name(struct uaedev_mount_info *mtinf, const char *volumepath, char *volumename, int size, int inserted, int fullcheck);

extern void filesys_reset (void);
extern void filesys_cleanup (void);
extern void filesys_prepare_reset (void);
extern void filesys_start_threads (void);
extern void filesys_flush_cache (void);
extern void filesys_free_handles (void);
extern void filesys_vsync (struct uaedev_mount_info *mountinfo);

extern void filesys_install (void);
extern void filesys_install_code (void);
extern void filesys_store_devinfo (uae_u8 *);
extern void hardfile_install (void);
extern void hardfile_reset (void);
extern void emulib_install (void);
extern void expansion_init (void);
extern void expansion_cleanup (void);

extern uae_u8 *rtarea;

#define RTAREA_BASE 0xF00000
extern void expansion_clear (void);