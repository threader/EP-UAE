 /*
  * UAE - The Un*x Amiga Emulator
  *
  * Unix file system handler for AmigaDOS
  *
  * Copyright 1997 Bernd Schmidt
  */

struct hardfilehandle;
struct mountedinfo;
struct uaedev_config_info;
struct uae_prefs;

struct hardfiledata {
    uae_u64 size;
    uae_u64 virtsize; // virtual size
    uae_u64 physsize; // physical size (dynamic disk)
    uae_u64 offset;
    int nrcyls;
    int secspertrack;
    int surfaces;
    int reservedblocks;
    unsigned int blocksize;
    struct hardfilehandle *handle;
    int handle_valid;
    int readonly;
    int dangerous;
    int flags;
    uae_u8 *cache;
    int cache_valid;
    uae_u64 cache_offset;
    char vendor_id[8 + 1];
    char product_id[16 + 1];
    char product_rev[4 + 1];
    char device_name[256];
    /* geometry from possible RDSK block */
    unsigned int cylinders;
    unsigned int sectors;
    unsigned int heads;
    uae_u64 size2;
    uae_u64 offset2;
    uae_u8 *virtual_rdb;
    uae_u64 virtual_size;
    int unitnum;
    int byteswap;
    int adide;

    uae_u8 *vhd_header;
    uae_u32 vhd_bamoffset;
    uae_u32 vhd_bamsize;
    uae_u32 vhd_blocksize;
    uae_u32 vhd_type;
    uae_u8 *vhd_sectormap;
    uae_u64 vhd_sectormapblock;
    uae_u32 vhd_bitmapsize;
    uae_u64 vhd_footerblock;

    int drive_empty;
    char *emptyname;
};

#define HFD_FLAGS_REALDRIVE 1

struct hd_hardfiledata {
    struct hardfiledata hfd;
    int bootpri;
    uae_u64 size;
    unsigned int cyls;
    unsigned int heads;
    unsigned int secspertrack;
    unsigned int cyls_def;
    unsigned int secspertrack_def;
    unsigned int heads_def;
    char *path;
    int ansi_version;
};

#define HD_CONTROLLER_UAE 0
#define HD_CONTROLLER_IDE0 1
#define HD_CONTROLLER_IDE1 2
#define HD_CONTROLLER_IDE2 3
#define HD_CONTROLLER_IDE3 4
#define HD_CONTROLLER_SCSI0 5
#define HD_CONTROLLER_SCSI1 6
#define HD_CONTROLLER_SCSI2 7
#define HD_CONTROLLER_SCSI3 8
#define HD_CONTROLLER_SCSI4 9
#define HD_CONTROLLER_SCSI5 10
#define HD_CONTROLLER_SCSI6 11
#define HD_CONTROLLER_PCMCIA_SRAM 12
#define HD_CONTROLLER_PCMCIA_IDE 13

#define HD_CONTROLLER_TYPE_UAE 0
#define HD_CONTROLLER_TYPE_IDE_AUTO 1
#define HD_CONTROLLER_TYPE_IDE_MB 1
#define HD_CONTROLLER_TYPE_SCSI_AUTO 2
#define HD_CONTROLLER_TYPE_SCSI_A2091 3
#define HD_CONTROLLER_TYPE_SCSI_A2091_2 4
#define HD_CONTROLLER_TYPE_SCSI_A4091 5
#define HD_CONTROLLER_TYPE_SCSI_A4091_2 6
#define HD_CONTROLLER_TYPE_SCSI_A3000 7
#define HD_CONTROLLER_TYPE_SCSI_A4000T 8
#define HD_CONTROLLER_TYPE_SCSI_CDTV 9
#define HD_CONTROLLER_TYPE_PCMCIA_SRAM 10
#define HD_CONTROLLER_TYPE_PCMCIA_IDE 11


#define HD_CONTROLLER_TYPE_IDE_FIRST 1
#define HD_CONTROLLER_TYPE_IDE_LAST 1
#define HD_CONTROLLER_TYPE_SCSI_FIRST 2
#define HD_CONTROLLER_TYPE_SCSI_LAST 9

#define FILESYS_VIRTUAL 0
#define FILESYS_HARDFILE 1
#define FILESYS_HARDFILE_RDB 2
#define FILESYS_HARDDRIVE 3

#define FILESYS_FLAG_DONOTSAVE 1

#define MAX_FILESYSTEM_UNITS 30

struct uaedev_mount_info;
extern struct uaedev_mount_info options_mountinfo;
extern void free_mountinfo (struct uaedev_mount_info *);

extern int nr_units (struct uaedev_mount_info *mountinfo);
extern int is_hardfile (struct uaedev_mount_info *mountinfo, int unit_no);
extern int *set_filesys_unit (struct uaedev_mount_info *mountinfo, int,
				     const char *devname, const char *volname, const char *rootdir,
				     int readonly, int secs, int surfaces, int reserved,
				     int blocksize, int bootpri, int donotmount, int autoboot,
                     const char *filesysdir, int hdc, int flags);
extern int *add_filesys_unit (struct uaedev_mount_info *mountinfo,
				     const char *devname, const char *volname, const char *rootdir,
				     int readonly, int secs, int surfaces, int reserved,
				     int blocksize, int bootpri, int donotmount, int autoboot,
                     const char *filesysdir, int hdc, int flags);
extern int *get_filesys_unit (struct uaedev_mount_info *mountinfo, int nr,
				     char **devname, char **volame, char **rootdir, int *readonly,
				     int *secspertrack, int *surfaces, int *reserved,
				     int *cylinders, uae_u64 *size, int *blocksize, int *bootpri,
				     char **filesysdir, int *flags);
extern int kill_filesys_unit (struct uaedev_mount_info *mountinfo, int);
extern int move_filesys_unit (struct uaedev_mount_info *mountinfo, int nr, int to);
extern int sprintf_filesys_unit (const struct uaedev_mount_info *mountinfo, char *buffer, int num);
//extern void write_filesys_config (const struct uaedev_mount_info *mountinfo, const char *unexpanded,
//				  const char *defaultpath, FILE *f);


extern void filesys_init (void);
extern void filesys_reset (void);
extern void filesys_cleanup (void);
extern void filesys_prepare_reset (void);
extern void filesys_start_threads (void);
extern void filesys_flush_cache (void);

extern struct hardfiledata *get_hardfile_data (int nr);
#define FILESYS_MAX_BLOCKSIZE 2048
 int hdf_open (struct hardfiledata *hfd, const TCHAR *name);
 int hdf_dup (struct hardfiledata *dhfd, const struct hardfiledata *shfd);
 void hdf_close (struct hardfiledata *hfd);
 int hdf_read (struct hardfiledata *hfd, void *buffer, uae_u64 offset, int len);
 int hdf_write (struct hardfiledata *hfd, void *buffer, uae_u64 offset, int len);
 int hdf_getnumharddrives (void);
 char *hdf_getnameharddrive (int index, int flags, int *sectorsize, int *dangerousdrive);
 int isspecialdrive (const char *name);
 void filesys_cleanup (void);
 int filesys_is_readonly (const char *path);
 int hdf_init (void);
 int get_native_path(struct uaedev_mount_info *mountinfo, uae_u32 lock, char *out);
 void hardfile_do_disk_change (struct uaedev_config_info *uci, int insert);

void hdf_hd_close(struct hd_hardfiledata *hfd);
int hdf_hd_open(struct hd_hardfiledata *hfd, const TCHAR *path, int blocksize, int readonly,
		       const TCHAR *devname, int sectors, int surfaces, int reserved,
		       int bootpri, const TCHAR *filesys);


 int vhd_create (const TCHAR *name, uae_u64 size, uae_u32);

 int hdf_init_target (void);
 int hdf_open_target (struct hardfiledata *hfd, const TCHAR *name);
 int hdf_dup_target (struct hardfiledata *dhfd, const struct hardfiledata *shfd);
 void hdf_close_target (struct hardfiledata *hfd);
 int hdf_read_target (struct hardfiledata *hfd, void *buffer, uae_u64 offset, int len);
 int hdf_write_target (struct hardfiledata *hfd, void *buffer, uae_u64 offset, int len);
 int hdf_resize_target (struct hardfiledata *hfd, uae_u64 newsize);
 void getchsgeometry (uae_u64 size, int *pcyl, int *phead, int *psectorspertrack);

// REMOVEME:
#if 0
int hardfile_remount (int nr);
#endif // 0
