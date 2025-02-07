 /*
  * UAE - The Un*x Amiga Emulator
  *
  * routines to handle compressed file automatically
  *
  * (c) 1996 Samuel Devulder
  */
#pragma once
#ifndef ZFILE_H
#define ZFILE_H

struct zfile;
extern int is_zlib;

//extern struct zfile *zfile_fopen (const char *, const char *);
//extern struct zfile *zfile_fopen_empty (const char *name, int size);
extern int zfile_exists (const char *name);
extern void zfile_fclose (struct zfile *);
//extern int zfile_fseek (struct zfile *z, long offset, int mode);
//extern long zfile_ftell (struct zfile *z);
extern size_t zfile_fread  (void *b, size_t l1, size_t l2, struct zfile *z);
extern size_t zfile_fwrite  (const void *b, size_t l1, size_t l2, struct zfile *z);
extern void zfile_exit (void);
extern int execute_command (char *);

struct zvolume;
struct zdirectory;
//#include "fsdb.h"
//#include "zarchive.h"
//#include "fsusage.h"

#define FSDB_DIR_SEPARATOR_S "/"

#define FS_DIRECTORY 0
#define FS_ARCHIVE 1
#define FS_CDFS 2

struct fs_dirhandle
{
	int isarch;
	union {
		struct zdirectory *zd;
		struct my_opendir_s *od;
	};
};
struct fs_filehandle
{
	int isarch;
	union {
		struct zfile *zf;
		struct my_openfile_s *of;
	};
};
extern int is_zlib;

typedef int (*zfile_callback)(struct zfile*, void*);

extern struct zfile *zfile_fopen (const TCHAR *, const TCHAR *, int mask);
extern struct zfile *zfile_fopen2 (const TCHAR *, const TCHAR *, int mask, int index);
extern struct zfile *zfile_fopen_empty (struct zfile*, const TCHAR *name, uae_u64 size);
extern struct zfile *zfile_fopen_data (const TCHAR *name, uae_u64 size, uae_u8 *data);
extern struct zfile *zfile_fopen_parent (struct zfile*, const TCHAR*, uae_u64 offset, uae_u64 size);

extern int zfile_exists (const TCHAR *name);
extern void zfile_fclose (struct zfile *);
extern uae_s64 zfile_fseek (struct zfile *z, uae_s64 offset, int mode);
extern uae_s64 zfile_ftell (struct zfile *z);
extern uae_s64 zfile_size (struct zfile *z);
extern size_t zfile_fread  (void *b, size_t l1, size_t l2, struct zfile *z);
extern size_t zfile_fwrite  (const void *b, size_t l1, size_t l2, struct zfile *z);
extern TCHAR *zfile_fgets (TCHAR *s, int size, struct zfile *z);
extern char *zfile_fgetsa (char *s, int size, struct zfile *z);
extern size_t zfile_fputs (struct zfile *z, TCHAR *s);
extern int zfile_getc (struct zfile *z);
extern int zfile_putc (int c, struct zfile *z);
extern int zfile_ferror (struct zfile *z);
extern uae_u8 *zfile_getdata (struct zfile *z, uae_s64 offset, int len);
extern void zfile_exit (void);
extern int execute_command (TCHAR *);

extern int zfile_iscompressed (struct zfile *z);
extern int zfile_zcompress (struct zfile *dst, void *src, int size);
extern int zfile_zuncompress (void *dst, int dstsize, struct zfile *src, int srcsize);
extern int zfile_gettype (struct zfile *z);
extern uae_u32 zfile_crc32 (struct zfile *f);
extern int zfile_zopen (const TCHAR *name, zfile_callback zc, void *user);
extern TCHAR *zfile_getname (struct zfile *f);
extern TCHAR *zfile_getfilename (struct zfile *f);
extern uae_u32 zfile_crc32 (struct zfile *f);
extern struct zfile *zfile_dup (struct zfile *f);
extern struct zfile *zfile_gunzip (struct zfile *z);
extern int zfile_is_diskimage (const TCHAR *name);
extern int iszip (struct zfile *z);
extern int zfile_convertimage (const TCHAR *src, const TCHAR *dst);
//extern struct zfile *zuncompress (struct znode*, struct zfile *z, int dodefault, int mask, int *retcode, int index);
extern struct zfile *zuncompress (struct zfile *z);
extern void zfile_seterror (const TCHAR *format, ...);
extern TCHAR *zfile_geterror (void);

#define ZFD_NONE 0
#define ZFD_ARCHIVE 1 //zip/lha..
#define ZFD_ADF 2 //adf as a filesystem
#define ZFD_HD 4 //rdb/hdf
#define ZFD_UNPACK 8 //gzip,dms
#define ZFD_RAWDISK 16  //fdi->adf,ipf->adf etc..
#define ZFD_CD 32 //cue/iso, cue has priority over iso
#define ZFD_DISKHISTORY 0x100 //allow diskhistory (if disk image)
#define ZFD_CHECKONLY 0x200 //file exists checkc
#define ZFD_DELAYEDOPEN 0x400 //do not unpack, just get metadata
#define ZFD_NORECURSE 0x10000 // do not recurse archives
#define ZFD_NORMAL (ZFD_ARCHIVE|ZFD_UNPACK)
#define ZFD_ALL 0x0000ffff

#define ZFD_RAWDISK_AMIGA 0x10000
#define ZFD_RAWDISK_PC 0x200000

#define ZFILE_UNKNOWN 0
#define ZFILE_CONFIGURATION 1
#define ZFILE_DISKIMAGE 2
#define ZFILE_ROM 3
#define ZFILE_KEY 4
#define ZFILE_HDF 5
#define ZFILE_STATEFILE 6
#define ZFILE_NVR 7
#define ZFILE_HDFRDB 8
#define ZFILE_CDIMAGE 9

extern const char *uae_archive_extensions[];
extern const char *uae_ignoreextensions[];
extern const char *uae_diskimageextensions[];

extern struct zvolume *zfile_fopen_archive (const TCHAR *filename);
extern struct zvolume *zfile_fopen_archive_root (const TCHAR *filename);
extern void zfile_fclose_archive (struct zvolume *zv);
//extern int zfile_fs_usage_archive (const TCHAR *path, const TCHAR *disk, struct fs_usage *fsp);
extern int zfile_stat_archive (const TCHAR *path, struct _stat64 *statbuf);
//extern void *zfile_opendir_archive (const TCHAR *path);
extern struct zdirectory *zfile_opendir_archive (const TCHAR *path);
extern struct zdirectory *zfile_opendir_archive2 (const TCHAR *path, int flags);
extern void zfile_closedir_archive (struct zdirectory *);
//extern int zfile_readdir_archive (void*, TCHAR*);
//extern void zfile_resetdir_archive (void*);
extern int zfile_readdir_archive (struct zdirectory *, TCHAR*);
extern unsigned int zfile_fill_file_attrs_archive (const TCHAR *path, int *isdir, int *flags, TCHAR **comment);
//extern uae_s64 zfile_lseek_archive (void *d, uae_s64 offset, int whence);
extern uae_s64 zfile_lseek_archive (struct zfile *d, uae_s64 offset, int whence);
extern uae_s64 zfile_fsize_archive (struct zfile *d);
extern unsigned int zfile_read_archive (struct zfile *d, void *b, unsigned int size);
//extern unsigned int zfile_read_archive (void *d, void *b, unsigned int size);
extern void zfile_close_archive (void *d);
extern struct zfile *zfile_open_archive (const TCHAR *path, int flags);
//extern void *zfile_open_archive (const TCHAR *path, int flags);
extern int zfile_exists_archive(const TCHAR *path, const TCHAR *rel);

extern void timeval_to_amiga (struct mytimeval *tv, int* days, int* mins, int* ticks);
extern void amiga_to_timeval (struct mytimeval *tv, int days, int mins, int ticks);

#endif // ZFILE_H