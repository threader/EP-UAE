/* fsusage.c -- return space usage of mounted filesystems
   Copyright (C) 1991, 1992, 1996 Free Software Foundation, Inc.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2, or (at your option)
   any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software Foundation,
   Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.  */

#include "sysconfig.h"
#include "target.h"

#if defined TARGET_AMIGAOS
#include <devices/timer.h>
#endif


#include <stdlib.h>
#include <sys/types.h>

#ifdef HAVE_SYS_STAT_H
#include <sys/stat.h>
#endif

#include "fsusage.h"

/* Return the number of TOSIZE-byte blocks used by
   BLOCKS FROMSIZE-byte blocks, rounding away from zero.
   TOSIZE must be positive.  Return -1 if FROMSIZE is not positive.  */

static long adjust_blocks (long blocks, int fromsize, int tosize)
{
  if (tosize <= 0)
    abort ();
  if (fromsize <= 0)
    return -1;

  if (fromsize == tosize)	/* e.g., from 512 to 512 */
    return blocks;
  else if (fromsize > tosize)	/* e.g., from 2048 to 512 */
    return blocks * (fromsize / tosize);
  else				/* e.g., from 256 to 512 */
    return (blocks + (blocks < 0 ? -1 : 1)) / (tosize / fromsize);
}

#ifdef _WIN32
#include "sysdeps.h"
#include <windows.h>
int get_fs_usage (const char *path, const char *disk, struct fs_usage *fsp)
{
    char buf2[MAX_DPATH];
    DWORD SectorsPerCluster;
    DWORD BytesPerSector;
    DWORD NumberOfFreeClusters;
    DWORD TotalNumberOfClusters;

	GetFullPathName (path, sizeof buf2, buf2, NULL);

    buf2[3] = 0;

    if (!GetDiskFreeSpace (buf2, &SectorsPerCluster, &BytesPerSector,
			   &NumberOfFreeClusters, &TotalNumberOfClusters)) {
		/* lasterror = GetLastError ();*/
		return -1;
    }

    /* HACK ALERT! WinNT returns 0 in TotalNumberOfClusters for an audio-CD, which calls the GURU! */
    if ((TotalNumberOfClusters == 0) && (GetDriveType (buf2) == DRIVE_CDROM))
		TotalNumberOfClusters = 327680;

    BytesPerSector *= SectorsPerCluster;
    fsp->fsu_blocks = adjust_blocks (TotalNumberOfClusters, BytesPerSector, 512);
    fsp->fsu_bavail = adjust_blocks (NumberOfFreeClusters, BytesPerSector, 512);

    return 0;
}

#else /* ! _WIN32 */

#if defined TARGET_AMIGAOS

#include <dos/dos.h>
#include <exec/memory.h>
#include <proto/exec.h>
#include <proto/dos.h>

int get_fs_usage (const char *path, const char *disk, struct fs_usage *fsp)
{
    struct InfoData *info = (struct InfoData *)AllocVec(sizeof *info, MEMF_ANY);
    int result = -1;

    if (info) {
	BPTR lock = Lock (path, SHARED_LOCK);
	if (lock) {
	    if (Info (lock, info)) {
		fsp->fsu_blocks = adjust_blocks (info->id_NumBlocks,
						 info->id_BytesPerBlock,
						 512);
		fsp->fsu_bfree = fsp->fsu_bavail =
				  adjust_blocks (info->id_NumBlocks - info->id_NumBlocksUsed,
						 info->id_BytesPerBlock,
						 512);
		fsp->fsu_files = fsp->fsu_ffree = -1;

		result = 0;
	    }
	    UnLock (lock);
	}
	FreeVec (info);
    }
    return result;
}

#else /* ! TARGET_AMIGAOS */

#ifdef __BEOS__

#include <be/kernel/fs_info.h>

int get_fs_usage (const char *path, const char *disk, struct fs_usage *fsp)
{
    int result = -1;
    dev_t device = dev_for_path (path);

    if (device >0) {
	fs_info info;
	if (fs_stat_dev (device, &info) == 0) {
	    fsp->fsu_blocks = adjust_blocks (info.total_blocks,
					     info.block_size,
					     512);
	    fsp->fsu_bfree = fsp->fsu_bavail =
			      adjust_blocks (info.free_blocks,
					     info.block_size,
					     512);
	    fsp->fsu_files = info.total_nodes;
	    fsp->fsu_ffree = info.free_nodes;

	    result = 0;
    	}
    }
    return result;
};

#else /* ! __BEOS__ */

//int statfs ();

#if HAVE_UNISTD_H
# include <unistd.h>
#endif

#if HAVE_SYS_PARAM_H
# include <sys/param.h>
#endif

#if HAVE_SYS_MOUNT_H
# include <sys/mount.h>
#endif

#if HAVE_SYS_VFS_H
# include <sys/vfs.h>
#endif

#if HAVE_SYS_FS_S5PARAM_H	/* Fujitsu UXP/V */
# include <sys/fs/s5param.h>
#endif

#if defined (HAVE_SYS_FILSYS_H) && !defined (_CRAY)
# include <sys/filsys.h>	/* SVR2 */
#endif

#if HAVE_FCNTL_H
# include <fcntl.h>
#endif

#if HAVE_SYS_STATFS_H
# include <sys/statfs.h>
#endif

#if HAVE_DUSTAT_H		/* AIX PS/2 */
# include <sys/dustat.h>
#endif

#if HAVE_SYS_STATVFS_H		/* SVR4 */
# include <sys/statvfs.h>
//int statvfs ();
#endif

/* Read LEN bytes at PTR from descriptor DESC, retrying if interrupted.
   Return the actual number of bytes read, zero for EOF, or negative
   for an error.  */

static int safe_read (int desc, char *ptr, int len)
{
  int n_chars;

  if (len <= 0)
    return len;

#ifdef EINTR
  do
    {
      n_chars = read (desc, ptr, len);
    }
  while (n_chars < 0 && errno == EINTR);
#else
  n_chars = read (desc, ptr, len);
#endif

  return n_chars;
}

/* Fill in the fields of FSP with information about space usage for
   the filesystem on which PATH resides.
   DISK is the device on which PATH is mounted, for space-getting
   methods that need to know it.
   Return 0 if successful, -1 if not.  When returning -1, ensure that
   ERRNO is either a system error value, or zero if DISK is NULL
   on a system that requires a non-NULL value.  */
int
get_fs_usage (path, disk, fsp)
     const char *path;
     const char *disk;
     struct fs_usage *fsp;
{
#ifdef STAT_STATFS3_OSF1
# define CONVERT_BLOCKS(B) adjust_blocks ((B), fsd.f_fsize, 512)

  struct statfs fsd;

  if (statfs (path, &fsd, sizeof (struct statfs)) != 0)
    return -1;

#endif /* STAT_STATFS3_OSF1 */

#ifdef STAT_STATFS2_FS_DATA	/* Ultrix */
# define CONVERT_BLOCKS(B) adjust_blocks ((B), 1024, 512)

  struct fs_data fsd;

  if (statfs (path, &fsd) != 1)
    return -1;
  fsp->fsu_blocks = CONVERT_BLOCKS (fsd.fd_req.btot);
  fsp->fsu_bfree = CONVERT_BLOCKS (fsd.fd_req.bfree);
  fsp->fsu_bavail = CONVERT_BLOCKS (fsd.fd_req.bfreen);
  fsp->fsu_files = fsd.fd_req.gtot;
  fsp->fsu_ffree = fsd.fd_req.gfree;

#endif /* STAT_STATFS2_FS_DATA */

#ifdef STAT_READ_FILSYS		/* SVR2 */
# ifndef SUPERBOFF
#  define SUPERBOFF (SUPERB * 512)
# endif
# define CONVERT_BLOCKS(B) \
    adjust_blocks ((B), (fsd.s_type == Fs2b ? 1024 : 512), 512)

  struct filsys fsd;
  int fd;

  if (! disk)
    {
      errno = 0;
      return -1;
    }

  fd = open (disk, O_RDONLY);
  if (fd < 0)
    return -1;
  lseek (fd, (long) SUPERBOFF, 0);
  if (safe_read (fd, (char *) &fsd, sizeof fsd) != sizeof fsd)
    {
      close (fd);
      return -1;
    }
  close (fd);
  fsp->fsu_blocks = CONVERT_BLOCKS (fsd.s_fsize);
  fsp->fsu_bfree = CONVERT_BLOCKS (fsd.s_tfree);
  fsp->fsu_bavail = CONVERT_BLOCKS (fsd.s_tfree);
  fsp->fsu_files = (fsd.s_isize - 2) * INOPB * (fsd.s_type == Fs2b ? 2 : 1);
  fsp->fsu_ffree = fsd.s_tinode;

#endif /* STAT_READ_FILSYS */

#ifdef STAT_STATFS2_BSIZE	/* 4.3BSD, SunOS 4, HP-UX, AIX */
# define CONVERT_BLOCKS(B) adjust_blocks ((B), fsd.f_bsize, 512)

  struct statfs fsd;

  if (statfs (path, &fsd) < 0)
    return -1;

# ifdef STATFS_TRUNCATES_BLOCK_COUNTS

  /* In SunOS 4.1.2, 4.1.3, and 4.1.3_U1, the block counts in the
     struct statfs are truncated to 2GB.  These conditions detect that
     truncation, presumably without botching the 4.1.1 case, in which
     the values are not truncated.  The correct counts are stored in
     undocumented spare fields.  */
  if (fsd.f_blocks == 0x1fffff && fsd.f_spare[0] > 0)
    {
      fsd.f_blocks = fsd.f_spare[0];
      fsd.f_bfree = fsd.f_spare[1];
      fsd.f_bavail = fsd.f_spare[2];
    }
# endif /* STATFS_TRUNCATES_BLOCK_COUNTS */

#endif /* STAT_STATFS2_BSIZE */

#ifdef STAT_STATFS2_FSIZE	/* 4.4BSD */
# define CONVERT_BLOCKS(B) adjust_blocks ((B), fsd.f_fsize, 512)

  struct statfs fsd;

  if (statfs (path, &fsd) < 0)
    return -1;

#endif /* STAT_STATFS2_FSIZE */

#ifdef STAT_STATFS4		/* SVR3, Dynix, Irix, AIX */
# if _AIX || defined(_CRAY)
#  define CONVERT_BLOCKS(B) adjust_blocks ((B), fsd.f_bsize, 512)
#  ifdef _CRAY
#   define f_bavail f_bfree
#  endif
# else
#  define CONVERT_BLOCKS(B) (B)
#  ifndef _SEQUENT_		/* _SEQUENT_ is DYNIX/ptx */
#   ifndef DOLPHIN		/* DOLPHIN 3.8.alfa/7.18 has f_bavail */
#    define f_bavail f_bfree
#   endif
#  endif
# endif

  struct statfs fsd;

  if (statfs (path, &fsd, sizeof fsd, 0) < 0)
    return -1;
  /* Empirically, the block counts on most SVR3 and SVR3-derived
     systems seem to always be in terms of 512-byte blocks,
     no matter what value f_bsize has.  */

#endif /* STAT_STATFS4 */

#ifdef STAT_STATVFS		/* SVR4 */
# define CONVERT_BLOCKS(B) \
    adjust_blocks ((B), fsd.f_frsize ? fsd.f_frsize : fsd.f_bsize, 512)

  struct statvfs fsd;

  if (statvfs (path, &fsd) < 0)
    return -1;
  /* f_frsize isn't guaranteed to be supported.  */

#endif /* STAT_STATVFS */

#if !defined(STAT_STATFS2_FS_DATA) && !defined(STAT_READ_FILSYS)
				/* !Ultrix && !SVR2 */

  fsp->fsu_blocks = CONVERT_BLOCKS (fsd.f_blocks);
  fsp->fsu_bfree = CONVERT_BLOCKS (fsd.f_bfree);
  fsp->fsu_bavail = CONVERT_BLOCKS (fsd.f_bavail);
  fsp->fsu_files = fsd.f_files;
  fsp->fsu_ffree = fsd.f_ffree;

#endif /* not STAT_STATFS2_FS_DATA && not STAT_READ_FILSYS */

  return 0;
}

#if defined(_AIX) && defined(_I386)
/* AIX PS/2 does not supply statfs.  */

int
statfs (path, fsb)
     char *path;
     struct statfs *fsb;
{
  struct stat stats;
  struct dustat fsd;

  if (stat (path, &stats))
    return -1;
  if (dustat (stats.st_dev, 0, &fsd, sizeof (fsd)))
    return -1;
  fsb->f_type   = 0;
  fsb->f_bsize  = fsd.du_bsize;
  fsb->f_blocks = fsd.du_fsize - fsd.du_isize;
  fsb->f_bfree  = fsd.du_tfree;
  fsb->f_bavail = fsd.du_tfree;
  fsb->f_files  = (fsd.du_isize - 2) * fsd.du_inopb;
  fsb->f_ffree  = fsd.du_tinode;
  fsb->f_fsid.val[0] = fsd.du_site;
  fsb->f_fsid.val[1] = fsd.du_pckno;
  return 0;
}

#endif /* _AIX && _I386 */

#endif /* ! __BEOS__ */
#endif /* ! TARGET_AMIGAOS */
#endif /* ! _WIN32 */
