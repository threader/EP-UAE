/*
 * UAE - The Un*x Amiga Emulator
 *
 * Unix file system handler for AmigaDOS
 *
 * Copyright 1996 Ed Hanway
 * Copyright 1996, 1997 Bernd Schmidt
 *
 * Version 0.4: 970308
 *
 * Based on example code (c) 1988 The Software Distillery
 * and published in Transactor for the Amiga, Volume 2, Issues 2-5.
 * (May - August 1989)
 *
 * Known limitations:
 * Does not support several (useless) 2.0+ packet types.
 * May not return the correct error code in some cases.
 * Does not check for sane values passed by AmigaDOS.  May crash the emulation
 * if passed garbage values.
 * Could do tighter checks on malloc return values.
 * Will probably fail spectacularly in some cases if the filesystem is
 * modified at the same time by another process while UAE is running.
 */

#include "sysconfig.h"
#include "sysdeps.h"

#include "threaddep/thread.h"
#include "options.h"
#include "uae.h"
#include "misc.h"
#include "memory.h"
#include "custom.h"
#include "events.h"
#include "newcpu.h"
#include "filesys.h"
#include "autoconf.h"
#include "traps.h"
#include "fsusage.h"
#include "native2amiga.h"
#include "scsidev.h"
#include "fsdb.h"
#include "zfile.h"
#include "gui.h"
#include "gayle.h"
#include "savestate.h"
#include "a2091.h"
#include "cdtv.h"
#include "consolehook.h"
#include "isofs_api.h"

//FIXME: ---start
#ifdef TARGET_AMIGAOS
# include <dos/dos.h>
# include <proto/dos.h>
# include <dos/obsolete.h>
#endif

#if !defined(_LINUX_)
#define my_rmdir rmdir
#define my_unlink unlink
#define my_rename rename
#define my_truncate truncate
#define my_opendir opendir
#define my_closedir closedir
#define my_open fopen
#define my_close fclose
#define my_lseek fseek
#endif

#if 0
#define my_rmdir rmdir
#define my_unlink unlink
#define my_rename rename
#define my_truncate truncate
#define my_opendir opendir
#define my_closedir closedir
#define my_open fopen
#define my_close fclose
#define my_lseek fseek
//#define my_strdup strdup
//#define _stat64 stat
#endif 
#define TRUE 1
#define FALSE 0
//FIXME: ---end

#define TRACING_ENABLED 0
#if TRACING_ENABLED
#define TRACE(x)	do { write_log x; } while(0)
#define DUMPLOCK(u,x)	dumplock(u,x)
#else
#define TRACE(x)
#define DUMPLOCK(u,x)
#endif

static uae_sem_t test_sem;

int bootrom_header, bootrom_items;
static uae_u32 dlg (uae_u32 a)
{
	return (dbg (a + 0) << 24) | (dbg (a + 1) << 16) | (dbg (a + 2) << 8) | (dbg (a + 3) << 0);
}

static void aino_test (a_inode *aino)
{
#ifdef AINO_DEBUG
    a_inode *aino2 = aino, *aino3;
    for (;;) {
		if (!aino || !aino->next)
		    return;
		if ((aino->checksum1 ^ aino->checksum2) != 0xaaaa5555) {
			write_log ("PANIC: corrupted or freed but used aino detected!", aino);
		}
		aino3 = aino;
		aino = aino->next;
		if (aino->prev != aino3) {
			write_log ("PANIC: corrupted aino linking!\n");
		    break;
		}
		if (aino == aino2) break;
    }
#endif
}

static void aino_test_init (a_inode *aino)
{
#ifdef AINO_DEBUG
    aino->checksum1 = (uae_u32)aino;
    aino->checksum2 = aino->checksum1 ^ 0xaaaa5555;
#endif
}
/* note */ 
/*
 * This _should_ be no issue for us, but let's rather use a guaranteed
 * thread safe function if we have one.
 * This used to be broken in glibc versions <= 2.0.1 (I think). I hope
 * no one is using this these days.
 * Michael Krause says it's also broken in libc5. ARRRGHHHHHH!!!!
 */
#if 0 && defined HAVE_READDIR_R

static struct dirent *my_readdir (DIR *dirstream, struct dirent *space)
{
    struct dirent *loc;
    if (readdir_r (dirstream, space, &loc) == 0) {
	/* Success */
	return loc;
    }
    return 0;
}

#else
#define my_readdir(dirstream, space) readdir(dirstream)
#endif

uaecptr filesys_initcode;
static uae_u32 fsdevname, filesys_configdev;
static int filesys_in_interrupt;
static uae_u32 mountertask;
static int automountunit = -1;

#define FS_STARTUP 0
#define FS_GO_DOWN 1

#define DEVNAMES_PER_HDF 32

typedef struct {
	int open;
	char *devname; /* device name, e.g. UAE0: */
    uaecptr devname_amiga;
    uaecptr startup;
	char *volname; /* volume name, e.g. CDROM, WORK, etc. */
	int volflags; /* volume flags, readonly, stream uaefsdb support */
	char *rootdir; /* root native directory/hdf. empty drive if invalid path */
	struct zvolume *zarchive;
	char *rootdirdiff; /* "diff" file/directory */
    int readonly; /* disallow write access? */
	bool unknown_media; /* ID_UNREADABLE_DISK */
	int bootpri; /* boot priority. -128 = no autoboot, -129 = no mount */
    int devno;
	int controller;
	int wasisempty; /* if true, this unit was created empty */
	int canremove; /* if true, this unit can be safely ejected and remounted */
	int configureddrive; /* if true, this is drive that was manually configured */

    struct hardfiledata hf;

    /* Threading stuff */
    smp_comm_pipe *volatile unit_pipe, *volatile back_pipe;
    uae_thread_id tid;
    struct _unit *self;
    /* Reset handling */
/* note */
    volatile uae_sem_t reset_sync_sem;
    volatile int reset_state;

    /* RDB stuff */
    uaecptr rdb_devname_amiga[DEVNAMES_PER_HDF];
    int rdb_lowcyl;
    int rdb_highcyl;
    int rdb_cylblocks;
    uae_u8 *rdb_filesysstore;
    int rdb_filesyssize;
	char *filesysdir;

} UnitInfo;

struct uaedev_mount_info {
    int num_units;
    UnitInfo ui[MAX_FILESYSTEM_UNITS];
};

static struct uaedev_mount_info current_mountinfo;
struct uaedev_mount_info options_mountinfo;

int nr_units (struct uaedev_mount_info *mountinfo)
{
	if (!mountinfo)
	mountinfo = &current_mountinfo;
	int i, cnt = 0;
	for (i = 0; i < MAX_FILESYSTEM_UNITS; i++) {
		if (mountinfo->ui[i].open)
			cnt++;
	}
	return cnt;
}
/* note */
int nr_directory_units (struct uaedev_mount_info *mountinfo, struct uae_prefs *p)
{
	if (!mountinfo)
	mountinfo = &current_mountinfo;

	int i, cnt = 0;
	if (p) {
		for (i = 0; i < p->mountitems; i++) {
			if (p->mountconfig[i].controller == 0)
				cnt++;
		}
	} else {
		for (i = 0; i < MAX_FILESYSTEM_UNITS; i++) {
			if (mountinfo->ui[i].open && mountinfo->ui[i].controller == 0)
				cnt++;
		}
	}
	return cnt;
}

int is_hardfile (struct uaedev_mount_info *mountinfo, int unit_no)
{
	if (!mountinfo)
	mountinfo = &current_mountinfo;
	if (mountinfo->ui[unit_no].volname || mountinfo->ui[unit_no].wasisempty)
		return FILESYS_VIRTUAL;
	if (mountinfo->ui[unit_no].hf.secspertrack == 0) {
		if (mountinfo->ui[unit_no].hf.flags & 1)
		    return FILESYS_HARDDRIVE;
		return FILESYS_HARDFILE_RDB;
    }
    return FILESYS_HARDFILE;
}

static void close_filesys_unit (UnitInfo *uip)
{
	if (!uip->open)
		return;
    if (uip->hf.handle_valid)
		hdf_close (&uip->hf);
    if (uip->volname != 0)
		xfree (uip->volname);
    if (uip->devname != 0)
		xfree (uip->devname);
    if (uip->rootdir != 0)
		xfree (uip->rootdir);
    if (uip->unit_pipe)
		xfree (uip->unit_pipe);
    if (uip->back_pipe)
		xfree (uip->back_pipe);

    uip->unit_pipe = 0;
    uip->back_pipe = 0;

	uip->hf.handle_valid = 0;
    uip->volname = 0;
    uip->devname = 0;
    uip->rootdir = 0;
	uip->open = 0;
}

static UnitInfo *getuip(struct uaedev_mount_info *mountinfo, struct uae_prefs *p, int index)
{
	if (!mountinfo)
	mountinfo = &current_mountinfo;

	if (index < 0)
		return NULL;
	index = p->mountconfig[index].configoffset;
	if (index < 0)
		return NULL;
	return &mountinfo->ui[index];
}

int get_filesys_unitconfig (struct uaedev_mount_info *mountinfo, struct uae_prefs *p, int index, struct mountedinfo *mi)
{
	UnitInfo *ui = getuip(mountinfo, p, index);
	struct uaedev_config_info *uci = &p->mountconfig[index];
	UnitInfo uitmp;

	memset (mi, 0, sizeof (struct mountedinfo));
	memset (&uitmp, 0, sizeof uitmp);
	if (!ui) {
		ui = &uitmp;
		if (!uci->ishdf) {
			mi->ismounted = 1;
			if (uci->rootdir && _tcslen(uci->rootdir) == 0)
				return FILESYS_VIRTUAL;
			if (my_existsfile (uci->rootdir)) {
				mi->ismedia = 1;
				return FILESYS_VIRTUAL;
    }
			if (my_getvolumeinfo (uci->rootdir) < 0)
				return -1;
			mi->ismedia = 1;
			return FILESYS_VIRTUAL;
		} else {
			ui->hf.readonly = 1;
			ui->hf.blocksize = uci->blocksize;
			if (!hdf_open (&ui->hf, uci->rootdir)) {
				mi->ismedia = 0;
				mi->ismounted = 1;
				if (uci->reserved == 0 && uci->sectors == 0 && uci->surfaces == 0) {
					if (ui->hf.flags & 1)
						return FILESYS_HARDDRIVE;
					return FILESYS_HARDFILE_RDB;
				}
				return -1;
			}
			mi->ismedia = 1;
			if (ui->hf.drive_empty)
				mi->ismedia = 0;
			hdf_close (&ui->hf);
		}
	} else {
		if (!ui->controller || (ui->controller && p->cs_ide)) {
			mi->ismounted = 1;
			if (uci->ishdf)
				mi->ismedia = ui->hf.drive_empty ? 0 : 1;
			else
				mi->ismedia = 1;
		}
	}
	mi->size = ui->hf.virtsize;
	mi->nrcyls = (int)(uci->sectors * uci->surfaces ? (ui->hf.virtsize / uci->blocksize) / (uci->sectors * uci->surfaces) : 0);
	if (!uci->ishdf)
		return FILESYS_VIRTUAL;
	if (uci->reserved == 0 && uci->sectors == 0 && uci->surfaces == 0) {
		if (ui->hf.flags & 1)
			return FILESYS_HARDDRIVE;
		return FILESYS_HARDFILE_RDB;
	}
	return FILESYS_HARDFILE;
}

static void stripsemicolon (char *s)
{
	if (!s)
		return;
	while (_tcslen(s) > 0 && s[_tcslen(s) - 1] == ':')
		s[_tcslen(s) - 1] = 0;
}
static void stripspace (char *s)
{
	int i = 0;
	if (!s)
		return;
	for ( ; i < (int)_tcslen (s); i++) {
		if (s[i] == ' ')
			s[i] = '_';
	}
}
static void striplength (char *s, size_t len)
{
	if (!s)
		return;
	if (_tcslen (s) <= len)
		return;
	s[len] = 0;
}
static void fixcharset (char *s)
{
	char tmp[MAX_DPATH];
	if (!s)
		return;
	//ua_fs_copy (tmp, MAX_DPATH, s, '_');
	strcpy (tmp, s);
	au_fs_copy (s, strlen (tmp) + 1, tmp);
}

char *validatevolumename (char *s)
{
	stripsemicolon (s);
	stripspace (s);
	fixcharset (s);
	striplength (s, 30);
	return s;
}
char *validatedevicename (char *s)
{
	stripsemicolon (s);
	stripspace (s);
	fixcharset (s);
	striplength (s, 30);
	return s;
}

char *filesys_createvolname (const char *volname, const char *rootdir, const char *def)
{
	char *nvol = NULL;
	int i, archivehd;
	char *p = NULL;

	archivehd = -1;
	if (my_existsfile (rootdir))
		archivehd = 1;
	else if (my_existsdir (rootdir))
		archivehd = 0;

	if ((!volname || _tcslen (volname) == 0) && rootdir && archivehd >= 0) {
		p = my_strdup (rootdir);
		for (i = _tcslen (p) - 1; i >= 0; i--) {
			TCHAR c = p[i];
			if (c == ':' || c == '/' || c == '\\') {
				if (i == (int)_tcslen (p) - 1)
					continue;
				if (!_tcscmp (p + i, ":\\")) {
					xfree (p);
					p = xmalloc (TCHAR, 10);
					p[0] = rootdir[0];
					p[1] = 0;
					i = 0;
				} else {
					i++;
				}
				break;
			}
		}
		if (i >= 0)
			nvol = my_strdup (p + i);
	}
	if (!nvol && archivehd >= 0) {
		if (volname && _tcslen (volname) > 0)
			nvol = my_strdup (volname);
		else
			nvol = my_strdup (def);
	}
	if (!nvol) {
		if (volname && _tcslen (volname))
			nvol = my_strdup (volname);
		else
			nvol = my_strdup ("");
	}
	validatevolumename (nvol);
	xfree (p);
	return nvol;
}

static int set_filesys_volume (const char *rootdir, int *flags, int *readonly, int *emptydrive, struct zvolume **zvp)
{
	*emptydrive = 0;
//FIXME: we dont support.. yet.. -mustafa
//	if (my_existsfile (rootdir)) {
	if (0) {
		struct zvolume *zv;
		zv = 1; //zfile_fopen_archive (rootdir);
		if (!zv) {
			write_log ("'%s' is not a supported archive file\n", rootdir);
			return -1;
		}
		*zvp = zv;
		*flags = MYVOLUMEINFO_ARCHIVE;
		*readonly = 1;
	} else {
		*flags = my_getvolumeinfo (rootdir);
		if (*flags < 0) {
			if (rootdir && rootdir[0])
				write_log ("directory '%s' not found, mounting as empty drive\n", rootdir);
			*emptydrive = 1;
			*flags = 0;
		} else if ((*flags) & MYVOLUMEINFO_READONLY) {
			write_log ("'%s' set to read-only\n", rootdir);
			*readonly = 1;
		}
	}
	return 1;
}

static int *set_filesys_unit_1 (struct uaedev_mount_info *mountinfo,int nr,
	const char *devname, const char *volname, const char *rootdir, int readonly,
	int secspertrack, int surfaces, int reserved,
	int blocksize, int bootpri, int donotmount, int autoboot,
	const char *filesysdir, int hdc, int flags)
{
	UnitInfo *ui;
	int i;
	int emptydrive = 0;

	if (hdc)
		return -1;
	if (nr < 0) {
		for (nr = 0; nr < MAX_FILESYSTEM_UNITS; nr++) {
			if (!mountinfo->ui[nr].open)
				break;
		}
		if (nr == MAX_FILESYSTEM_UNITS) {
			write_log ("No slot allocated for this unit\n");
			return -1;
		}
	}

	for (i = 0; i < MAX_FILESYSTEM_UNITS; i++) {
		if (nr == i || !mountinfo->ui[i].open)
			continue;
		if (rootdir && _tcslen (rootdir) > 0 && !_tcsicmp (mountinfo->ui[i].rootdir, rootdir)) {
			write_log ("directory/hardfile '%s' already added\n", rootdir);
			return -1;
		}
	}

	ui = &mountinfo->ui[nr];
	memset (ui, 0, sizeof (UnitInfo));

	if (volname != NULL) {
		int flags = 0;
		emptydrive = 1;
		if (rootdir) {
			if (set_filesys_volume (rootdir, &flags, &readonly, &emptydrive, &ui->zarchive) < 0)
				return -1;
		}
		ui->volname = filesys_createvolname (volname, rootdir, "harddrive");
		ui->volflags = flags;
	} else {
		ui->hf.secspertrack = secspertrack;
		ui->hf.surfaces = surfaces;
		ui->hf.reservedblocks = reserved;
		ui->hf.blocksize = blocksize;
		ui->hf.unitnum = nr;
		ui->volname = 0;
		ui->hf.readonly = readonly;
		if (!hdf_open (&ui->hf, rootdir) && !readonly) {
			ui->hf.readonly = readonly = 1;
			hdf_open (&ui->hf, rootdir);
		}
		ui->hf.readonly = readonly;
		if (!ui->hf.drive_empty) {
			if (ui->hf.handle_valid == 0) {
				write_log ("Hardfile %s not found\n", ui->hf.device_name);
				goto err;
			}
			if ((ui->hf.blocksize & (ui->hf.blocksize - 1)) != 0 || ui->hf.blocksize == 0) {
				write_log ("Hardfile %s bad blocksize\n", ui->hf.device_name);
				goto err;
			}
			if ((ui->hf.secspertrack || ui->hf.surfaces || ui->hf.reservedblocks) &&
				(ui->hf.secspertrack < 1 || ui->hf.surfaces < 1 || ui->hf.surfaces > 1023 ||
				ui->hf.reservedblocks < 0 || ui->hf.reservedblocks > 1023) != 0) {
					write_log ("Hardfile %s bad hardfile geometry\n", ui->hf.device_name);
					goto err;
			}
			if (ui->hf.blocksize > ui->hf.virtsize || ui->hf.virtsize == 0) {
				write_log ("Hardfile %s too small (%d)\n", ui->hf.device_name, ui->hf.virtsize);
				goto err;
			}
			ui->hf.nrcyls = (int)(ui->hf.secspertrack * ui->hf.surfaces ? (ui->hf.virtsize / ui->hf.blocksize) / (ui->hf.secspertrack * ui->hf.surfaces) : 0);
		}
	}
	ui->self = 0;
	ui->reset_state = FS_STARTUP;
	ui->wasisempty = emptydrive;
	ui->canremove = emptydrive && (flags & MYVOLUMEINFO_REUSABLE);
	ui->rootdir = my_strdup (rootdir);
    if (devname != 0 && strlen (devname) != 0)
	ui->devname = my_strdup (devname);
	stripsemicolon(ui->devname);
	if (filesysdir && filesysdir[0])
		ui->filesysdir = my_strdup (filesysdir);
	ui->readonly = readonly;
	if (!autoboot)
		bootpri = -128;
	if (donotmount)
		bootpri = -129;
	if (bootpri < -129)
		bootpri = -129;
	if (bootpri > 127)
		bootpri = 127;
	ui->bootpri = bootpri;
	ui->open = 1;

	return nr;
err:
	if (ui->hf.handle_valid)
		hdf_close (&ui->hf);
	return -1;
}

int *set_filesys_unit (struct uaedev_mount_info *mountinfo, int nr,
	const char *devname, const char *volname, const char *rootdir, int readonly,
	int secspertrack, int surfaces, int reserved,
	int blocksize, int bootpri, int donotmount, int autoboot,
	const char *filesysdir, int hdc, int flags)
{
	int ret;

	ret = set_filesys_unit_1 (mountinfo, nr, devname, volname, rootdir, readonly,
		secspertrack, surfaces, reserved, blocksize, bootpri, donotmount, autoboot,
		filesysdir, hdc, flags);
	return ret;
}

int *add_filesys_unit (struct uaedev_mount_info *mountinfo, const char *devname, const char *volname, const char *rootdir, int readonly,
	int secspertrack, int surfaces, int reserved,
	int blocksize, int bootpri, int donotmount, int autoboot,
	const char *filesysdir, int hdc, int flags)
{
	int ret;

	if (nr_units(currprefs.mountinfo) >= MAX_FILESYSTEM_UNITS)
		return -1;

	ret = set_filesys_unit_1 (mountinfo, -1, devname, volname, rootdir, readonly,
		secspertrack, surfaces, reserved, blocksize,
		bootpri, donotmount, autoboot, filesysdir, hdc, flags);
	return ret;
}

int kill_filesys_unitconfig (struct uae_prefs *p, int nr)
{
	struct uaedev_config_info *uci;

	if (nr < 0)
		return 0;
	uci = &p->mountconfig[nr];
	hardfile_do_disk_change (uci, 0);
	if (uci->configoffset >= 0 && uci->controller == 0)
		filesys_media_change (currprefs.mountinfo, uci->rootdir, 0, uci);
	while (nr < MOUNT_CONFIG_SIZE) {
		memmove (&p->mountconfig[nr], &p->mountconfig[nr + 1], sizeof (struct uaedev_config_info));
		nr++;
	}
	p->mountitems--;
	memset (&p->mountconfig[MOUNT_CONFIG_SIZE - 1], 0, sizeof (struct uaedev_config_info));
	return 1;
}

int move_filesys_unitconfig (struct uae_prefs *p, int nr, int to)
{
	struct uaedev_config_info *uci1, *uci2, tmpuci;

	uci1 = &p->mountconfig[nr];
	uci2 = &p->mountconfig[to];
	if (nr == to)
		return 0;
	memcpy (&tmpuci, uci1, sizeof (struct uaedev_config_info));
	memcpy (uci1, uci2, sizeof (struct uaedev_config_info));
	memcpy (uci2, &tmpuci, sizeof (struct uaedev_config_info));
	return 1;
}


static void filesys_addexternals (void);

static void initialize_mountinfo (struct uaedev_mount_info *mountinfo)
{
	int i;
	struct uaedev_config_info *uci;
	UnitInfo *uip = &mountinfo->ui[0];

	for (i = 0; i < currprefs.mountitems; i++) {
		int idx;
		uci = &currprefs.mountconfig[i];
		if (uci->controller == HD_CONTROLLER_UAE) {
			idx = set_filesys_unit_1 (mountinfo, -1, uci->devname, uci->ishdf ? NULL : uci->volname, uci->rootdir,
				uci->readonly, uci->sectors, uci->surfaces, uci->reserved,
				uci->blocksize, uci->bootpri, uci->donotmount, uci->autoboot, uci->filesys, 0, MYVOLUMEINFO_REUSABLE);
			if (idx >= 0) {
				UnitInfo *ui;
				uci->configoffset = idx;
				ui = &mountinfo->ui[idx];
				ui->configureddrive = 1;
			}
		} else if (uci->controller <= HD_CONTROLLER_IDE3) {
#ifdef GAYLE
			gayle_add_ide_unit (uci->controller - HD_CONTROLLER_IDE0, uci->rootdir, uci->blocksize, uci->readonly,
				uci->devname, uci->sectors, uci->surfaces, uci->reserved,
				uci->bootpri, uci->filesys);
#endif
		} else if (uci->controller <= HD_CONTROLLER_SCSI6) {
			if (currprefs.cs_mbdmac) {
#ifdef A2091
				a3000_add_scsi_unit (uci->controller - HD_CONTROLLER_SCSI0, uci->rootdir, uci->blocksize, uci->readonly,
					uci->devname, uci->sectors, uci->surfaces, uci->reserved,
					uci->bootpri, uci->filesys);
#endif
			}  else if (currprefs.cs_a2091) {
#ifdef A2091
				a2091_add_scsi_unit (uci->controller - HD_CONTROLLER_SCSI0, uci->rootdir, uci->blocksize, uci->readonly,
					uci->devname, uci->sectors, uci->surfaces, uci->reserved,
					uci->bootpri, uci->filesys);
#endif
			} else if (currprefs.cs_cdtvscsi) {
#ifdef CDTV
				cdtv_add_scsi_unit (uci->controller - HD_CONTROLLER_SCSI0, uci->rootdir, uci->blocksize, uci->readonly,
					uci->devname, uci->sectors, uci->surfaces, uci->reserved,
					uci->bootpri, uci->filesys);
#endif
			}
		} else if (uci->controller == HD_CONTROLLER_PCMCIA_SRAM) {
#ifdef GAYLE
			gayle_add_pcmcia_sram_unit (uci->rootdir, uci->readonly);
#endif
		}
	}
	//filesys_addexternals ();
}

int sprintf_filesys_unit (const struct uaedev_mount_info *mountinfo, char *buffer, int num)
{
	const UnitInfo *uip = mountinfo->ui;

	if (uip[num].volname != 0)
		_stprintf (buffer, "(DH%d:) Filesystem, %s: %s %s", num, uip[num].volname,
		uip[num].rootdir, uip[num].readonly ? "ro" : "");
	else
		_stprintf (buffer, _T("(DH%d:) Hardfile, \"%s\", size %d Mbytes"), num,
		uip[num].rootdir, (int)(uip[num].hf.virtsize / (1024 * 1024)));
	return 0;
}

void free_mountinfo (struct uaedev_mount_info *mountinfo)
{
    int i;
	for (i = 0; i < MAX_FILESYSTEM_UNITS; i++)
		close_filesys_unit (mountinfo->ui + i);
#ifdef GAYLE
	gayle_free_units ();
#endif
}

struct hardfiledata *get_hardfile_data (int nr)
{
    UnitInfo *uip = current_mountinfo.ui;
	if (nr < 0 || nr >= MAX_FILESYSTEM_UNITS || uip[nr].open == 0 || is_hardfile (&current_mountinfo, nr) == FILESYS_VIRTUAL)
		return 0;
    return &uip[nr].hf;
}

/* minimal AmigaDOS definitions */

/* field offsets in DosPacket */
#define dp_Type 8
#define dp_Res1	12
#define dp_Res2 16
#define dp_Arg1 20
#define dp_Arg2 24
#define dp_Arg3 28
#define dp_Arg4 32
#define dp_Arg5 36

#define DP64_INIT       -3L

#define dp64_Type 8
#define dp64_Res0 12
#define dp64_Res2 16
#define dp64_Res1 24
#define dp64_Arg1 32
#define dp64_Arg2 40
#define dp64_Arg3 48
#define dp64_Arg4 52
#define dp64_Arg5 56

/* result codes */
#define DOS_TRUE ((uae_u32)-1L)
#define DOS_FALSE (0L)

#define MAXFILESIZE32 (0x7fffffff)

/* Passed as type to Lock() */
#define SHARED_LOCK         -2     /* File is readable by others */
#define ACCESS_READ         -2     /* Synonym */
#define EXCLUSIVE_LOCK      -1     /* No other access allowed    */
#define ACCESS_WRITE        -1     /* Synonym */

/* packet types */
#define ACTION_CURRENT_VOLUME	7
#define ACTION_LOCATE_OBJECT	8
#define ACTION_RENAME_DISK	9
#define ACTION_FREE_LOCK	15
#define ACTION_DELETE_OBJECT	16
#define ACTION_RENAME_OBJECT	17
#define ACTION_MORE_CACHE	18
#define ACTION_COPY_DIR		19
#define ACTION_SET_PROTECT	21
#define ACTION_CREATE_DIR	22
#define ACTION_EXAMINE_OBJECT	23
#define ACTION_EXAMINE_NEXT	24
#define ACTION_DISK_INFO	25
#define ACTION_INFO		26
#define ACTION_FLUSH		27
#define ACTION_SET_COMMENT	28
#define ACTION_PARENT		29
#define ACTION_SET_DATE		34
#define ACTION_FIND_WRITE	1004
#define ACTION_FIND_INPUT	1005
#define ACTION_FIND_OUTPUT	1006
#define ACTION_END		1007
#define ACTION_SEEK		1008
#define ACTION_WRITE_PROTECT	1023
#define ACTION_IS_FILESYSTEM	1027
#define ACTION_READ		'R'
#define ACTION_WRITE		'W'

/* 2.0+ packet types */
#define ACTION_INHIBIT		31
#define ACTION_SET_FILE_SIZE	1022
#define ACTION_LOCK_RECORD	2008
#define ACTION_FREE_RECORD	2009
#define ACTION_SAME_LOCK	40
#define ACTION_CHANGE_MODE	1028
#define ACTION_FH_FROM_LOCK	1026
#define ACTION_COPY_DIR_FH	1030
#define ACTION_PARENT_FH	1031
#define ACTION_EXAMINE_ALL	1033
#define ACTION_EXAMINE_FH	1034
#define ACTION_EXAMINE_ALL_END	1035

#define ACTION_MAKE_LINK	1021
#define ACTION_READ_LINK	1024

#define ACTION_FORMAT		1020
#define ACTION_IS_FILESYSTEM	1027
#define ACTION_ADD_NOTIFY	4097
#define ACTION_REMOVE_NOTIFY	4098

#define ACTION_CHANGE_FILE_POSITION64  8001
#define ACTION_GET_FILE_POSITION64     8002
#define ACTION_CHANGE_FILE_SIZE64      8003
#define ACTION_GET_FILE_SIZE64         8004

#define DISK_TYPE		0x444f5301 /* DOS\1 */

typedef struct {
    uae_u32 uniq;
    /* The directory we're going through.  */
    a_inode *aino;
    /* The file we're going to look up next.  */
    a_inode *curr_file;
} ExamineKey;

typedef struct key {
    struct key *next;
    a_inode *aino;
    uae_u32 uniq;
	struct fs_filehandle *fd;
	uae_u64 file_pos;
    int dosmode;
    int createmode;
    int notifyactive;
} Key;

typedef struct notify {
    struct notify *next;
    uaecptr notifyrequest;
    char *fullname;
    char *partname;
} Notify;

typedef struct exallkey {
	uae_u32 id;
	struct fs_dirhandle *dirhandle;
	TCHAR *fn;
	uaecptr control;
} ExAllKey;

/* Since ACTION_EXAMINE_NEXT is so braindamaged, we have to keep
 * some of these around
 */

#define EXKEYS 128
#define EXALLKEYS 100
#define MAX_AINO_HASH 128
#define NOTIFY_HASH_SIZE 127

/* handler state info */

typedef struct _unit {
    struct _unit *next;

    /* Amiga stuff */
    uaecptr dosbase;
    uaecptr volume;
    uaecptr port;	/* Our port */
    uaecptr locklist;

    /* Native stuff */
    uae_s32 unit;	/* unit number */
    UnitInfo ui;	/* unit startup info */
    char tmpbuf3[256];

    /* Dummy message processing */
    uaecptr dummy_message;
    volatile unsigned int cmds_sent;
    volatile unsigned int cmds_complete;
    volatile unsigned int cmds_acked;

    /* ExKeys */
    ExamineKey examine_keys[EXKEYS];
    int next_exkey;
    unsigned long total_locked_ainos;

	/* ExAll */
	ExAllKey exalls[EXALLKEYS];
	uae_u32 exallid;

    /* Keys */
    struct key *keys;

    a_inode rootnode;
    unsigned long aino_cache_size;
    a_inode *aino_hash[MAX_AINO_HASH];
    unsigned long nr_cache_hits;
    unsigned long nr_cache_lookups;

    struct notify *notifyhash[NOTIFY_HASH_SIZE];

	int volflags;
	uae_u32 lockkey;
	int inhibited;
	int canremovable;
	int mountcount;
	struct zvolume *zarchive;

	int reinsertdelay;
	TCHAR *newvolume;
	TCHAR *newrootdir;
	int newreadonly;
	int newflags;

} Unit;

static uae_u32 a_uniq, key_uniq;

typedef uae_u8 *dpacket;
#define PUT_PCK_RES1(p,v) do { do_put_mem_long ((uae_u32 *)((p) + dp_Res1), (v)); } while (0)
#define PUT_PCK_RES2(p,v) do { do_put_mem_long ((uae_u32 *)((p) + dp_Res2), (v)); } while (0)
#define GET_PCK_TYPE(p) ((uae_s32)(do_get_mem_long ((uae_u32 *)((p) + dp_Type))))
#define GET_PCK_RES1(p) ((uae_s32)(do_get_mem_long ((uae_u32 *)((p) + dp_Res1))))
#define GET_PCK_RES2(p) ((uae_s32)(do_get_mem_long ((uae_u32 *)((p) + dp_Res2))))
#define GET_PCK_ARG1(p) ((uae_s32)(do_get_mem_long ((uae_u32 *)((p) + dp_Arg1))))
#define GET_PCK_ARG2(p) ((uae_s32)(do_get_mem_long ((uae_u32 *)((p) + dp_Arg2))))
#define GET_PCK_ARG3(p) ((uae_s32)(do_get_mem_long ((uae_u32 *)((p) + dp_Arg3))))
#define GET_PCK_ARG4(p) ((uae_s32)(do_get_mem_long ((uae_u32 *)((p) + dp_Arg4))))
#define GET_PCK_ARG5(p) ((uae_s32)(do_get_mem_long ((uae_u32 *)((p) + dp_Arg5))))

#define PUT_PCK64_RES0(p,v) do { do_put_mem_long ((uae_u32 *)((p) + dp64_Res0), (v)); } while (0)
#define PUT_PCK64_RES1(p,v) do { do_put_mem_long ((uae_u32 *)((p) + dp64_Res1), (((uae_u64)v) >> 32)); do_put_mem_long ((uae_u32 *)((p) + dp64_Res1 + 4), ((uae_u32)v)); } while (0)
#define PUT_PCK64_RES2(p,v) do { do_put_mem_long ((uae_u32 *)((p) + dp64_Res2), (v)); } while (0)

#define GET_PCK64_TYPE(p) ((uae_s32)(do_get_mem_long ((uae_u32 *)((p) + dp64_Type))))
#define GET_PCK64_RES0(p) ((uae_s32)(do_get_mem_long ((uae_u32 *)((p) + dp64_Res0))))
#define GET_PCK64_RES1(p) ( (((uae_s64)(do_get_mem_long ((uae_u32 *)((p) + dp64_Res1)))) << 32) | (((uae_s64)(do_get_mem_long ((uae_u32 *)((p) + dp64_Res1 + 4)))) << 0) )
#define GET_PCK64_ARG1(p) ((uae_s32)(do_get_mem_long ((uae_u32 *)((p) + dp64_Arg1))))
#define GET_PCK64_ARG2(p) ( (((uae_s64)(do_get_mem_long ((uae_u32 *)((p) + dp64_Arg2)))) << 32) | (((uae_s64)(do_get_mem_long ((uae_u32 *)((p) + dp64_Arg2 + 4)))) << 0) )
#define GET_PCK64_ARG3(p) ((uae_s32)(do_get_mem_long ((uae_u32 *)((p) + dp64_Arg3))))
#define GET_PCK64_ARG4(p) ((uae_s32)(do_get_mem_long ((uae_u32 *)((p) + dp64_Arg4))))
#define GET_PCK64_ARG5(p) ( (((uae_s64)(do_get_mem_long ((uae_u32 *)((p) + dp64_Arg5)))) << 32) | (((uae_s64)(do_get_mem_long ((uae_u32 *)((p) + dp64_Arg5 + 4)))) << 0) )

static int flush_cache (Unit *unit, int num);

static char *char1 (uaecptr addr)
{
	static uae_char buf[1024];
	static char bufx[1024];
    unsigned int i = 0;
    do {
		buf[i] = get_byte (addr);
		addr++;
    } while (buf[i++] && i < sizeof(buf));
	return au_fs_copy (bufx, sizeof (bufx) / sizeof (TCHAR), buf);
}

static TCHAR *bstr1 (uaecptr addr)
{
	static char bufx[257];
	static uae_char buf[257];
    int i;
    int n = get_byte (addr);
    addr++;

    for (i = 0; i < n; i++, addr++)
		buf[i] = get_byte (addr);
    buf[i] = 0;
	return au_fs_copy (bufx, sizeof (bufx) / sizeof (TCHAR), buf);
}

static char *bstr (Unit *unit, uaecptr addr)
{
    int i;
    int n = get_byte (addr);
	uae_char buf[257];

    addr++;
    for (i = 0; i < n; i++, addr++)
		buf[i] = get_byte (addr);
	buf[i] = 0;
	au_fs_copy (unit->tmpbuf3, sizeof (unit->tmpbuf3) / sizeof (TCHAR), buf);
	return unit->tmpbuf3;
}
/* note */
static char *bstr_cut (Unit *unit, uaecptr addr)
{
	char *p = unit->tmpbuf3;
	int i, colon_seen = 0, off;
    int n = get_byte (addr);
	uae_char buf[257];

	off = 0;
    addr++;
    for (i = 0; i < n; i++, addr++) {
	uae_u8 c = get_byte (addr);
		buf[i] = c;
	if (c == '/' || (c == ':' && colon_seen++ == 0))
			off = i + 1;
    }
	buf[i] = 0;
	au_fs_copy (unit->tmpbuf3, sizeof (unit->tmpbuf3) / sizeof (TCHAR), buf);
	return &p[off];
}

/* convert time_t to/from AmigaDOS time */
static const uae_s64 msecs_per_day = 24 * 60 * 60 * 1000;
static const uae_s64 diff = ((8 * 365 + 2) * (24 * 60 * 60)) * (uae_u64)1000;

void timeval_to_amiga (struct mytimeval *tv, int *days, int *mins, int *ticks)
{
	/* tv.tv_sec is secs since 1-1-1970 */
	/* days since 1-1-1978 */
	/* mins since midnight */
	/* ticks past minute @ 50Hz */

	uae_s64 t = tv->tv_sec * 1000 + tv->tv_usec / 1000;
	t -= diff;
	if (t < 0)
		t = 0;
	*days = t / msecs_per_day;
	t -= *days * msecs_per_day;
	*mins = t / (60 * 1000);
	t -= *mins * (60 * 1000);
	*ticks = t / (1000 / 50);
}

void amiga_to_timeval (struct mytimeval *tv, int days, int mins, int ticks)
{
	uae_s64 t;

	if (days < 0)
		days = 0;
	if (days > 9900 * 365)
		days = 9900 * 365; // in future far enough?
	if (mins < 0 || mins >= 24 * 60)
		mins = 0;
	if (ticks < 0 || ticks >= 60 * 50)
		ticks = 0;

	t = ticks * 20;
	t += mins * (60 * 1000);
	t += ((uae_u64)days) * msecs_per_day;
	t += diff;

	tv->tv_sec = t / 1000;
	tv->tv_usec = (t % 1000) * 1000;
}

static Unit *units = 0;
static int unit_num = 0;

static Unit*
find_unit (uaecptr port)
{
    Unit* u;
    for (u = units; u; u = u->next)
	if (u->port == port)
	    break;

    return u;
}

static struct fs_dirhandle *fs_opendir (Unit *u, const TCHAR *nname)
{
	struct fs_dirhandle *fsd = xmalloc (struct fs_dirhandle, 1);
/*	fsd->isarch = !!(u->volflags & MYVOLUMEINFO_ARCHIVE);
	if (fsd->isarch)
		fsd->zd = zfile_opendir_archive (nname);
	else*/
		fsd->od = my_opendir (nname);
	return fsd;
}
static void fs_closedir (struct fs_dirhandle *fsd)
{
	if (!fsd)
		return;
	if (fsd->isarch  == FS_ARCHIVE)
		zfile_closedir_archive (fsd->zd);
	else if (fsd->isarch == FS_DIRECTORY)
		my_closedir (fsd->od);
/*	else if (fsd->isarch == FS_CDFS)
		isofs_closedir (fsd->isod);*/
	xfree (fsd);
}
static struct fs_filehandle *fs_open (Unit *unit, const TCHAR *name, char *flags)
{
	struct fs_filehandle *fsf = xmalloc (struct fs_filehandle, 1);
/*	fsf->isarch = !!(unit->volflags & MYVOLUMEINFO_ARCHIVE);
	if (fsf->isarch)
		fsf->zf = zfile_open_archive (name, flags);
	else*/
write_log(">>>>>>>>>>>>>>>>>>>>>>>\n\nopen: %s %s\n", name, flags);
		fsf->of = my_open (name, flags);
	return fsf;
}
static void fs_close (struct fs_filehandle *fd)
{
	if (!fd)
		return;
/*	if (fd->isarch)
		zfile_close_archive (fd->zf);
	else*/
		my_close (fd->of);
}
static unsigned int fs_read (struct fs_filehandle *fsf, void *b, unsigned int size)
{
	if (fsf->isarch == FS_ARCHIVE)
		return zfile_read_archive (fsf->zf, b, size);
	else if (fsf->isarch == FS_DIRECTORY)
		return my_read (fsf->of, b, size);
/*	else if (fsf->isarch == FS_CDFS)
		return isofs_read (fsf->isof, b, size);*/
	return 0;
}
static unsigned int fs_write (struct fs_filehandle *fsf, void *b, unsigned int size)
{
	if (fsf->isarch == FS_DIRECTORY)
		return my_write (fsf->of, b, size);
	return 0;
}

/* return value = old position. -1 = error. */
static uae_s64 fs_lseek64 (struct fs_filehandle *fsf, uae_s64 offset, int whence)
{
	if (fsf->isarch == FS_ARCHIVE)
		return zfile_lseek_archive (fsf->zf, offset, whence);
	else if (fsf->isarch == FS_DIRECTORY)
		return my_lseek (fsf->of, offset, whence);
/*	else if (fsf->isarch == FS_CDFS)
		return isofs_lseek (fsf->isof, offset, whence);*/
	return -1;
}
static uae_s32 fs_lseek (struct fs_filehandle *fsf, uae_s32 offset, int whence)
{
	uae_s64 v = fs_lseek64 (fsf, offset, whence);
	if (v < 0 || v > 0x7fffffff)
		return -1;
	return (uae_s32)v;
}
static uae_s64 fs_fsize64 (struct fs_filehandle *fsf)
{
	if (fsf->isarch == FS_ARCHIVE)
		return zfile_fsize_archive (fsf->zf);
	else if (fsf->isarch == FS_DIRECTORY)
		return my_fsize (fsf->of);
/*	else if (fsf->fstype == FS_CDFS)
		return isofs_fsize (fsf->isof);*/
	return -1;
}

/* note puae 2.x timeval */
static void set_volume_name (Unit *unit)
{
	int namelen;
	int i;
	char *s;

	s = ua_fs (unit->ui.volname, -1);
	namelen = strlen (s);
	put_byte (unit->volume + 44, namelen);
	for (i = 0; i < namelen; i++)
		put_byte (unit->volume + 45 + i, s[i]);
	put_byte (unit->volume + 45 + namelen, 0);
	xfree (s);
	unit->rootnode.aname = unit->ui.volname;
	unit->rootnode.nname = unit->ui.rootdir;
	unit->rootnode.mountcount = unit->mountcount;
}

static int filesys_isvolume (Unit *unit)
{
	if (!unit->volume)
		return 0;
	return get_byte (unit->volume + 44) || unit->ui.unknown_media;
}

static void clear_exkeys (Unit *unit)
{
	int i;
	a_inode *a;
	for (i = 0; i < EXKEYS; i++) {
		unit->examine_keys[i].aino = 0;
		unit->examine_keys[i].curr_file = 0;
		unit->examine_keys[i].uniq = 0;
	}
	for (i = 0; i < EXALLKEYS; i++) {
		fs_closedir (unit->exalls[i].dirhandle);
		unit->exalls[i].dirhandle = NULL;
		xfree (unit->exalls[i].fn);
		unit->exalls[i].fn = NULL;
		unit->exalls[i].id = 0;
	}
	unit->exallid = 0;
	unit->next_exkey = 1;
	a = &unit->rootnode;
	while (a) {
		a->exnext_count = 0;
		if (a->locked_children) {
			a->locked_children = 0;
			unit->total_locked_ainos--;
		}
		a = a->next;
		if (a == &unit->rootnode)
			break;
	}
}

int filesys_eject (struct uaedev_mount_info *mountinfo, int nr)
{
	UnitInfo *ui = &mountinfo->ui[nr];
	Unit *u = ui->self;

	if (!mountertask)
		return 0;
	if (!ui->open || u == NULL)
		return 0;
	if (is_hardfile (&current_mountinfo, nr) != FILESYS_VIRTUAL)
		return 0;
	if (!filesys_isvolume (u))
		return 0;
	//zfile_fclose_archive (u->zarchive);
	u->zarchive = NULL;
	u->mountcount++;
	write_log ("FILESYS: removed volume '%s'\n", u->ui.volname);
	flush_cache (u, -1);
	put_byte (u->volume + 172 - 32, -2);
	uae_Signal (get_long (u->volume + 176 - 32), 1 << 13);
	return 1;
}

void filesys_vsync (struct uaedev_mount_info *mountinfo)
{
	Unit *u;

	for (u = units; u; u = u->next) {
		if (u->reinsertdelay > 0) {
			u->reinsertdelay--;
			if (u->reinsertdelay == 0) {
				filesys_insert (&current_mountinfo, u->unit, u->newvolume, u->newrootdir, u->newreadonly, u->newflags);
				xfree (u->newvolume);
				u->newvolume = NULL;
				xfree (u->newrootdir);
				u->newrootdir = NULL;
			}
		}
	}
}
static void filesys_delayed_change (Unit *u, int frames, const TCHAR *rootdir, const TCHAR *volume, int readonly, int flags)
{
	u->reinsertdelay = 50;
	u->newflags = flags;
	u->newreadonly = readonly;
	u->newrootdir = my_strdup (rootdir);
	if (volume)
		u->newvolume = my_strdup (volume);
	filesys_eject(&current_mountinfo, u->unit);
	if (!rootdir || _tcslen (rootdir) == 0)
		u->reinsertdelay = 0;
	if (u->reinsertdelay > 0)
		write_log ("FILESYS: delayed insert %d: '%s' ('%s')\n", u->unit, volume ? volume : "<none>", rootdir);
}

int filesys_media_change (struct uaedev_mount_info *mountinfo, const char *rootdir, int inserted, struct uaedev_config_info *uci)
{
	Unit *u;
	UnitInfo *ui;
	int nr = -1;
	char volname[MAX_DPATH], *volptr;
	char devname[MAX_DPATH];

	if (!mountertask)
		return 0;
	if (automountunit >= 0)
		return -1;
	
	write_log (_T("filesys_media_change('%s',%d,%p)\n"), rootdir, inserted, uci);
	
	nr = -1;
	for (u = units; u; u = u->next) {
		if (is_hardfile (&current_mountinfo, u->unit) == FILESYS_VIRTUAL) {
			ui = &mountinfo->ui[u->unit];
			if (ui->rootdir && !memcmp (ui->rootdir, rootdir, _tcslen (rootdir)) && _tcslen (rootdir) + 3 >= _tcslen (ui->rootdir)) {
				if (filesys_isvolume (u) && inserted) {
					if (uci)
						filesys_delayed_change (u, 50, rootdir, uci->volname, uci->readonly, 0);
					return 0;
				}
				nr = u->unit;
				break;
			}
		}
	}
	ui = NULL;
	if (nr >= 0)
		ui = &mountinfo->ui[nr];
	/* only configured drives have automount support if automount is disabled */
	if ((!ui || !ui->configureddrive) && (inserted == 0 || inserted == 1))
		return 0;
	if (nr < 0 && !inserted)
		return 0;
	/* already mounted volume was ejected? */
	if (nr >= 0 && !inserted)
		return filesys_eject (&current_mountinfo, nr);
	if (inserted) {
		if (uci) {
			volptr = my_strdup (uci->volname);
		} else {
			volname[0] = 0;
			//FIXME: target_get_volume_name (&mountinfo, rootdir, volname, MAX_DPATH, 1, 0);
			volptr = volname;
			if (!volname[0])
				volptr = NULL;
			if (ui && ui->configureddrive && ui->volname) {
				volptr = volname;
				_tcscpy (volptr, ui->volname);
			}
		}
		if (!volptr) {
			volptr = filesys_createvolname (NULL, rootdir, "removable");
			_tcscpy (volname, volptr);
			xfree (volptr);
			volptr = volname;
		}

		/* new volume inserted and it was previously mounted? */
		if (nr >= 0) {
			if (!filesys_isvolume (u)) /* not going to mount twice */
				return filesys_insert (&current_mountinfo, nr, volptr, rootdir, -1, -1);
			return 0;
		}
		if (inserted < 0) /* -1 = only mount if already exists */
			return 0;
		/* new volume inserted and it was not previously mounted? 
		* perhaps we have some empty device slots? */
		nr = filesys_insert (&current_mountinfo, -1, volptr, rootdir, 0, 0);
		if (nr >= 100) {
			if (uci)
				uci->configoffset = nr - 100;
			return nr;
		}
		/* nope, uh, need black magic now.. */
		if (uci)
			_tcscpy (devname, uci->devname);
		else
			_stprintf (devname, "RDH%d", nr_units(currprefs.mountinfo));
		nr = add_filesys_unit (mountinfo, devname, volptr, rootdir, 0, 0, 0, 0, 0, 0, 0, 1, NULL, 0, MYVOLUMEINFO_REUSABLE);
		if (nr < 0)
			return 0;
		if (inserted > 1)
			mountinfo->ui[nr].canremove = 1;
		automountunit = nr;
		uae_Signal (mountertask, 1 << 13);
		/* poof */
		if (uci)
			uci->configoffset = nr;
		return 100 + nr;
	}
	return 0;
}
// REMOVEME:
#if 0
int hardfile_remount (int nr)
{
	/* this does work but every media reinsert duplicates the device.. */
#if 0
	if (!mountertask)
		return 0;
	automountunit = nr;
	uae_Signal (mountertask, 1 << 13);
#endif
	return 1;
}
#endif // 0
/* note */
int filesys_insert (struct uaedev_mount_info *mountinfo, int nr, TCHAR *volume, const TCHAR *rootdir, int readonly, int flags)
{
	struct uaedev_config_info *uci;
	int emptydrive = 0;
	UnitInfo *ui;
	Unit *u;

	if (!mountertask)
		return 0;
	if (nr < 0) {
		for (u = units; u; u = u->next) {
			if (is_hardfile (&current_mountinfo, u->unit) == FILESYS_VIRTUAL) {
				if (!filesys_isvolume (u) && mountinfo->ui[u->unit].canremove)
					break;
			}
		}
		if (!u) {
			for (u = units; u; u = u->next) {
				if (is_hardfile (&current_mountinfo, u->unit) == FILESYS_VIRTUAL) {
					if (mountinfo->ui[u->unit].canremove)
						break;
				}
			}
		}
		if (!u)
			return 0;
		nr = u->unit;
		ui = &mountinfo->ui[nr];
	} else {
		ui = &mountinfo->ui[nr];
		u = ui->self;
	}
	uci = &currprefs.mountconfig[nr];

	if (!ui->open || u == NULL)
		return 0;
	if (u->reinsertdelay)
		return -1;
	if (is_hardfile (&mountinfo, nr) != FILESYS_VIRTUAL)
		return 0;
	if (filesys_isvolume (u)) {
		filesys_delayed_change (u, 50, rootdir, volume, readonly, flags);
		return -1;
	}
	u->mountcount++;
	clear_exkeys (u);
	xfree (u->ui.rootdir);
	ui->rootdir = u->ui.rootdir = my_strdup (rootdir);
	flush_cache (u, -1);
	if (set_filesys_volume (rootdir, &flags, &readonly, &emptydrive, &u->zarchive) < 0)
		return 0;
	if (emptydrive)
		return 0;
	xfree (u->ui.volname);
	ui->volname = u->ui.volname = filesys_createvolname (volume, rootdir, "removable");
	set_volume_name (u);
	write_log ("FILESYS: inserted volume NR=%d RO=%d '%s' ('%s')\n", nr, readonly, ui->volname, rootdir);
	if (flags >= 0)
		ui->volflags = u->volflags = u->ui.volflags = flags;
	_tcscpy (uci->volname, ui->volname);
	_tcscpy (uci->rootdir, rootdir);
	if (readonly >= 0)
		uci->readonly = ui->readonly = u->ui.readonly = readonly;
	put_byte (u->volume + 44, 0);
	put_byte (u->volume + 172 - 32, 1);
	uae_Signal (get_long (u->volume + 176 - 32), 1 << 13);
	return 100 + nr;
}

/* flags and comments supported? */
static int fsdb_cando (Unit *unit)
{
	if (unit->volflags & MYVOLUMEINFO_ARCHIVE)
		return 1;
	if (currprefs.filesys_custom_uaefsdb  && (unit->volflags & MYVOLUMEINFO_STREAMS))
		return 1;
	if (!currprefs.filesys_no_uaefsdb)
		return 1;
	return 0;
}

static void prepare_for_open (TCHAR *name)
{
#if 0
	struct _stat64 statbuf;
	int mode;

	if (-1 == stat (name, &statbuf))
		return;

	mode = statbuf.st_mode;
	mode |= S_IRUSR;
	mode |= S_IWUSR;
	mode |= S_IXUSR;
	chmod (name, mode);
#endif
}

static void de_recycle_aino (Unit *unit, a_inode *aino)
{
    aino_test (aino);
    if (aino->next == 0 || aino == &unit->rootnode)
		return;
    aino->next->prev = aino->prev;
    aino->prev->next = aino->next;
    aino->next = aino->prev = 0;
    unit->aino_cache_size--;
}

static void dispose_aino (Unit *unit, a_inode **aip, a_inode *aino)
{
    int hash = aino->uniq % MAX_AINO_HASH;
    if (unit->aino_hash[hash] == aino)
		unit->aino_hash[hash] = 0;

    if (aino->dirty && aino->parent)
		fsdb_dir_writeback (aino->parent);

    *aip = aino->sibling;
    xfree (aino->aname);
	xfree (aino->comment);
    xfree (aino->nname);
    xfree (aino);
}

static void free_all_ainos (Unit *u, a_inode *parent)
{
	a_inode *a;
	while ((a = parent->child)) {
		free_all_ainos (u, a);
		dispose_aino (u, &parent->child, a);
	}
}

static int flush_cache (Unit *unit, int num)
{
	int i = 0;
	int cnt = 100;

	write_log ("FILESYS: flushing cache unit %d (max %d items)\n", unit->unit, num);
	if (num == 0)
		num = -1;
	while (i < num || num < 0) {
		int ii = i;
		a_inode *parent = unit->rootnode.prev->parent;
		a_inode **aip;

		aip = &parent->child;
		aino_test (parent);
		if (parent && !parent->locked_children) {
			for (;;) {
				a_inode *aino = *aip;
				aino_test (aino);
				if (aino == 0)
					break;
				/* Not recyclable if next == 0 (i.e., not chained into
				recyclable list), or if parent directory is being
				ExNext()ed.  */
				if (aino->next == 0) {
					aip = &aino->sibling;
				} else {
					if (aino->shlock > 0 || aino->elock)
						write_log ("panic: freeing locked a_inode!\n");
					de_recycle_aino (unit, aino);
					dispose_aino (unit, aip, aino);
					i++;
				}
			}
		}
		{ //if (unit->rootnode.next != unit->rootnode.prev) {
			/* In the previous loop, we went through all children of one
			parent.  Re-arrange the recycled list so that we'll find a
			different parent the next time around.
			(infinite loop if there is only one parent?)
			*/
			int maxloop = 10000;
			do {
				unit->rootnode.next->prev = unit->rootnode.prev;
				unit->rootnode.prev->next = unit->rootnode.next;
				unit->rootnode.next = unit->rootnode.prev;
				unit->rootnode.prev = unit->rootnode.prev->prev;
				unit->rootnode.prev->next = unit->rootnode.next->prev = &unit->rootnode;
			} while (unit->rootnode.prev->parent == parent && maxloop-- > 0);
		}
		if (i == ii)
			cnt--;
		if (cnt <= 0)
			break;
	}
	return unit->aino_cache_size > 0 ? 0 : 1;
}

static void recycle_aino (Unit *unit, a_inode *new_aino)
{
    aino_test (new_aino);
    if (new_aino->dir || new_aino->shlock > 0
		|| new_aino->elock || new_aino == &unit->rootnode)
		/* Still in use */
		return;

	TRACE (("Recycling; cache size %d, total_locked %d\n",
	    unit->aino_cache_size, unit->total_locked_ainos));
    if (unit->aino_cache_size > 5000 + unit->total_locked_ainos) {
		/* Reap a few. */
		flush_cache (unit, 50);
#if 0
	{
	    char buffer[40];
	    sprintf (buffer, "%d ainos reaped.\n", i);
	    TRACE ((buffer));
	}
#endif
    }

    aino_test (new_aino);
    /* Chain it into circular list. */
    new_aino->next = unit->rootnode.next;
    new_aino->prev = &unit->rootnode;
    new_aino->prev->next = new_aino;
    new_aino->next->prev = new_aino;
    aino_test (new_aino->next);
    aino_test (new_aino->prev);

    unit->aino_cache_size++;
}

void filesys_flush_cache (void)
{
}

static void update_child_names (Unit *unit, a_inode *a, a_inode *parent)
{
	int l0 = _tcslen (parent->nname) + 2;

    while (a != 0) {
		char *name_start;
		char *new_name;
		char dirsep[2] = { FSDB_DIR_SEPARATOR, '\0' };

		a->parent = parent;
		name_start = _tcsrchr (a->nname, FSDB_DIR_SEPARATOR);
		if (name_start == 0) {
			write_log ("malformed file name");
		}
		name_start++;
	new_name = (char *)xmalloc (char, strlen (name_start) + l0);
		_tcscpy (new_name, parent->nname);
		_tcscat (new_name, dirsep);
		_tcscat (new_name, name_start);
		xfree (a->nname);
		a->nname = new_name;
		if (a->child)
		    update_child_names (unit, a->child, a);
		a = a->sibling;
    }
}

static void move_aino_children (Unit *unit, a_inode *from, a_inode *to)
{
    aino_test (from);
    aino_test (to);
    to->child = from->child;
    from->child = 0;
    update_child_names (unit, to->child, to);
}

static void delete_aino (Unit *unit, a_inode *aino)
{
    a_inode **aip;

    TRACE(("deleting aino %x\n", aino->uniq));

    aino_test (aino);
    aino->dirty = 1;
    aino->deleted = 1;
    de_recycle_aino (unit, aino);

    /* If any ExKeys are currently pointing at us, advance them.  */
    if (aino->parent->exnext_count > 0) {
		int i;
		TRACE(("entering exkey validation\n"));
		for (i = 0; i < EXKEYS; i++) {
		    ExamineKey *k = unit->examine_keys + i;
		    if (k->uniq == 0)
				continue;
	    	if (k->aino == aino->parent) {
				TRACE(("Same parent found for %d\n", i));
				if (k->curr_file == aino) {
				    k->curr_file = aino->sibling;
					TRACE(("Advancing curr_file\n"));
				}
		    }
		}
    }

    aip = &aino->parent->child;
    while (*aip != aino && *aip != 0)
		aip = &(*aip)->sibling;
    if (*aip != aino) {
		write_log ("Couldn't delete aino.\n");
		return;
    }
    dispose_aino (unit, aip, aino);
}

static a_inode *lookup_sub (a_inode *dir, uae_u32 uniq)
{
    a_inode **cp = &dir->child;
    a_inode *c, *retval;

    for (;;) {
		c = *cp;
		if (c == 0)
		    return 0;

		if (c->uniq == uniq) {
		    retval = c;
		    break;
		}
		if (c->dir) {
		    a_inode *a = lookup_sub (c, uniq);
		    if (a != 0) {
				retval = a;
				break;
		    }
		}
		cp = &c->sibling;
    }
    if (! dir->locked_children) {
		/* Move to the front to speed up repeated lookups.  Don't do this if
	   an ExNext is going on in this directory, or we'll terminally
	   confuse it.  */
		*cp = c->sibling;
		c->sibling = dir->child;
		dir->child = c;
    }
    return retval;
}

static a_inode *lookup_aino (Unit *unit, uae_u32 uniq)
{
    a_inode *a;
    int hash = uniq % MAX_AINO_HASH;

    if (uniq == 0)
		return &unit->rootnode;
    a = unit->aino_hash[hash];
    if (a == 0 || a->uniq != uniq)
		a = lookup_sub (&unit->rootnode, uniq);
    else
		unit->nr_cache_hits++;
    unit->nr_cache_lookups++;
    unit->aino_hash[hash] = a;
    aino_test (a);
    return a;
}

char *build_nname (const char *d, const char *n)
{
	char dsep[2] = { FSDB_DIR_SEPARATOR, 0 };
	char *p = xmalloc (char, _tcslen (d) + _tcslen (n) + 2);
	_tcscpy (p, d);
	_tcscat (p, dsep);
	_tcscat (p, n);
	return p;
}

char *build_aname (const char *d, const char *n)
{
    char *p = xmalloc (char, _tcslen (d) + _tcslen (n) + 2);
	_tcscpy (p, d);
	_tcscat (p, "/");
	_tcscat (p, n);
    return p;
}

/* This gets called to translate an Amiga name that some program used to
 * a name that we can use on the native filesystem.  */
static char *get_nname (Unit *unit, a_inode *base, char *rel,
			char **modified_rel)
{
    char *found;

    *modified_rel = 0;

    aino_test (base);

    /* If we have a mapping of some other aname to "rel", we must pretend
     * it does not exist.
     * This can happen for example if an Amiga program creates a
     * file called ".".  We can't represent this in our filesystem,
     * so we create a special file "uae_xxx" and record the mapping
     * aname "." -> nname "uae_xxx" in the database.  Then, the Amiga
     * program looks up "uae_xxx" (yes, it's contrived).  The filesystem
     * should not make the uae_xxx file visible to the Amiga side.  */
    if (fsdb_used_as_nname (base, rel))
		return 0;
    /* A file called "." (or whatever else is invalid on this filesystem)
	* does not exist, as far as the Amiga side is concerned.  */
    if (fsdb_name_invalid (rel))
		return 0;

    /* See if we have a file that has the same name as the aname we are
     * looking for.  */
    found = fsdb_search_dir (base->nname, rel);
    if (found == 0)
		return found;
    if (found == rel)
		return build_nname (base->nname, rel);

    *modified_rel = found;
    return build_nname (base->nname, found);
}

static char *create_nname (Unit *unit, a_inode *base, char *rel)
{
    char *p;

    aino_test (base);
    /* We are trying to create a file called REL.  */

    /* If the name is used otherwise in the directory (or globally), we
     * need a new unique nname.  */
    if (fsdb_name_invalid (rel) || fsdb_used_as_nname (base, rel)) {
#if 0
	oh_dear:
#endif
		if (currprefs.filesys_no_uaefsdb && !(base->volflags & MYVOLUMEINFO_STREAMS)) {
			write_log ("illegal filename '%s', no stream supporting filesystem and uaefsdb disabled\n", rel);
			return 0;
		}
		p = fsdb_create_unique_nname (base, rel);
		return p;
    }
    p = build_nname (base->nname, rel);
#if 0
    /* Delete this code once we know everything works.  */
    if (access (p, R_OK) >= 0 || errno != ENOENT) {
	write_log ("Filesystem in trouble... please report.\n");
	xfree (p);
	goto oh_dear;
    }
#endif
    return p;
}

static int fill_file_attrs (Unit *u, a_inode *base, a_inode *c)
{
	if (0 /*u->volflags & MYVOLUMEINFO_ARCHIVE*/) {
		int isdir, flags;
		char *comment;
		zfile_fill_file_attrs_archive (c->nname, &isdir, &flags, &comment);
		c->dir = isdir;
		c->amigaos_mode = 0;
		if (flags >= 0)
			c->amigaos_mode = flags;
		c->comment = comment;
		return 1;
	} else {
		return fsdb_fill_file_attrs (base, c);
	}
	return 0;
}

static int test_softlink (a_inode *aino)
{
	int err;
	if (aino->softlink && my_resolvesoftlink (aino->nname, -1))
		err = ERROR_IS_SOFT_LINK;
	else
		err = ERROR_OBJECT_NOT_AROUND;
	return err;
}

static void handle_softlink (Unit *unit, dpacket packet, a_inode *aino)
{
	PUT_PCK_RES1 (packet, DOS_FALSE);
	PUT_PCK_RES2 (packet, test_softlink (aino));
}

/*
 * This gets called if an ACTION_EXAMINE_NEXT happens and we hit an object
 * for which we know the name on the native filesystem, but no corresponding
 * Amiga filesystem name.
 * @@@ For DOS filesystems, it might make sense to declare the new name
 * "weak", so that it can get overriden by a subsequent call to get_nname().
 * That way, if someone does "dir :" and there is a file "foobar.inf", and
 * someone else tries to open "foobar.info", get_nname() could maybe made to
 * figure out that this is supposed to be the file "foobar.inf".
 * DOS sucks...
 */
static char *get_aname (Unit *unit, a_inode *base, char *rel)
{
    return my_strdup (rel);
}

static void init_child_aino_tree (Unit *unit, a_inode *base, a_inode *aino)
{
	/* Update tree structure */
	aino->parent = base;
	aino->child = 0;
	aino->sibling = base->child;
	base->child = aino;
	aino->next = aino->prev = 0;
	aino->volflags = unit->volflags;
}

static void init_child_aino (Unit *unit, a_inode *base, a_inode *aino)
{
    aino->uniq = ++a_uniq;
    if (a_uniq == 0xFFFFFFFF) {
		write_log ("Running out of a_inodes (prepare for big trouble)!\n");
    }
    aino->shlock = 0;
    aino->elock = 0;

    aino->dirty = 0;
    aino->deleted = 0;
	aino->mountcount = unit->mountcount;

    /* For directories - this one isn't being ExNext()ed yet.  */
    aino->locked_children = 0;
    aino->exnext_count = 0;
    /* But the parent might be.  */
    if (base->exnext_count) {
		unit->total_locked_ainos++;
		base->locked_children++;
    }
	init_child_aino_tree (unit, base, aino);

    aino_test_init (aino);
    aino_test (aino);
}

static a_inode *new_child_aino (Unit *unit, a_inode *base, char *rel)
{
    char *modified_rel;
    char *nn;
	a_inode *aino = NULL;
	int isarch = unit->volflags & MYVOLUMEINFO_ARCHIVE;

    TRACE(("new_child_aino %s, %s\n", base->aname, rel));

	if (!isarch)
	    aino = fsdb_lookup_aino_aname (base, rel);
    if (aino == 0) {
		nn = get_nname (unit, base, rel, &modified_rel);
		if (nn == 0)
		    return 0;

		aino = xcalloc (a_inode, 1);
		if (aino == 0)
		    return 0;
		aino->aname = modified_rel ? modified_rel : my_strdup (rel);
		aino->nname = nn;

		aino->comment = 0;
		aino->has_dbentry = 0;

		if (!fill_file_attrs (unit, base, aino)) {
		    xfree (aino);
		    return 0;
		}
		if (aino->dir)
		    fsdb_clean_dir (aino);
    }
    init_child_aino (unit, base, aino);

    recycle_aino (unit, aino);
    TRACE(("created aino %x, lookup, amigaos_mode %d\n", aino->uniq, aino->amigaos_mode));
    return aino;
}

static a_inode *create_child_aino (Unit *unit, a_inode *base, char *rel, int isdir)
{
	a_inode *aino = xcalloc (a_inode, 1);
    if (aino == 0)
		return 0;

    aino->nname = create_nname (unit, base, rel);
    if (!aino->nname) {
		free (aino);
		return 0;
    }
    aino->aname = my_strdup (rel);

    init_child_aino (unit, base, aino);
    aino->amigaos_mode = 0;
    aino->dir = isdir;

    aino->comment = 0;
    aino->has_dbentry = 0;
    aino->dirty = 1;

    recycle_aino (unit, aino);
    TRACE(("created aino %x, create\n", aino->uniq));
    return aino;
}

static a_inode *lookup_child_aino (Unit *unit, a_inode *base, char *rel, uae_u32 *err)
{
    a_inode *c = base->child;
	int l0 = _tcslen (rel);

    aino_test (base);
    aino_test (c);

    if (base->dir == 0) {
	*err = ERROR_OBJECT_WRONG_TYPE;
	return 0;
    }

    while (c != 0) {
		int l1 = _tcslen (c->aname);
	if (l0 <= l1 && same_aname (rel, c->aname + l1 - l0)
			&& (l0 == l1 || c->aname[l1-l0-1] == '/') && c->mountcount == unit->mountcount)
	    break;
	c = c->sibling;
    }
    if (c != 0)
	return c;
    c = new_child_aino (unit, base, rel);
    if (c == 0)
	*err = ERROR_OBJECT_NOT_AROUND;
    return c;
}

/* Different version because for this one, REL is an nname.  */
static a_inode *lookup_child_aino_for_exnext (Unit *unit, a_inode *base, char *rel, uae_u32 *err)
{
    a_inode *c = base->child;
	int l0 = _tcslen (rel);
	int isarch = unit->volflags & MYVOLUMEINFO_ARCHIVE;

    aino_test (base);
    aino_test (c);

    *err = 0;
    while (c != 0) {
		int l1 = _tcslen (c->nname);
		/* Note: using _tcscmp here.  */
		if (l0 <= l1 && _tcscmp (rel, c->nname + l1 - l0) == 0
			&& (l0 == l1 || c->nname[l1-l0-1] == FSDB_DIR_SEPARATOR) && c->mountcount == unit->mountcount)
		    break;
		c = c->sibling;
    }
    if (c != 0)
		return c;
	if (!isarch)
	    c = fsdb_lookup_aino_nname (base, rel);
    if (c == 0) {
		c = xcalloc (a_inode, 1);
		if (c == 0) {
		    *err = ERROR_NO_FREE_STORE;
		    return 0;
		}

		c->nname = build_nname (base->nname, rel);
		c->aname = get_aname (unit, base, rel);
		c->comment = 0;
		c->has_dbentry = 0;
		if (!fill_file_attrs (unit, base, c)) {
		    xfree (c);
		    *err = ERROR_NO_FREE_STORE;
		    return 0;
		}
		if (c->dir && !isarch)
		    fsdb_clean_dir (c);
    }
    init_child_aino (unit, base, c);

    recycle_aino (unit, c);
    TRACE(("created aino %x, exnext\n", c->uniq));

    return c;
}

static a_inode *get_aino (Unit *unit, a_inode *base, const char *rel, uae_u32 *err)
{
    char *tmp;
    char *p;
    a_inode *curr;
    int i;

    aino_test (base);

    *err = 0;
    TRACE(("get_path(%s,%s)\n", base->aname, rel));

    /* root-relative path? */
    for (i = 0; rel[i] && rel[i] != '/' && rel[i] != ':'; i++)
		;
    if (':' == rel[i])
		rel += i+1;

    tmp = my_strdup (rel);
    p = tmp;
    curr = base;

    while (*p) {
		/* start with a slash? go up a level. */
		if (*p == '/') {
		    if (curr->parent != 0)
				curr = curr->parent;
		    p++;
		} else {
		    a_inode *next;

			char *component_end;
			component_end = _tcschr (p, '/');
			if (component_end != 0)
				*component_end = '\0';
			next = lookup_child_aino (unit, curr, p, err);
			if (next == 0) {
				/* if only last component not found, return parent dir. */
				if (*err != ERROR_OBJECT_NOT_AROUND || component_end != 0)
					curr = 0;
				/* ? what error is appropriate? */
				break;
			}
			curr = next;
			if (component_end)
				p = component_end+1;
			else
				break;
		}
	}
    xfree (tmp);
    return curr;
}


static uae_u32 notifyhash (char *s)
{
    uae_u32 hash = 0;
	while (*s)
		hash = (hash << 5) + *s++;
    return hash % NOTIFY_HASH_SIZE;
}

static Notify *new_notify (Unit *unit, char *name)
{
	Notify *n = xmalloc (Notify, 1);
	uae_u32 hash = notifyhash (name);
    n->next = unit->notifyhash[hash];
    unit->notifyhash[hash] = n;
    n->partname = name;
    return n;
}

static void free_notify_item (Notify *n)
{
	xfree (n->fullname);
	xfree (n->partname);
	xfree (n);
}


static void startup_update_unit (Unit *unit, UnitInfo *uinfo)
{
    if (!unit)
		return;
    unit->ui.devname = uinfo->devname;
    xfree (unit->ui.volname);
    unit->ui.volname = my_strdup (uinfo->volname); /* might free later for rename */
    unit->ui.rootdir = uinfo->rootdir;
    unit->ui.readonly = uinfo->readonly;
    unit->ui.unit_pipe = uinfo->unit_pipe;
    unit->ui.back_pipe = uinfo->back_pipe;
	unit->ui.wasisempty = uinfo->wasisempty;
	unit->ui.canremove = uinfo->canremove;
}

static Unit *startup_create_unit (UnitInfo *uinfo, int num)
{
    int i;
	Unit *unit, *u;

	unit = xcalloc (Unit, 1);
	/* keep list in insertion order */
	u = units;
	if (u) {
		while (u->next)
			u = u->next;
		u->next = unit;
	} else {
	    units = unit;
	}
    uinfo->self = unit;

    unit->volume = 0;
	unit->port = m68k_areg (&regs, 5);
	unit->unit = num;

    startup_update_unit (unit, uinfo);

    unit->cmds_complete = 0;
    unit->cmds_sent = 0;
    unit->cmds_acked = 0;
	clear_exkeys (unit);
    unit->total_locked_ainos = 0;
    unit->keys = 0;
	for (i = 0; i < NOTIFY_HASH_SIZE; i++) {
		Notify *n = unit->notifyhash[i];
		while (n) {
			Notify *n2 = n;
			n = n->next;
			xfree (n2->fullname);
			xfree (n2->partname);
			xfree (n2);
		}
		unit->notifyhash[i] = 0;
	}

    unit->rootnode.aname = uinfo->volname;
    unit->rootnode.nname = uinfo->rootdir;
    unit->rootnode.sibling = 0;
    unit->rootnode.next = unit->rootnode.prev = &unit->rootnode;
    unit->rootnode.uniq = 0;
    unit->rootnode.parent = 0;
    unit->rootnode.child = 0;
    unit->rootnode.dir = 1;
    unit->rootnode.amigaos_mode = 0;
    unit->rootnode.shlock = 0;
    unit->rootnode.elock = 0;
    unit->rootnode.comment = 0;
    unit->rootnode.has_dbentry = 0;
	unit->rootnode.volflags = uinfo->volflags;
    aino_test_init (&unit->rootnode);
    unit->aino_cache_size = 0;
    for (i = 0; i < MAX_AINO_HASH; i++)
		unit->aino_hash[i] = 0;
    return unit;
}

#ifdef UAE_FILESYS_THREADS
static void *filesys_thread (void *unit_v);
#endif
static void filesys_start_thread (UnitInfo *ui, int nr)
{
	ui->unit_pipe = 0;
	ui->back_pipe = 0;
	ui->reset_state = FS_STARTUP;
	if (savestate_state != STATE_RESTORE) {
		ui->startup = 0;
		ui->self = 0;
	}
#ifdef UAE_FILESYS_THREADS
	if (is_hardfile (&mountinfo, nr) == FILESYS_VIRTUAL) {
		ui->unit_pipe = xmalloc (smp_comm_pipe, 1);
		ui->back_pipe = xmalloc (smp_comm_pipe, 1);
		init_comm_pipe (ui->unit_pipe, 100, 3);
		init_comm_pipe (ui->back_pipe, 100, 1);
		uae_start_thread ("filesys", filesys_thread, (void *)ui, &ui->tid);
	}
#endif
	if (savestate_state == STATE_RESTORE)
		startup_update_unit (ui->self, ui);
}

static uae_u32 REGPARAM2 startup_handler (struct uaedev_mount_info *mountinfo, TrapContext *context)
{
    /* Just got the startup packet. It's in A4. DosBase is in A2,
	* our allocated volume structure is in A3, A5 is a pointer to
     * our port. */
    uaecptr rootnode = get_long (m68k_areg (&context->regs, 2) + 34);
    uaecptr dos_info = get_long (rootnode + 24) << 2;
    uaecptr pkt = m68k_dreg (&context->regs, 3);
    uaecptr arg2 = get_long (pkt + dp_Arg2);
	uaecptr devnode;
	int i;
	char *devname = bstr1 (get_long (pkt + dp_Arg1) << 2);
	char *s;
    Unit *unit;
    UnitInfo *uinfo;
	int late = 0;
	int ed, ef;

    /* find UnitInfo with correct device name */
	s = _tcschr (devname, ':');
    if (s)
		*s = '\0';

	for (i = 0; i < MAX_FILESYSTEM_UNITS; i++) {
		/* Hardfile volume name? */
		if (!&current_mountinfo.ui[i].open)
	    	continue;
		if (is_hardfile(&current_mountinfo, i) != FILESYS_VIRTUAL)
			continue;
		if (current_mountinfo.ui[i].startup == arg2)
		    break;
    }

	if (i == MAX_FILESYSTEM_UNITS) {
		write_log ("Failed attempt to mount device '%s'\n", devname);
		put_long (pkt + dp_Res1, DOS_FALSE);
		put_long (pkt + dp_Res2, ERROR_DEVICE_NOT_MOUNTED);
		return 0;
    }
    uinfo = &current_mountinfo.ui + i;

	ed = my_existsdir (uinfo->rootdir);
	ef = my_existsfile (uinfo->rootdir);
	if (!uinfo->wasisempty && !ef && !ed)
	{
		write_log ("Failed attempt to mount device '%s'\n", devname);
		put_long (pkt + dp_Res1, DOS_FALSE);
		put_long (pkt + dp_Res2, ERROR_DEVICE_NOT_MOUNTED);
		return 0;
	}

	if (!uinfo->unit_pipe) {
		late = 1;
		filesys_start_thread (uinfo, i);
	}
	unit = startup_create_unit (uinfo, i);
	unit->volflags = uinfo->volflags;

/*    write_comm_pipe_int (unit->ui.unit_pipe, -1, 1);*/

	write_log ("FS: %s (flags=%08X,E=%d,ED=%d,EF=%d,native='%s') starting..\n",
		unit->ui.volname, unit->volflags, uinfo->wasisempty, ed, ef, unit->ui.rootdir);

    /* fill in our process in the device node */
	devnode = get_long (pkt + dp_Arg3) << 2;
	put_long (devnode + 8, unit->port);
    unit->dosbase = m68k_areg (&context->regs, 2);

    /* make new volume */
    unit->volume = m68k_areg (&context->regs, 3) + 32;
#ifdef UAE_FILESYS_THREADS
    unit->locklist = m68k_areg (&context->regs, 3) + 8;
#else
    unit->locklist = m68k_areg (&context->regs, 3);
#endif
    unit->dummy_message = m68k_areg (&context->regs, 3) + 12;

    put_long (unit->dummy_message + 10, 0);

    put_long (unit->volume + 4, 2); /* Type = dt_volume */
    put_long (unit->volume + 12, 0); /* Lock */
	put_long (unit->volume + 16, 3800 + i); /* Creation Date */
    put_long (unit->volume + 20, 0);
    put_long (unit->volume + 24, 0);
    put_long (unit->volume + 28, 0); /* lock list */
    put_long (unit->volume + 40, (unit->volume + 44) >> 2); /* Name */

	put_byte (unit->volume + 44, 0);
	if (!uinfo->wasisempty) {
		set_volume_name (unit);
		fsdb_clean_dir (&unit->rootnode);
	}

    put_long (unit->volume + 8, unit->port);
    put_long (unit->volume + 32, DISK_TYPE);

    put_long (pkt + dp_Res1, DOS_TRUE);

	return 1 | (late ? 2 : 0);
}

static void
	do_info (Unit *unit, dpacket packet, uaecptr info)
{
    struct fs_usage fsu;
	int ret;

/*	if (unit->volflags & MYVOLUMEINFO_ARCHIVE)
		ret = 0; //zfile_fs_usage_archive (unit->ui.rootdir, 0, &fsu);
	else*/
		ret = get_fs_usage (unit->ui.rootdir, 0, &fsu);
	if (ret != 0) {
		PUT_PCK_RES1 (packet, DOS_FALSE);
		PUT_PCK_RES2 (packet, dos_errno ());
		return;
    }
    put_long (info, 0); /* errors */
    put_long (info + 4, unit->unit); /* unit number */
    put_long (info + 8, unit->ui.readonly ? 80 : 82); /* state  */
    put_long (info + 12, fsu.fsu_blocks ); /* numblocks */
    put_long (info + 16, fsu.fsu_blocks - fsu.fsu_bavail); /* inuse */
    put_long (info + 20, 1024); /* bytesperblock */
    put_long (info + 24, DISK_TYPE); /* disk type */
    put_long (info + 28, unit->volume >> 2); /* volume node */
    put_long (info + 32, 0); /* inuse */
    PUT_PCK_RES1 (packet, DOS_TRUE);
}

static void
action_disk_info (Unit *unit, dpacket packet)
{
	TRACE(("ACTION_DISK_INFO\n"));
    do_info(unit, packet, GET_PCK_ARG1 (packet) << 2);
}

static void
action_info (Unit *unit, dpacket packet)
{
	TRACE(("ACTION_INFO\n"));
    do_info(unit, packet, GET_PCK_ARG2 (packet) << 2);
}

static void free_key (Unit *unit, Key *k)
{
    Key *k1;
    Key *prev = 0;
    for (k1 = unit->keys; k1; k1 = k1->next) {
		if (k == k1) {
		    if (prev)
				prev->next = k->next;
		    else
				unit->keys = k->next;
		    break;
		}
		prev = k1;
    }

	if (k->fd != NULL)
		fs_close (k->fd);

    xfree(k);
}

static Key *lookup_key (Unit *unit, uae_u32 uniq)
{
    Key *k;
	unsigned int total = 0;
    /* It's hardly worthwhile to optimize this - most of the time there are
     * only one or zero keys. */
    for (k = unit->keys; k; k = k->next) {
		total++;
		if (uniq == k->uniq)
		    return k;
    }
	write_log ("Error: couldn't find key %u / %u!\n", uniq, total);

    /* There isn't much hope we will recover. Unix would kill the process,
     * AmigaOS gets killed by it. */
    write_log ("Better reset that Amiga - the system is messed up.\n");
    return 0;
}

static Key *new_key (Unit *unit)
{
    Key *k = xcalloc(Key, 1);
	k->uniq = ++key_uniq;
    k->fd = NULL;
    k->file_pos = 0;
    k->next = unit->keys;
    unit->keys = k;

    return k;
}

#if TRACING_ENABLED
static void
dumplock (Unit *unit, uaecptr lock)
{
    a_inode *a;
	TRACE(("LOCK: 0x%lx", lock));
    if (!lock) {
		TRACE(("\n"));
		return;
	}
	TRACE(("{ next=0x%lx, mode=%ld, handler=0x%lx, volume=0x%lx, aino %lx ",
	   get_long (lock) << 2, get_long (lock+8),
	   get_long (lock+12), get_long (lock+16),
	   get_long (lock + 4)));
    a = lookup_aino (unit, get_long (lock + 4));
    if (a == 0) {
		TRACE(("not found!"));
    } else {
		TRACE(("%s", a->nname));
	}
	TRACE((" }\n"));
}
#endif

static a_inode *find_aino (Unit *unit, uaecptr lock, const char *name, uae_u32 *err)
{
    a_inode *a;

    if (lock) {
		a_inode *olda = lookup_aino (unit, get_long (lock + 4));
		if (olda == 0) {
		    /* That's the best we can hope to do. */
		    a = get_aino (unit, &unit->rootnode, name, err);
		} else {
			TRACE(("aino: 0x%08lx", (unsigned long int)olda->uniq));
			TRACE((" \"%s\"\n", olda->nname));
		    a = get_aino (unit, olda, name, err);
		}
    } else {
		a = get_aino (unit, &unit->rootnode, name, err);
    }
    if (a) {
		TRACE(("aino=\"%s\"\n", a->nname));
    }
    aino_test (a);
    return a;
}

static uaecptr make_lock (Unit *unit, uae_u32 uniq, long mode)
{
    /* allocate lock from the list kept by the assembly code */
    uaecptr lock;

    lock = get_long (unit->locklist);
    put_long (unit->locklist, get_long (lock));
    lock += 4;

    put_long (lock + 4, uniq);
    put_long (lock + 8, mode);
    put_long (lock + 12, unit->port);
    put_long (lock + 16, unit->volume >> 2);

    /* prepend to lock chain */
    put_long (lock, get_long (unit->volume + 28));
    put_long (unit->volume + 28, lock >> 2);

    DUMPLOCK(unit, lock);
    return lock;
}

static void free_notify (Unit *unit, int hash, Notify *n)
{
    Notify *n1, *prev = 0;
    for (n1 = unit->notifyhash[hash]; n1; n1 = n1->next) {
	if (n == n1) {
	    if (prev)
		prev->next = n->next;
	    else
		unit->notifyhash[hash] = n->next;
	    break;
	}
	prev = n1;
    }
    xfree(n);
}

#define NOTIFY_CLASS	0x40000000
#define NOTIFY_CODE	0x1234

#ifndef TARGET_AMIGAOS
# define NRF_SEND_MESSAGE 1
# define NRF_SEND_SIGNAL 2
# define NRF_WAIT_REPLY 8
# define NRF_NOTIFY_INITIAL 16
# define NRF_MAGIC (1 << 31)
#endif

static void notify_send (Unit *unit, Notify *n)
{
    uaecptr nr = n->notifyrequest;
    int flags = get_long (nr + 12);

    if (flags & NRF_SEND_MESSAGE) {
		if (!(flags & NRF_WAIT_REPLY) || ((flags & NRF_WAIT_REPLY) && !(flags & NRF_MAGIC))) {
		    uae_NotificationHack (unit->port, nr);
		} else if (flags & NRF_WAIT_REPLY) {
		    put_long (nr + 12, get_long (nr + 12) | NRF_MAGIC);
		}
    } else if (flags & NRF_SEND_SIGNAL) {
		uae_Signal (get_long (nr + 16), 1 << get_byte (nr + 20));
    }
}

static void notify_check (Unit *unit, a_inode *a)
{
    Notify *n;
    int hash = notifyhash (a->aname);
    for (n = unit->notifyhash[hash]; n; n = n->next) {
		uaecptr nr = n->notifyrequest;
		if (same_aname(n->partname, a->aname)) {
			uae_u32 err;
	    a_inode *a2 = find_aino (unit, 0, n->fullname, &err);
	    if (err == 0 && a == a2)
			notify_send (unit, n);
	}
    }
    if (a->parent) {
		hash = notifyhash (a->parent->aname);
		for (n = unit->notifyhash[hash]; n; n = n->next) {
			uaecptr nr = n->notifyrequest;
		    if (same_aname(n->partname, a->parent->aname)) {
				uae_u32 err;
				a_inode *a2 = find_aino (unit, 0, n->fullname, &err);
				if (err == 0 && a->parent == a2)
				    notify_send (unit, n);
		    }
		}
    }
}

static void
action_add_notify (Unit *unit, dpacket packet)
{
    uaecptr nr = GET_PCK_ARG1 (packet);
    int flags;
    Notify *n;
	char *name, *p, *partname;

	TRACE(("ACTION_ADD_NOTIFY\n"));

    name = my_strdup (char1 (get_long (nr + 4)));
    flags = get_long (nr + 12);

    if (!(flags & (NRF_SEND_MESSAGE | NRF_SEND_SIGNAL))) {
		PUT_PCK_RES1 (packet, DOS_FALSE);
		PUT_PCK_RES2 (packet, ERROR_BAD_NUMBER);
		return;
    }

#if 0
    write_log ("Notify:\n");
    write_log ("nr_Name '%s'\n", char1 (get_long (nr + 0)));
    write_log ("nr_FullName '%s'\n", name);
    write_log ("nr_UserData %08.8X\n", get_long (nr + 8));
    write_log ("nr_Flags %08.8X\n", flags);
    if (flags & NRF_SEND_MESSAGE) {
		write_log ("Message NotifyRequest, port = %08.8X\n", get_long (nr + 16));
    } else if (flags & NRF_SEND_SIGNAL) {
		write_log ("Signal NotifyRequest, Task = %08.8X signal = %d\n", get_long (nr + 16), get_long (nr + 20));
    } else {
		write_log ("corrupt NotifyRequest\n");
    }
#endif

	p = name + _tcslen (name) - 1;
	if (p[0] == ':')
		p--;
	while (p > name && p[0] != ':' && p[0] != '/')
		p--;
	if (p[0] == ':' || p[0] == '/')
		p++;
    partname = my_strdup (p);
    n = new_notify (unit, partname);
    n->notifyrequest = nr;
    n->fullname = name;
    if (flags & NRF_NOTIFY_INITIAL) {
		uae_u32 err;
		a_inode *a = find_aino (unit, 0, n->fullname, &err);
		if (err == 0)
		    notify_send (unit, n);
    }
    PUT_PCK_RES1 (packet, DOS_TRUE);
}
static void
action_remove_notify (Unit *unit, dpacket packet)
{
    uaecptr nr = GET_PCK_ARG1 (packet);
    Notify *n;
    int hash;

	TRACE(("ACTION_REMOVE_NOTIFY\n"));
    for (hash = 0; hash < NOTIFY_HASH_SIZE; hash++) {
		for (n = unit->notifyhash[hash]; n; n = n->next) {
		    if (n->notifyrequest == nr) {
				//write_log ("NotifyRequest %08.8X freed\n", n->notifyrequest);
				xfree (n->fullname);
				xfree (n->partname);
				free_notify (unit, hash, n);
				PUT_PCK_RES1 (packet, DOS_TRUE);
				return;
		    }
		}
	}
	write_log (_T("Tried to free non-existing NotifyRequest %08X\n"), nr);
	PUT_PCK_RES1 (packet, DOS_TRUE);
}

static void free_lock (Unit *unit, uaecptr lock)
{
    if (! lock)
	return;

    if (lock == get_long (unit->volume + 28) << 2) {
		put_long (unit->volume + 28, get_long (lock));
    } else {
		uaecptr current = get_long (unit->volume + 28);
		uaecptr next = 0;
		while (current) {
		    next = get_long (current << 2);
		    if (lock == next << 2)
				break;
		    current = next;
		}
		if (!current) {
			write_log ("tried to unlock non-existing lock %x\n", lock);
			return;
		}
		put_long (current << 2, get_long (lock));
    }
    lock -= 4;
    put_long (lock, get_long (unit->locklist));
    put_long (unit->locklist, lock);
}

static void
action_lock (Unit *unit, dpacket packet)
{
    uaecptr lock = GET_PCK_ARG1 (packet) << 2;
    uaecptr name = GET_PCK_ARG2 (packet) << 2;
    long mode = GET_PCK_ARG3 (packet);
    a_inode *a;
    uae_u32 err;

    if (mode != SHARED_LOCK && mode != EXCLUSIVE_LOCK) {
		TRACE(("Bad mode %d (should be %d or %d).\n", mode, SHARED_LOCK, EXCLUSIVE_LOCK));
		mode = SHARED_LOCK;
    }

	TRACE(("ACTION_LOCK(0x%lx, \"%s\", %d)\n", lock, bstr (unit, name), mode));
    DUMPLOCK(unit, lock);

	a = find_aino (unit, lock, bstr (unit, name), &err);
	if (err == 0 && a->softlink) {
		err = test_softlink (a);
	}
	if (err == 0 && (a->elock || (mode != SHARED_LOCK && a->shlock > 0))) {
		err = ERROR_OBJECT_IN_USE;
    }
    /* Lock() doesn't do access checks. */
    if (err != 0) {
		PUT_PCK_RES1 (packet, DOS_FALSE);
		PUT_PCK_RES2 (packet, err);
		return;
    }
    if (mode == SHARED_LOCK)
		a->shlock++;
    else
		a->elock = 1;
    de_recycle_aino (unit, a);
    PUT_PCK_RES1 (packet, make_lock (unit, a->uniq, mode) >> 2);
}

static void action_free_lock (Unit *unit, dpacket packet)
{
    uaecptr lock = GET_PCK_ARG1 (packet) << 2;
    a_inode *a;
	TRACE(("ACTION_FREE_LOCK(0x%lx)\n", lock));
    DUMPLOCK(unit, lock);

    a = lookup_aino (unit, get_long (lock + 4));
    if (a == 0) {
		PUT_PCK_RES1 (packet, DOS_FALSE);
		PUT_PCK_RES2 (packet, ERROR_OBJECT_NOT_AROUND);
		return;
    }
    if (a->elock)
		a->elock = 0;
    else
		a->shlock--;
    recycle_aino (unit, a);
    free_lock(unit, lock);

    PUT_PCK_RES1 (packet, DOS_TRUE);
}

static uaecptr
	action_dup_lock_2 (Unit *unit, dpacket packet, uae_u32 uniq)
{
    uaecptr out;
    a_inode *a;

	a = lookup_aino (unit, uniq);
    if (a == 0) {
		PUT_PCK_RES1 (packet, DOS_FALSE);
		PUT_PCK_RES2 (packet, ERROR_OBJECT_NOT_AROUND);
		return 0;
    }
    /* DupLock()ing exclusive locks isn't possible, says the Autodoc, but
     * at least the RAM-Handler seems to allow it. Let's see what happens
     * if we don't. */
    if (a->elock) {
		PUT_PCK_RES1 (packet, DOS_FALSE);
		PUT_PCK_RES2 (packet, ERROR_OBJECT_IN_USE);
		return 0;
    }
    a->shlock++;
    de_recycle_aino (unit, a);
    out = make_lock (unit, a->uniq, -2) >> 2;
    PUT_PCK_RES1 (packet, out);
    return out;
}

static void
action_dup_lock (Unit *unit, dpacket packet)
{
    uaecptr lock = GET_PCK_ARG1 (packet) << 2;
	TRACE(("ACTION_DUP_LOCK(0x%lx)\n", lock));
	if (!lock) {
		PUT_PCK_RES1 (packet, 0);
		return;
	}
	action_dup_lock_2 (unit, packet, get_long (lock + 4));
}


static void
	action_lock_from_fh (Unit *unit, dpacket packet)
{
	Key *k = lookup_key (unit, GET_PCK_ARG1 (packet));
	TRACE(("ACTION_COPY_DIR_FH(0x%lx)\n", GET_PCK_ARG1 (packet)));
	if (k == 0) {
		PUT_PCK_RES1 (packet, DOS_FALSE);
		return;
	}
	action_dup_lock_2 (unit, packet, k->aino->uniq);
}

#if !defined TARGET_AMIGAOS || !defined WORDS_BIGENDIAN
/* convert time_t to/from AmigaDOS time */
const int secs_per_day = 24 * 60 * 60;
//const int diff = (8 * 365 + 2) * (24 * 60 * 60);

static void
get_time (time_t t, long* days, long* mins, long* ticks)
{
	/* time_t is secs since 1-1-1970 */
    /* days since 1-1-1978 */
    /* mins since midnight */
    /* ticks past minute @ 50Hz */

# if !defined _WIN32 && !defined BEOS && !defined TARGET_AMIGAOS
    /*
     * On Unix-like systems, t is in UTC. The Amiga
     * requires local time, so we have to take account of
     * this difference if we can. This ain't easy to do in
     * a portable, thread-safe way.
     */
#  if defined HAVE_LOCALTIME_R && defined HAVE_TIMEGM
    struct tm tm;

    /* Convert t to local time */
    localtime_r (&t, &tm);

    /* Calculate local time in seconds since the Unix Epoch */
    t = timegm (&tm);
#  endif
# endif

    /* Adjust for difference between Unix and Amiga epochs */
    t -= diff;
	if (t < 0)
		t = 0;
    /* Calculate Amiga date-stamp */
    *days = t / secs_per_day;
    t -= *days * secs_per_day;
    *mins = t / 60;
    t -= *mins * 60;
    *ticks = t * 50;
}

/*
 * Convert Amiga date-stamp to host time in seconds since the start
 * of the Unix epoch
 */
static time_t
put_time (long days, long mins, long ticks)
{
    time_t t;

	if (days < 0)
		days = 0;
	if (days > 9900 * 365)
		days = 9900 * 365; // in future far enough?
	if (mins < 0 || mins >= 24 * 60)
		mins = 0;
	if (ticks < 0 || ticks >= 60 * 50)
		ticks = 0;

    t  = ticks / 50;
    t += mins * 60;
	t += ((uae_u64)days) * secs_per_day;
    t += diff;

# if !defined _WIN32 && !defined BEOS
    /*
     * t is still in local time zone. For Unix-like systems
     * we need a time in UTC, so we have to take account of
     * the difference if we can. This ain't easy to do in
     * a portable, thread-safe way.
     */
#  if defined HAVE_GMTIME_R && defined HAVE_LOCALTIME_R
    {
	struct tm tm;
	struct tm now_tm;
	time_t now_t;

	gmtime_r (&t, &tm);

	/*
	 * tm now contains the desired time in local time zone, not taking account
	 * of DST. To fix this, we determine if DST is in effect now and stuff that
	 * into tm.
	 */
	now_t = time (0);
	localtime_r (&now_t, &now_tm);
	tm.tm_isdst = now_tm.tm_isdst;

	/* Convert time to UTC in seconds since the Unix epoch */
	t = mktime (&tm);
    }
#  endif
# endif
    return t;
}
#endif

static void free_exkey (Unit *unit, ExamineKey *ek)
{
    if (--ek->aino->exnext_count == 0) {
		TRACE (("Freeing ExKey and reducing total_locked from %d by %d\n",
			unit->total_locked_ainos, ek->aino->locked_children));
		unit->total_locked_ainos -= ek->aino->locked_children;
		ek->aino->locked_children = 0;
    }
    ek->aino = 0;
    ek->uniq = 0;
}

static ExamineKey *lookup_exkey (Unit *unit, uae_u32 uniq)
{
    ExamineKey *ek;
    int i;

    ek = unit->examine_keys;
    for (i = 0; i < EXKEYS; i++, ek++) {
		/* Did we find a free one? */
		if (ek->uniq == uniq)
		    return ek;
    }
	write_log ("Houston, we have a BIG problem.\n");
    return 0;
}

/* This is so sick... who invented ACTION_EXAMINE_NEXT? What did he THINK??? */
static ExamineKey *new_exkey (Unit *unit, a_inode *aino)
{
    uae_u32 uniq;
    uae_u32 oldest = 0xFFFFFFFE;
    ExamineKey *ek, *oldest_ek = 0;
    int i;

    ek = unit->examine_keys;
    for (i = 0; i < EXKEYS; i++, ek++) {
		/* Did we find a free one? */
		if (ek->aino == 0)
		    continue;
		if (ek->uniq < oldest)
		    oldest = (oldest_ek = ek)->uniq;
    }
    ek = unit->examine_keys;
    for (i = 0; i < EXKEYS; i++, ek++) {
		/* Did we find a free one? */
		if (ek->aino == 0)
		    goto found;
    }
    /* This message should usually be harmless. */
	write_log ("Houston, we have a problem (%s).\n", aino->nname);
    free_exkey (unit, oldest_ek);
    ek = oldest_ek;
found:

    uniq = unit->next_exkey;
    if (uniq >= 0xFFFFFFFE) {
		/* Things will probably go wrong, but most likely the Amiga will crash
		 * before this happens because of something else. */
		uniq = 1;
	}
	unit->next_exkey = uniq + 1;
	ek->aino = aino;
	ek->curr_file = 0;
	ek->uniq = uniq;
	return ek;
}

static void move_exkeys (Unit *unit, a_inode *from, a_inode *to)
{
    int i;
    unsigned long tmp = 0;
    for (i = 0; i < EXKEYS; i++) {
		ExamineKey *k = unit->examine_keys + i;
		if (k->uniq == 0)
		    continue;
		if (k->aino == from) {
		    k->aino = to;
		    tmp++;
		}
    }
    if (tmp != from->exnext_count)
		write_log ("filesys.c: Bug in ExNext bookkeeping.  BAD.\n");
    to->exnext_count = from->exnext_count;
    to->locked_children = from->locked_children;
    from->exnext_count = 0;
    from->locked_children = 0;
}

static void
get_fileinfo (Unit *unit, dpacket packet, uaecptr info, a_inode *aino)
{
	struct _stat64 statbuf;
	long days, mins, ticks;
    int i, n, entrytype;
	int fsdb_can = fsdb_cando (unit);
	char *xs;
	char *x, *x2;

	memset (&statbuf, 0, sizeof statbuf);
	/* No error checks - this had better work. */
/*	if (unit->volflags & MYVOLUMEINFO_ARCHIVE)
		zfile_stat_archive (aino->nname, &statbuf);
	else*/
		stat (aino->nname, &statbuf);

    if (aino->parent == 0) {
		/* Guru book says ST_ROOT = 1 (root directory, not currently used)
		* but some programs really expect 2 from root dir..
		 */
		entrytype = 2;
		xs = unit->ui.volname;
    } else {
		entrytype = aino->dir ? 2 : -3;
		xs = aino->aname;
    }
    put_long (info + 4, entrytype);
    /* AmigaOS docs say these have to contain the same value. */
    put_long (info + 120, entrytype);

	TRACE(("name=\"%s\"\n", xs));
	x2 = x = ua_fs (xs, -1);
    n = strlen (x);
    if (n > 106)
	n = 106;
    i = 8;
    put_byte (info + i, n); i++;
    while (n--)
		put_byte (info + i, *x), i++, x++;
    while (i < 108)
		put_byte (info + i, 0), i++;
	xfree (x2);

#if defined TARGET_AMIGAOS && defined WORDS_BIGENDIAN
	 BPTR lock;
	struct FileInfoBlock fib __attribute__((aligned(4)));

	 if ((lock = Lock (aino->nname, SHARED_LOCK))) {
	     Examine (lock, &fib);
	     UnLock (lock);
	 }
	 put_long (info + 124, fib.fib_Size);
	 put_long (info + 128, fib.fib_NumBlocks);
	 put_long (info + 132, fib.fib_Date.ds_Days);
	 put_long (info + 136, fib.fib_Date.ds_Minute);
	 put_long (info + 140, fib.fib_Date.ds_Tick);
#else
	put_long (info + 116, fsdb_can ? aino->amigaos_mode : fsdb_mode_supported (aino));
	put_long (info + 124, statbuf.st_size > MAXFILESIZE32 ? MAXFILESIZE32 : statbuf.st_size);
# ifdef HAVE_ST_BLOCKS
	put_long (info + 128, statbuf.st_blocks);
# else
	put_long (info + 128, (statbuf.st_size + 511) / 512);
# endif
	get_time (statbuf.st_mtime, &days, &mins, &ticks);
	put_long (info + 132, days);
	put_long (info + 136, mins);
	put_long (info + 140, ticks);
#endif
	if (aino->comment == 0 || !fsdb_can)
		put_long (info + 144, 0);
    else {
		TRACE(("comment=\"%s\"\n", aino->comment));
		i = 144;
		xs = aino->comment;
		if (!xs)
			xs= "";
		x2 = x = ua_fs (xs, -1);
	n = strlen (x);
	if (n > 78)
	    n = 78;
	put_byte (info + i, n); i++;
	while (n--)
	    put_byte (info + i, *x), i++, x++;
	while (i < 224)
	    put_byte (info + i, 0), i++;
		xfree (x2);
    }
    PUT_PCK_RES1 (packet, DOS_TRUE);
}

int get_native_path (struct uaedev_mount_info *mountinfo, uae_u32 lock, char *out)
{
	int i = 0;
	for (i = 0; i < MAX_FILESYSTEM_UNITS; i++) {
		if (mountinfo->ui[i].self) {
			a_inode *a = lookup_aino (mountinfo->ui[i].self, get_long ((lock << 2) + 4));
			if (a) {
				_tcscpy (out, a->nname);
				return 0;
			}
		}
	}
	return -1;
}

#define EXALL_DEBUG 0
#define EXALL_END 0xde1111ad

static ExAllKey *getexall (Unit *unit, uaecptr control, int id)
{
	int i;
	if (id < 0) {
		for (i = 0; i < EXALLKEYS; i++) {
			if (unit->exalls[i].id == 0) {
				unit->exallid++;
				if (unit->exallid == EXALL_END)
					unit->exallid++;
				unit->exalls[i].id = unit->exallid;
				unit->exalls[i].control = control;
				return &unit->exalls[i];
			}
		}
	} else if (id > 0) {
		for (i = 0; i < EXALLKEYS; i++) {
			if (unit->exalls[i].id == id)
				return &unit->exalls[i];
		}
	}
	return NULL;
}

static int exalldo (uaecptr exalldata, uae_u32 exalldatasize, uae_u32 type, uaecptr control, Unit *unit, a_inode *aino)
{
	uaecptr exp = exalldata;
	int i;
	uae_u32 size, size2;
	int entrytype;
	TCHAR *xs = NULL, *commentx = NULL;
	uae_u32 flags = 15;
	long days = 0, mins = 0, ticks = 0;
	struct _stat64 statbuf;
	int fsdb_can = fsdb_cando (unit);
	uae_u16 uid = 0, gid = 0;
	char *x = NULL, *comment = NULL;
	int ret = 0;

	memset (&statbuf, 0, sizeof statbuf);
/*	if (unit->volflags & MYVOLUMEINFO_ARCHIVE)
		zfile_stat_archive (aino->nname, &statbuf);
	else*/
		stat (aino->nname, &statbuf);

	if (aino->parent == 0) {
		entrytype = 2;
		xs = unit->ui.volname;
	} else {
		entrytype = aino->dir ? 2 : -3;
		xs = aino->aname;
	}
	x = ua_fs (xs, -1);

	size = 0;
	size2 = 4;
	if (type >= 1) {
		size2 += 4;
		size += strlen (x) + 1;
		size = (size + 3) & ~3;
	}
	if (type >= 2)
		size2 += 4;
	if (type >= 3)
		size2 += 4;
	if (type >= 4) {
		flags = fsdb_can ? aino->amigaos_mode : fsdb_mode_supported (aino);
		size2 += 4;
	}
	if (type >= 5) {
		get_time (statbuf.st_mtime, &days, &mins, &ticks);
		size2 += 12;
	}
	if (type >= 6) {
		size2 += 4;
		if (aino->comment == 0 || !fsdb_can)
			commentx = _T("");
		else
			commentx = aino->comment;
		comment = ua_fs (commentx, -1);
		size += strlen (comment) + 1;
		size = (size + 3) & ~3;
	}
	if (type >= 7) {
		size2 += 4;
		uid = 0;
		gid = 0;
	}

	i = get_long (control + 0);
	while (i > 0) {
		exp = get_long (exp); /* ed_Next */
		i--;
	}

	if (exalldata + exalldatasize - exp < size + size2)
		goto end; /* not enough space */

#if EXALL_DEBUG > 0
	write_log ("ID=%d, %d, %08x: '%s'%s\n",
		get_long (control + 4), get_long (control + 0), exp, xs, aino->dir ? " [DIR]" : "");
#endif

	put_long (exp, exp + size + size2); /* ed_Next */
	if (type >= 1) {
		put_long (exp + 4, exp + size2);
		for (i = 0; i <= (int)strlen (x); i++) {
			put_byte (exp + size2, x[i]);
			size2++;
		}
	}
	if (type >= 2)
		put_long (exp + 8, entrytype);
	if (type >= 3)
		put_long (exp + 12, statbuf.st_size > MAXFILESIZE32 ? MAXFILESIZE32 : statbuf.st_size);
	if (type >= 4)
		put_long (exp + 16, flags);
	if (type >= 5) {
		put_long (exp + 20, days);
		put_long (exp + 24, mins);
		put_long (exp + 28, ticks);
	}
	if (type >= 6) {
		put_long (exp + 32, exp + size2);
		put_byte (exp + size2, strlen (comment));
		for (i = 0; i <= (int)strlen (comment); i++) {
			put_byte (exp + size2, comment[i]);
			size2++;
		}
	}
	if (type >= 7) {
		put_word (exp + 36, uid);
		put_word (exp + 38, gid);
	}
	put_long (control + 0, get_long (control + 0) + 1);
	ret = 1;
end:
	xfree (x);
	xfree (comment);
	return ret;
}

static int action_examine_all_do (Unit *unit, uaecptr lock, ExAllKey *eak, uaecptr exalldata, uae_u32 exalldatasize, uae_u32 type, uaecptr control)
{
	a_inode *aino, *base = NULL;
    struct dirent *ok = NULL;
	uae_u32 err;
	struct fs_dirhandle *d;
	char fn[MAX_DPATH];

	if (lock != 0)
		base = lookup_aino (unit, get_long (lock + 4));
	if (base == 0)
		base = &unit->rootnode;
	for (;;) {
		d = eak->dirhandle;
		if (!eak->fn) {
			do {
/*				if (d->isarch)
					ok = zfile_readdir_archive (d->zd, fn);
				else*/
					ok = readdir (d->od);
			} while (ok && !d->isarch && fsdb_name_invalid (ok->d_name));
			if (!ok)
				return 0;
		} else {
			_tcscpy (ok->d_name, eak->fn);
			xfree (eak->fn);
			eak->fn = NULL;
		}
		aino = lookup_child_aino_for_exnext (unit, base, ok->d_name, &err);
		if (!aino)
			return 0;
		eak->id = unit->exallid++;
		put_long (control + 4, eak->id);
		if (!exalldo (exalldata, exalldatasize, type, control, unit, aino)) {
			eak->fn = my_strdup (ok->d_name); /* no space in exallstruct, save current entry */
			break;
		}
	}
	return 1;
}

static int action_examine_all_end (Unit *unit, dpacket packet)
{
	uae_u32 id;
	uae_u32 doserr = 0;
	ExAllKey *eak;
	uaecptr control = GET_PCK_ARG5 (packet);

	if (kickstart_version < 36)
		return 0;
	id = get_long (control + 4);
	eak = getexall (unit, control, id);
#if EXALL_DEBUG > 0
	write_log ("EXALL_END ID=%d %x\n", id, eak);
#endif
	if (!eak) {
		write_log ("FILESYS: EXALL_END non-existing ID %d\n", id);
		doserr = ERROR_OBJECT_WRONG_TYPE;
	} else {
		eak->id = 0;
		fs_closedir (eak->dirhandle);
		xfree (eak->fn);
		eak->fn = NULL;
		eak->dirhandle = NULL;
	}
	if (doserr) {
		PUT_PCK_RES1 (packet, DOS_FALSE);
		PUT_PCK_RES2 (packet, doserr);
	} else {
		PUT_PCK_RES1 (packet, DOS_TRUE);
	}
	return 1;
}

static int action_examine_all (Unit *unit, dpacket packet)
{
	uaecptr lock = GET_PCK_ARG1 (packet) << 2;
	uaecptr exalldata = GET_PCK_ARG2 (packet);
	uae_u32 exalldatasize = GET_PCK_ARG3 (packet);
	uae_u32 type = GET_PCK_ARG4 (packet);
	uaecptr control = GET_PCK_ARG5 (packet);

	ExAllKey *eak = NULL;
	a_inode *base;
	struct fs_dirhandle *d;
	int ok, i;
	uaecptr exp;
	uae_u32 id, doserr = ERROR_NO_MORE_ENTRIES;

	ok = 0;

#if EXALL_DEBUG > 0
	write_log ("exall: %08x %08x-%08x %d %d %08x\n",
		lock, exalldata, exalldata + exalldatasize, exalldatasize, type, control);
	write_log ("exall: MatchString %08x, MatchFunc %08x\n",
		get_long (control + 8), get_long (control + 12));
#endif

	put_long (control + 0, 0); /* eac_Entries */

	/* EXAMINE ALL might use dos.library MatchPatternNoCase() which is >=36 */
	if (kickstart_version < 36)
		return 0;

	if (type == 0 || type > 7) {
		doserr = ERROR_BAD_NUMBER;
		goto fail;
	}

	PUT_PCK_RES1 (packet, DOS_TRUE);
	id = get_long (control + 4);
	if (id == EXALL_END) {
		write_log ("FILESYS: EXALL called twice with ERROR_NO_MORE_ENTRIES\n");
		goto fail; /* already ended exall() */
	}
	if (id) {
		eak = getexall (unit, control, id);
		if (!eak) {
			write_log ("FILESYS: EXALL non-existing ID %d\n", id);
			doserr = ERROR_OBJECT_WRONG_TYPE;
			goto fail;
		}
		if (!action_examine_all_do (unit, lock, eak, exalldata, exalldatasize, type, control))
			goto fail;
		if (get_long (control + 0) == 0) {
			/* uh, no space for first entry.. */
			doserr = ERROR_NO_FREE_STORE;
			goto fail;
		}

	} else {

		eak = getexall (unit, control, -1);
		if (!eak)
			goto fail;
		if (lock != 0)
			base = lookup_aino (unit, get_long (lock + 4));
		if (base == 0)
			base = &unit->rootnode;
#if EXALL_DEBUG > 0
		write_log("exall: ID=%d '%s'\n", eak->id, base->nname);
#endif
		d = fs_opendir (unit, base->nname);
		if (!d)
			goto fail;
		eak->dirhandle = d;
		put_long (control + 4, eak->id);
		if (!action_examine_all_do (unit, lock, eak, exalldata, exalldatasize, type, control))
			goto fail;
		if (get_long (control + 0) == 0) {
			/* uh, no space for first entry.. */
			doserr = ERROR_NO_FREE_STORE;
			goto fail;
		}

	}
	ok = 1;

fail:
	/* Clear last ed_Next. This "list" is quite non-Amiga like.. */
	exp = exalldata;
	i = get_long (control + 0);
	for (;;) {
		if (i <= 1) {
			if (exp)
				put_long (exp, 0);
			break;
		}
		exp = get_long (exp); /* ed_Next */
		i--;
	}
#if EXALL_DEBUG > 0
	write_log("ok=%d, err=%d, eac_Entries = %d\n", ok, ok ? -1 : doserr, get_long (control + 0));
#endif

	if (!ok) {
		PUT_PCK_RES1 (packet, DOS_FALSE);
		PUT_PCK_RES2 (packet, doserr);
		if (eak) {
			eak->id = 0;
			fs_closedir (eak->dirhandle);
			eak->dirhandle = NULL;
			xfree (eak->fn);
			eak->fn = NULL;
		}
		if (doserr == ERROR_NO_MORE_ENTRIES)
			put_long (control + 4, EXALL_END);
	}
	return 1;
}

static uae_u32 REGPARAM2 exall_helper (TrapContext *context)
{
	int i;
	Unit *u;
	uaecptr packet = m68k_areg (&context->regs, 4);
	uaecptr control = get_long (packet + dp_Arg5);
	uae_u32 id = get_long (control + 4);

#if EXALL_DEBUG > 0
	write_log ("FILESYS: EXALL extra round ID=%d\n", id);
#endif
	if (id == EXALL_END)
		return 1;
	for (u = units; u; u = u->next) {
		for (i = 0; i < EXALLKEYS; i++) {
			if (u->exalls[i].id == id && u->exalls[i].control == control) {
				action_examine_all (u, get_real_address (packet));
			}
		}
	}
	return 1;
}

static void action_examine_object (Unit *unit, dpacket packet)
{
    uaecptr lock = GET_PCK_ARG1 (packet) << 2;
    uaecptr info = GET_PCK_ARG2 (packet) << 2;
    a_inode *aino = 0;

    TRACE(("ACTION_EXAMINE_OBJECT(0x%lx,0x%lx)\n", lock, info));
    DUMPLOCK(unit, lock);

    if (lock != 0)
		aino = lookup_aino (unit, get_long (lock + 4));
    if (aino == 0)
		aino = &unit->rootnode;

    get_fileinfo (unit, packet, info, aino);
    if (aino->dir) {
		put_long (info, 0xFFFFFFFF);
    } else
		put_long (info, 0);
}

/* Read a directory's contents, create a_inodes for each file, and
   mark them as locked in memory so that recycle_aino will not reap
   them.
   We do this to avoid problems with the host OS: we don't want to
   leave the directory open on the host side until all ExNext()s have
   finished - they may never finish!  */

static void populate_directory (Unit *unit, a_inode *base)
{
	struct fs_dirhandle *d;
    a_inode *aino;

	d = fs_opendir (unit, base->nname);
    if (!d)
		return;
    for (aino = base->child; aino; aino = aino->sibling) {
	base->locked_children++;
	unit->total_locked_ainos++;
    }
    TRACE(("Populating directory, child %p, locked_children %d\n",
	   base->child, base->locked_children));
    for (;;) {
		char fn[MAX_DPATH];
	    struct dirent *ok;
		uae_u32 err;

		/* Find next file that belongs to the Amiga fs (skipping things
	   	like "..", "." etc.  */
		do {
/*			if (d->isarch)
				ok = zfile_readdir_archive(d->zd, fn);
			else*/
				ok = readdir (d->od);
		} while (ok && !d->isarch && fsdb_name_invalid (ok->d_name));
		if (!ok)
	    	break;
		/* This calls init_child_aino, which will notice that the parent is
	   	being ExNext()ed, and it will increment the locked counts.  */
		aino = lookup_child_aino_for_exnext (unit, base, ok->d_name, &err);
    }
	fs_closedir (d);
}

static void do_examine (Unit *unit, dpacket packet, ExamineKey *ek, uaecptr info)
{
	for (;;) {
		TCHAR *name;
    	if (ek->curr_file == 0)
			break;
		name = ek->curr_file->nname;
    	get_fileinfo (unit, packet, info, ek->curr_file);
    	ek->curr_file = ek->curr_file->sibling;
		if (!(unit->volflags & MYVOLUMEINFO_ARCHIVE) && !fsdb_exists(name)) {
			TRACE (("%s orphaned", name));
			continue;
		}
		TRACE (("curr_file set to %p %s\n", ek->curr_file,
			ek->curr_file ? ek->curr_file->aname : "NULL"));
		return;
	}
	TRACE(("no more entries\n"));
    free_exkey (unit, ek);
    PUT_PCK_RES1 (packet, DOS_FALSE);
    PUT_PCK_RES2 (packet, ERROR_NO_MORE_ENTRIES);
}

static void action_examine_next (Unit *unit, dpacket packet)
{
    uaecptr lock = GET_PCK_ARG1 (packet) << 2;
    uaecptr info = GET_PCK_ARG2 (packet) << 2;
    a_inode *aino = 0;
    ExamineKey *ek;
    uae_u32 uniq;

	TRACE(("ACTION_EXAMINE_NEXT(0x%lx,0x%lx)\n", lock, info));
	gui_flicker_led (LED_HD, unit->unit, 1);
    DUMPLOCK(unit, lock);

    if (lock != 0)
	aino = lookup_aino (unit, get_long (lock + 4));
    if (aino == 0)
	aino = &unit->rootnode;
	for(;;) {
    	uniq = get_long (info);
    	if (uniq == 0) {
			write_log ("ExNext called for a file! (Houston?)\n");
			goto no_more_entries;
    	} else if (uniq == 0xFFFFFFFE)
			goto no_more_entries;
    	else if (uniq == 0xFFFFFFFF) {
			TRACE(("Creating new ExKey\n"));
			ek = new_exkey (unit, aino);
			if (ek) {
			    if (aino->exnext_count++ == 0)
					populate_directory (unit, aino);
				ek->curr_file = aino->child;
				TRACE(("Initial curr_file: %p %s\n", ek->curr_file,
					ek->curr_file ? ek->curr_file->aname : "NULL"));
			}
	    } else {
			TRACE(("Looking up ExKey\n"));
			ek = lookup_exkey (unit, get_long (info));
	    }
	    if (ek == 0) {
			write_log ("Couldn't find a matching ExKey. Prepare for trouble.\n");
			goto no_more_entries;
	    }
	    put_long (info, ek->uniq);
		if (!ek->curr_file || ek->curr_file->mountcount == unit->mountcount)
			break;
		ek->curr_file = ek->curr_file->sibling;
		if (!ek->curr_file)
			goto no_more_entries;
	}
    do_examine (unit, packet, ek, info);
    return;

  no_more_entries:
    PUT_PCK_RES1 (packet, DOS_FALSE);
    PUT_PCK_RES2 (packet, ERROR_NO_MORE_ENTRIES);
}

static void do_find (Unit *unit, dpacket packet, int mode, int create, int fallback)
{
    uaecptr fh = GET_PCK_ARG1 (packet) << 2;
    uaecptr lock = GET_PCK_ARG2 (packet) << 2;
    uaecptr name = GET_PCK_ARG3 (packet) << 2;
    a_inode *aino;
    Key *k;
	struct fs_filehandle *fd;
    uae_u32 err;
    char openmode;
    int aino_created = 0;
	int isarch = unit->volflags & MYVOLUMEINFO_ARCHIVE;

    TRACE(("ACTION_FIND_*(0x%lx,0x%lx,\"%s\",%d,%d)\n", fh, lock, bstr (unit, name), mode, create));
    DUMPLOCK(unit, lock);

    aino = find_aino (unit, lock, bstr (unit, name), &err);

    if (aino == 0 || (err != 0 && err != ERROR_OBJECT_NOT_AROUND)) {
	/* Whatever it is, we can't handle it. */
	PUT_PCK_RES1 (packet, DOS_FALSE);
	PUT_PCK_RES2 (packet, err);
	return;
    }
    if (err == 0) {
	/* Object exists. */
	if (aino->dir) {
	    PUT_PCK_RES1 (packet, DOS_FALSE);
	    PUT_PCK_RES2 (packet, ERROR_OBJECT_WRONG_TYPE);
	    return;
	}
	if (aino->elock || (create == 2 && aino->shlock > 0)) {
	    PUT_PCK_RES1 (packet, DOS_FALSE);
	    PUT_PCK_RES2 (packet, ERROR_OBJECT_IN_USE);
	    return;
	}
	if (create == 2 && (aino->amigaos_mode & A_FIBF_DELETE) != 0) {
	    PUT_PCK_RES1 (packet, DOS_FALSE);
	    PUT_PCK_RES2 (packet, ERROR_DELETE_PROTECTED);
	    return;
	}
	if (create != 2) {
	    if ((((mode & aino->amigaos_mode) & A_FIBF_WRITE) != 0 || unit->ui.readonly)
		&& fallback)
	    {
		mode &= ~A_FIBF_WRITE;
	    }
	    /* Kick 1.3 doesn't check read and write access bits - maybe it would be
	     * simpler just not to do that either. */
	    if ((mode & A_FIBF_WRITE) != 0 && unit->ui.readonly) {
		PUT_PCK_RES1 (packet, DOS_FALSE);
		PUT_PCK_RES2 (packet, ERROR_DISK_WRITE_PROTECTED);
		return;
	    }
	    if (((mode & aino->amigaos_mode) & A_FIBF_WRITE) != 0
		|| mode == 0)
	    {
		PUT_PCK_RES1 (packet, DOS_FALSE);
		PUT_PCK_RES2 (packet, ERROR_WRITE_PROTECTED);
		return;
	    }
	    if (((mode & aino->amigaos_mode) & A_FIBF_READ) != 0) {
		PUT_PCK_RES1 (packet, DOS_FALSE);
		PUT_PCK_RES2 (packet, ERROR_READ_PROTECTED);
		return;
	    }
	}
    } else if (create == 0) {
	PUT_PCK_RES1 (packet, DOS_FALSE);
	PUT_PCK_RES2 (packet, err);
	return;
    } else {
	/* Object does not exist. aino points to containing directory. */
	aino = create_child_aino (unit, aino, my_strdup (bstr_cut (unit, name)), 0);
	if (aino == 0) {
	    PUT_PCK_RES1 (packet, DOS_FALSE);
	    PUT_PCK_RES2 (packet, ERROR_DISK_IS_FULL); /* best we can do */
	    return;
	}
	aino_created = 1;
    }

    prepare_for_open (aino->nname);

    openmode = ((mode & A_FIBF_READ) == 0 ? "wb" : (mode & A_FIBF_WRITE) == 0 ? "rb" : "r+b");
	if (create) openmode = "w+b";

	fd = fs_open (unit, aino->nname, openmode);
	if (fd == NULL) {
	if (aino_created)
	    delete_aino (unit, aino);
	PUT_PCK_RES1 (packet, DOS_FALSE);
		/* archive and fd == NULL = corrupt archive or out of memory */
		PUT_PCK_RES2 (packet, isarch ? ERROR_OBJECT_NOT_AROUND : dos_errno ());
	return;
    }

    k = new_key (unit);
    k->fd = fd;
    k->aino = aino;
    k->dosmode = mode;
    k->createmode = create;
    k->notifyactive = create ? 1 : 0;

	if (create && isarch)
	fsdb_set_file_attrs (aino);

    put_long (fh+36, k->uniq);
	if (create == 2) {
	aino->elock = 1;
		// clear comment if file already existed
		if (aino->comment) {
			xfree (aino->comment);
			aino->comment = 0;
		}
		fsdb_set_file_attrs (aino);
	} else {
	aino->shlock++;
	}
    de_recycle_aino (unit, aino);
    PUT_PCK_RES1 (packet, DOS_TRUE);
}

static void
action_fh_from_lock (Unit *unit, dpacket packet)
{
    uaecptr fh = GET_PCK_ARG1 (packet) << 2;
    uaecptr lock = GET_PCK_ARG2 (packet) << 2;
    a_inode *aino;
    Key *k;
	struct fs_filehandle *fd;
    char openmode;
    int mode;
	int isarch = unit->volflags & MYVOLUMEINFO_ARCHIVE;

	TRACE(("ACTION_FH_FROM_LOCK(0x%lx,0x%lx)\n",fh,lock));
    DUMPLOCK(unit,lock);

    if (!lock) {
	PUT_PCK_RES1 (packet, DOS_FALSE);
	PUT_PCK_RES2 (packet, 0);
	return;
    }

    aino = lookup_aino (unit, get_long (lock + 4));
    if (aino == 0)
	aino = &unit->rootnode;
    mode = aino->amigaos_mode; /* Use same mode for opened filehandle as existing Lock() */

    prepare_for_open (aino->nname);

    TRACE (("  mode is %d\n", mode));
/* note */
    openmode = (((mode & A_FIBF_READ) ? "wb"
		 : (mode & A_FIBF_WRITE) ? "rb"
		 : "r+b"));

   /* the files on CD really can have the write-bit set.  */
    if (unit->ui.readonly)
		openmode = "rb";

	fd = fs_open (unit, aino->nname, openmode);

	if (fd == NULL) {
		PUT_PCK_RES1 (packet, DOS_FALSE);
		PUT_PCK_RES2 (packet, dos_errno());
		return;
    }
    k = new_key (unit);
    k->fd = fd;
    k->aino = aino;

    put_long (fh + 36, k->uniq);
    /* I don't think I need to play with shlock count here, because I'm
       opening from an existing lock ??? */

    /* Is this right?  I don't completely understand how this works.  Do I
       also need to free_lock() my lock, since nobody else is going to? */
    de_recycle_aino (unit, aino);
	free_lock (unit, lock); /* lock must be unlocked */
    PUT_PCK_RES1 (packet, DOS_TRUE);
    /* PUT_PCK_RES2 (packet, k->uniq); - this shouldn't be necessary, try without it */
}

static void
action_find_input (Unit *unit, dpacket packet)
{
	do_find (unit, packet, A_FIBF_READ | A_FIBF_WRITE, 0, 1);
}

static void
action_find_output (Unit *unit, dpacket packet)
{
    if (unit->ui.readonly) {
	PUT_PCK_RES1 (packet, DOS_FALSE);
	PUT_PCK_RES2 (packet, ERROR_DISK_WRITE_PROTECTED);
	return;
    }
    do_find(unit, packet, A_FIBF_READ|A_FIBF_WRITE, 2, 0);
}

static void
action_find_write (Unit *unit, dpacket packet)
{
    if (unit->ui.readonly) {
	PUT_PCK_RES1 (packet, DOS_FALSE);
	PUT_PCK_RES2 (packet, ERROR_DISK_WRITE_PROTECTED);
	return;
    }
    do_find(unit, packet, A_FIBF_READ|A_FIBF_WRITE, 1, 0);
}

/* change file/dir's parent dir modification time */
static void updatedirtime (a_inode *a1, int now)
{
#if !defined TARGET_AMIGAOS || !defined WORDS_BIGENDIAN
    struct stat statbuf;
    struct utimbuf ut;
    long days, mins, ticks;

    if (!a1->parent)
	return;
    if (!now) {
	if (stat (a1->nname, &statbuf) == -1)
	    return;
	get_time (statbuf.st_mtime, &days, &mins, &ticks);
	ut.actime = ut.modtime = put_time(days, mins, ticks);
	utime (a1->parent->nname, &ut);
    } else {
	utime (a1->parent->nname, NULL);
    }
#endif
}

static void
action_end (Unit *unit, dpacket packet)
{
    Key *k;
    TRACE(("ACTION_END(0x%lx)\n", GET_PCK_ARG1 (packet)));

    k = lookup_key (unit, GET_PCK_ARG1 (packet));
    if (k != 0) {
	if (k->notifyactive) {
	    notify_check (unit, k->aino);
	    updatedirtime (k->aino, 1);
	}
	if (k->aino->elock)
	    k->aino->elock = 0;
	else
	    k->aino->shlock--;
	recycle_aino (unit, k->aino);
	free_key (unit, k);
    }
    PUT_PCK_RES1 (packet, DOS_TRUE);
    PUT_PCK_RES2 (packet, 0);
}

static void
	action_read (Unit *unit, dpacket packet)
{
    Key *k = lookup_key (unit, GET_PCK_ARG1 (packet));
    uaecptr addr = GET_PCK_ARG2 (packet);
    long size = (uae_s32)GET_PCK_ARG3 (packet);
    int actual;

    if (k == 0) {
	PUT_PCK_RES1 (packet, DOS_FALSE);
	/* PUT_PCK_RES2 (packet, EINVAL); */
	return;
    }
    TRACE(("ACTION_READ(%s,0x%lx,%ld)\n",k->aino->nname,addr,size));
	gui_flicker_led (LED_HD, unit->unit, 1);
#ifdef RELY_ON_LOADSEG_DETECTION
    /* HACK HACK HACK HACK
     * Try to detect a LoadSeg() */
    if (k->file_pos == 0 && size >= 4) {
		unsigned char buf[4];
		off_t currpos = fs_lseek (unit, k->fd, 0, SEEK_CUR);
		//my_read (k->fd, buf, 4);
		fread (buf, 1, 4, k->fd);
		fs_lseek (unit, k->fd, currpos, SEEK_SET);
		if (buf[0] == 0 && buf[1] == 0 && buf[2] == 3 && buf[3] == 0xF3)
	    	possible_loadseg();
    }
#endif
    if (valid_address (addr, size)) {
		uae_u8 *realpt;
		realpt = get_real_address (addr);
		actual = fs_read (k->fd, realpt, size);

	if (actual == 0) {
	    PUT_PCK_RES1 (packet, 0);
	    PUT_PCK_RES2 (packet, 0);
	} else if (actual < 0) {
	    PUT_PCK_RES1 (packet, 0);
	    PUT_PCK_RES2 (packet, dos_errno());
	} else {
	    PUT_PCK_RES1 (packet, actual);
	    k->file_pos += actual;
	}
    } else {
		uae_u8 *buf;
	off_t old, filesize;

	write_log ("unixfs warning: Bad pointer passed for read: %08x, size %d\n", addr, size);
	/* ugh this is inefficient but easy */

		old = fs_lseek (k->fd, 0, SEEK_CUR);
		filesize = fs_lseek (k->fd, 0, SEEK_END);
		fs_lseek (k->fd, old, SEEK_SET);
	if (size > filesize)
	    size = filesize;

		buf = xmalloc (uae_u8, size);
	if (!buf) {
	    PUT_PCK_RES1 (packet, -1);
	    PUT_PCK_RES2 (packet, ERROR_NO_FREE_STORE);
	    return;
	}
		actual = fs_read (k->fd, buf, size);

	if (actual < 0) {
	    PUT_PCK_RES1 (packet, 0);
	    PUT_PCK_RES2 (packet, dos_errno());
	} else {
	    int i;
	    PUT_PCK_RES1 (packet, actual);
	    for (i = 0; i < actual; i++)
		put_byte (addr + i, buf[i]);
	    k->file_pos += actual;
	}
	xfree (buf);
    }
    TRACE(("=%d\n", actual));
}

static void
action_write (Unit *unit, dpacket packet)
{
	Key *k = lookup_key (unit, GET_PCK_ARG1 (packet));
	uaecptr addr = GET_PCK_ARG2 (packet);
	uae_u32 size = GET_PCK_ARG3 (packet);
	uae_u32 actual;
	uae_u8 *buf;
	uae_u32 i;

	if (k == 0) {
		PUT_PCK_RES1 (packet, DOS_FALSE);
		/* PUT_PCK_RES2 (packet, EINVAL); */
		return;
	}

	gui_flicker_led (LED_HD, unit->unit, 2);
	TRACE((_T("ACTION_WRITE(%s,0x%x,%d)\n"), k->aino->nname, addr, size));
//	if (unit->ui.readonly || unit->ui.locked) {
	if (unit->ui.readonly) {
		PUT_PCK_RES1 (packet, DOS_FALSE);
		PUT_PCK_RES2 (packet, ERROR_DISK_WRITE_PROTECTED);
		return;
	}

	if (size == 0) {
		actual = 0;
		PUT_PCK_RES1 (packet, 0);
		PUT_PCK_RES2 (packet, 0);
	} else if (valid_address (addr, size)) {
		uae_u8 *realpt = get_real_address (addr);

		if (fs_lseek64 (k->fd, k->file_pos, SEEK_SET) < 0) {
			PUT_PCK_RES1 (packet, 0);
			PUT_PCK_RES2 (packet, dos_errno ());
			return;
		}

		actual = fs_write (k->fd, realpt, size);
	} else {
		write_log (_T("unixfs warning: Bad pointer passed for write: %08x, size %d\n"), addr, size);
		/* ugh this is inefficient but easy */

		if (fs_lseek64 (k->fd, k->file_pos, SEEK_SET) < 0) {
			PUT_PCK_RES1 (packet, 0);
			PUT_PCK_RES2 (packet, dos_errno ());
			return;
		}

		buf = xmalloc (uae_u8, size);
		if (!buf) {
			PUT_PCK_RES1 (packet, -1);
			PUT_PCK_RES2 (packet, ERROR_NO_FREE_STORE);
			return;
		}

		for (i = 0; i < size; i++)
			buf[i] = get_byte (addr + i);

		actual = fs_write (k->fd, buf, size);
		xfree (buf);
	}

	TRACE((_T("=%d\n"), actual));
	PUT_PCK_RES1 (packet, actual);
	if (actual != size)
		PUT_PCK_RES2 (packet, dos_errno ());
	if ((uae_s32)actual != -1)
		k->file_pos += actual;

    k->notifyactive = 1;
}

static void
action_seek (Unit *unit, dpacket packet)
{
	Key *k = lookup_key (unit, GET_PCK_ARG1 (packet));
	long pos = (uae_s32)GET_PCK_ARG2 (packet);
	long mode = (uae_s32)GET_PCK_ARG3 (packet);
	uae_s64 res;
	uae_s64 cur;
	int whence = SEEK_CUR;
	uae_s64 temppos = 0, filesize = 0;

	if (k == 0) {
		PUT_PCK_RES1 (packet, -1);
		PUT_PCK_RES2 (packet, ERROR_INVALID_LOCK);
		return;
	}

	if (mode > 0)
		whence = SEEK_END;
	if (mode < 0)
		whence = SEEK_SET;

	cur = k->file_pos;
	TRACE((_T("ACTION_SEEK(%s,%ld,%ld)=%lld\n"), k->aino->nname, pos, mode, cur));
	gui_flicker_led (LED_HD, unit->unit, 1);

	filesize = fs_fsize64 (k->fd);
	if (whence == SEEK_CUR)
		temppos = cur + pos;
	if (whence == SEEK_SET)
		temppos = pos;
	if (whence == SEEK_END)
		temppos = filesize + pos;
	if (filesize < temppos) {
		PUT_PCK_RES1 (packet, -1);
		PUT_PCK_RES2 (packet, ERROR_SEEK_ERROR);
		return;
	}

	res = fs_lseek64 (k->fd, pos, whence);
	if (-1 == res || cur > MAXFILESIZE32) {
		PUT_PCK_RES1 (packet, -1);
		PUT_PCK_RES2 (packet, ERROR_SEEK_ERROR);
		fs_lseek64 (k->fd, cur, SEEK_SET);
	} else {
		PUT_PCK_RES1 (packet, cur);
		k->file_pos = fs_lseek64 (k->fd, 0, SEEK_CUR);
	}
}

static void
action_set_protect (Unit *unit, dpacket packet)
{
    uaecptr lock = GET_PCK_ARG2 (packet) << 2;
    uaecptr name = GET_PCK_ARG3 (packet) << 2;
    uae_u32 mask = GET_PCK_ARG4 (packet);
    a_inode *a;
    uae_u32 err;

    TRACE(("ACTION_SET_PROTECT(0x%lx,\"%s\",0x%lx)\n", lock, bstr (unit, name), mask));

    if (unit->ui.readonly) {
	PUT_PCK_RES1 (packet, DOS_FALSE);
	PUT_PCK_RES2 (packet, ERROR_DISK_WRITE_PROTECTED);
	return;
    }

    a = find_aino (unit, lock, bstr (unit, name), &err);
    if (err != 0) {
	PUT_PCK_RES1 (packet, DOS_FALSE);
	PUT_PCK_RES2 (packet, err);
	return;
    }

    a->amigaos_mode = mask;
	if (!fsdb_cando (unit))
		a->amigaos_mode = fsdb_mode_supported (a);
    err = fsdb_set_file_attrs (a);
    if (err != 0) {
	PUT_PCK_RES1 (packet, DOS_FALSE);
	PUT_PCK_RES2 (packet, err);
    } else {
	PUT_PCK_RES1 (packet, DOS_TRUE);
    }
    notify_check (unit, a);
	gui_flicker_led (LED_HD, unit->unit, 2);
}

static void action_set_comment (Unit * unit, dpacket packet)
{
    uaecptr lock = GET_PCK_ARG2 (packet) << 2;
    uaecptr name = GET_PCK_ARG3 (packet) << 2;
    uaecptr comment = GET_PCK_ARG4 (packet) << 2;
    char *commented = NULL;
    a_inode *a;
    uae_u32 err;

    if (unit->ui.readonly) {
	PUT_PCK_RES1 (packet, DOS_FALSE);
	PUT_PCK_RES2 (packet, ERROR_DISK_WRITE_PROTECTED);
	return;
    }

	if (fsdb_cando (unit)) {
    commented = bstr (unit, comment);
		if (_tcslen (commented) > 80) {
			PUT_PCK_RES1 (packet, DOS_FALSE);
			PUT_PCK_RES2 (packet, ERROR_COMMENT_TOO_BIG);
			return;
		}
		if (_tcslen (commented) > 0) {
			TCHAR *p = commented;
			commented = xmalloc (char, 81);
			_tcsncpy (commented, p, 80);
			commented[80] = 0;
		} else {
			commented = NULL;
		}
	}
	TRACE (("ACTION_SET_COMMENT(0x%lx,\"%s\")\n", lock, commented));

    a = find_aino (unit, lock, bstr (unit, name), &err);
    if (err != 0) {
	PUT_PCK_RES1 (packet, DOS_FALSE);
	PUT_PCK_RES2 (packet, err);

	maybe_free_and_out:
	if (commented)
	    xfree (commented);
	return;
    }
    PUT_PCK_RES1 (packet, DOS_TRUE);
    PUT_PCK_RES2 (packet, 0);
    if (a->comment == 0 && commented == 0)
		goto maybe_free_and_out;
	if (a->comment != 0 && commented != 0 && _tcscmp (a->comment, commented) == 0)
		goto maybe_free_and_out;
    if (a->comment)
		xfree (a->comment);
    a->comment = commented;
    fsdb_set_file_attrs (a);
    notify_check (unit, a);
	gui_flicker_led (LED_HD, unit->unit, 2);
}

static void
action_same_lock (Unit *unit, dpacket packet)
{
    uaecptr lock1 = GET_PCK_ARG1 (packet) << 2;
    uaecptr lock2 = GET_PCK_ARG2 (packet) << 2;

    TRACE(("ACTION_SAME_LOCK(0x%lx,0x%lx)\n",lock1,lock2));
    DUMPLOCK(unit, lock1); DUMPLOCK(unit, lock2);

    if (!lock1 || !lock2) {
	PUT_PCK_RES1 (packet, lock1 == lock2 ? DOS_TRUE : DOS_FALSE);
    } else {
	PUT_PCK_RES1 (packet, get_long (lock1 + 4) == get_long (lock2 + 4) ? DOS_TRUE : DOS_FALSE);
    }
}

static void
action_change_mode (Unit *unit, dpacket packet)
{
#define CHANGE_LOCK 0
#define CHANGE_FH 1
    /* will be CHANGE_FH or CHANGE_LOCK value */
    long type = GET_PCK_ARG1 (packet);
    /* either a file-handle or lock */
    uaecptr object = GET_PCK_ARG2 (packet) << 2;
    /* will be EXCLUSIVE_LOCK/SHARED_LOCK if CHANGE_LOCK,
	* or MODE_OLDFILE/MODE_NEWFILE/MODE_READWRITE if CHANGE_FH *
	* Above is wrong, it is always *_LOCK. TW. */
    long mode = GET_PCK_ARG3 (packet);
    unsigned long uniq;
    a_inode *a = NULL, *olda = NULL;
    uae_u32 err = 0;
	TRACE(("ACTION_CHANGE_MODE(0x%lx,%d,%d)\n", object, type, mode));

	if (! object || (type != CHANGE_FH && type != CHANGE_LOCK)) {
		PUT_PCK_RES1 (packet, DOS_FALSE);
		PUT_PCK_RES2 (packet, ERROR_INVALID_LOCK);
		return;
    }
/* note */
    /* @@@ Brian: shouldn't this be good enough to support
       CHANGE_FH?  */
    if (type == CHANGE_FH)
	mode = (mode == 1006 ? -1 : -2);

	if (type == CHANGE_LOCK) {
		uniq = get_long (object + 4);
	} else {
		Key *k = lookup_key (unit, get_long (object + 36));
		if (!k) {
		    PUT_PCK_RES1 (packet, DOS_FALSE);
		    PUT_PCK_RES2 (packet, ERROR_OBJECT_NOT_AROUND);
		    return;
		}
		uniq = k->aino->uniq;
    }
    a = lookup_aino (unit, uniq);

	if (! a) {
		err = ERROR_INVALID_LOCK;
	} else {
		if (mode == -1) {
			if (a->shlock > 1) {
				err = ERROR_OBJECT_IN_USE;
			} else {
				a->shlock = 0;
				a->elock = 1;
		    }
		} else { /* Must be SHARED_LOCK == -2 */
		    a->elock = 0;
		    a->shlock++;
		}
    }

    if (err) {
		PUT_PCK_RES1 (packet, DOS_FALSE);
		PUT_PCK_RES2 (packet, err);
		return;
    } else {
		de_recycle_aino (unit, a);
		PUT_PCK_RES1 (packet, DOS_TRUE);
    }
}

static void
action_parent_common (Unit *unit, dpacket packet, unsigned long uniq)
{
    a_inode *olda = lookup_aino (unit, uniq);
    if (olda == 0) {
	PUT_PCK_RES1 (packet, DOS_FALSE);
	PUT_PCK_RES2 (packet, ERROR_INVALID_LOCK);
	return;
    }

    if (olda->parent == 0) {
	PUT_PCK_RES1 (packet, 0);
	PUT_PCK_RES2 (packet, 0);
	return;
    }
    if (olda->parent->elock) {
	PUT_PCK_RES1 (packet, DOS_FALSE);
	PUT_PCK_RES2 (packet, ERROR_OBJECT_IN_USE);
	return;
    }
    olda->parent->shlock++;
    de_recycle_aino (unit, olda->parent);
    PUT_PCK_RES1 (packet, make_lock (unit, olda->parent->uniq, -2) >> 2);
}

static void
action_parent_fh (Unit *unit, dpacket packet)
{
    Key *k = lookup_key (unit, GET_PCK_ARG1 (packet));
    if (!k) {
	PUT_PCK_RES1 (packet, DOS_FALSE);
	PUT_PCK_RES2 (packet, ERROR_OBJECT_NOT_AROUND);
	return;
    }
    action_parent_common (unit, packet, k->aino->uniq);
}

static void
action_parent (Unit *unit, dpacket packet)
{
    uaecptr lock = GET_PCK_ARG1 (packet) << 2;

    TRACE(("ACTION_PARENT(0x%lx)\n",lock));

    if (!lock) {
	PUT_PCK_RES1 (packet, 0);
	PUT_PCK_RES2 (packet, 0);
    } else {
	action_parent_common (unit, packet, get_long (lock + 4));
    }
    TRACE(("=%x %d\n", GET_PCK_RES1 (packet), GET_PCK_RES2 (packet)));
}

static void
action_create_dir (Unit *unit, dpacket packet)
{
    uaecptr lock = GET_PCK_ARG1 (packet) << 2;
    uaecptr name = GET_PCK_ARG2 (packet) << 2;
    a_inode *aino;
    uae_u32 err;

    TRACE(("ACTION_CREATE_DIR(0x%lx,\"%s\")\n", lock, bstr (unit, name)));

    if (unit->ui.readonly) {
	PUT_PCK_RES1 (packet, DOS_FALSE);
	PUT_PCK_RES2 (packet, ERROR_DISK_WRITE_PROTECTED);
	return;
    }

    aino = find_aino (unit, lock, bstr (unit, name), &err);
    if (aino == 0 || (err != 0 && err != ERROR_OBJECT_NOT_AROUND)) {
	PUT_PCK_RES1 (packet, DOS_FALSE);
	PUT_PCK_RES2 (packet, err);
	return;
    }
    if (err == 0) {
	/* Object exists. */
	PUT_PCK_RES1 (packet, DOS_FALSE);
	PUT_PCK_RES2 (packet, ERROR_OBJECT_EXISTS);
	return;
    }
    /* Object does not exist. aino points to containing directory. */
    aino = create_child_aino (unit, aino, my_strdup (bstr_cut (unit, name)), 1);
    if (aino == 0) {
	PUT_PCK_RES1 (packet, DOS_FALSE);
	PUT_PCK_RES2 (packet, ERROR_DISK_IS_FULL); /* best we can do */
	return;
    }

    if (mkdir (aino->nname, 0777) == -1) {
		PUT_PCK_RES1 (packet, DOS_FALSE);
		PUT_PCK_RES2 (packet, dos_errno());
		return;
    }
    aino->shlock = 1;
    fsdb_set_file_attrs (aino);
    de_recycle_aino (unit, aino);
    notify_check (unit, aino);
    updatedirtime (aino, 0);
    PUT_PCK_RES1 (packet, make_lock (unit, aino->uniq, -2) >> 2);
	gui_flicker_led (LED_HD, unit->unit, 2);
}

static void
action_examine_fh (Unit *unit, dpacket packet)
{
    Key *k;
    a_inode *aino = 0;
    uaecptr info = GET_PCK_ARG2 (packet) << 2;

	TRACE(("ACTION_EXAMINE_FH(0x%lx,0x%lx)\n",
	   GET_PCK_ARG1 (packet), GET_PCK_ARG2 (packet) ));

    k = lookup_key (unit, GET_PCK_ARG1 (packet));
    if (k != 0)
		aino = k->aino;
    if (aino == 0)
		aino = &unit->rootnode;

    get_fileinfo (unit, packet, info, aino);
    if (aino->dir)
		put_long (info, 0xFFFFFFFF);
    else
		put_long (info, 0);
}

/* For a nice example of just how contradictory documentation can be, see the
 * Autodoc for DOS:SetFileSize and the Packets.txt description of this packet...
 * This implementation tries to mimic the behaviour of the Kick 3.1 ramdisk
 * (which seems to match the Autodoc description). */
static void
action_set_file_size (Unit *unit, dpacket packet)
{
    Key *k, *k1;
    off_t offset = GET_PCK_ARG2 (packet);
    long mode = (uae_s32)GET_PCK_ARG3 (packet);
    int whence = SEEK_CUR;

	if (mode > 0)
		whence = SEEK_END;
	if (mode < 0)
		whence = SEEK_SET;

    TRACE(("ACTION_SET_FILE_SIZE(0x%lx, %d, 0x%x)\n", GET_PCK_ARG1 (packet), offset, mode));

    k = lookup_key (unit, GET_PCK_ARG1 (packet));
    if (k == 0) {
		PUT_PCK_RES1 (packet, DOS_TRUE);
		PUT_PCK_RES2 (packet, ERROR_OBJECT_NOT_AROUND);
		return;
    }

	gui_flicker_led (LED_HD, unit->unit, 1);
    k->notifyactive = 1;
    /* If any open files have file pointers beyond this size, truncate only
     * so far that these pointers do not become invalid.  */
    for (k1 = unit->keys; k1; k1 = k1->next) {
		if (k != k1 && k->aino == k1->aino) {
		    if (k1->file_pos > offset)
				offset = (off_t)k1->file_pos;
		}
    }

    /* Write one then truncate: that should give the right size in all cases.  */
	//offset = fs_lseek (k->fd, offset, whence);
	fs_lseek (k->fd, offset, whence);
	offset = fs_lseek (k->fd, 0, SEEK_CUR);
	fs_write (k->fd, /* whatever */(uae_u8*)&k1, 1);
    if (k->file_pos > offset)
		k->file_pos = offset;
	fs_lseek (k->fd, (off_t)k->file_pos, SEEK_SET);

    /* Brian: no bug here; the file _must_ be one byte too large after writing
       The write is supposed to guarantee that the file can't be smaller than
       the requested size, the truncate guarantees that it can't be larger.
       If we were to write one byte earlier we'd clobber file data.  */
	if (my_truncate (k->aino->nname, offset) == -1) {
		PUT_PCK_RES1 (packet, DOS_FALSE);
		PUT_PCK_RES2 (packet, dos_errno ());
		return;
    }

    PUT_PCK_RES1 (packet, offset);
    PUT_PCK_RES2 (packet, 0);
}

static int relock_do(Unit *unit, a_inode *a1)
{
    Key *k1, *knext;
    int wehavekeys = 0;

    for (k1 = unit->keys; k1; k1 = knext) {
		knext = k1->next;
		if (k1->aino == a1 && k1->fd) {
		    wehavekeys++;
			fs_close (k1->fd);
			write_log ("handle %p freed\n", k1->fd);
		}
    }
    return wehavekeys;
}

static void relock_re(Unit *unit, a_inode *a1, a_inode *a2, int failed)
{
    Key *k1, *knext;

    for (k1 = unit->keys; k1; k1 = knext) {
		knext = k1->next;
		if (k1->aino == a1 && k1->fd) {
		    char mode = (k1->dosmode & A_FIBF_READ) == 0 ? "wb" : (k1->dosmode & A_FIBF_WRITE) == 0 ? "rb" : "r+b";
		    //mode |= O_BINARY;
		    if (failed) {
				/* rename still failed, restore fd */
				k1->fd = fs_open (unit, a1->nname, mode);
				write_log ("restoring old handle '%s' %d\n", a1->nname, k1->dosmode);
		    } else {
				/* transfer fd to new name */
				if (a2) {
				    k1->aino = a2;
				k1->fd = fs_open (unit, a2->nname, mode);
					write_log ("restoring new handle '%s' %d\n", a2->nname, k1->dosmode);
				} else {
					write_log ("no new handle, deleting old lock(s).\n");
				}
		    }
			if (k1->fd == NULL) {
				write_log ("relocking failed '%s' -> '%s'\n", a1->nname, a2->nname);
				free_key (unit, k1);
		    } else {
				fs_lseek64 (k1->fd, k1->file_pos, SEEK_SET);
		    }
		}
    }
}

static void
action_delete_object (Unit *unit, dpacket packet)
{
    uaecptr lock = GET_PCK_ARG1 (packet) << 2;
    uaecptr name = GET_PCK_ARG2 (packet) << 2;
    a_inode *a;
    uae_u32 err;

	TRACE(("ACTION_DELETE_OBJECT(0x%lx,\"%s\")\n", lock, bstr (unit, name)));

    if (unit->ui.readonly) {
		PUT_PCK_RES1 (packet, DOS_FALSE);
		PUT_PCK_RES2 (packet, ERROR_DISK_WRITE_PROTECTED);
		return;
    }

    a = find_aino (unit, lock, bstr (unit, name), &err);

    if (err != 0) {
		PUT_PCK_RES1 (packet, DOS_FALSE);
		PUT_PCK_RES2 (packet, err);
		return;
    }
    if (a->amigaos_mode & A_FIBF_DELETE) {
		PUT_PCK_RES1 (packet, DOS_FALSE);
		PUT_PCK_RES2 (packet, ERROR_DELETE_PROTECTED);
		return;
    }
    if (a->shlock > 0 || a->elock) {
		PUT_PCK_RES1 (packet, DOS_FALSE);
		PUT_PCK_RES2 (packet, ERROR_OBJECT_IN_USE);
		return;
    }
    if (a->dir) {
		/* This should take care of removing the fsdb if no files remain.  */
		fsdb_dir_writeback (a);
		if (my_rmdir (a->nname) == -1) {
		    PUT_PCK_RES1 (packet, DOS_FALSE);
		    PUT_PCK_RES2 (packet, dos_errno());
		    return;
		}
    } else {
		if (my_unlink (a->nname) == -1) {
		    PUT_PCK_RES1 (packet, DOS_FALSE);
		    PUT_PCK_RES2 (packet, dos_errno());
		    return;
		}
    }
    notify_check (unit, a);
    updatedirtime (a, 1);
    if (a->child != 0) {
		write_log ("Serious error in action_delete_object.\n");
		a->deleted = 1;
    } else {
		delete_aino (unit, a);
    }
    PUT_PCK_RES1 (packet, DOS_TRUE);
	gui_flicker_led (LED_HD, unit->unit, 2);
}

static void
action_set_date (Unit *unit, dpacket packet)
{
    uaecptr lock = GET_PCK_ARG2 (packet) << 2;
    uaecptr name = GET_PCK_ARG3 (packet) << 2;
    uaecptr date = GET_PCK_ARG4 (packet);
    a_inode *a;
    struct utimbuf ut;
    uae_u32 err;

	TRACE(("ACTION_SET_DATE(0x%lx,\"%s\")\n", lock, bstr (unit, name)));

    if (unit->ui.readonly) {
		PUT_PCK_RES1 (packet, DOS_FALSE);
		PUT_PCK_RES2 (packet, ERROR_DISK_WRITE_PROTECTED);
		return;
    }
/* note */
#if defined TARGET_AMIGAOS && defined WORDS_BIGENDIAN
    if (err == 0 && SetFileDate (a->nname, (struct DateStamp *) date) == DOSFALSE)
	err = IoErr ();
#else
	ut.actime = ut.modtime = put_time(get_long (date), get_long (date + 4), get_long (date + 8));
    a = find_aino (unit, lock, bstr (unit, name), &err);
    if (err == 0 && utime (a->nname, &ut) == -1)
		err = dos_errno ();
#endif
    if (err != 0) {
		PUT_PCK_RES1 (packet, DOS_FALSE);
		PUT_PCK_RES2 (packet, err);
	} else {
	    notify_check (unit, a);
		PUT_PCK_RES1 (packet, DOS_TRUE);
	}
	gui_flicker_led (LED_HD, unit->unit, 2);
}

static void
action_rename_object (Unit *unit, dpacket packet)
{
    uaecptr lock1 = GET_PCK_ARG1 (packet) << 2;
    uaecptr name1 = GET_PCK_ARG2 (packet) << 2;
    uaecptr lock2 = GET_PCK_ARG3 (packet) << 2;
    uaecptr name2 = GET_PCK_ARG4 (packet) << 2;
    a_inode *a1, *a2;
    uae_u32 err1, err2;
    Key *k1, *knext;
    int wehavekeys = 0;

	TRACE(("ACTION_RENAME_OBJECT(0x%lx,\"%s\",", lock1, bstr (unit, name1)));
	TRACE(("0x%lx,\"%s\")\n", lock2, bstr (unit, name2)));

    if (unit->ui.readonly) {
		PUT_PCK_RES1 (packet, DOS_FALSE);
		PUT_PCK_RES2 (packet, ERROR_DISK_WRITE_PROTECTED);
		return;
    }

    a1 = find_aino (unit, lock1, bstr (unit, name1), &err1);
    if (err1 != 0) {
		PUT_PCK_RES1 (packet, DOS_FALSE);
		PUT_PCK_RES2 (packet, err1);
		return;
    }
/* note */
    /* rename always fails if file is open for writing */
    for (k1 = unit->keys; k1; k1 = knext) {
        knext = k1->next;
	if (k1->aino == a1 && k1->fd >= 0 && k1->createmode == 2) {
	    PUT_PCK_RES1 (packet, DOS_FALSE);
	    PUT_PCK_RES2 (packet, ERROR_OBJECT_IN_USE);
	    return;
	}
    }

    /* See whether the other name already exists in the filesystem.  */
    a2 = find_aino (unit, lock2, bstr (unit, name2), &err2);

    if (a2 == a1) {
		/* Renaming to the same name, but possibly different case.  */
		if (_tcscmp (a1->aname, bstr_cut (unit, name2)) == 0) {
		    /* Exact match -> do nothing.  */
		    notify_check (unit, a1);
		    updatedirtime (a1, 1);
		    PUT_PCK_RES1 (packet, DOS_TRUE);
		    return;
		}
		a2 = a2->parent;
    } else if (a2 == 0 || err2 != ERROR_OBJECT_NOT_AROUND) {
		PUT_PCK_RES1 (packet, DOS_FALSE);
		PUT_PCK_RES2 (packet, err2 == 0 ? ERROR_OBJECT_EXISTS : err2);
		return;
    }

    a2 = create_child_aino (unit, a2, bstr_cut (unit, name2), a1->dir);
    if (a2 == 0) {
		PUT_PCK_RES1 (packet, DOS_FALSE);
		PUT_PCK_RES2 (packet, ERROR_DISK_IS_FULL); /* best we can do */
		return;
    }

	if (-1 == my_rename (a1->nname, a2->nname)) {
		int ret = -1;
		/* maybe we have open file handles that caused failure? */
		write_log ("rename '%s' -> '%s' failed, trying relocking..\n", a1->nname, a2->nname);
		wehavekeys = relock_do (unit, a1);
		/* try again... */
		ret = my_rename (a1->nname, a2->nname);
		/* restore locks */
		relock_re (unit, a1, a2, ret == -1 ? 1 : 0);
		if (ret == -1) {
		    delete_aino (unit, a2);
		    PUT_PCK_RES1 (packet, DOS_FALSE);
		    PUT_PCK_RES2 (packet, dos_errno ());
		    return;
		}
    }

    notify_check (unit, a1);
    notify_check (unit, a2);
    a2->comment = a1->comment;
    a1->comment = 0;
    a2->amigaos_mode = a1->amigaos_mode;
    a2->uniq = a1->uniq;
    a2->elock = a1->elock;
    a2->shlock = a1->shlock;
    a2->has_dbentry = a1->has_dbentry;
    a2->db_offset = a1->db_offset;
    a2->dirty = 0;
/* note */
#if 0
    /* Ugly fix: update any keys to point to new aino */
    for (k1 = unit->keys; k1; k1 = knext) {
	knext = k1->next;
	if (k1->aino == a1) k1->aino=a2;
    }
#endif

    move_exkeys (unit, a1, a2);
    move_aino_children (unit, a1, a2);
    delete_aino (unit, a1);
    a2->dirty = 1;
    if (a2->parent)
		fsdb_dir_writeback (a2->parent);
    updatedirtime (a2, 1);
    fsdb_set_file_attrs (a2);
    if (a2->elock > 0 || a2->shlock > 0 || wehavekeys > 0)
		de_recycle_aino (unit, a2);
    PUT_PCK_RES1 (packet, DOS_TRUE);
	gui_flicker_led (LED_HD, unit->unit, 2);
}

static void
	action_current_volume (Unit *unit, dpacket packet)
{
	if (filesys_isvolume(unit))
	    PUT_PCK_RES1 (packet, unit->volume >> 2);
	else
		PUT_PCK_RES1 (packet, 0);
}

static void
	action_rename_disk (Unit *unit, dpacket packet)
{
    uaecptr name = GET_PCK_ARG1 (packet) << 2;

    TRACE(("ACTION_RENAME_DISK(\"%s\")\n", bstr (unit, name)));

    if (unit->ui.readonly) {
	PUT_PCK_RES1 (packet, DOS_FALSE);
	PUT_PCK_RES2 (packet, ERROR_DISK_WRITE_PROTECTED);
	return;
    }
/* see v089 */
    /* get volume name */
    xfree (unit->ui.volname);
	unit->ui.volname = bstr1 (name);
	set_volume_name (unit);

    PUT_PCK_RES1 (packet, DOS_TRUE);
}

static void
	action_is_filesystem (Unit *unit, dpacket packet)
{
	TRACE(("ACTION_IS_FILESYSTEM()\n"));
    PUT_PCK_RES1 (packet, DOS_TRUE);
}

static void
	action_flush (Unit *unit, dpacket packet)
{
	TRACE(("ACTION_FLUSH()\n"));
    PUT_PCK_RES1 (packet, DOS_TRUE);
	flush_cache (unit, 0);
}

static void
	action_more_cache (Unit *unit, dpacket packet)
{
	TRACE(("ACTION_MORE_CACHE()\n"));
	PUT_PCK_RES1 (packet, 50); /* bug but AmigaOS expects it */
	if (GET_PCK_ARG1 (packet) != 0)
		flush_cache (unit, 0);
}

static void
	action_inhibit (Unit *unit, dpacket packet)
{
	PUT_PCK_RES1 (packet, DOS_TRUE);
	flush_cache (unit, 0);
	unit->inhibited = GET_PCK_ARG1 (packet) != 0;
	TRACE((_T("ACTION_INHIBIT(%d:%d)\n"), unit->unit, unit->inhibited));
}

static void
	action_write_protect (Unit *unit, dpacket packet)
{
	TRACE(("ACTION_WRITE_PROTECT()\n"));
	PUT_PCK_RES1 (packet, DOS_TRUE);
	if (GET_PCK_ARG1 (packet)) {
		if (!(unit->ui.readonly & 2)) {
			unit->ui.readonly |= 2;
			unit->lockkey = GET_PCK_ARG2 (packet);
		}
	} else {
		if (unit->ui.readonly & 2) {
			if (unit->lockkey == (uae_u32)GET_PCK_ARG2 (packet) || unit->lockkey == 0) {
				unit->ui.readonly &= ~2;
			} else {
				PUT_PCK_RES1 (packet, DOS_FALSE);
				PUT_PCK_RES2 (packet, 0);
			}
		}
	}
}

static void action_change_file_position64 (Unit *unit, dpacket packet)
{
	Key *k = lookup_key (unit, GET_PCK64_ARG1 (packet));
	uae_s64 pos = GET_PCK64_ARG2 (packet);
	long mode = GET_PCK64_ARG3 (packet);
	long whence = SEEK_CUR;
	uae_s64 res, cur;

	PUT_PCK64_RES0 (packet, DP64_INIT);

	if (k == 0) {
		PUT_PCK64_RES1 (packet, DOS_FALSE);
		PUT_PCK64_RES2 (packet, ERROR_INVALID_LOCK);
		return;
	}

	if (mode > 0)
		whence = SEEK_END;
	if (mode < 0)
		whence = SEEK_SET;

	TRACE((_T("ACTION_CHANGE_FILE_POSITION64(%s,%lld,%ld)\n"), k->aino->nname, pos, mode));
	gui_flicker_led (LED_HD, unit->unit, 1);

	cur = k->file_pos;
	{
		uae_s64 temppos = 0;
		uae_s64 filesize = fs_fsize64 (k->fd);

		if (whence == SEEK_CUR)
			temppos = cur + pos;
		if (whence == SEEK_SET)
			temppos = pos;
		if (whence == SEEK_END)
			temppos = filesize + pos;
		if (filesize < temppos) {
			res = -1;
			PUT_PCK64_RES1 (packet, res);
			PUT_PCK64_RES2 (packet, ERROR_SEEK_ERROR);
			return;
		}
	}
	res = fs_lseek64 (k->fd, pos, whence);

	if (-1 == res) {
		PUT_PCK64_RES1 (packet, DOS_FALSE);
		PUT_PCK64_RES2 (packet, ERROR_SEEK_ERROR);
	} else {
		PUT_PCK64_RES1 (packet, true);
		PUT_PCK64_RES2 (packet, 0);
		k->file_pos = fs_lseek64 (k->fd, 0, SEEK_CUR);
	}
	TRACE((_T("= oldpos %lld newpos %lld\n"), cur, (long long) k->file_pos));

}

static void action_get_file_position64 (Unit *unit, dpacket packet)
{
	Key *k = lookup_key (unit, GET_PCK64_ARG1 (packet));

	PUT_PCK64_RES0 (packet, DP64_INIT);

	if (k == 0) {
		PUT_PCK64_RES1 (packet, -1);
		PUT_PCK64_RES2 (packet, ERROR_INVALID_LOCK);
		return;
	}
	TRACE((_T("ACTION_GET_FILE_POSITION64(%s)=%lld\n"), k->aino->nname, (long long) k->file_pos));
	PUT_PCK64_RES1 (packet, k->file_pos);
	PUT_PCK64_RES2 (packet, 0);
}

static void action_change_file_size64 (Unit *unit, dpacket packet)
{
	Key *k, *k1;
	uae_s64 offset = GET_PCK64_ARG2 (packet);
	long mode = (uae_s32)GET_PCK64_ARG3 (packet);
	int whence = SEEK_CUR;

	PUT_PCK64_RES0 (packet, DP64_INIT);

	if (mode > 0)
		whence = SEEK_END;
	if (mode < 0)
		whence = SEEK_SET;

	TRACE(("ACTION_CHANGE_FILE_SIZE64(0x%lx, %I64d, 0x%x)\n", GET_PCK64_ARG1 (packet), offset, mode));

	k = lookup_key (unit, GET_PCK64_ARG1 (packet));
	if (k == 0) {
		PUT_PCK64_RES1 (packet, DOS_FALSE);
		PUT_PCK64_RES2 (packet, ERROR_OBJECT_NOT_AROUND);
		return;
	}

	gui_flicker_led (LED_HD, unit->unit, 1);
	k->notifyactive = 1;
	/* If any open files have file pointers beyond this size, truncate only
	* so far that these pointers do not become invalid.  */
	for (k1 = unit->keys; k1; k1 = k1->next) {
		if (k != k1 && k->aino == k1->aino) {
			if (k1->file_pos > offset)
				offset = k1->file_pos;
		}
	}

	/* Write one then truncate: that should give the right size in all cases.  */
	fs_lseek (k->fd, offset, whence);
	offset = fs_lseek64 (k->fd, offset, whence);
	fs_write (k->fd, /* whatever */(uae_u8*)&k1, 1);
	if (k->file_pos > offset)
		k->file_pos = offset;
	fs_lseek64 (k->fd, k->file_pos, SEEK_SET);

	if (my_truncate (k->aino->nname, offset) == -1) {
		PUT_PCK64_RES1 (packet, DOS_FALSE);
		PUT_PCK64_RES2 (packet, dos_errno ());
		return;
	}

	PUT_PCK64_RES1 (packet, DOS_TRUE);
	PUT_PCK64_RES2 (packet, 0);
}


static void action_get_file_size64 (Unit *unit, dpacket packet)
{
	Key *k = lookup_key (unit, GET_PCK64_ARG1 (packet));
	uae_s64 old, filesize;

	PUT_PCK64_RES0 (packet, DP64_INIT);

	if (k == 0) {
		PUT_PCK64_RES1 (packet, DOS_FALSE);
		PUT_PCK64_RES2 (packet, ERROR_INVALID_LOCK);
		return;
	}
	TRACE(("ACTION_GET_FILE_SIZE64(%s)\n", k->aino->nname));
	old = fs_lseek64 (k->fd, 0, SEEK_CUR);
	if (old >= 0) {
		filesize = fs_lseek64 (k->fd, 0, SEEK_END);
		if (filesize >= 0) {
			fs_lseek64 (k->fd, old, SEEK_SET);
			PUT_PCK64_RES1 (packet, filesize);
			PUT_PCK64_RES2 (packet, 0);
			return;
		}
	}
	PUT_PCK64_RES1 (packet, DOS_FALSE);
	PUT_PCK64_RES2 (packet, ERROR_SEEK_ERROR);
}

/* We don't want multiple interrupts to be active at the same time. I don't
 * know whether AmigaOS takes care of that, but this does. */
static uae_sem_t singlethread_int_sem;

static uae_u32 REGPARAM2 exter_int_helper (TrapContext *context)
{
    UnitInfo *uip = current_mountinfo.ui;
    uaecptr port;
	int n = m68k_dreg (&context->regs, 0);
    static int unit_no;

	switch (n) {
     case 0:
	/* Determine whether a given EXTER interrupt is for us. */
		if (uae_int_requested & 1) {
	    if (uae_sem_trywait (&singlethread_int_sem) != 0)
		/* Pretend it isn't for us. We might get it again later. */
		return 0;
	    /* Clear the interrupt flag _before_ we do any processing.
	     * That way, we can get too many interrupts, but never not
	     * enough. */
			filesys_in_interrupt++;
			uae_int_requested &= ~1;
	    unit_no = 0;
	    return 1;
	}
	return 0;
     case 1:
	/* Release a message_lock. This is called as soon as the message is
	 * received by the assembly code. We use the opportunity to check
	 * whether we have some locks that we can give back to the assembler
	 * code.
	 * Note that this is called from the main loop, unlike the other cases
	 * in this switch statement which are called from the interrupt handler.
	 */
#ifdef UAE_FILESYS_THREADS
	{
	    Unit *unit = find_unit (m68k_areg (&context->regs, 5));
	    uaecptr msg = m68k_areg (&context->regs, 4);
	    unit->cmds_complete = unit->cmds_acked;
	    while (comm_pipe_has_data (unit->ui.back_pipe)) {
		uaecptr locks, lockend;
		int cnt = 0;
		locks = read_comm_pipe_int_blocking (unit->ui.back_pipe);
		lockend = locks;
		while (get_long (lockend) != 0) {
		    if (get_long (lockend) == lockend) {
			write_log ("filesystem lock queue corrupted!\n");
			break;
		    }
		    lockend = get_long (lockend);
		    cnt++;
		}
		TRACE(("%d %x %x %x\n", cnt, locks, lockend, m68k_areg (&context->regs, 3)));
		put_long (lockend, get_long (m68k_areg (&context->regs, 3)));
		put_long (m68k_areg (&context->regs, 3), locks);
	    }
	}
#else
	write_log ("exter_int_helper should not be called with arg 1!\n");
#endif
	break;
     case 2:
	/* Find work that needs to be done:
	 * return d0 = 0: none
	 *        d0 = 1: PutMsg(), port in a0, message in a1
	 *        d0 = 2: Signal(), task in a1, signal set in d1
	 *        d0 = 3: ReplyMsg(), message in a1
	 *        d0 = 4: Cause(), interrupt in a1
// NOTE 
	 *        d0 = 5: AllocMem(), size in d0, flags in d1, pointer to address at a0
	 *        d0 = 6: FreeMem(), memory in a1, size in d0
//
	 *        d0 = 5: Send FileNofication message, port in a0, notifystruct in a1
	 */

#ifdef SUPPORT_THREADS
	/* First, check signals/messages */
	while (comm_pipe_has_data (&native2amiga_pending)) {
	    int cmd = read_comm_pipe_int_blocking (&native2amiga_pending);
	    switch (cmd) {
	     case 0: /* Signal() */
		m68k_areg (&context->regs, 1) = read_comm_pipe_u32_blocking (&native2amiga_pending);
		m68k_dreg (&context->regs, 1) = read_comm_pipe_u32_blocking (&native2amiga_pending);
		return 2;

	     case 1: /* PutMsg() */
		m68k_areg (&context->regs, 0) = read_comm_pipe_u32_blocking (&native2amiga_pending);
		m68k_areg (&context->regs, 1) = read_comm_pipe_u32_blocking (&native2amiga_pending);
		return 1;

	     case 2: /* ReplyMsg() */
		m68k_areg (&context->regs, 1) = read_comm_pipe_u32_blocking (&native2amiga_pending);
		return 3;

	     case 3: /* Cause() */
		m68k_areg (&context->regs, 1) = read_comm_pipe_u32_blocking (&native2amiga_pending);
		return 4;

	     case 4: /* NotifyHack() */
		m68k_areg (&context->regs, 0) = read_comm_pipe_u32_blocking (&native2amiga_pending);
		m68k_areg (&context->regs, 1) = read_comm_pipe_u32_blocking (&native2amiga_pending);
		return 5;

	     default:
		write_log ("exter_int_helper: unknown native action %d\n", cmd);
		break;
	    }
	}
#endif

	/* Find some unit that needs a message sent, and return its port,
	 * or zero if all are done.
	 * Take care not to dereference self for units that didn't have their
	 * startup packet sent. */
	for (;;) {
	    if (unit_no >= current_mountinfo.num_units)
		return 0;

			if (uip[unit_no].open && uip[unit_no].self != 0
		&& uip[unit_no].self->cmds_acked == uip[unit_no].self->cmds_complete
		&& uip[unit_no].self->cmds_acked != uip[unit_no].self->cmds_sent)
		break;
	    unit_no++;
	}
	uip[unit_no].self->cmds_acked = uip[unit_no].self->cmds_sent;
	port = uip[unit_no].self->port;
	if (port) {
	    m68k_areg (&context->regs, 0) = port;
	    m68k_areg (&context->regs, 1) = find_unit (port)->dummy_message;
	    unit_no++;
	    return 1;
	}
	break;
	case 3:
		uae_sem_wait (&singlethread_int_sem);
		break;
     case 4:
	/* Exit the interrupt, and release the single-threading lock. */
		filesys_in_interrupt--;
	uae_sem_post (&singlethread_int_sem);
	break;

     default:
	write_log ("Shouldn't happen in exter_int_helper.\n");
	break;
    }
    return 0;
}

static int handle_packet (Unit *unit, dpacket pck)
{
    uae_s32 type = GET_PCK_TYPE (pck);
    PUT_PCK_RES2 (pck, 0);

	if (unit->inhibited && filesys_isvolume(unit)
		&& type != ACTION_INHIBIT && type != ACTION_MORE_CACHE
		&& type != ACTION_DISK_INFO) {
			PUT_PCK_RES1 (pck, DOS_FALSE);
			PUT_PCK_RES2 (pck, ERROR_NOT_A_DOS_DISK);
			return 1;
	}
	if (type != ACTION_INHIBIT && type != ACTION_CURRENT_VOLUME
		&& type != ACTION_IS_FILESYSTEM && type != ACTION_MORE_CACHE
		&& type != ACTION_WRITE_PROTECT
		&& !filesys_isvolume(unit)) {
			PUT_PCK_RES1 (pck, DOS_FALSE);
			PUT_PCK_RES2 (pck, ERROR_NO_DISK);
			return 1;
	}

    switch (type) {
     case ACTION_LOCATE_OBJECT: action_lock (unit, pck); break;
     case ACTION_FREE_LOCK: action_free_lock (unit, pck); break;
     case ACTION_COPY_DIR: action_dup_lock (unit, pck); break;
     case ACTION_DISK_INFO: action_disk_info (unit, pck); break;
     case ACTION_INFO: action_info (unit, pck); break;
     case ACTION_EXAMINE_OBJECT: action_examine_object (unit, pck); break;
     case ACTION_EXAMINE_NEXT: action_examine_next (unit, pck); break;
     case ACTION_FIND_INPUT: action_find_input (unit, pck); break;
     case ACTION_FIND_WRITE: action_find_write (unit, pck); break;
     case ACTION_FIND_OUTPUT: action_find_output (unit, pck); break;
     case ACTION_END: action_end (unit, pck); break;
     case ACTION_READ: action_read (unit, pck); break;
     case ACTION_WRITE: action_write (unit, pck); break;
     case ACTION_SEEK: action_seek (unit, pck); break;
     case ACTION_SET_PROTECT: action_set_protect (unit, pck); break;
     case ACTION_SET_COMMENT: action_set_comment (unit, pck); break;
     case ACTION_SAME_LOCK: action_same_lock (unit, pck); break;
     case ACTION_PARENT: action_parent (unit, pck); break;
     case ACTION_CREATE_DIR: action_create_dir (unit, pck); break;
     case ACTION_DELETE_OBJECT: action_delete_object (unit, pck); break;
     case ACTION_RENAME_OBJECT: action_rename_object (unit, pck); break;
     case ACTION_SET_DATE: action_set_date (unit, pck); break;
     case ACTION_CURRENT_VOLUME: action_current_volume (unit, pck); break;
     case ACTION_RENAME_DISK: action_rename_disk (unit, pck); break;
     case ACTION_IS_FILESYSTEM: action_is_filesystem (unit, pck); break;
     case ACTION_FLUSH: action_flush (unit, pck); break;
	case ACTION_MORE_CACHE: action_more_cache (unit, pck); break;
	case ACTION_INHIBIT: action_inhibit (unit, pck); break;
	case ACTION_WRITE_PROTECT: action_write_protect (unit, pck); break;

     /* 2.0+ packet types */
     case ACTION_SET_FILE_SIZE: action_set_file_size (unit, pck); break;
     case ACTION_EXAMINE_FH: action_examine_fh (unit, pck); break;
     case ACTION_FH_FROM_LOCK: action_fh_from_lock (unit, pck); break;
     case ACTION_COPY_DIR_FH: action_lock_from_fh (unit, pck); break;
     case ACTION_CHANGE_MODE: action_change_mode (unit, pck); break;
     case ACTION_PARENT_FH: action_parent_fh (unit, pck); break;
     case ACTION_ADD_NOTIFY: action_add_notify (unit, pck); break;
     case ACTION_REMOVE_NOTIFY: action_remove_notify (unit, pck); break;
	case ACTION_EXAMINE_ALL: return action_examine_all (unit, pck);
	case ACTION_EXAMINE_ALL_END: return action_examine_all_end (unit, pck);

		/* OS4+ packet types */
	case ACTION_CHANGE_FILE_POSITION64: action_change_file_position64 (unit, pck); break;
	case ACTION_GET_FILE_POSITION64: action_get_file_position64 (unit, pck); break;
	case ACTION_CHANGE_FILE_SIZE64: action_change_file_size64 (unit, pck); break;
	case ACTION_GET_FILE_SIZE64: action_get_file_size64 (unit, pck); break;

     /* unsupported packets */
     case ACTION_LOCK_RECORD:
     case ACTION_FREE_RECORD:
     case ACTION_MAKE_LINK:
     case ACTION_READ_LINK:
     case ACTION_FORMAT:
	 return 0;
     default:
	write_log ("FILESYS: UNKNOWN PACKET %x\n", type);
	return 0;
    }
    return 1;
}

#ifdef UAE_FILESYS_THREADS
static void *filesys_thread (void *unit_v)
{
    UnitInfo *ui = (UnitInfo *)unit_v;

    uae_set_thread_priority (2);
    for (;;) {
	uae_u8 *pck;
	uae_u8 *msg;
	uae_u32 morelocks;

	pck = (uae_u8 *)read_comm_pipe_pvoid_blocking (ui->unit_pipe);
	msg = (uae_u8 *)read_comm_pipe_pvoid_blocking (ui->unit_pipe);
	morelocks = (uae_u32)read_comm_pipe_int_blocking (ui->unit_pipe);

	if (ui->reset_state == FS_GO_DOWN) {
	    if (pck != 0)
		continue;
	    /* Death message received. */
	    uae_sem_post ((uae_sem_t *)&ui->reset_sync_sem);
	    /* Die.  */
	    return 0;
	}

	put_long (get_long (morelocks), get_long (ui->self->locklist));
	put_long (ui->self->locklist, morelocks);
	if (! handle_packet (ui->self, pck)) {
	    PUT_PCK_RES1 (pck, DOS_FALSE);
	    PUT_PCK_RES2 (pck, ERROR_ACTION_NOT_KNOWN);
	}
	/* Mark the packet as processed for the list scan in the assembly code. */
	do_put_mem_long ((uae_u32 *)(msg + 4), -1);
	/* Acquire the message lock, so that we know we can safely send the
	 * message. */
	ui->self->cmds_sent++;
	/* The message is sent by our interrupt handler, so make sure an interrupt
	 * happens. */
	do_uae_int_requested ();
	/* Send back the locks. */
	if (get_long (ui->self->locklist) != 0)
	    write_comm_pipe_int (ui->back_pipe, (int)(get_long (ui->self->locklist)), 0);
	put_long (ui->self->locklist, 0);

    }
    return 0;
}
#endif

/* Talk about spaghetti code... */
static uae_u32 REGPARAM2 filesys_handler (TrapContext *context)
{
    Unit *unit = find_unit (m68k_areg (&context->regs, 5));
    uaecptr packet_addr = m68k_dreg (&context->regs, 3);
    uaecptr message_addr = m68k_areg (&context->regs, 4);
    uae_u8 *pck;
    uae_u8 *msg;
    if (! valid_address (packet_addr, 36) || ! valid_address (message_addr, 14)) {
		write_log ("FILESYS: Bad address %x/%x passed for packet.\n", packet_addr, message_addr);
	goto error2;
    }
    pck = get_real_address (packet_addr);
    msg = get_real_address (message_addr);

    do_put_mem_long ((uae_u32 *)(msg + 4), -1);
    if (!unit || !unit->volume) {
		write_log ("FILESYS: was not initialized.\n");
	goto error;
    }
#ifdef UAE_FILESYS_THREADS
    {
	uae_u32 morelocks;
	if (!unit->ui.unit_pipe)
	    goto error;
	/* Get two more locks and hand them over to the other thread. */
	morelocks = get_long (m68k_areg (&context->regs, 3));
	put_long (m68k_areg (&context->regs, 3), get_long (get_long (morelocks)));
	put_long (get_long (morelocks), 0);

	/* The packet wasn't processed yet. */
	do_put_mem_long ((uae_u32 *)(msg + 4), 0);
	write_comm_pipe_pvoid (unit->ui.unit_pipe, (void *)pck, 0);
	write_comm_pipe_pvoid (unit->ui.unit_pipe, (void *)msg, 0);
	write_comm_pipe_int (unit->ui.unit_pipe, (int)morelocks, 1);
	/* Don't reply yet. */
	return 1;
    }
#endif

    if (! handle_packet (unit, pck)) {
	error:
	PUT_PCK_RES1 (pck, DOS_FALSE);
	PUT_PCK_RES2 (pck, ERROR_ACTION_NOT_KNOWN);
    }
    TRACE(("reply: %8lx, %ld\n", GET_PCK_RES1 (pck), GET_PCK_RES2 (pck)));

    error2:

    return 0;
}

static void init_filesys_diagentry (void)
{
    do_put_mem_long ((uae_u32 *)(filesysory + 0x2100), EXPANSION_explibname);
    do_put_mem_long ((uae_u32 *)(filesysory + 0x2104), filesys_configdev);
    do_put_mem_long ((uae_u32 *)(filesysory + 0x2108), EXPANSION_doslibname);
    do_put_mem_long ((uae_u32 *)(filesysory + 0x210c), current_mountinfo.num_units);
    native2amiga_startup ();
}

void filesys_start_threads (void)
{
    UnitInfo *uip;
    int i;
	uip = current_mountinfo.ui;
	filesys_in_interrupt = 0;
	for (i = 0; i < MAX_FILESYSTEM_UNITS; i++) {
		UnitInfo *ui = &uip[i];
		if (!ui->open)
			continue;
		filesys_start_thread (ui, i);
	}
}

void filesys_cleanup (void)
{
	filesys_free_handles ();
    free_mountinfo (&current_mountinfo);
}

void filesys_free_handles (void)
{
	Unit *u, *u1;
	for (u = units; u; u = u1) {
		Key *k1, *knext;
		u1 = u->next;
		for (k1 = u->keys; k1; k1 = knext) {
			knext = k1->next;
			if (k1->fd)
				fs_close (k1->fd);
			xfree (k1);
		}
		u->keys = NULL;
		free_all_ainos (u, &u->rootnode);
		u->rootnode.next = u->rootnode.prev = &u->rootnode;
		u->aino_cache_size = 0;
		xfree (u->newrootdir);
		xfree (u->newvolume);
		u->newrootdir = NULL;
		u->newvolume = NULL;
	}
}

static void filesys_reset2 (void)
{
    Unit *u, *u1;

    /* We get called once from customreset at the beginning of the program
     * before filesys_start_threads has been called. Survive that.  */
    if (savestate_state == STATE_RESTORE)
		return;

	filesys_free_handles ();
    for (u = units; u; u = u1) {
	u1 = u->next;
	xfree (u);
    }
    units = 0;
	key_uniq = 0;
	a_uniq = 0;
	free_mountinfo (&current_mountinfo);
}

void filesys_reset (void)
{
	if (savestate_state == STATE_RESTORE)
		return;
	filesys_reset2 ();
	initialize_mountinfo (&current_mountinfo);
}

static void filesys_prepare_reset2 (void)
{
#ifdef UAE_FILESYS_THREADS
	UnitInfo *uip;
	//    Unit *u;
	int i;

    uip = current_mountinfo.ui;
	for (i = 0; i < MAX_FILESYSTEM_UNITS; i++) {
		if (uip[i].open && uip[i].unit_pipe != 0) {
	    uae_sem_init ((uae_sem_t *)&uip[i].reset_sync_sem, 0, 0);
	    uip[i].reset_state = FS_GO_DOWN;
	    /* send death message */
	    write_comm_pipe_int (uip[i].unit_pipe, 0, 0);
	    write_comm_pipe_int (uip[i].unit_pipe, 0, 0);
	    write_comm_pipe_int (uip[i].unit_pipe, 0, 1);
	    uae_sem_wait ((uae_sem_t *)&uip[i].reset_sync_sem);
			uae_end_thread (&uip[i].tid);
		}
	}
#endif
	filesys_free_handles();
#if 0
    u = units;
    while (u != 0) {
	free_all_ainos (u, &u->rootnode);
	u->rootnode.next = u->rootnode.prev = &u->rootnode;
	u->aino_cache_size = 0;
	u = u->next;
    }
#endif
}

void filesys_prepare_reset (void)
{
	if (savestate_state == STATE_RESTORE)
		return;
	filesys_prepare_reset2 ();
}

static uae_u32 REGPARAM2 filesys_diagentry (TrapContext *context)
{
    uaecptr resaddr = m68k_areg (&context->regs, 2) + 0x10;
    uaecptr start = resaddr;
    uaecptr residents, tmp;

	TRACE (("filesystem: diagentry called\n"));

    filesys_configdev = m68k_areg (&context->regs, 3);
    init_filesys_diagentry ();

    if (ROM_hardfile_resid != 0) {
	/* Build a struct Resident. This will set up and initialize
	 * the uae.device */
	put_word (resaddr + 0x0, 0x4AFC);
	put_long (resaddr + 0x2, resaddr);
	put_long (resaddr + 0x6, resaddr + 0x1A); /* Continue scan here */
	put_word (resaddr + 0xA, 0x8101); /* RTF_AUTOINIT|RTF_COLDSTART; Version 1 */
	put_word (resaddr + 0xC, 0x0305); /* NT_DEVICE; pri 05 */
	put_long (resaddr + 0xE, ROM_hardfile_resname);
	put_long (resaddr + 0x12, ROM_hardfile_resid);
	put_long (resaddr + 0x16, ROM_hardfile_init); /* calls filesys_init */
    }
    resaddr += 0x1A;
    tmp = resaddr;
    /* The good thing about this function is that it always gets called
     * when we boot. So we could put all sorts of stuff that wants to be done
     * here.
     * We can simply add more Resident structures here. Although the Amiga OS
     * only knows about the one at address DiagArea + 0x10, we scan for other
     * Resident structures and call InitResident() for them at the end of the
     * diag entry. */

	resaddr = uaeres_startup (resaddr);
#ifdef BSDSOCKET
	resaddr = bsdlib_startup (resaddr);
#endif
#ifdef SCSIEMU
    resaddr = scsidev_startup(resaddr);
#endif
#ifdef SANA2
	resaddr = netdev_startup (resaddr);
#endif
#ifdef UAESERIAL
	resaddr = uaeserialdev_startup (resaddr);
#endif

	/* scan for Residents and return pointer to array of them */
	residents = resaddr;
	while (tmp < residents && tmp > start) {
		if (get_word (tmp) == 0x4AFC &&
			get_long (tmp + 0x2) == tmp) {
				put_word (resaddr, 0x227C);         /* move.l #tmp,a1 */
				put_long (resaddr + 2, tmp);
				put_word (resaddr + 6, 0x7200);     /* moveq #0,d1 */
				put_long (resaddr + 8, 0x4EAEFF9A); /* jsr -$66(a6) ; InitResident */
				resaddr += 12;
				tmp = get_long (tmp + 0x6);
		} else {
			tmp += 2;
		}
	}
    /* call setup_exter */
    put_word (resaddr +  0, 0x2079);
	put_long (resaddr +  2, rtarea_base + bootrom_header + 4 + 5 * 4); /* move.l RTAREA_BASE+setup_exter,a0 */
    put_word (resaddr +  6, 0xd1fc);
	put_long (resaddr +  8, rtarea_base + bootrom_header); /* add.l #RTAREA_BASE+bootrom_header,a0 */
    put_word (resaddr + 12, 0x4e90); /* jsr (a0) */
    put_word (resaddr + 14, 0x7001); /* moveq.l #1,d0 */
    put_word (resaddr + 16, RTS);

    m68k_areg (&context->regs, 0) = residents;
    return 1;
}

/* don't forget filesys.asm! */
#define PP_MAXSIZE 4 * 96
#define PP_FSSIZE 400
#define PP_FSPTR 404
#define PP_FSRES 408
#define PP_EXPLIB 412
#define PP_FSHDSTART 416

static uae_u32 REGPARAM2 filesys_dev_bootfilesys (TrapContext *context)
{
    uaecptr devicenode = m68k_areg (&context->regs, 3);
    uaecptr parmpacket = m68k_areg (&context->regs, 1);
    uaecptr fsres = get_long (parmpacket + PP_FSRES);
    uaecptr fsnode;
    uae_u32 dostype, dostype2;
    UnitInfo *uip = current_mountinfo.ui;
    uae_u32 no = m68k_dreg (&context->regs, 6);
    int unit_no = no & 65535;
    int type = is_hardfile (&current_mountinfo, unit_no);

    if (type == FILESYS_VIRTUAL)
	return 0;
    dostype = get_long (parmpacket + 80);
    fsnode = get_long (fsres + 18);
    while (get_long (fsnode)) {
	dostype2 = get_long (fsnode + 14);
	if (dostype2 == dostype) {
			int i;
			uae_u32 pf = get_long (fsnode + 22); // fse_PatchFlags
			for (i = 0; i < 32; i++) {
				if (pf & (1 << i))
					put_long (devicenode + 4 + i * 4, get_long (fsnode + 22 + 4 + i * 4));
	    }
	    return 1;
	}
	fsnode = get_long (fsnode);
    }
    return 0;
}

extern void picasso96_alloc (TrapContext*);
static uae_u32 REGPARAM2 filesys_init_storeinfo (TrapContext *context)
{
	int ret = -1;
	switch (m68k_dreg (&context->regs, 1))
	{
	case 1:
		mountertask = m68k_areg (&context->regs, 1);
#ifdef PICASSO96
		picasso96_alloc (context);
#endif
		break;
	case 2:
		ret = automountunit;
		automountunit = -1;
		break;
	case 3:
		return 0;
	}
	return ret;
}

/* Remember a pointer AmigaOS gave us so we can later use it to identify
 * which unit a given startup message belongs to.  */
static uae_u32 REGPARAM2 filesys_dev_remember (TrapContext *context)
{
    uae_u32 no = m68k_dreg (&context->regs, 6);
    int unit_no = no & 65535;
    int sub_no = no >> 16;
    UnitInfo *uip = &current_mountinfo.ui[unit_no];
    int i;
    uaecptr devicenode = m68k_areg (&context->regs, 3);
    uaecptr parmpacket = m68k_areg (&context->regs, 1);

    /* copy filesystem loaded from RDB */
    if (get_long (parmpacket + PP_FSPTR)) {
	for (i = 0; i < uip->rdb_filesyssize; i++)
	    put_byte (get_long (parmpacket + PP_FSPTR) + i, uip->rdb_filesysstore[i]);
	xfree (uip->rdb_filesysstore);
	uip->rdb_filesysstore = 0;
	uip->rdb_filesyssize = 0;
    }
    if ( ((uae_s32)m68k_dreg (&context->regs, 3)) >= 0)
	uip->startup = get_long (devicenode + 28);
    return devicenode;
}

static int legalrdbblock (UnitInfo *uip, unsigned int block)
{
	if (block <= 0)
		return 0;
	if ((uae_u64)block >= (uip->hf.virtsize / uip->hf.blocksize) )
		return 0;
    return 1;
}

static uae_u32 rl (uae_u8 *p)
{
    return (p[0] << 24) | (p[1] << 16) | (p[2] << 8) | (p[3]);
}

static int rdb_checksum (const char *id, uae_u8 *p, int block)
{
    uae_u32 sum = 0;
    int i, blocksize;

	if (memcmp (id, p, 4))
		return 0;
	blocksize = rl (p + 4);
	if (blocksize < 1 || blocksize * 4 > FILESYS_MAX_BLOCKSIZE)
		return 0;
	for (i = 0; i < blocksize; i++)
		sum += rl (p + i * 4);
	sum = -sum;
	if (sum) {
		TCHAR *s = au (id);
		write_log (_T("RDB: block %d ('%s') checksum error\n"), block, s);
		xfree (s);
		return 0;
	}
	return 1;
}

static int device_isdup (uaecptr expbase, TCHAR *devname)
{
	uaecptr bnode, dnode, name;
	int len, i;
	TCHAR dname[256];

	bnode = get_long (expbase + 74); /* expansion.library bootnode list */
	while (get_long (bnode)) {
		dnode = get_long (bnode + 16); /* device node */
		name = get_long (dnode + 40) << 2; /* device name BSTR */
		len = get_byte (name);
		for (i = 0; i < len; i++)
			dname[i] = get_byte (name + 1 + i);
		dname[len] = 0;
		if (!_tcsicmp (devname, dname))
			return 1;
		bnode = get_long (bnode);
	}
	return 0;
}

static TCHAR *device_dupfix (uaecptr expbase, const TCHAR *devname)
{
	int modified;
	TCHAR newname[256];

	_tcscpy (newname, devname);
    modified = 1;
    while (modified) {
	modified = 0;
		if (device_isdup (expbase, newname)) {
			if (_tcslen (newname) > 2 && newname[_tcslen (newname) - 2] == '_') {
				newname[_tcslen (newname) - 1]++;
		    } else {
				_tcscat (newname, "_0");
		    }
		    modified = 1;
	}
    }
	return strdup (newname);
}

static void dump_partinfo (const char *name, int num, uaecptr pp, int partblock)
{
    uae_u32 dostype = get_long (pp + 80);
	uae_u64 size;

	size = ((uae_u64)get_long (pp + 20)) * 4 * get_long (pp + 28) * get_long (pp + 36) * (get_long (pp + 56) - get_long (pp + 52) + 1);

	write_log ("RDB: '%s' dostype=%08X. PartBlock=%d\n", name, dostype, partblock);
	write_log ("BlockSize: %d, Surfaces: %d, SectorsPerBlock %d\n",
	       get_long (pp + 20) * 4, get_long (pp + 28), get_long (pp + 32));
	write_log ("SectorsPerTrack: %d, Reserved: %d, LowCyl %d, HighCyl %d, Size %dM\n",
	       get_long (pp + 36), get_long (pp + 40), get_long (pp + 52), get_long (pp + 56), (uae_u32)(size >> 20));

	write_log ("Buffers: %d, BufMemType: %08x, MaxTransfer: %08x, BootPri: %d\n",
	       get_long (pp + 60), get_long (pp + 64), get_long (pp + 68), get_long (pp + 76));
}

#define rdbmnt write_log (_T("Mounting uaehf.device %d (%d) (size=%llu):\n"), unit_no, partnum, hfd->virtsize);

static int rdb_mount (UnitInfo *uip, int unit_no, unsigned int partnum, uaecptr parmpacket)
{
    unsigned int lastblock = 63, blocksize, readblocksize;
    int driveinitblock, badblock;
    uae_u8 bufrdb[FILESYS_MAX_BLOCKSIZE], *buf = 0;
    uae_u8 *fsmem = 0;
    unsigned int rdblock, partblock, fileblock, i;
    int lsegblock;
    uae_u32 flags;
    struct hardfiledata *hfd = &uip->hf;
    uae_u32 dostype;
    uaecptr fsres, fsnode;
    int err = 0;
    int oldversion, oldrevision;
    int newversion, newrevision;
	TCHAR *s;

	write_log ("%s:\n", uip->rootdir);
	if (hfd->drive_empty) {
		rdbmnt
		write_log ("ignored, drive is empty\n");
		return -2;
	}
    if (hfd->blocksize == 0) {
		rdbmnt
		write_log ("failed, blocksize == 0\n");
		return -1;
	}
	if ((lastblock > 0) && (((uae_u64)lastblock * hfd->blocksize) > hfd->virtsize) ) {
		rdbmnt
		write_log ("failed, too small (%d*%d > %I64u)\n", lastblock, hfd->blocksize, hfd->virtsize);
		return -2;
    }
    for (rdblock = 0; rdblock < lastblock; rdblock++) {
		hdf_read (hfd, bufrdb, rdblock * hfd->blocksize, hfd->blocksize);
		if (rdb_checksum ("RDSK", bufrdb, rdblock))
		    break;
		if (rdb_checksum ("CDSK", bufrdb, rdblock))
			break;
		hdf_read (hfd, bufrdb, rdblock * hfd->blocksize, hfd->blocksize);
		if (!memcmp ("RDSK", bufrdb, 4)) {
			bufrdb[0xdc] = 0;
			bufrdb[0xdd] = 0;
			bufrdb[0xde] = 0;
			bufrdb[0xdf] = 0;
			if (rdb_checksum ("RDSK", bufrdb, rdblock)) {
				write_log (_T("Windows trashed RDB detected, fixing..\n"));
				hdf_write (hfd, bufrdb, rdblock * hfd->blocksize, hfd->blocksize);
				break;
		    }
		}
    }
    if (rdblock == lastblock) {
		rdbmnt
		write_log ("failed, no RDB detected\n");
		return -2;
    }
    blocksize = rl (bufrdb + 16);
    readblocksize = blocksize > hfd->blocksize ? blocksize : hfd->blocksize;
    badblock = rl (bufrdb + 24);
    if (badblock != -1) {
		rdbmnt
		write_log ("RDB: badblock list is not yet supported. Contact the author.\n");
		return -2;
    }
    driveinitblock = rl (bufrdb + 36);
    if (driveinitblock != -1) {
		rdbmnt
		write_log ("RDB: driveinit is not yet supported. Contact the author.\n");
		return -2;
    }
    hfd->cylinders = rl (bufrdb + 64);
    hfd->sectors = rl (bufrdb + 68);
    hfd->heads = rl (bufrdb + 72);
    fileblock = rl (bufrdb + 32);

	if (partnum == 0) {
		write_log ("RDB: RDSK detected at %d, FSHD=%d, C=%d S=%d H=%d\n",
			rdblock, fileblock, hfd->cylinders, hfd->sectors, hfd->heads);
	}

    buf = xmalloc (uae_u8, readblocksize);
    for (i = 0; i <= partnum; i++) {
		if (i == 0)
			partblock = rl (bufrdb + 28);
		else
			partblock = rl (buf + 4 * 4);
		if (!legalrdbblock (uip, partblock)) {
		    err = -2;
		    goto error;
		}
		memset (buf, 0, readblocksize);
		hdf_read (hfd, buf, partblock * hfd->blocksize, readblocksize);
		if (!rdb_checksum ("PART", buf, partblock)) {
		    err = -2;
		    goto error;
		}
    }

    rdbmnt
    flags = rl (buf + 20);
    if (flags & 2) { /* do not mount */
		err = -1;
		write_log ("RDB: Automount disabled, not mounting\n");
		goto error;
    }

    if (!(flags & 1)) /* not bootable */
		m68k_dreg (&regs, 7) = m68k_dreg (&regs, 7) & ~1;

    buf[37 + buf[36]] = 0; /* zero terminate BSTR */
	s = au ((char*)buf + 37);
	uip->rdb_devname_amiga[partnum] = ds (device_dupfix (get_long (parmpacket + PP_EXPLIB), s));
	xfree (s);
    put_long (parmpacket, uip->rdb_devname_amiga[partnum]); /* name */
    put_long (parmpacket + 4, ROM_hardfile_resname);
    put_long (parmpacket + 8, uip->devno);
    put_long (parmpacket + 12, 0); /* Device flags */
    for (i = 0; i < PP_MAXSIZE; i++)
	put_byte (parmpacket + 16 + i, buf[128 + i]);
	dump_partinfo (buf + 37, uip->devno, parmpacket, partblock);
    dostype = get_long (parmpacket + 80);

    if (dostype == 0) {
		write_log ("RDB: mount failed, dostype=0\n");
		err = -1;
		goto error;
    }

	if (hfd->cylinders * hfd->sectors * hfd->heads * blocksize > hfd->virtsize)
		write_log ("RDB: WARNING: end of partition > size of disk!\n");

    err = 2;

    /* load custom filesystems if needed */
	if (fileblock == -1 || !legalrdbblock (uip, fileblock))
		goto error;

    fsres = get_long (parmpacket + PP_FSRES);
    if (!fsres) {
		write_log ("RDB: FileSystem.resource not found, this shouldn't happen!\n");
		goto error;
    }

    for (;;) {
	if (!legalrdbblock (uip, fileblock)) {
	    err = -1;
	    goto error;
	}
	hdf_read (hfd, buf, fileblock * hfd->blocksize, readblocksize);
	if (!rdb_checksum ("FSHD", buf, fileblock)) {
	    err = -1;
	    goto error;
	}
	fileblock = rl (buf + 16);
	if ((dostype >> 8) == (rl (buf + 32) >> 8))
	    break;
    }

    fsnode = get_long (fsres + 18);
    while (get_long (fsnode)) {
	if (get_long (fsnode + 14) == dostype)
	    break;
	fsnode = get_long (fsnode);
    }

    oldversion = oldrevision = -1;
    newversion = (buf[36] << 8) | buf[37];
    newrevision = (buf[38] << 8) | buf[39];
    write_log ("RDB: RDB filesystem %08.8X version %d.%d\n", dostype, newversion, newrevision);
    if (get_long (fsnode)) {
	oldversion = get_word (fsnode + 18);
	oldrevision = get_word (fsnode + 20);
	} else {
		fsnode = 0;
	}

	for (;;) {
		if (fileblock == -1) {
			if (!fsnode)
				write_log ("RDB: required FS %08X not in FileSystem.resource or in RDB\n", dostype);
			goto error;
		}
		if (!legalrdbblock (uip, fileblock)) {
			write_log ("RDB: corrupt FSHD pointer %d\n", fileblock);
			goto error;
		}
		memset (buf, 0, readblocksize);
		hdf_read (hfd, buf, fileblock * hfd->blocksize, readblocksize);
		if (!rdb_checksum ("FSHD", buf, fileblock)) {
			write_log ("RDB: checksum error in FSHD block %d\n", fileblock);
			goto error;
		}
		fileblock = rl (buf + 16);
		if ((dostype >> 8) == (rl (buf + 32) >> 8))
			break;
	}
	newversion = (buf[36] << 8) | buf[37];
	newrevision = (buf[38] << 8) | buf[39];

	write_log ("RDB: RDB filesystem %08X version %d.%d\n", dostype, newversion, newrevision);
	if (fsnode) {
		write_log ("RDB: %08X in FileSystem.resource version %d.%d\n", dostype, oldversion, oldrevision);
    }
    if (newversion * 65536 + newrevision <= oldversion * 65536 + oldrevision && oldversion >= 0) {
		write_log ("RDB: FS in FileSystem.resource is newer or same, ignoring RDB filesystem\n");
	goto error;
    }

    for (i = 0; i < 140; i++)
	put_byte (parmpacket + PP_FSHDSTART + i, buf[32 + i]);
    put_long (parmpacket + PP_FSHDSTART, dostype);
    /* we found required FSHD block */
	fsmem = xmalloc (uae_u8, 262144);
    lsegblock = rl (buf + 72);
    i = 0;
    for (;;) {
		int pb = lsegblock;
	if (!legalrdbblock (uip, lsegblock))
	    goto error;
		memset (buf, 0, readblocksize);
	hdf_read (hfd, buf, lsegblock * hfd->blocksize, readblocksize);
	if (!rdb_checksum ("LSEG", buf, lsegblock))
	    goto error;
	lsegblock = rl (buf + 16);
		if (lsegblock == pb)
			goto error;
		if ((i + 1) * (blocksize - 20) >= 262144)
			goto error;
	memcpy (fsmem + i * (blocksize - 20), buf + 20, blocksize - 20);
	i++;
	if (lsegblock == -1)
	    break;
    }
	write_log ("RDB: Filesystem loaded, %d bytes\n", i * (blocksize - 20));
    put_long (parmpacket + PP_FSSIZE, i * (blocksize - 20)); /* RDB filesystem size hack */
    uip->rdb_filesysstore = fsmem;
    uip->rdb_filesyssize = i * (blocksize - 20);
    xfree (buf);
    return 2;
error:
    xfree (buf);
    xfree (fsmem);
    return err;
}

static void addfakefilesys (uaecptr parmpacket, uae_u32 dostype)
{
    int i;

    for (i = 0; i < 140; i++)
	put_byte (parmpacket + PP_FSHDSTART + i, 0);
    put_long (parmpacket + 80, dostype);
    put_long (parmpacket + PP_FSHDSTART, dostype);
    put_long (parmpacket + PP_FSHDSTART + 8, 0x100 | (dostype == 0x444f5300 ? 0x0 : 0x80));
    put_long (parmpacket + PP_FSHDSTART + 44, 0xffffffff);
}

static int dofakefilesys (UnitInfo *uip, uaecptr parmpacket)
{
    int i, size;
	TCHAR tmp[MAX_DPATH];
	uae_u8 buf[512];
    struct zfile *zf;
    uae_u32 dostype, fsres, fsnode;

	memset (buf, 0, 4);
	hdf_read (&uip->hf, buf, 0, 512);
	dostype = (buf[0] << 24) | (buf[1] << 16) |(buf[2] << 8) | buf[3];
    if (dostype == 0)
		return FILESYS_HARDFILE;
    fsres = get_long (parmpacket + PP_FSRES);
    fsnode = get_long (fsres + 18);
    while (get_long (fsnode)) {
		if (get_long (fsnode + 14) == dostype) {
			if (kickstart_version < 36) {
				addfakefilesys (parmpacket, dostype);
			} else if ((dostype & 0xffffff00) != 0x444f5300) {
				addfakefilesys (parmpacket, dostype);
			}
			return FILESYS_HARDFILE;
		}
		fsnode = get_long (fsnode);
    }

    tmp[0] = 0;
	if (uip->filesysdir && _tcslen (uip->filesysdir) > 0) {
		_tcscpy (tmp, uip->filesysdir);
    } else if ((dostype & 0xffffff00) == 0x444f5300) {
		_tcscpy (tmp, currprefs.romfile);
		i = _tcslen (tmp);
		while (i > 0 && tmp[i - 1] != '/' && tmp[i - 1] != '\\')
		    i--;
		_tcscpy (tmp + i, "FastFileSystem");
    }
    if (tmp[0] == 0) {
		write_log ("RDB: no filesystem for dostype 0x%08X\n", dostype);
		if ((dostype & 0xffffff00) == 0x444f5300)
			return FILESYS_HARDFILE;
		write_log ("RDB: mounted without filesys\n");
		return FILESYS_HARDFILE;
    }
	write_log ("RDB: fakefilesys, trying to load '%s', dostype 0x%08X\n", tmp, dostype);
	zf = zfile_fopen (tmp, "rb", ZFD_NORMAL);
    if (!zf) {
		write_log ("RDB: filesys not found\n");
		if ((dostype & 0xffffff00) == 0x444f5300)
			return FILESYS_HARDFILE;
		write_log ("RDB: mounted without filesys\n");
		return FILESYS_HARDFILE;
    }

    zfile_fseek (zf, 0, SEEK_END);
    size = zfile_ftell (zf);
	if (size > 0) {
	    zfile_fseek (zf, 0, SEEK_SET);
		uip->rdb_filesysstore = xmalloc (uae_u8, size);
	    zfile_fread (uip->rdb_filesysstore, size, 1, zf);
	}
    zfile_fclose (zf);
    uip->rdb_filesyssize = size;
    put_long (parmpacket + PP_FSSIZE, uip->rdb_filesyssize);
    addfakefilesys (parmpacket, dostype);
	write_log ("HDF: faked RDB filesystem %08X loaded\n", dostype);
	return FILESYS_HARDFILE;
}

static void get_new_device (int type, uaecptr parmpacket, TCHAR **devname, uaecptr *devname_amiga, int unit_no)
{
	TCHAR buffer[80];
	uaecptr expbase = get_long (parmpacket + PP_EXPLIB);

	if (*devname == 0 || _tcslen (*devname) == 0) {
		int un = unit_no;
		for (;;) {
			_stprintf (buffer, "DH%d", un++);
			if (!device_isdup (expbase, buffer))
				break;
		}
	} else {
		_tcscpy (buffer, *devname);
	}
	*devname_amiga = ds (device_dupfix (expbase, buffer));
    if (type == FILESYS_VIRTUAL)
	write_log ("FS: mounted virtual unit %s (%s)\n", buffer, current_mountinfo.ui[unit_no].rootdir);
    else
		write_log ("FS: mounted HDF unit %s (%04x-%08x, %s)\n", buffer,
		(uae_u32)(current_mountinfo.ui[unit_no].hf.virtsize >> 32),
		(uae_u32)(current_mountinfo.ui[unit_no].hf.virtsize),
	    current_mountinfo.ui[unit_no].rootdir);
}

/* Fill in per-unit fields of a parampacket */
static uae_u32 REGPARAM2 filesys_dev_storeinfo (TrapContext *context)
{
    UnitInfo *uip = current_mountinfo.ui;
    uae_u32 no = m68k_dreg (&context->regs, 6) & 0x7fffffff;
    int unit_no = no & 65535;
    int sub_no = no >> 16;
    int type = is_hardfile (&current_mountinfo, unit_no);
    uaecptr parmpacket = m68k_areg (&context->regs, 0);

    if (type == FILESYS_HARDFILE_RDB || type == FILESYS_HARDDRIVE) {
		/* RDB hardfile */
		uip[unit_no].devno = unit_no;
		return rdb_mount (&uip[unit_no], unit_no, sub_no, parmpacket);
    }
    if (sub_no)
		return -2;
	write_log ("Mounting uaehf.device %d (%d):\n", unit_no, sub_no);
    get_new_device (type, parmpacket, &uip[unit_no].devname, &uip[unit_no].devname_amiga, unit_no);
    uip[unit_no].devno = unit_no;
    put_long (parmpacket, uip[unit_no].devname_amiga);
    put_long (parmpacket + 4, type != FILESYS_VIRTUAL ? ROM_hardfile_resname : fsdevname);
    put_long (parmpacket + 8, uip[unit_no].devno);
    put_long (parmpacket + 12, 0); /* Device flags */
    put_long (parmpacket + 16, 16); /* Env. size */
    put_long (parmpacket + 20, uip[unit_no].hf.blocksize >> 2); /* longwords per block */
    put_long (parmpacket + 24, 0); /* unused */
    put_long (parmpacket + 28, uip[unit_no].hf.surfaces); /* heads */
    put_long (parmpacket + 32, 1); /* sectors per block */
    put_long (parmpacket + 36, uip[unit_no].hf.secspertrack); /* sectors per track */
    put_long (parmpacket + 40, uip[unit_no].hf.reservedblocks); /* reserved blocks */
    put_long (parmpacket + 44, 0); /* unused */
    put_long (parmpacket + 48, 0); /* interleave */
    put_long (parmpacket + 52, 0); /* lowCyl */
	put_long (parmpacket + 56, uip[unit_no].hf.nrcyls <= 0 ? 0 : uip[unit_no].hf.nrcyls - 1); /* hiCyl */
    put_long (parmpacket + 60, 50); /* Number of buffers */
    put_long (parmpacket + 64, 0); /* Buffer mem type */
    put_long (parmpacket + 68, 0x7FFFFFFF); /* largest transfer */
    put_long (parmpacket + 72, ~1); /* addMask (?) */
    put_long (parmpacket + 76, uip[unit_no].bootpri); /* bootPri */
    put_long (parmpacket + 80, 0x444f5300); /* DOS\0 */
    if (type == FILESYS_HARDFILE)
		type = dofakefilesys (&uip[unit_no], parmpacket);
	if (uip[unit_no].bootpri < -127)
		m68k_dreg (&context->regs, 7) = m68k_dreg (&context->regs, 7) & ~1; /* do not boot */
	if (uip[unit_no].bootpri < -128)
		return -1; /* do not mount */
    return type;
}

static uae_u32 REGPARAM2 mousehack_done (TrapContext *context)
{
	int mode = m68k_dreg (&context->regs, 1);
	if (mode < 10) {
		uaecptr diminfo = m68k_areg (&context->regs, 2);
		uaecptr dispinfo = m68k_areg (&context->regs, 3);
		uaecptr vp = m68k_areg (&context->regs, 4);
		return input_mousehack_status (mode, diminfo, dispinfo, vp, m68k_dreg (&context->regs, 2));
	} else if (mode == 10) {
		;//amiga_clipboard_die ();
	} else if (mode == 11) {
		;//amiga_clipboard_got_data (m68k_areg (regs, 2), m68k_dreg (regs, 2), m68k_dreg (regs, 0) + 8);
	} else if (mode == 12) {
		;//amiga_clipboard_want_data ();
	} else if (mode == 13) {
		return amiga_clipboard_proc_start ();
	} else if (mode == 14) {
		;//amiga_clipboard_task_start (m68k_dreg (regs, 0));
	} else if (mode == 15) {
		;//amiga_clipboard_init ();
	} else if (mode == 16) {
		uaecptr a2 = m68k_areg (&context->regs, 2);
		input_mousehack_mouseoffset (a2);
	} else if (mode == 100) {
		return consolehook_activate () ? 1 : 0;
	} else if (mode == 101) {
		consolehook_ret (m68k_areg (&context->regs, 1), m68k_areg (&context->regs, 2));
	} else if (mode == 102) {
		uaecptr ret = consolehook_beginio (m68k_areg (&context->regs, 1));
		put_long (m68k_areg (&context->regs, 7) + 4 * 4, ret);
	} else {
		write_log ("Unknown mousehack hook %d\n", mode);
	}
	return 1;
}

void filesys_install (void)
{
    uaecptr loop;

	TRACE (("Installing filesystem\n"));

	uae_sem_init (&singlethread_int_sem, 0, 1);
	uae_sem_init (&test_sem, 0, 1);

	ROM_filesys_resname = ds("UAEunixfs.resource");
	ROM_filesys_resid = ds("UAE unixfs 0.4");

	fsdevname = ds ("uae.device"); /* does not really exist */

    ROM_filesys_diagentry = here();
	calltrap (deftrap2 (filesys_diagentry, 0, "filesys_diagentry"));
    dw(0x4ED0); /* JMP (a0) - jump to code that inits Residents */

    loop = here ();

	org (rtarea_base + 0xFF18);
	calltrap (deftrap2 (filesys_dev_bootfilesys, 0, "filesys_dev_bootfilesys"));
    dw (RTS);

    /* Special trap for the assembly make_dev routine */
	org (rtarea_base + 0xFF20);
	calltrap (deftrap2 (filesys_dev_remember, 0, "filesys_dev_remember"));
    dw (RTS);

	org (rtarea_base + 0xFF28);
	calltrap (deftrap2 (filesys_dev_storeinfo, 0, "filesys_dev_storeinfo"));
    dw (RTS);

	org (rtarea_base + 0xFF30);
	calltrap (deftrap2 (filesys_handler, 0, "filesys_handler"));
    dw (RTS);

	org (rtarea_base + 0xFF38);
	calltrap (deftrap2 (mousehack_done, 0, "mousehack_done"));
    dw (RTS);

	org (rtarea_base + 0xFF40);
	calltrap (deftrap2 (startup_handler, 0, "startup_handler"));
    dw (RTS);

	org (rtarea_base + 0xFF48);
	calltrap (deftrap2 (filesys_init_storeinfo, TRAPFLAG_EXTRA_STACK, "filesys_init_storeinfo"));
	dw (RTS);

	org (rtarea_base + 0xFF50);
	calltrap (deftrap2 (exter_int_helper, 0, "exter_int_helper"));
	dw (RTS);

	org (rtarea_base + 0xFF58);
	calltrap (deftrap2 (exall_helper, 0, "exall_helper"));
	dw (RTS);

    org (loop);
}

void filesys_install_code (void)
{
	uae_u32 a, b;

	bootrom_header = 3 * 4;
    align(4);
	a = here ();
#include "filesys_bootrom.c"

	bootrom_items = dlg (a + 8);
    /* The last offset comes from the code itself, look for it near the top. */
	EXPANSION_bootcode = a + bootrom_header + bootrom_items * 4 - 4;
	b = a + bootrom_header + 3 * 4 - 4;
	filesys_initcode = a + dlg (b) + bootrom_header - 4;
}

#ifdef SAVESTATE

static uae_u8 *restore_filesys_hardfile (UnitInfo *ui, const uae_u8 *src)
{
	struct hardfiledata *hfd = &ui->hf;
	const TCHAR *s;

	hfd->virtsize = restore_u64();
	hfd->offset = restore_u64();
	hfd->nrcyls = restore_u32();
	hfd->secspertrack = restore_u32();
	hfd->surfaces = restore_u32();
	hfd->reservedblocks = restore_u32();
	hfd->blocksize = restore_u32();
	hfd->readonly = restore_u32();
	hfd->flags = restore_u32();
	hfd->cylinders = restore_u32();
	hfd->sectors = restore_u32();
	hfd->heads = restore_u32();
	s = restore_string();
	_tcscpy (hfd->vendor_id, s);
	xfree(s);
	s = restore_string();
	_tcscpy (hfd->product_id, s);
	xfree(s);
	s = restore_string();
	_tcscpy (hfd->product_rev, s);
	xfree(s);
	s = restore_string();
	_tcscpy (hfd->device_name, s);
	xfree(s);
	return src;
}

static uae_u8 *save_filesys_hardfile (UnitInfo *ui, uae_u8 *dst)
{
	struct hardfiledata *hfd = &ui->hf;

	save_u64 (hfd->virtsize);
	save_u64 (hfd->offset);
	save_u32 (hfd->nrcyls);
	save_u32 (hfd->secspertrack);
	save_u32 (hfd->surfaces);
	save_u32 (hfd->reservedblocks);
	save_u32 (hfd->blocksize);
	save_u32 (hfd->readonly);
	save_u32 (hfd->flags);
	save_u32 (hfd->cylinders);
	save_u32 (hfd->sectors);
	save_u32 (hfd->heads);
	save_string (hfd->vendor_id);
	save_string (hfd->product_id);
	save_string (hfd->product_rev);
	save_string (hfd->device_name);
	return dst;
}

static a_inode *restore_filesys_get_base (Unit *u, TCHAR *npath)
{
	const TCHAR *path, *p = 0, *p2 = 0;
	a_inode *a;
	int cnt, err, i;

	/* no '/' = parent is root */
	if (!_tcschr (npath, '/'))
		return &u->rootnode;

	/* iterate from root to last to previous path part,
	* create ainos if not already created.
	*/
	path = xcalloc(TCHAR, _tcslen (npath) + 2);
	cnt = 1;
	for (;;) {
		_tcscpy (path, npath);
		_tcscat (path, "/");
		p = path;
		for (i = 0; i < cnt ;i++) {
			if (i > 0)
				p++;
			while (*p != '/' && *p != 0)
				p++;
		}
		if (*p) {
			//*p = 0;
			err = 0;
			get_aino (u, &u->rootnode, path, &err);
			if (err) {
				write_log ("*** FS: missing path '%s'!\n", path);
				return NULL;
			}
			cnt++;
		} else {
			break;
		}
	}

	/* find base (parent) of last path part */
	_tcscpy (path, npath);
	p = path;
	a = u->rootnode.child;
	for (;;) {
		if (*p == 0) {
			write_log ("*** FS: base aino NOT found '%s' ('%s')\n", a->nname, npath);
			xfree (path);
			return NULL;
		}
		p2 = p;
		while(*p2 != '/' && *p2 != '\\' && *p2 != 0)
			p2++;
		//*p2 = 0;
		while (a) {
			if (!same_aname(p, a->aname)) {
				a = a->sibling;
				continue;
			}
			p = p2 + 1;
			if (*p == 0) {
				write_log ("FS: base aino found '%s' ('%s')\n", a->nname, npath);
				xfree (path);
				return a;
			}
			a = a->child;
			break;
		}
		if (!a) {
			write_log ("*** FS: path part '%s' not found ('%s')\n", p, npath);
			xfree (path);
			return NULL;
		}
	}
}

static TCHAR *makenativepath (UnitInfo *ui, TCHAR *apath)
{
	uae_u32 i = 0;
	TCHAR *pn;
	/* create native path. FIXME: handle 'illegal' characters */
	pn = xcalloc (TCHAR, _tcslen (apath) + 1 + _tcslen (ui->rootdir) + 1);
	_stprintf (pn, "%s/%s", ui->rootdir, apath);
	if (FSDB_DIR_SEPARATOR != '/') {
		for ( ; i < _tcslen (pn); i++) {
			if (pn[i] == '/')
				pn[i] = FSDB_DIR_SEPARATOR;
		}
	}
	return pn;
}

static uae_u8 *restore_aino (UnitInfo *ui, Unit *u, const uae_u8 *src)
{
	TCHAR *p, *p2, *pn;
	uae_u32 flags;
	int missing;
	a_inode *base, *a;

	missing = 0;
	a = xcalloc (a_inode, 1);
	a->uniq = restore_u64 ();
	a->locked_children = restore_u32 ();
	a->exnext_count = restore_u32 ();
	a->shlock = restore_u32 ();
	flags = restore_u32 ();
	if (flags & 1)
		a->elock = 1;
	/* full Amiga-side path without drive, eg. "C/SetPatch" */
	p = restore_string ();
	/* root (p = volume label) */
	if (a->uniq == 0) {
		a->nname = my_strdup (ui->rootdir);
		a->aname = p;
		a->dir = 1;
		if (ui->volflags < 0) {
			write_log ("FS: Volume '%s' ('%s') missing!\n", a->aname, a->nname);
		} else {
			a->volflags = ui->volflags;
			recycle_aino (u, a);
			write_log ("FS: Lock (root) '%s' ('%s')\n", a->aname, a->nname);
		}
		return src;
	}
	p2 = _tcsrchr(p, '/');
	if (p2)
		p2++;
	else
		p2 = p;
	pn = makenativepath (ui, p);
	a->nname = pn;
	a->aname = my_strdup (p2);
	/* find parent of a->aname (Already restored previously. I hope..) */
	if (p2 != p)
		p2[-1] = 0;
	base = restore_filesys_get_base (u, p);
	xfree(p);
	if (flags & 2) {
		a->dir = 1;
		if (!my_existsdir(a->nname))
			write_log ("*** FS: Directory '%s' missing!\n", a->nname);
		else
			fsdb_clean_dir (a);
	} else {
		if (!my_existsfile(a->nname))
			write_log ("*** FS: File '%s' missing!\n", a->nname);
	}
	if (base) {
		fill_file_attrs (u, base, a);
		init_child_aino_tree (u, base, a);
	} else {
		write_log ("*** FS: parent directory missing '%s' ('%s')\n", a->aname, a->nname);
		missing = 1;
	}
	if (missing) {
		write_log ("*** FS: Lock restore failed '%s' ('%s')\n", a->aname, a->nname);
		xfree (a->nname);
		xfree (a->aname);
		xfree (a);
	} else {
		write_log ("FS: Lock '%s' ('%s')\n", a->aname, a->nname);
		recycle_aino (u, a);
	}
	return src;
}

static uae_u8 *restore_key (UnitInfo *ui, Unit *u, const uae_u8 *src)
{
	int savedsize, uniq;
	const TCHAR *p, *pn;
	mode_t openmode;
	int err;
	int missing;
	a_inode *a;
	Key *k;

	missing = 0;
	k = xcalloc (Key, 1);
	k->uniq = restore_u64 ();
	k->file_pos = restore_u32 ();
	k->createmode = restore_u32 ();
	k->dosmode = restore_u32 ();
	savedsize = restore_u32 ();
	uniq = restore_u64 ();
	p = restore_string ();
	restore_u64 ();
	restore_u64 ();
	pn = makenativepath (ui, p);
	openmode = ((k->dosmode & A_FIBF_READ) == 0 ? O_WRONLY
		: (k->dosmode & A_FIBF_WRITE) == 0 ? O_RDONLY
		: O_RDWR);
	write_log ("FS: open file '%s' ('%s'), pos=%d\n", p, pn, k->file_pos);
	a = get_aino (u, &u->rootnode, p, &err);
	if (!a)
		write_log ("*** FS: Open file aino creation failed '%s'\n", p);
	missing = 1;
	if (a) {
		missing = 0;
		k->aino = a;
		if (a->uniq != uniq)
			write_log ("*** FS: Open file '%s' aino id %d != %d\n", p, uniq, a->uniq);
		if (!my_existsfile(pn)) {
			write_log ("*** FS: Open file '%s' is missing, creating dummy file!\n", p);
			k->fd = fs_open (u, pn, openmode | O_CREAT |O_BINARY);
			if (k->fd) {
				uae_u8 *buf = xcalloc (uae_u8, 10000);
				int sp = savedsize;
				while (sp) {
					int s = sp >= 10000 ? 10000 : sp;
					fs_write (k->fd, buf, s);
					sp -= s;
				}
				xfree(buf);
				write_log ("*** FS: dummy file created\n");
			} else {
				write_log ("*** FS: Open file '%s', couldn't create dummy file!\n", p);
			}
		} else {
			k->fd = fs_open (u, pn, openmode | O_BINARY);
		}
		if (!k->fd) {
			write_log ("*** FS: Open file '%s' failed to open!\n", p);
			missing = 1;
		} else {
			uae_u64 s;
			s = fs_lseek64 (k->fd, 0, SEEK_END);
			if (s != savedsize)
				write_log ("FS: restored file '%s' size changed! orig=%d, now=%d!!\n", p, savedsize, s);
			if (k->file_pos > s) {
				write_log ("FS: restored filepos larger than size of file '%s'!! %d > %d\n", p, k->file_pos, s);
				k->file_pos = s;
			}
			fs_lseek64 (k->fd, k->file_pos, SEEK_SET);
		}
	}
	xfree (p);
	if (missing) {
		xfree(k);
	} else {
		k->next = u->keys;
		u->keys = k;
	}
	return src;
}

static uae_u8 *restore_notify (UnitInfo *ui, Unit *u, const uae_u8 *src)
{
	Notify *n = xcalloc (Notify, 1);
	uae_u32 hash;
	const TCHAR *s;

	n->notifyrequest = restore_u32 ();
	s = restore_string ();
	n->fullname = xmalloc (TCHAR, _tcslen (ui->volname) + 2 + _tcslen (s) + 1);
	_stprintf (n->fullname, "%s:%s", ui->volname, s);
	xfree(s);
	s = _tcsrchr (n->fullname, '/');
	if (s)
		s++;
	else
		s = n->fullname;
	n->partname = my_strdup (s);
	hash = notifyhash (n->fullname);
	n->next = u->notifyhash[hash];
	u->notifyhash[hash] = n;
	write_log ("FS: notify %08X '%s' '%s'\n", n->notifyrequest, n->fullname, n->partname);
	return src;
}

static uae_u8 *restore_exkey (UnitInfo *ui, Unit *u, const uae_u8 *src)
{
	restore_u64 ();
	restore_u64 ();
	restore_u64 ();
	return src;
}

static uae_u8 *restore_filesys_virtual (UnitInfo *ui, const uae_u8 *src, int num)
{
	Unit *u = startup_create_unit (ui, num);
	int cnt;

	u->dosbase = restore_u32 ();
	u->volume = restore_u32 ();
	u->port = restore_u32 ();
	u->locklist = restore_u32 ();
	u->dummy_message = restore_u32 ();
	u->cmds_sent = restore_u64 ();
	u->cmds_complete = restore_u64 ();
	u->cmds_acked = restore_u64 ();
	u->next_exkey = restore_u32 ();
	u->total_locked_ainos = restore_u32 ();
	u->volflags = ui->volflags;

	cnt = restore_u32 ();
	write_log ("FS: restoring %d locks\n", cnt);
	while (cnt-- > 0)
		src = restore_aino(ui, u, src);

	cnt = restore_u32 ();
	write_log ("FS: restoring %d open files\n", cnt);
	while (cnt-- > 0)
		src = restore_key(ui, u, src);

	cnt = restore_u32 ();
	write_log ("FS: restoring %d notifications\n", cnt);
	while (cnt-- > 0)
		src = restore_notify (ui, u, src);

	cnt = restore_u32 ();
	write_log ("FS: restoring %d exkeys\n", cnt);
	while (cnt-- > 0)
		src = restore_exkey (ui, u, src);

	return src;
}

static TCHAR *getfullaname(a_inode *a)
{
	TCHAR *p;
	int first = 1;

	p = xcalloc (TCHAR, 2000);
	while (a) {
		int len = _tcslen (a->aname);
		memmove (p + len + 1, p, (_tcslen (p) + 1) * sizeof (TCHAR));
		memcpy (p, a->aname, _tcslen (a->aname) * sizeof (TCHAR));
		if (!first)
			p[len] = '/';
		first = 0;
		a = a->parent;
		if (a && a->uniq == 0)
			return p;
	}
	return p;
}

/* scan and save all Lock()'d files */
static int recurse_aino (UnitInfo *ui, a_inode *a, int cnt, uae_u8 **dstp)
{
	uae_u8 *dst = NULL;
	int dirty = 0;
	a_inode *a2 = a;

	if (dstp)
		dst = *dstp;
	while (a) {
		//write_log("recurse '%s' '%s' %d %08x\n", a->aname, a->nname, a->uniq, a->parent);
		if (a->elock || a->shlock || a->uniq == 0) {
			if (dst) {
				TCHAR *fn;
				write_log ("%04x s=%d e=%d d=%d '%s' '%s'\n", a->uniq, a->shlock, a->elock, a->dir, a->aname, a->nname);
				fn = getfullaname(a);
				write_log ("->'%s'\n", fn);
				save_u64 (a->uniq);
				save_u32 (a->locked_children);
				save_u32 (a->exnext_count);
				save_u32 (a->shlock);
				save_u32 ((a->elock ? 1 : 0) | (a->dir ? 2 : 0));
				save_string (fn);
				xfree(fn);
			}
			cnt++;
		}
		if (a->dirty)
			dirty = 1;
		if (a->child)
			cnt = recurse_aino (ui, a->child, cnt, &dst);
		a = a->sibling;
	}
	if (dirty && a2->parent)
		fsdb_dir_writeback (a2->parent);
	if (dst)
		*dstp = dst;
	return cnt;
}

static uae_u8 *save_key (uae_u8 *dst, Key *k)
{
	TCHAR *fn = getfullaname (k->aino);
	uae_u64 size;
	save_u64 (k->uniq);
	save_u32 ((uae_u32)k->file_pos);
	save_u32 (k->createmode);
	save_u32 (k->dosmode);
	size = fs_lseek (k->fd, 0, SEEK_END);
	save_u32 ((uae_u32)size);
	save_u64 (k->aino->uniq);
	fs_lseek (k->fd, k->file_pos, SEEK_SET);
	save_string (fn);
	save_u64 (k->file_pos);
	save_u64 (size);
	write_log ("'%s' uniq=%d size=%d seekpos=%d mode=%d dosmode=%d\n",
		fn, k->uniq, size, k->file_pos, k->createmode, k->dosmode);
	xfree (fn);
	return dst;
}

static uae_u8 *save_notify (UnitInfo *ui, uae_u8 *dst, Notify *n)
{
	TCHAR *s;
	save_u32 (n->notifyrequest);
	s = n->fullname;
	if (_tcslen (s) >= _tcslen (ui->volname) && !_tcsncmp (n->fullname, ui->volname, _tcslen (ui->volname)))
		s = n->fullname + _tcslen (ui->volname) + 1;
	save_string (s);
	write_log ("FS: notify %08X '%s'\n", n->notifyrequest, n->fullname);
	return dst;
}

static uae_u8 *save_exkey (uae_u8 *dst, ExamineKey *ek)
{
	save_u64 (ek->uniq);
	save_u64 (ek->aino->uniq);
	save_u64 (ek->curr_file->uniq);
	return dst;
}

static uae_u8 *save_filesys_virtual (UnitInfo *ui, uae_u8 *dst)
{
	Unit *u = ui->self;
	Key *k;
	int cnt, i, j;

	write_log ("FSSAVE: '%s'\n", ui->devname);
	save_u32 (u->dosbase);
	save_u32 (u->volume);
	save_u32 (u->port);
	save_u32 (u->locklist);
	save_u32 (u->dummy_message);
	save_u64 (u->cmds_sent);
	save_u64 (u->cmds_complete);
	save_u64 (u->cmds_acked);
	save_u32 (u->next_exkey);
	save_u32 (u->total_locked_ainos);
	cnt = recurse_aino (ui, &u->rootnode, 0, NULL);
	save_u32 (cnt);
	write_log ("%d open locks\n", cnt);
	cnt = recurse_aino (ui, &u->rootnode, 0, &dst);
	cnt = 0;
	for (k = u->keys; k; k = k->next)
		cnt++;
	save_u32 (cnt);
	write_log ("%d open files\n", cnt);
	for (k = u->keys; k; k = k->next)
		dst = save_key (dst, k);
	for (j = 0; j < 2; j++) {
		cnt = 0;
		for (i = 0; i < NOTIFY_HASH_SIZE; i++) {
			Notify *n = u->notifyhash[i];
			while (n) {
				if (j > 0)
					dst = save_notify (ui, dst, n);
				cnt++;
				n = n->next;
			}
		}
		if (j == 0) {
			save_u32 (cnt);
			write_log ("%d notify requests\n", cnt);
		}
	}
	for (j = 0; j < 2; j++) {
		cnt = 0;
		for (i = 0; i < EXKEYS; i++) {
			ExamineKey *ek = &u->examine_keys[i];
			if (ek->uniq) {
				cnt++;
				if (j > 0)
					dst = save_exkey (dst, ek);
			}
		}
		if (j == 0) {
			save_u32 (cnt);
			write_log ("%d exkeys\n", cnt);
		}
	}
	write_log ("END\n");
	return dst;
}

uae_u8 *save_filesys_common (uae_u32 *len)
{
	uae_u8 *dstbak, *dst;
	if (nr_units (currprefs.mountinfo) == 0)
		return NULL;
	dstbak = dst = xmalloc (uae_u8, 1000);
	save_u32 (2);
	save_u64 (a_uniq);
	save_u64 (key_uniq);
	*len = dst - dstbak;
	return dstbak;
}

uae_u8 *restore_filesys_common (const uae_u8 *src)
{
	if (restore_u32 () != 2)
		return src;
	filesys_prepare_reset2 ();
	filesys_reset2 ();
	a_uniq = restore_u64 ();
	key_uniq = restore_u64 ();
	return src;
}

uae_u8 *save_filesys (int num, uae_u32 *len)
{
	uae_u8 *dstbak, *dst;
	UnitInfo *ui;
	int type = is_hardfile (&current_mountinfo, num);

	ui = &current_mountinfo.ui[num];
	if (!ui->open)
		return NULL;
	/* not initialized yet, do not save */
	if (type == FILESYS_VIRTUAL && (ui->self == NULL || ui->volname == NULL))
		return NULL;
	write_log ("FS_FILESYS: '%s' '%s'\n", ui->devname, ui->volname);
	dstbak = dst = xmalloc (uae_u8, 100000);
	save_u32 (2); /* version */
	save_u32 (ui->devno);
	save_u16 (type);
	save_string (ui->rootdir);
	save_string (ui->devname);
	save_string (ui->volname);
	save_string (ui->filesysdir);
	save_u8 (ui->bootpri);
	save_u8 (ui->readonly);
	save_u32 (ui->startup);
	save_u32 (filesys_configdev);
	if (type == FILESYS_VIRTUAL)
		dst = save_filesys_virtual (ui, dst);
	if (type == FILESYS_HARDFILE || type == FILESYS_HARDFILE_RDB)
		dst = save_filesys_hardfile (ui, dst);
	*len = dst - dstbak;
	return dstbak;
}

uae_u8 *restore_filesys (struct uaedev_mount_info *mountinfo, const uae_u8 *src)
{
	int type, devno;
	UnitInfo *ui;
	const TCHAR *devname = 0, *volname = 0, *rootdir = 0, *filesysdir = 0;
	int bootpri, readonly;

	if (restore_u32 () != 2)
		return src;
	devno = restore_u32 ();
	type = restore_u16 ();
	rootdir = restore_string ();
	devname = restore_string ();
	volname = restore_string ();
	filesysdir = restore_string ();
	bootpri = restore_u8 ();
	readonly = restore_u8 ();
	ui = &mountinfo->ui[devno];
	ui->startup = restore_u32 ();
	filesys_configdev = restore_u32 ();
	if (type == FILESYS_HARDFILE || type == FILESYS_HARDFILE_RDB) {
		src = restore_filesys_hardfile (ui, src);
		xfree (volname);
		volname = NULL;
	}
	if (set_filesys_unit (mountinfo, devno, devname, volname, rootdir, readonly,
		ui->hf.secspertrack, ui->hf.surfaces, ui->hf.reservedblocks, ui->hf.blocksize,
		bootpri, 0, 1, filesysdir[0] ? filesysdir : NULL, 0, 0) < 0) {
			write_log ("filesys '%s' failed to restore\n", rootdir);
			goto end;
	}
	if (type == FILESYS_VIRTUAL)
		src = restore_filesys_virtual (ui, src, devno);
	write_log ("'%s' restored\n", rootdir);
end:
	xfree (rootdir);
	xfree (devname);
	xfree (volname);
	xfree (filesysdir);
	return src;
}

int save_filesys_cando (void)
{
	if (nr_units (currprefs.mountinfo) == 0)
		return -1;
	return filesys_in_interrupt ? 0 : 1;
}

#endif