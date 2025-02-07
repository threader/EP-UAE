 /*
  * UAE - The Un*x Amiga Emulator
  *
  * routines to handle compressed file automatically
  *
  * (c) 1996 Samuel Devulder, Tim Gunn
*     2002-2007 Toni Wilen
  */

#define ZLIB_WINAPI
#define RECURSIVE_ARCHIVES 1
//#define ZFILE_DEBUG

#include "sysconfig.h"
#include "sysdeps.h"

#include "options.h"
#include "zfile.h"
#include "unzip.h"
#include "disk.h"
#include "gui.h"
#include "crc32.h"
#include "fsdb.h"
#include "fsusage.h"
#include "zarchive.h"
#include "diskutil.h"
#include "fdi2raw.h"

#include "unzip.h"
#include "dms/cdata.h"
#include "dms/pfile.h"
#include <zlib.h>

static struct zfile *zlist = 0;

const TCHAR *uae_archive_extensions[] = { "zip", "rar", "7z", "lha", "lzh", "lzx", "tar", NULL };

#define MAX_CACHE_ENTRIES 10

struct zdisktrack
{
	void *data;
	int len;
};
struct zdiskimage
{
	int tracks;
	struct zdisktrack zdisktracks[2 * 84];
};
struct zcache
{
	TCHAR *name;
	struct zdiskimage *zd;
	void *data;
	int size;
	struct zcache *next;
	time_t tm;
};
static struct zcache *zcachedata;

static struct zcache *cache_get (const TCHAR *name)
{
	struct zcache *zc = zcachedata;
	while (zc) {
		if (!_tcscmp (name, zc->name)) {
			zc->tm = time (NULL);
			return zc;
		}
		zc = zc->next;
	}
	return NULL;
}

static void zcache_flush (void)
{
}

static void zcache_free_data (struct zcache *zc)
{
	int i;
	if (zc->zd) {
		for (i = 0; i < zc->zd->tracks; i++) {
			xfree (zc->zd->zdisktracks[i].data);
		}
		xfree (zc->zd);
	}
	xfree (zc->data);
	xfree (zc->name);
}

static void zcache_free (struct zcache *zc)
{
	struct zcache *pl = NULL;
	struct zcache *l  = zcachedata;
	struct zcache *nxt;

	while (l != zc) {
		if (l == 0)
			return;
		pl = l;
		l = l->next;
	}
	if (l)
		nxt = l->next;
	zcache_free_data (zc);
	if (l == 0)
		return;
	if(!pl)
		zcachedata = nxt;
	else
		pl->next = nxt;
}

static void zcache_close (void)
{
	struct zcache *zc = zcachedata;
	while (zc) {
		struct zcache *n = zc->next;
		zcache_free_data (zc);
		xfree (n);
		zc = n;
	}
}

static void zcache_check (void)
{
	int cnt = 0;
	struct zcache *zc = zcachedata, *last = NULL;
	while (zc) {
		last = zc;
		zc = zc->next;
		cnt++;
	}
	write_log ("CACHE: %d\n", cnt);
	if (cnt >= MAX_CACHE_ENTRIES && last)
		zcache_free (last);
}

static struct zcache *zcache_put (const TCHAR *name, struct zdiskimage *data)
{
	struct zcache *zc;
	
	zcache_check ();
	zc = xcalloc (struct zcache, 1);
	zc->next = zcachedata;
	zcachedata = zc;
	zc->zd = data;
	zc->name = my_strdup (name);
	zc->tm = time (NULL);
	return zc;
}

static void checkarchiveparent (struct zfile *z)
{
	// unpack completely if opened in PEEK mode
	if (z->archiveparent)
		archive_unpackzfile (z);
}

static struct zfile *zfile_create (struct zfile *prev)
{
    struct zfile *z;

	z = xmalloc (struct zfile, 1);
    if (!z)
		return 0;
    memset (z, 0, sizeof *z);
    z->next = zlist;
    zlist = z;
	z->opencnt = 1;
	if (prev) {
		z->zfdmask = prev->zfdmask;
	}
    return z;
}

static void zfile_free (struct zfile *f)
{
    if (f->f)
		fclose (f->f);
    if (f->deleteafterclose) {
		unlink (f->name);
		write_log ("deleted temporary file '%s'\n", f->name);
    }
	xfree (f->name);
	xfree (f->data);
	xfree (f->mode);
	xfree (f->userdata);
	xfree (f);
}

void zfile_exit (void)
{
    struct zfile *l;
    while ((l = zlist)) {
		zlist = l->next;
		zfile_free (l);
    }
}

void zfile_fclose (struct zfile *f)
{
    struct zfile *pl = NULL;
    struct zfile *l  = zlist;
    struct zfile *nxt;

	//write_log ("%p\n", f);
    if (!f)
	return;
	if (f->opencnt < 0) {
		write_log ("zfile: tried to free already closed filehandle!\n");
		return;
	}
	f->opencnt--;
	if (f->opencnt > 0)
		return;
	f->opencnt = -100;
	if (f->parent) {
		f->parent->opencnt--;
		if (f->parent->opencnt <= 0)
			zfile_fclose (f->parent);
	}
    while (l != f) {
		if (l == 0) {
			write_log ("zfile: tried to free already freed or nonexisting filehandle!\n");
		    return;
		}
		pl = l;
		l = l->next;
    }
	if (l)
		nxt = l->next;
    zfile_free (f);
    if (l == 0)
		return;
    if(!pl)
		zlist = nxt;
    else
	pl->next = nxt;
}

static void removeext (TCHAR *s, TCHAR *ext)
{
	if (_tcslen (s) < _tcslen (ext))
		return;
	if (_tcsicmp (s + _tcslen (s) - _tcslen (ext), ext) == 0)
		s[_tcslen (s) - _tcslen (ext)] = 0;
}

static uae_u8 exeheader[]={0x00,0x00,0x03,0xf3,0x00,0x00,0x00,0x00};
static TCHAR *diskimages[] = { "adf", "adz", "ipf", "fdi", "dms", "wrp", "dsq", 0 };

int zfile_gettype (struct zfile *z)
{
    uae_u8 buf[8];
	TCHAR *ext;

	if (!z || !z->name)
		return ZFILE_UNKNOWN;
	ext = _tcsrchr (z->name, '.');
    if (ext != NULL) {
	int i;
	ext++;
		for (i = 0; diskimages[i]; i++) {
			if (strcasecmp (ext, diskimages[i]) == 0)
		    return ZFILE_DISKIMAGE;
		}
	if (strcasecmp (ext, "adf") == 0)
	    return ZFILE_DISKIMAGE;
	if (strcasecmp (ext, "adz") == 0)
	    return ZFILE_DISKIMAGE;
	if (strcasecmp (ext, "roz") == 0)
	    return ZFILE_ROM;
	if (strcasecmp (ext, "ipf") == 0)
	    return ZFILE_DISKIMAGE;
	if (strcasecmp (ext, "fdi") == 0)
	    return ZFILE_DISKIMAGE;
	if (strcasecmp (ext, "uss") == 0)
	    return ZFILE_STATEFILE;
	if (strcasecmp (ext, "dms") == 0)
	    return ZFILE_DISKIMAGE;
	if (strcasecmp (ext, "rom") == 0)
	    return ZFILE_ROM;
	if (strcasecmp (ext, "key") == 0)
	    return ZFILE_KEY;
	if (strcasecmp (ext, "nvr") == 0)
	    return ZFILE_NVR;
	if (strcasecmp (ext, "uae") == 0)
	    return ZFILE_CONFIGURATION;
		if (strcasecmp (ext, "cue") == 0 || strcasecmp (ext, "iso") == 0)
			return ZFILE_CDIMAGE;
    }
    memset (buf, 0, sizeof (buf));
    zfile_fread (buf, 8, 1, z);
    zfile_fseek (z, -8, SEEK_CUR);
    if (!memcmp (buf, exeheader, sizeof(buf)))
		return ZFILE_DISKIMAGE;
	if (!memcmp (buf, "RDSK", 4))
		return ZFILE_HDFRDB;
	if (!memcmp (buf, "DOS", 3)) {
		if (z->size < 4 * 1024 * 1024)
			return ZFILE_DISKIMAGE;
		else
			return ZFILE_HDF;
	}
	if (ext != NULL) {
		if (strcasecmp (ext, "hdf") == 0)
			return ZFILE_HDF;
		if (strcasecmp (ext, "hdz") == 0)
			return ZFILE_HDF;
	}
    return ZFILE_UNKNOWN;
}

#if 0
#define TMP_PREFIX "uae_"

static struct zfile *createinputfile (struct zfile *z)
{
    FILE *f;
    struct zfile *z2;
    char *name;

    z2 = zfile_create ();
    if (!z->data) {
	z2->name = strdup (z->name);
	return z2;
    }
    name = tempnam (0, TMP_PREFIX);
    f = fopen (name, "wb");
    if (!f) return 0;
    write_log ("created temporary file '%s'\n", name);
    fwrite (z->data, z->size, 1, f);
    fclose (f);
    z2->name = name;
    z2->deleteafterclose = 1;
    return z2;
}

static struct zfile *createoutputfile (struct zfile *z)
{
    struct zfile *z2;
    char *name;

    name = tempnam (0, TMP_PREFIX);
    z2 = zfile_create ();
    z2->name = name;
    z2->deleteafterclose = 1;
    write_log ("allocated temporary file name '%s'\n", name);
    return z2;
}

/* we want to delete temporary files as early as possible */
static struct zfile *updateoutputfile (struct zfile *z)
{
    struct zfile *z2 = 0;
    int size;
    FILE *f = fopen (z->name, "rb");
    for (;;) {
	if (!f)
	    break;
        fseek (f, 0, SEEK_END);
	size = ftell (f);
	fseek (f, 0, SEEK_SET);
	if (!size)
	    break;
        z2 = zfile_fopen_empty (z->name, size);
	if (!z2)
	    break;
        fread (z2->data, size, 1, f);
	fclose (f);
	zfile_fclose (z);
	return z2;
    }
    if (f)
	fclose (f);
    zfile_fclose (z);
    zfile_fclose (z2);
    return 0;
}
#endif

#define VHD_DYNAMIC 3
#define VHD_FIXED 2

STATIC_INLINE uae_u32 gl (uae_u8 *p)
{
	return (p[0] << 24) | (p[1] << 16) | (p[2] << 8) | (p[3] << 0);
}

static uae_u32 vhd_checksum (uae_u8 *p, int offset)
{
	int i;
	uae_u32 sum;

	sum = 0;
	for (i = 0; i < 512; i++) {
		if (offset >= 0 && i >= offset && i < offset + 4)
			continue;
		sum += p[i];
	}
	return ~sum;
}

struct zfile_vhd
{
	int vhd_type;
	uae_u64 virtsize;
	uae_u32 vhd_bamoffset;
	uae_u32 vhd_blocksize;
	uae_u8 *vhd_header, *vhd_sectormap;
	uae_u64 vhd_footerblock;
	uae_u32 vhd_bamsize;
	uae_u64 vhd_sectormapblock;
	uae_u32 vhd_bitmapsize;
};


static uae_u64 vhd_fread2 (struct zfile *zf, void *dataptrv, uae_u64 offset, uae_u64 len)
{
	uae_u32 bamoffset;
	uae_u32 sectoroffset;
	uae_u64 read;
	struct zfile *zp = zf->parent;
	struct zfile_vhd *zvhd = (struct zfile_vhd*)zf->userdata;
	uae_u8 *dataptr = (uae_u8*)dataptrv;

	//write_log ("%08x %08x\n", (uae_u32)offset, (uae_u32)len);
	read = 0;
	if (offset & 511)
		return read;
	if (len & 511)
		return read;
	while (len > 0) {
		bamoffset = (offset / zvhd->vhd_blocksize) * 4 + zvhd->vhd_bamoffset;
		sectoroffset = gl (zvhd->vhd_header + bamoffset);
		if (sectoroffset == 0xffffffff) {
			memset (dataptr, 0, 512);
			read += 512;
		} else {
			int bitmapoffsetbits;
			int bitmapoffsetbytes;
			uae_u64 sectormapblock;

			bitmapoffsetbits = (offset / 512) % (zvhd->vhd_blocksize / 512);
			bitmapoffsetbytes = bitmapoffsetbits / 8;
			sectormapblock = sectoroffset * 512 + (bitmapoffsetbytes & ~511);
			if (zvhd->vhd_sectormapblock != sectormapblock) {
				// read sector bitmap
				//write_log ("BM %08x\n", sectormapblock);
				zfile_fseek (zp, sectormapblock, SEEK_SET);
				if (zfile_fread (zvhd->vhd_sectormap, 1, 512, zp) != 512)
					return read;
				zvhd->vhd_sectormapblock = sectormapblock;
			}
			// block allocated in bitmap?
			if (zvhd->vhd_sectormap[bitmapoffsetbytes & 511] & (1 << (7 - (bitmapoffsetbits & 7)))) {
				// read data block
				int block = sectoroffset * 512 + zvhd->vhd_bitmapsize + bitmapoffsetbits * 512;
				//write_log ("DB %08x\n", block);
				zfile_fseek (zp, block, SEEK_SET);
				if (zfile_fread (dataptr, 1, 512, zp) != 512)
					return read;
			} else {
				memset (dataptr, 0, 512);
			}
			read += 512;
		}
		len -= 512;
		dataptr += 512;
		offset += 512;
	}
	return read;
}
static uae_s64 vhd_fread (void *data, uae_u64 l1, uae_u64 l2, struct zfile *zf)
{
	uae_u64 size = l1 * l2;
	uae_u64 out = 0;
	int len = 0;

	if ((zf->seek & 511) || (size & 511)) {
		uae_u8 tmp[512];

		if (zf->seek & 511) {
			int s;
			s = 512 - (zf->seek & 511);
			vhd_fread2 (zf, tmp, zf->seek & ~511, 512);
			memcpy ((uae_u8*)data + len, tmp + 512 - s, s);
			len += s;
			out += s;
			zf->seek += s;
		}
		while (size > 0) {
			int s = size > 512 ? 512 : size;
			vhd_fread2 (zf, tmp, zf->seek, 512);
			memcpy ((uae_u8*)data + len, tmp, s);
			zf->seek += s;
			size -= s;
			out += s;
		}
	} else {
		out = vhd_fread2 (zf, data, zf->seek, size);
		zf->seek += out;
		out /= l1;
	}
	return out;
}

static struct zfile *vhd (struct zfile *z)
{
	uae_u8 tmp[512], tmp2[512];
	uae_u32 v;
	struct zfile_vhd *zvhd;
	uae_u64 fsize;

	zvhd = xcalloc (struct zfile_vhd, 1);
	zfile_fseek (z, 0, SEEK_END);
	fsize = zfile_ftell (z);
	zfile_fseek (z, 0, SEEK_SET);
	if (zfile_fread (tmp, 1, 512, z) != 512)
		goto nonvhd;
	v = gl (tmp + 8); // features
	if ((v & 3) != 2)
		goto nonvhd;
	v = gl (tmp + 8 + 4); // version
	if ((v >> 16) != 1)
		goto nonvhd;
	zvhd->vhd_type = gl (tmp + 8 + 4 + 4 + 8 + 4 + 4 + 4 + 4 + 8 + 8 + 4);
	if (zvhd->vhd_type != VHD_FIXED && zvhd->vhd_type != VHD_DYNAMIC)
		goto nonvhd;
	v = gl (tmp + 8 + 4 + 4 + 8 + 4 + 4 + 4 + 4 + 8 + 8 + 4 + 4);
	if (v == 0)
		goto nonvhd;
	if (vhd_checksum (tmp, 8 + 4 + 4 + 8 + 4 + 4 + 4 + 4 + 8 + 8 + 4 + 4) != v)
		goto nonvhd;
	zfile_fseek (z, fsize - sizeof tmp2, SEEK_SET);
	if (zfile_fread (tmp2, 1, 512, z) != 512)
		goto end;
	if (memcmp (tmp, tmp2, sizeof tmp))
		goto nonvhd;
	zvhd->vhd_footerblock = fsize - 512;
	zvhd->virtsize = (uae_u64)(gl (tmp + 8 + 4 + 4 + 8 + 4 + 4 +4 + 4 + 8)) << 32;
	zvhd->virtsize |= gl (tmp + 8 + 4 + 4 + 8 + 4 + 4 +4 + 4 + 8 + 4);
	if (zvhd->vhd_type == VHD_DYNAMIC) {
		uae_u32 size;
		zvhd->vhd_bamoffset = gl (tmp + 8 + 4 + 4 + 4);
		if (zvhd->vhd_bamoffset == 0 || zvhd->vhd_bamoffset >= fsize)
			goto end;
		zfile_fseek (z, zvhd->vhd_bamoffset, SEEK_SET);
		if (zfile_fread (tmp, 1, 512, z) != 512)
			goto end;
		v = gl (tmp + 8 + 8 + 8 + 4 + 4 + 4);
		if (vhd_checksum (tmp, 8 + 8 + 8 + 4 + 4 + 4) != v)
			goto end;
		v = gl (tmp + 8 + 8 + 8);
		if ((v >> 16) != 1)
			goto end;
		zvhd->vhd_blocksize = gl (tmp + 8 + 8 + 8 + 4 + 4);
		zvhd->vhd_bamoffset = gl (tmp + 8 + 8 + 4);
		zvhd->vhd_bamsize = (((zvhd->virtsize + zvhd->vhd_blocksize - 1) / zvhd->vhd_blocksize) * 4 + 511) & ~511;
		size = zvhd->vhd_bamoffset + zvhd->vhd_bamsize;
		zvhd->vhd_header = xmalloc (uae_u8, size);
		zfile_fseek (z, 0, SEEK_SET);
		if (zfile_fread (zvhd->vhd_header, 1, size, z) != size)
			goto end;
		zvhd->vhd_sectormap = xmalloc (uae_u8, 512);
		zvhd->vhd_sectormapblock = -1;
		zvhd->vhd_bitmapsize = ((zvhd->vhd_blocksize / (8 * 512)) + 511) & ~511;
	}
	z = zfile_fopen_parent (z, NULL, 0, zvhd->virtsize);
	z->useparent = 0;
	z->dataseek = 1;
	z->userdata = zvhd;
	z->zfileread = vhd_fread;
	write_log ("%s is VHD %s image, virtual size=%dK\n",
		zfile_getname (z),
		zvhd->vhd_type == 2 ? "fixed" : "dynamic",
		zvhd->virtsize / 1024);
	return z;
nonvhd:
end:
	return z;
}

struct zfile *zfile_gunzip (struct zfile *z)
{
    uae_u8 header[2 + 1 + 1 + 4 + 1 + 1];
    z_stream zs;
    int i, size, ret, first;
    uae_u8 flags;
	uae_s64 offset;
	TCHAR name[MAX_DPATH];
    uae_u8 buffer[8192];
    struct zfile *z2;
    uae_u8 b;

	_tcscpy (name, z->name);
    memset (&zs, 0, sizeof (zs));
    memset (header, 0, sizeof (header));
    zfile_fread (header, sizeof (header), 1, z);
    flags = header[3];
    if (header[0] != 0x1f && header[1] != 0x8b)
		return NULL;
    if (flags & 2) /* multipart not supported */
		return NULL;
    if (flags & 32) /* encryption not supported */
		return NULL;
    if (flags & 4) { /* skip extra field */
		zfile_fread (&b, 1, 1, z);
		size = b;
		zfile_fread (&b, 1, 1, z);
		size |= b << 8;
		zfile_fseek (z, size + 2, SEEK_CUR);
    }
    if (flags & 8) { /* get original file name */
		uae_char aname[MAX_DPATH];
		i = 0;
		do {
			zfile_fread (aname + i, 1, 1, z);
		} while (i < MAX_DPATH - 1 && aname[i++]);
		aname[i] = 0;
		memcpy (name, aname, MAX_DPATH);
    }
    if (flags & 16) { /* skip comment */
		i = 0;
		do {
			b = 0;
		    zfile_fread (&b, 1, 1, z);
		} while (b);
    }
	removeext (name, ".gz");
    offset = zfile_ftell (z);
    zfile_fseek (z, -4, SEEK_END);
    zfile_fread (&b, 1, 1, z);
    size = b;
    zfile_fread (&b, 1, 1, z);
    size |= b << 8;
    zfile_fread (&b, 1, 1, z);
    size |= b << 16;
    zfile_fread (&b, 1, 1, z);
    size |= b << 24;
	if (size < 8 || size > 256 * 1024 * 1024) /* safety check */
		return NULL;
    zfile_fseek (z, offset, SEEK_SET);
    z2 = zfile_fopen_empty (z, name, size);
    if (!z2)
		return NULL;
    zs.next_out = z2->data;
    zs.avail_out = size;
    first = 1;
    do {
		zs.next_in = buffer;
		zs.avail_in = zfile_fread (buffer, 1, sizeof (buffer), z);
		if (first) {
			if (inflateInit2 (&zs, -MAX_WBITS) != Z_OK)
				break;
		    first = 0;
		}
		ret = inflate (&zs, 0);
    } while (ret == Z_OK);
    inflateEnd (&zs);
    if (ret != Z_STREAM_END || first != 0) {
		zfile_fclose (z2);
		return NULL;
    }
    zfile_fclose (z);
    return z2;
}

static void truncate880k (struct zfile *z)
{
	int i;
	uae_u8 *b;

	if (z == NULL || z->data == NULL)
		return;
	if (z->size < 880 * 512 * 2) {
		int size = 880 * 512 * 2 - z->size;
		b = xcalloc (uae_u8, size);
		zfile_fwrite (b, size, 1, z);
		xfree (b);
		return;
	}
	for (i = 880 * 512 * 2; i < z->size; i++) {
		if (z->data[i])
			return;
	}
	z->size = 880 * 512 * 2;
}

static struct zfile *extadf (struct zfile *z, int index, int *retcode)
{
	int i, r;
	struct zfile *zo;
	uae_u16 *mfm;
	uae_u16 *amigamfmbuffer;
	uae_u8 writebuffer_ok[32], *outbuf;
	int tracks, len, offs, pos;
	uae_u8 buffer[2 + 2 + 4 + 4];
	int outsize;
	TCHAR newname[MAX_DPATH];
	TCHAR *ext;
	int cantrunc = 0;
	int done = 0;

	if (index > 1)
		return NULL;

	mfm = xcalloc (uae_u16, 32000 / 2);
	amigamfmbuffer = xcalloc (uae_u16, 32000 / 2);
	outbuf = xcalloc (uae_u8, 16384);

	zfile_fread (buffer, 1, 8, z);
	zfile_fread (buffer, 1, 4, z);
	tracks = buffer[2] * 256 + buffer[3];
	offs = 8 + 2 + 2 + tracks * (2 + 2 + 4 + 4);

	_tcscpy (newname, zfile_getname (z));
	ext = _tcsrchr (newname, '.');
	if (ext) {
		_tcscpy (newname + _tcslen (newname) - _tcslen (ext), ".std.adf");
	} else {
		_tcscat (newname, ".std.adf");
	}
	if (index > 0)
		_tcscpy (newname + _tcslen (newname) - 4, ".ima");

	zo = zfile_fopen_empty (z, newname, 0);
	if (!zo)
		goto end;

	if (retcode)
		*retcode = 1;
	pos = 12;
	outsize = 0;
	for (i = 0; i < tracks; i++) {
		int type, bitlen;

		zfile_fseek (z, pos, SEEK_SET);
		zfile_fread (buffer, 2 + 2 + 4 + 4, 1, z);
		pos = zfile_ftell (z);
		type = buffer[2] * 256 + buffer[3];
		len = buffer[5] * 65536 + buffer[6] * 256 + buffer[7];
		bitlen = buffer[9] * 65536 + buffer[10] * 256 + buffer[11];

		zfile_fseek (z, offs, SEEK_SET);
		if (type == 1) {
			zfile_fread (mfm, len, 1, z);
			memset (writebuffer_ok, 0, sizeof writebuffer_ok);
			memset (outbuf, 0, 16384);
			if (index == 0) {
				r = isamigatrack (amigamfmbuffer, (uae_u8*)mfm, len, outbuf, writebuffer_ok, i, &outsize);
				if (r < 0 && i == 0) {
					zfile_seterror ("'%s' is not AmigaDOS formatted", zo->name);
					goto end;
				}
				if (i == 0)
					done = 1;
			} else {
				r = ispctrack (amigamfmbuffer, (uae_u8*)mfm, len, outbuf, writebuffer_ok, i, &outsize);
				if (r < 0 && i == 0) {
					zfile_seterror ("'%s' is not PC formatted", zo->name);
					goto end;
				}
				if (i == 0)
					done = 1;
			}
		} else {
			outsize = 512 * 11;
			if (bitlen / 8 > 18000)
				outsize *= 2;
			zfile_fread (outbuf, outsize, 1, z);
			cantrunc = 1;
			if (index == 0)
				done = 1;
		}
		zfile_fwrite (outbuf, outsize, 1, zo);

		offs += len;

	}
	if (done == 0)
		goto end;
	zfile_fclose (z);
	xfree (mfm);
	xfree (amigamfmbuffer);
	if (cantrunc)
		truncate880k (zo);
	return zo;
end:
	zfile_fclose (zo);
	xfree (mfm);
	xfree (amigamfmbuffer);
	return NULL;
}


#include "fdi2raw.h"
static struct zfile *fdi (struct zfile *z, int index, int *retcode)
{
	unsigned int i, j, r;
	struct zfile *zo;
	TCHAR *orgname = zfile_getname (z);
	TCHAR *ext = _tcsrchr (orgname, '.');
	TCHAR newname[MAX_DPATH];
	uae_u16 *amigamfmbuffer;
	uae_u8 writebuffer_ok[32], *outbuf;
	unsigned int tracks, len;
	int outsize;
	FDI *fdi;
	int startpos = 0;
	uae_u8 tmp[12];
	struct zcache *zc;

	if (index > 2)
		return NULL;

	zc = cache_get (z->name);
	if (!zc) {
		uae_u16 *mfm;
		struct zdiskimage *zd;
		fdi = fdi2raw_header (z);
		if (!fdi)
			return NULL;
		mfm = xcalloc (uae_u16, 32000 / 2);
		zd = xcalloc (struct zdiskimage, 1);
		tracks = fdi2raw_get_last_track (fdi);
		zd->tracks = tracks;
		for (i = 0; i < tracks; i++) {
			uae_u8 *buf, *p;
			fdi2raw_loadtrack (fdi, mfm, NULL, i, &len, NULL, NULL, 1);
			len /= 8;
			buf = p = xmalloc (uae_u8, len);
			for (j = 0; j < len / 2; j++) {
				uae_u16 v = mfm[j];
				*p++ = v >> 8;
				*p++ = v;
			}
			zd->zdisktracks[i].data = buf;
			zd->zdisktracks[i].len = len;
		}
		fdi2raw_header_free (fdi);
		zc = zcache_put (z->name, zd);
	}

	amigamfmbuffer = xcalloc (uae_u16, 32000 / 2);
	outbuf = xcalloc (uae_u8, 16384);
	tracks = zc->zd->tracks;
	if (ext) {
		_tcscpy (newname, orgname);
		_tcscpy (newname + _tcslen (newname) - _tcslen (ext), ".adf");
	} else {
		_tcscat (newname, ".adf");
	}
	if (index == 1)
		_tcscpy (newname + _tcslen (newname) - 4, ".ima");
	if (index == 2)
		_tcscpy (newname + _tcslen (newname) - 4, ".ext.adf");
	zo = zfile_fopen_empty (z, newname, 0);
	if (!zo)
		goto end;
	if (retcode)
		*retcode = 1;
	if (index > 1) {
		zfile_fwrite ("UAE-1ADF", 8, 1, zo);
		tmp[0] = 0; tmp[1] = 0; /* flags (reserved) */
		tmp[2] = 0; tmp[3] = tracks; /* number of tracks */
		zfile_fwrite (tmp, 4, 1, zo);
		memset (tmp, 0, sizeof tmp);
		tmp[2] = 0; tmp[3] = 1; /* track type */
		startpos = zfile_ftell (zo);
		for (i = 0; i < tracks; i++)
			zfile_fwrite (tmp, sizeof tmp, 1, zo);
	}
	outsize = 0;
	for (i = 0; i < tracks; i++) {
		uae_u8 *p = (uae_u8*)zc->zd->zdisktracks[i].data;
		len = zc->zd->zdisktracks[i].len;
		memset (writebuffer_ok, 0, sizeof writebuffer_ok);
		memset (outbuf, 0, 16384);
		if (index == 0) {
			r = isamigatrack (amigamfmbuffer, p, len, outbuf, writebuffer_ok, i, &outsize);
			if (r < 0 && i == 0) {
				zfile_seterror ("'%s' is not AmigaDOS formatted", orgname);
				goto end;
			}
			zfile_fwrite (outbuf, outsize, 1, zo);
		} else if (index == 1) {
			r = ispctrack (amigamfmbuffer, p, len, outbuf, writebuffer_ok, i, &outsize);
			if (r < 0 && i == 0) {
				zfile_seterror ("'%s' is not PC formatted", orgname);
				goto end;
			}
			zfile_fwrite (outbuf, outsize, 1, zo);
		} else {
			int pos = zfile_ftell (zo);
			unsigned int maxlen = len > 12798 ? len : 12798;
			int lenb = len * 8;

			if (maxlen & 1)
				maxlen++;
			zfile_fseek (zo, startpos + i * 12 + 4, SEEK_SET);
			tmp[4] = 0; tmp[5] = 0; tmp[6] = maxlen >> 8; tmp[7] = maxlen;
			tmp[8] = lenb >> 24; tmp[9] = lenb >> 16; tmp[10] = lenb >> 8; tmp[11] = lenb;
			zfile_fwrite (tmp + 4, 2, 4, zo);
			zfile_fseek (zo, pos, SEEK_SET);
			zfile_fwrite (p, 1, len, zo);
			if (maxlen > len)
				zfile_fwrite (outbuf, 1, maxlen - len, zo);
		}
	}
	zfile_fclose (z);
	xfree (amigamfmbuffer);
	xfree (outbuf);
	if (index == 0)
		truncate880k (zo);
	return zo;
end:
	zfile_fclose (zo);
	xfree (amigamfmbuffer);
	xfree (outbuf);
	return NULL;
}

#ifdef CAPS
#include "caps/capsimage.h"
static struct zfile *ipf (struct zfile *z, int index, int *retcode)
{
	int i, j, r;
	struct zfile *zo;
	TCHAR *orgname = zfile_getname (z);
	TCHAR *ext = _tcsrchr (orgname, '.');
	TCHAR newname[MAX_DPATH];
	uae_u16 *amigamfmbuffer;
	uae_u8 writebuffer_ok[32];
	int tracks, len;
	int outsize;
	int startpos = 0;
	uae_u8 *outbuf;
	uae_u8 tmp[12];
	struct zcache *zc;

	if (index > 2)
		return NULL;

	zc = cache_get (z->name);
	if (!zc) {
		uae_u16 *mfm;
		struct zdiskimage *zd;
		if (!caps_loadimage (z, 0, &tracks))
			return NULL;
		mfm = xcalloc (uae_u16, 32000 / 2);
		zd = xcalloc (struct zdiskimage, 1);
		zd->tracks = tracks;
		for (i = 0; i < tracks; i++) {
			uae_u8 *buf, *p;
			int mrev, gapo;
			caps_loadtrack (mfm, NULL, 0, i, &len, &mrev, &gapo);
			//write_log ("%d: %d %d %d\n", i, mrev, gapo, len);
			len /= 8;
			buf = p = xmalloc (uae_u8, len);
			for (j = 0; j < len / 2; j++) {
				uae_u16 v = mfm[j];
				*p++ = v >> 8;
				*p++ = v;
			}
			zd->zdisktracks[i].data = buf;
			zd->zdisktracks[i].len = len;
		}
		caps_unloadimage (0);
		zc = zcache_put (z->name, zd);
	}

	outbuf = xcalloc (uae_u8, 16384);
	amigamfmbuffer = xcalloc (uae_u16, 32000 / 2);
	if (ext) {
		_tcscpy (newname, orgname);
		_tcscpy (newname + _tcslen (newname) - _tcslen (ext), ".adf");
	} else {
		_tcscat (newname, ".adf");
	}
	if (index == 1)
		_tcscpy (newname + _tcslen (newname) - 4, ".ima");
	if (index == 2)
		_tcscpy (newname + _tcslen (newname) - 4, ".ext.adf");

	zo = zfile_fopen_empty (z, newname, 0);
	if (!zo)
		goto end;

	if (retcode)
		*retcode = 1;

	tracks = zc->zd->tracks;

	if (index > 1) {
		zfile_fwrite ("UAE-1ADF", 8, 1, zo);
		tmp[0] = 0; tmp[1] = 0; /* flags (reserved) */
		tmp[2] = 0; tmp[3] = tracks; /* number of tracks */
		zfile_fwrite (tmp, 4, 1, zo);
		memset (tmp, 0, sizeof tmp);
		tmp[2] = 0; tmp[3] = 1; /* track type */
		startpos = zfile_ftell (zo);
		for (i = 0; i < tracks; i++)
			zfile_fwrite (tmp, sizeof tmp, 1, zo);
	}

	outsize = 0;
	for (i = 0; i < tracks; i++) {
		uae_u8 *p = (uae_u8*)zc->zd->zdisktracks[i].data;
		len = zc->zd->zdisktracks[i].len;
		memset (writebuffer_ok, 0, sizeof writebuffer_ok);
		memset (outbuf, 0, 16384);
		if (index == 0) {
			r = isamigatrack (amigamfmbuffer, p, len, outbuf, writebuffer_ok, i, &outsize);
			if (r < 0 && i == 0) {
				zfile_seterror ("'%s' is not AmigaDOS formatted", orgname);
				goto end;
			}
			zfile_fwrite (outbuf, 1, outsize, zo);
		} else if (index == 1) {
			r = ispctrack (amigamfmbuffer, p, len, outbuf, writebuffer_ok, i, &outsize);
			if (r < 0 && i == 0) {
				zfile_seterror ("'%s' is not PC formatted", orgname);
				goto end;
			}
			zfile_fwrite (outbuf, outsize, 1, zo);
		} else {
			int pos = zfile_ftell (zo);
			int maxlen = len > 12798 ? len : 12798;
			int lenb = len * 8;

			if (maxlen & 1)
				maxlen++;
			zfile_fseek (zo, startpos + i * 12 + 4, SEEK_SET);
			tmp[4] = 0; tmp[5] = 0; tmp[6] = maxlen >> 8; tmp[7] = maxlen;
			tmp[8] = lenb >> 24; tmp[9] = lenb >> 16; tmp[10] = lenb >> 8; tmp[11] = lenb;
			zfile_fwrite (tmp + 4, 2, 4, zo);
			zfile_fseek (zo, pos, SEEK_SET);
			zfile_fwrite (p, 1, len, zo);
			if (maxlen > len)
				zfile_fwrite (outbuf, 1, maxlen - len, zo);
		}
	}
	zfile_fclose (z);
	xfree (amigamfmbuffer);
	xfree (outbuf);
	if (index == 0)
		truncate880k (zo);
	return zo;
end:
	zfile_fclose (zo);
	xfree (amigamfmbuffer);
	xfree (outbuf);
	return NULL;
}
#endif

static struct zfile *dsq (struct zfile *z, int lzx)
{
	struct zfile *zi = NULL;
	struct zvolume *zv = NULL;
/*
	if (lzx) {
		zv = archive_directory_lzx (z);
		if (zv) {
			if (zv->root.child)
				zi = archive_access_lzx (zv->root.child);
		}
	} else {
		zi = z;
	}
*/
	zi = z;
	if (zi) {
		uae_u8 *buf = zfile_getdata (zi, 0, -1);
		if (!memcmp (buf, "PKD\x13", 4) || !memcmp (buf, "PKD\x11", 4)) {
			TCHAR *fn;
			int sectors = buf[18];
			int heads = buf[15];
			int blocks = (buf[6] << 8) | buf[7];
			int blocksize = (buf[10] << 8) | buf[11];
			struct zfile *zo;
			int size = blocks * blocksize;
			int off = buf[3] == 0x13 ? 52 : 32;
			int i;

			if (size < 1760 * 512)
				size = 1760 * 512;

			if (zfile_getfilename (zi) && _tcslen (zfile_getfilename (zi))) {
				fn = xmalloc (TCHAR, (_tcslen (zfile_getfilename (zi)) + 5));
				_tcscpy (fn, zfile_getfilename (zi));
				_tcscat (fn, ".adf");
			} else {
				fn = my_strdup ("dsq.adf");
			}
			zo = zfile_fopen_empty (z, fn, size);
			xfree (fn);
			for (i = 0; i < blocks / (sectors / heads); i++) {
				zfile_fwrite (buf + off, sectors * blocksize / heads, 1, zo);
				off += sectors * (blocksize + 16) / heads;
			}
			//FIXME: zfile_fclose_archive (zv);
			zfile_fclose (z);
			xfree (buf);
			return zo;
		}
		xfree (buf);
	}
	if (lzx)
		zfile_fclose (zi);
	return z;
}

static struct zfile *wrp (struct zfile *z)
{
	//return unwarp (z);
}

static struct zfile *bunzip (const char *decompress, struct zfile *z)
{
    return z;
}

static struct zfile *lha (struct zfile *z)
{
    return z;
}

static const char *ignoreextensions[] =
    { ".gif", ".jpg", ".png", ".xml", ".pdf", ".txt", 0 };
static const char *diskimageextensions[] =
    { ".adf", ".adz", ".ipf", ".fdi", 0 };

static int isdiskimage (char *name)
{
    int i;

    i = 0;
    while (diskimageextensions[i]) {
	if (strlen (name) > 3 && !strcasecmp (name + strlen (name) - 4, diskimageextensions[i]))
	    return 1;
	i++;
    }
    return 0;
}

static struct zfile *dms (struct zfile *z, int index, int *retcode)
{
    int ret;
    struct zfile *zo;
	TCHAR *orgname = zfile_getname (z);
	TCHAR *ext = _tcsrchr (orgname, '.');
	TCHAR newname[MAX_DPATH];
	static int recursive;
	int i;
	struct zfile *zextra[DMS_EXTRA_SIZE] = { 0 };

	if (recursive)
		return NULL;
	if (ext) {
		_tcscpy (newname, orgname);
		_tcscpy (newname + _tcslen (newname) - _tcslen (ext), ".adf");
	} else {
		_tcscat (newname, ".adf");
	}

	zo = zfile_fopen_empty (z, newname, 1760 * 512);
    if (!zo)
		return NULL;
    ret = DMS_Process_File (z, zo, CMD_UNPACK, OPT_VERBOSE, 0, 0, 0, zextra);
    if (ret == NO_PROBLEM || ret == DMS_FILE_END) {
		int off = zfile_ftell (zo);
		if (off >= 1760 * 512 / 3 && off <= 1760 * 512 * 3 / 4) { // possible split dms?
			if (_tcslen (orgname) > 5) {
				TCHAR *s = orgname + _tcslen (orgname) - 5;
				if (!_tcsicmp (s, "a.dms")) {
					TCHAR *fn2 = my_strdup (orgname);
					struct zfile *z2;
					fn2[_tcslen (fn2) - 5]++;
					recursive++;
					z2 = zfile_fopen (fn2, "rb", z->zfdmask);
					recursive--;
					if (z2) {
						ret = DMS_Process_File (z2, zo, CMD_UNPACK, OPT_VERBOSE, 0, 0, 1, NULL);
						zfile_fclose (z2);
					}
					xfree (fn2);
				}
			}
		}
		zfile_fseek (zo, 0, SEEK_SET);
		if (index > 0) {
			zfile_fclose (zo);
			zo = NULL;
			for (i = 0; i < DMS_EXTRA_SIZE && zextra[i]; i++);
			if (index > i)
				goto end;
			zo = zextra[index - 1];
			zextra[index - 1] = NULL;
		}
		if (retcode)
			*retcode = 1;
	zfile_fclose (z);
		z = NULL;

	} else {
		zfile_fclose (zo);
		zo = NULL;
	}
end:
	for (i = 0; i < DMS_EXTRA_SIZE; i++)
		zfile_fclose (zextra[i]);
	return zo;
}

const TCHAR *uae_ignoreextensions[] =
{ ".gif", ".jpg", ".png", ".xml", ".pdf", ".txt", 0 };
const TCHAR *uae_diskimageextensions[] =
{ ".adf", ".adz", ".ipf", ".fdi", ".exe", ".dms", ".wrp", ".dsq", 0 };

int zfile_is_ignore_ext (const TCHAR *name)
{
	int i;
	const TCHAR *ext;

	ext = _tcsrchr (name, '.');
	if (!ext)
		return 0;
	for (i = 0; uae_ignoreextensions[i]; i++) {
		if (!strcasecmp (uae_ignoreextensions[i], ext))
			return 1;
	}
	return 0;
}

int zfile_is_diskimage (const TCHAR *name)
{
	int i;

	const TCHAR *ext = _tcsrchr (name, '.');
	if (!ext)
		return 0;
	i = 0;
	while (uae_diskimageextensions[i]) {
		if (!strcasecmp (ext, uae_diskimageextensions[i]))
			return HISTORY_FLOPPY;
		i++;
	}
	if (!_tcsicmp (ext, ".cue"))
		return HISTORY_CD;
	return -1;
}


static const TCHAR *archive_extensions[] = {
	 "7z", "rar", "zip", "lha", "lzh", "lzx",
	"adf", "adz", "dsq", "dms", "ipf", "fdi", "wrp", "ima",
	"hdf", "tar",
	NULL
};
static const TCHAR *plugins_7z[] = { "7z", "rar", "zip", "lha", "lzh", "lzx", "adf", "dsq", "hdf", "tar", NULL };
static const uae_char *plugins_7z_x[] = { "7z", "Rar!", "MK", NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL };
static const int plugins_7z_t[] = {
	ArchiveFormat7Zip, ArchiveFormatRAR, ArchiveFormatZIP, ArchiveFormatLHA, ArchiveFormatLHA, ArchiveFormatLZX,
	ArchiveFormatADF, ArchiveFormatADF, ArchiveFormatADF, ArchiveFormatTAR
};
static const int plugins_7z_m[] = {
	ZFD_ARCHIVE, ZFD_ARCHIVE, ZFD_ARCHIVE, ZFD_ARCHIVE, ZFD_ARCHIVE, ZFD_ARCHIVE,
	ZFD_ADF, ZFD_ADF, ZFD_ADF, ZFD_ARCHIVE
};

int iszip (struct zfile *z)
{
	TCHAR *name = z->name;
	TCHAR *ext = _tcsrchr (name, '.');
	uae_u8 header[32];
	int i;
	int mask = ZFD_NORMAL;//z->zfdmask;

	if (!ext)
		return 0;
	memset (header, 0, sizeof (header));
	zfile_fseek (z, 0, SEEK_SET);
	zfile_fread (header, sizeof (header), 1, z);
	zfile_fseek (z, 0, SEEK_SET);

	if (mask & ZFD_ARCHIVE) {
		if (!strcasecmp (ext, ".zip") || !strcasecmp (ext, ".rp9")) {
			if (header[0] == 'P' && header[1] == 'K')
				return ArchiveFormatZIP;
			return 0;
		}
	}
	if (mask & ZFD_ARCHIVE) {
		if (!strcasecmp (ext, ".7z")) {
			if (header[0] == '7' && header[1] == 'z')
				return ArchiveFormat7Zip;
			return 0;
		}
		if (!strcasecmp (ext, ".rar")) {
			if (header[0] == 'R' && header[1] == 'a' && header[2] == 'r' && header[3] == '!')
				return ArchiveFormatRAR;
			return 0;
		}
		if (!strcasecmp (ext, ".lha") || !strcasecmp (ext, ".lzh")) {
			if (header[2] == '-' && header[3] == 'l' && header[4] == 'h' && header[6] == '-')
				return ArchiveFormatLHA;
			return 0;
		}
		if (!strcasecmp (ext, ".lzx")) {
			if (header[0] == 'L' && header[1] == 'Z' && header[2] == 'X')
				return ArchiveFormatLZX;
			return 0;
		}
	}
	if (mask & ZFD_ADF) {
		if (!strcasecmp (ext, ".adf")) {
			if (header[0] == 'D' && header[1] == 'O' && header[2] == 'S' && (header[3] >= 0 && header[3] <= 7))
				return ArchiveFormatADF;
			if (isfat (header))
				return ArchiveFormatFAT;
			return 0;
		}
		if (!strcasecmp (ext, ".ima")) {
			if (isfat (header))
				return ArchiveFormatFAT;
		}
	}
	if (mask & ZFD_HD) {
		if (!strcasecmp (ext, ".hdf")) {
			if (header[0] == 'D' && header[1] == 'O' && header[2] == 'S' && (header[3] >= 0 && header[3] <= 7))
				return ArchiveFormatADF;
			if (header[0] == 'S' && header[1] == 'F' && header[2] == 'S')
				return ArchiveFormatADF;
			if (header[0] == 'R' && header[1] == 'D' && header[2] == 'S' && header[3] == 'K')
				return ArchiveFormatRDB;
			if (isfat (header))
				return ArchiveFormatFAT;
			return 0;
		}
	}
#if defined(ARCHIVEACCESS)
	for (i = 0; plugins_7z_x[i]; i++) {
		if ((plugins_7z_m[i] & mask) && plugins_7z_x[i] && !strcasecmp (ext + 1, plugins_7z[i]) &&
			!memcmp (header, plugins_7z_x[i], strlen (plugins_7z_x[i])))
			return plugins_7z_t[i];
	}
#endif
	return 0;
}

static struct zfile *unzip (struct zfile *z)
{
    unzFile uz;
    unz_file_info file_info;
    char filename_inzip[2048];
    struct zfile *zf;
    unsigned int err, zipcnt, i, we_have_file = 0;
    int select;
    char tmphist[MAX_DPATH];
    int first = 1;

    zf = 0;
    uz = unzOpen (z);
    if (!uz)
	return z;
    if (unzGoToFirstFile (uz) != UNZ_OK)
	return z;
    zipcnt = 1;
    tmphist[0] = 0;
    for (;;) {
	err = unzGetCurrentFileInfo(uz,&file_info,filename_inzip,sizeof(filename_inzip),NULL,0,NULL,0);
	if (err != UNZ_OK)
	    return z;
	if (file_info.uncompressed_size > 0) {
	    i = 0;
	    while (uae_ignoreextensions[i]) {
		if (strlen(filename_inzip) > strlen (uae_ignoreextensions[i]) &&
		    !strcasecmp (uae_ignoreextensions[i], filename_inzip + strlen (filename_inzip) - strlen (uae_ignoreextensions[i])))
		    break;
		i++;
	    }
	    if (!uae_ignoreextensions[i]) {
		if (tmphist[0]) {
		    DISK_history_add (tmphist, -1, 0, 0);
		    tmphist[0] = 0;
		    first = 0;
		}
		if (first) {
		    if (zfile_is_diskimage (filename_inzip))
			sprintf (tmphist,"%s/%s", z->name, filename_inzip);
		} else {
		    sprintf (tmphist,"%s/%s", z->name, filename_inzip);
		    DISK_history_add (tmphist, -1, 0, 0);
		    tmphist[0] = 0;
		}
		select = 0;
		if (!z->zipname)
		    select = 1;
		if (z->zipname && !strcasecmp (z->zipname, filename_inzip))
		    select = -1;
		if (z->zipname && z->zipname[0] == '#' && atol (z->zipname + 1) == (int)zipcnt)
		    select = -1;
		if (select && !we_have_file) {
		    unsigned int err = unzOpenCurrentFile (uz);
		    if (err == UNZ_OK) {
			zf = zfile_fopen_empty (NULL, filename_inzip, file_info.uncompressed_size);
			if (zf) {
			    err = unzReadCurrentFile  (uz, zf->data, file_info.uncompressed_size);
			    unzCloseCurrentFile (uz);
			    if (err == 0 || err == file_info.uncompressed_size) {
				zf = zuncompress (zf);
				if (select < 0 || zfile_gettype (zf)) {
				    we_have_file = 1;
				}
			    }
			}
			if (!we_have_file) {
			    zfile_fclose (zf);
			    zf = 0;
			}
		    }
		}
	    }
	}
	zipcnt++;
	err = unzGoToNextFile (uz);
	if (err != UNZ_OK)
	    break;
    }
    if (zf) {
	zfile_fclose (z);
	z = zf;
    }
    return z;
}

static struct zfile *zfile_fopen_nozip (const TCHAR *name, const TCHAR *mode)
{
	struct zfile *l;
	FILE *f;

	if(*name == '\0')
		return NULL;
	l = zfile_create (NULL);
	l->name = my_strdup (name);
	l->mode = my_strdup (mode);
	f = _tfopen (name, mode);
	if (!f) {
		zfile_fclose (l);
		return 0;
	}
	l->f = f;
	return l;
}

struct zfile *zuncompress (struct zfile *z)
{
	int retcode = 0, index = 0;
    char *name = z->name;
    char *ext = strrchr (name, '.');
    uae_u8 header[4];

    if (ext != NULL) {
		ext++;
	if (strcasecmp (ext, "zip") == 0)
		return unzip (z);
	if (strcasecmp (ext, "gz") == 0)
	     return zfile_gunzip (z);
	if (strcasecmp (ext, "adz") == 0)
	     return zfile_gunzip (z);
	if (strcasecmp (ext, "roz") == 0)
	     return zfile_gunzip (z);
	if (strcasecmp (ext, "dms") == 0)
	     return dms (z, index, retcode);
	if (strcasecmp (ext, "lha") == 0
		|| strcasecmp (ext, "lzh") == 0)
		return lha (z);
	memset (header, 0, sizeof (header));
	zfile_fseek (z, 0, SEEK_SET);
	zfile_fread (header, sizeof (header), 1, z);
	zfile_fseek (z, 0, SEEK_SET);
	if (header[0] == 0x1f && header[1] == 0x8b)
	    return zfile_gunzip (z);
	if (header[0] == 'P' && header[1] == 'K')
		return unzip (z);
	if (header[0] == 'D' && header[1] == 'M' && header[2] == 'S' && header[3] == '!')
	    return dms (z, index, retcode);
    }
    return z;
}

static struct zfile *openzip2 (const TCHAR *pname)
{
	int i = 0, j = 0;
	TCHAR v = 0;
	TCHAR name[MAX_DPATH];
	TCHAR zippath[MAX_DPATH];

	if (!pname)
		return NULL;

	memset(name,    0, sizeof(TCHAR) * MAX_DPATH);
	memset(zippath, 0, sizeof(TCHAR) * MAX_DPATH);

	_tcscpy (name, pname);
	i = _tcslen (name) - 2;
	while (i > 0) {
		if ((name[i] == '/' || name[i] == '\\') && i > 4) {
			v = name[i];
			name[i] = 0;
			for (j = 0; plugins_7z[j]; j++) {
				int len = _tcslen (plugins_7z[j]);
				if ((i > len) // otherwise the next check is done on a negative index! - Sven
				  && (name[i - len - 1] == '.') // and the next check on a negative address.
				  && !strcasecmp (name + i - len, plugins_7z[j])) {
					struct zfile *f = zfile_fopen_nozip (name, _T("rb"));
					if (f) {
						f->zipname = my_strdup (name + i + 1);
						return f;
					}
					break;
				}
			}
			name[i] = v;
		}
		i--;
	}
	return NULL;
}

static FILE *openzip (char *name, char *zippath)
{
    int i;
    char v;

    i = strlen (name) - 2;
    if (zippath)
	zippath[0] = 0;
    while (i > 0) {
	if ((name[i] == '/' || name[i] == '\\') && i > 4) {
	    v = name[i];
	    name[i] = 0;
	    if (!strcasecmp (name + i - 4, ".zip")) {
		FILE *f = fopen (name, "rb");
		if (f) {
		    if (zippath)
			strcpy (zippath, name + i + 1);
		    return f;
		}
	    }
	    name[i] = v;
	}
	i--;
    }
    return 0;
}

#ifdef SINGLEFILE
extern uae_u8 singlefile_data[];

static struct zfile *zfile_opensinglefile (struct zfile *l)
{
    uae_u8 *p = singlefile_data;
    int size, offset;
	TCHAR tmp[256], *s;

	_tcscpy (tmp, l->name);
	s = tmp + _tcslen (tmp) - 1;
	while (*s != 0 && *s != '/' && *s != '\\')
		s--;
    if (s > tmp)
		s++;
	write_log ("loading from singlefile: '%s'\n", tmp);
    while (*p++);
    offset = (p[0] << 24)|(p[1] << 16)|(p[2] << 8)|(p[3] << 0);
    p += 4;
    for (;;) {
		size = (p[0] << 24)|(p[1] << 16)|(p[2] << 8)|(p[3] << 0);
		if (!size)
		    break;
		if (!strcmpi (tmp, p + 4)) {
		    l->data = singlefile_data + offset;
		    l->size = size;
			write_log ("found, size %d\n", size);
		    return l;
		}
		offset += size;
		p += 4;
		p += _tcslen (p) + 1;
    }
	write_log ("not found\n");
    return 0;
}
#endif

static bool writeneeded (const TCHAR *mode)
{
	return _tcschr (mode, 'w') || _tcschr (mode, 'a') || _tcschr (mode, '+') || _tcschr (mode, 't');
}
/*
 * fopen() for a compressed file
 */
struct zfile *zfile_fopen (const char *name, const char *mode, int mask)
{
    struct zfile *l;
    FILE *f;
    char zipname[1000];

    if( *name == '\0' )
		return NULL;
    l = zfile_create (NULL);
	l->name = strdup (name);
	l->mode = strdup (mode);
#ifdef SINGLEFILE
    if (zfile_opensinglefile (l))
	return l;
#endif
    f = openzip (l->name, zipname);
    if (f) {
	if (strcasecmp (mode, "rb")) {
	    zfile_fclose (l);
	    fclose (f);
	    return 0;
	}
	l->zipname = strdup (zipname);
    }
    if (!f) {
	f = fopen (name, mode);
	if (!f) {
	    zfile_fclose (l);
	    return 0;
	}
    }
    l->f = f;
    l = zuncompress (l);
    return l;
}

static struct zfile *zfile_fopen_2 (const TCHAR *name, const TCHAR *mode, int mask)
{
	struct zfile *l;
	FILE *f;

	if(*name == '\0')
		return NULL;
#ifdef SINGLEFILE
	if (zfile_opensinglefile (l))
		return l;
#endif
	l = openzip2 (name);
	if (l) {
		if (writeneeded (mode)) {
			zfile_fclose (l);
			return 0;
		}
		l->zfdmask = mask;
	} else {
		struct mystat st;
		l = zfile_create (NULL);
		l->mode = my_strdup (mode);
		l->name = my_strdup (name);
		l->zfdmask = mask;
		if (!_tcsicmp (mode, _T("r"))) {
			f = my_opentext (l->name);
			l->textmode = 1;
		} else {
			f = _tfopen (l->name, mode);
		}
		if (!f) {
			zfile_fclose (l);
			return 0;
		}
		if (my_stat (l->name, &st))
			l->size = st.size;
		l->f = f;
	}
	return l;
}

static void manglefilename(TCHAR *out, const TCHAR *in)
{
	_tcscpy (out, in);
}

bool zfile_needwrite (struct zfile *zf)
{
	if (!zf->mode)
		return false;
	return writeneeded (zf->mode);
}


/*
* fopen() for a compressed file
*/
static struct zfile *zfile_fopen_x (const TCHAR *name, const TCHAR *mode, int mask, int index)
{
	int cnt = 10;
	struct zfile *l, *l2;
	TCHAR path[MAX_DPATH];

	if (_tcslen (name) == 0)
		return NULL;
	manglefilename (path, name);
	l = zfile_fopen_2 (path, mode, mask);
	if (!l)
		return 0;
	l2 = NULL;
	while (cnt-- > 0) {
		int rc;
		zfile_fseek (l, 0, SEEK_SET);
		//l2 = zuncompress (NULL, l, 0, mask, &rc, index);
		l2 = zuncompress (l);
		if (!l2) {
			if (rc < 0) {
				zfile_fclose (l);
				return NULL;
			}
			zfile_fseek (l, 0, SEEK_SET);
			break;
		} else {
			if (l2->parent == l)
				l->opencnt--;
		}
		l = l2;
	}
	return l;
}


static struct zfile *zfile_fopenx2 (const TCHAR *name, const TCHAR *mode, int mask, int index)
{
	struct zfile *f;
	TCHAR tmp[MAX_DPATH];

	f = zfile_fopen_x (name, mode, mask, index);
	if (f)
		return f;
	if (_tcslen (name) <= 2)
		return NULL;
	if (name[1] != ':') {
//		_tcscpy (tmp, start_path_data);
		_tcscpy (tmp, "./");
		_tcscat (tmp, name);
		f = zfile_fopen_x (tmp, mode, mask, index);
		if (f)
			return f;
	}
#if 0
	name += 2;
	if (name[0] == '/' || name[0] == '\\')
		name++;
	for (;;) {
		_tcscpy (tmp, start_path_data);
		_tcscpy (tmp, name);
		f = zfile_fopen_x (tmp, mode, mask);
		if (f)
			return f;
		while (name[0]) {
			name++;
			if (name[-1] == '/' || name[-1] == '\\')
				break;
		}
		if (name[0] == 0)
			break;
	}
#endif
	return NULL;
}

static struct zfile *zfile_fopenx (const TCHAR *name, const TCHAR *mode, int mask, int index)
{
	struct zfile *zf;
	//write_log (_T("zfile_fopen('%s','%s',%08x,%d)\n"), name, mode, mask, index);
	zf = zfile_fopenx2 (name, mode, mask, index);
	//write_log (_T("=%p\n"), zf);
	return zf;
}
#if 0
struct zfile *zfile_fopen (const TCHAR *name, const TCHAR *mode, int mask)
{
	return zfile_fopenx (name, mode, mask, 0);
}
#endif
/*struct zfile *zfile_fopen_mode (const TCHAR *name, const TCHAR *mode)
{
	return zfile_fopenx (name, mode, 0, 0);
}*/

struct zfile *zfile_fopen_nmmi (const TCHAR *name, const TCHAR *mode, int mask, int index)
{
	return zfile_fopenx (name, mode, mask, index);
}

struct zfile *zfile_dup (struct zfile *zf)
{
	struct zfile *nzf;
	if (!zf)
		return NULL;
	if (zf->archiveparent)
		checkarchiveparent (zf);
	if (zf->userdata)
		return NULL;
	if (!zf->data && zf->dataseek) {
		nzf = zfile_create (zf);
	} else if (zf->data) {
		nzf = zfile_create (zf);
		nzf->data = xmalloc (uae_u8, zf->size);
		memcpy (nzf->data, zf->data, zf->size);
		nzf->size = zf->size;
		nzf->datasize = zf->datasize;
	} else {
		if (zf->zipname) {
			nzf = openzip2 (zf->name);
			if (nzf)
				return nzf;
		}
		FILE *ff = _tfopen (zf->name, zf->mode);
		if (!ff)
			return NULL;
		nzf = zfile_create (zf);
		nzf->f = ff;
	}
	zfile_fseek (nzf, zf->seek, SEEK_SET);
	if (zf->name)
		nzf->name = my_strdup (zf->name);
	if (nzf->zipname)
		nzf->zipname = my_strdup (zf->zipname);
	nzf->zfdmask = zf->zfdmask;
	nzf->mode = my_strdup (zf->mode);
	nzf->size = zf->size;
	return nzf;
}

int zfile_exists (const TCHAR *name)
{
	struct zfile *z;

	if (my_existsfile (name))
		return 1;
	z = zfile_fopen (name, "rb", ZFD_NORMAL | ZFD_CHECKONLY);
	if (!z)
		return 0;
	zfile_fclose (z);
	return 1;
}

int zfile_iscompressed (struct zfile *z)
{
    return z->data ? 1 : 0;
}

struct zfile *zfile_fopen_empty (struct zfile *prev, const TCHAR *name, uae_u64 size)
{
    struct zfile *l;
    l = zfile_create (prev);
	l->name = name ? my_strdup (name) : "";
	if (size) {
		l->data = xcalloc (uae_u8, size);
		if (!l->data)  {
			xfree (l);
			return NULL;
		}
		l->size = size;
	} else {
		l->data = xcalloc (uae_u8, 1);
		l->size = 0;
	}
    return l;
}

struct zfile *zfile_fopen_parent (struct zfile *z, const TCHAR *name, uae_u64 offset, uae_u64 size)
{
	struct zfile *l;

	if (z == NULL)
		return NULL;
	l = zfile_create (z);
	if (name)
		l->name = my_strdup (name);
	else if (z->name)
		l->name = my_strdup (z->name);
	l->size = size;
	l->offset = offset;
	for (;;) {
		l->parent = z;
		l->useparent = 1;
		if (!z->parent)
			break;
		l->offset += z->offset;
		z = z->parent;
	}
	z->opencnt++;
	return l;
}

struct zfile *zfile_fopen_data (const TCHAR *name, uae_u64 size, uae_u8 *data)
{
	struct zfile *l;

	l = zfile_create (NULL);
	l->name = name ? my_strdup (name) : "";
	l->data = xmalloc (uae_u8, size);
	l->size = size;
	memcpy (l->data, data, size);
	return l;
}

uae_s64 zfile_size (struct zfile *z)
{
	return z->size;
}

uae_s64 zfile_ftell (struct zfile *z)
{
	if (z->data || z->dataseek || z->parent)
		return z->seek;
	return ftell (z->f);

}

uae_s64 zfile_fseek (struct zfile *z, uae_s64 offset, int mode)
{
	if (z->zfileseek)
		return z->zfileseek (z, offset, mode);
	if (z->data || z->dataseek || (z->parent && z->useparent)) {
		int ret = 0;
		switch (mode)
		{
	    case SEEK_SET:
			z->seek = offset;
			break;
	    case SEEK_CUR:
			z->seek += offset;
			break;
	    case SEEK_END:
			z->seek = z->size + offset;
			break;
		}
		if (z->seek < 0) {
			z->seek = 0;
			ret = 1;
		}
		if (z->seek > z->size) {
			z->seek = z->size;
			ret = 1;
		}
		return ret;
	} else {
		return fseek (z->f, offset, mode);
    }
	return 1;
}

size_t zfile_fread  (void *b, size_t l1, size_t l2, struct zfile *z)
{
	if (z->zfileread)
		return z->zfileread (b, l1, l2, z);
    if (z->data) {
		if (z->seek + l1 * l2 > z->size) {
			if (l1)
				l2 = (size_t)((z->size - z->seek) / (uae_s64)(l1));
			else
				l2 = 0;
			if (l2 < 0)
				l2 = 0;
		}
		memcpy (b, z->data + z->offset + z->seek, l1 * l2);
		z->seek += l1 * l2;
		return l2;
    }
	if (z->parent && z->useparent) {
		size_t ret;
		uae_s64 v;
		uae_s64 size = z->size;
		v = z->seek;
		if (v + l1 * l2 > size) {
			if (l1)
				l2 = (size - v) / l1;
			else
				l2 = 0;
			if (l2 < 0)
				l2 = 0;
		}
		zfile_fseek (z->parent, z->seek + z->offset, SEEK_SET);
		v = z->seek;
		ret = zfile_fread (b, l1, l2, z->parent);
		z->seek = v + l1 * ret;
		return ret;
	}
	return fread (b, l1, l2, z->f);
}

size_t zfile_fwrite (const void *b, size_t l1, size_t l2, struct zfile *z)
{
	if (z->zfilewrite)
		return z->zfilewrite (b, l1, l2, z);
	if (z->parent && z->useparent)
		return 0;
    if (z->data) {
		int off = z->seek + l1 * l2;
		if (off > z->size) {
			z->data = xrealloc (uae_u8, z->data, off);
			z->size = off;
		}
		memcpy (z->data + z->seek, b, l1 * l2);
		z->seek += l1 * l2;
		return l2;
    }
    return fwrite (b, l1, l2, z->f);
}

size_t zfile_fputs (struct zfile *z, TCHAR *s)
{
	size_t t;
	t = zfile_fwrite (s, strlen (s), 1, z);
	return t;
}

char *zfile_fgetsa (char *s, int size, struct zfile *z)
{
	checkarchiveparent (z);
	if (z->data) {
		char *os = s;
		int i;
		for (i = 0; i < size - 1; i++) {
			if (z->seek == z->size) {
				if (i == 0)
					return NULL;
				break;
			}
			*s = z->data[z->seek++];
			if (*s == '\n') {
				s++;
				break;
			}
			s++;
		}
		*s = 0;
		return os;
	} else {
		return fgets (s, size, z->f);
	}
}

TCHAR *zfile_fgets (TCHAR *s, int size, struct zfile *z)
{
	checkarchiveparent (z);
	if (z->data) {
		char s2[MAX_DPATH];
		char *p = s2;
		int i;
		for (i = 0; i < size - 1; i++) {
			if (z->seek == z->size) {
				if (i == 0)
					return NULL;
				break;
			}
			*p = z->data[z->seek++];
			if (*p == 0 && i == 0)
				return NULL;
			if (*p == '\n' || *p == 0) {
				p++;
				break;
			}
			p++;
		}
		*p = 0;
		if (size > (int)strlen (s2) + 1)
			size = (int)strlen (s2) + 1;
		memcpy (s, s2, size);
		return s + size;
	} else {
		char s2[MAX_DPATH];
		char *s1;
		s1 = fgets (s2, size, z->f);
		if (!s1)
			return NULL;
		if (size > (int)strlen (s2) + 1)
			size = (int)strlen (s2) + 1;
		memcpy (s, s2, size);
		return s + size;
	}
}

int zfile_putc (int c, struct zfile *z)
{
	uae_u8 b = (uae_u8)c;
	return zfile_fwrite (&b, 1, 1, z) ? 1 : -1;
}

int zfile_getc (struct zfile *z)
{
	checkarchiveparent (z);
	int out = -1;
	if (z->data) {
		if (z->seek < z->size) {
			out = z->data[z->seek++];
		}
	} else {
		out = fgetc (z->f);
	}
	return out;
}

int zfile_ferror (struct zfile *z)
{
	return 0;
}

uae_u8 *zfile_getdata (struct zfile *z, uae_s64 offset, int len)
{
	uae_s64 pos;
	uae_u8 *b;
	if (len < 0) {
		zfile_fseek (z, 0, SEEK_END);
		len = zfile_ftell (z);
		zfile_fseek (z, 0, SEEK_SET);
	}
	b = xmalloc (uae_u8, len);
	if (z->data) {
		memcpy (b, z->data + offset, len);
	} else {
		pos = zfile_ftell (z);
		zfile_fseek (z, offset, SEEK_SET);
		zfile_fread (b, len, 1, z);
		zfile_fseek (z, pos, SEEK_SET);
	}
	return b;
}

int zfile_zuncompress (void *dst, int dstsize, struct zfile *src, int srcsize)
{
	z_stream zs;
	int v;
	uae_u8 inbuf[4096];
	int incnt;
	size_t left = 1;

    memset (&zs, 0, sizeof(zs));
	if (inflateInit (&zs) != Z_OK)
		return 0;
	zs.next_out = (Bytef*)dst;
	zs.avail_out = dstsize;
	incnt = 0;
	v = Z_OK;
	while (left && (v == Z_OK) && (zs.avail_out > 0) ) {
		if (zs.avail_in == 0) {
			left = (srcsize > incnt) ? srcsize - incnt : 0;
			if (left > 0) {
				if (left > sizeof (inbuf))
					left = sizeof (inbuf);
				zs.next_in = inbuf;
				zs.avail_in = zfile_fread (inbuf, 1, left, src);
				incnt += left;
			}
		}
		v = inflate (&zs, 0);
    }
    inflateEnd (&zs);
    return 0;
}

int zfile_zcompress (struct zfile *f, void *src, int size)
{
    int v;
    z_stream zs;
    uae_u8 outbuf[4096];

#ifdef WIN32
    if (!is_zlib)
	return 0;
#endif
    memset (&zs, 0, sizeof (zs));
	if (deflateInit (&zs, Z_DEFAULT_COMPRESSION) != Z_OK)
		return 0;
	zs.next_in = (Bytef*)src;
    zs.avail_in = size;
    v = Z_OK;
    while (v == Z_OK) {
		zs.next_out = outbuf;
		zs.avail_out = sizeof (outbuf);
		v = deflate(&zs, Z_NO_FLUSH | Z_FINISH);
		if (sizeof(outbuf) - zs.avail_out > 0)
		    zfile_fwrite (outbuf, 1, sizeof (outbuf) - zs.avail_out, f);
    }
    deflateEnd (&zs);
    return zs.total_out;
}

TCHAR *zfile_getname (struct zfile *f)
{
	return f ? f->name : NULL;
}

TCHAR *zfile_getfilename (struct zfile *f)
{
	int i;
	if (f->name == NULL)
		return NULL;
	for (i = _tcslen (f->name) - 1; i >= 0; i--) {
		if (f->name[i] == '\\' || f->name[i] == '/' || f->name[i] == ':') {
			i++;
			return &f->name[i];
		}
	}
	return f->name;
}

uae_u32 zfile_crc32 (struct zfile *f)
{
    uae_u8 *p;
    int pos, size;
    uae_u32 crc;

    if (!f)
		return 0;
    if (f->data)
		return get_crc32 (f->data, f->size);
    pos = zfile_ftell (f);
    zfile_fseek (f, 0, SEEK_END);
    size = zfile_ftell (f);
	p = xmalloc (uae_u8, size);
    if (!p)
		return 0;
    memset (p, 0, size);
    zfile_fseek (f, 0, SEEK_SET);
    zfile_fread (p, 1, size, f);
    zfile_fseek (f, pos, SEEK_SET);
    crc = get_crc32 (p, size);
	xfree (p);
    return crc;
}
static struct zvolume *zvolume_list;

static void recurparent (TCHAR *newpath, struct znode *zn, int recurse)
{
	if (zn->parent && (&zn->volume->root != zn->parent || zn->volume->parentz == NULL)) {
		if (&zn->volume->root == zn->parent && zn->volume->parentz == NULL && !_tcscmp (zn->name, zn->parent->name))
			goto end;
		recurparent (newpath, zn->parent, recurse);
	} else {
		struct zvolume *zv = zn->volume;
		if (zv->parentz && recurse)
			recurparent (newpath, zv->parentz, recurse);
	}
end:
	if (newpath[0])
		_tcscat (newpath, FSDB_DIR_SEPARATOR_S);
	_tcscat (newpath, zn->name);
}

static struct znode *znode_alloc (struct znode *parent, const TCHAR *name)
{
	TCHAR fullpath[MAX_DPATH];
	TCHAR tmpname[MAX_DPATH];
	struct znode *zn = xcalloc (struct znode, 1);
	struct znode *zn2;

	_tcscpy (tmpname, name);
	zn2 = parent->child;
	while (zn2) {
		if (!_tcscmp (zn2->name, tmpname)) {
			TCHAR *ext = _tcsrchr (tmpname, '.');
			if (ext && ext > tmpname + 2 && ext[-2] == '.') {
				ext[-1]++;
			} else if (ext) {
				memmove (ext + 2, ext, (_tcslen (ext) + 1) * sizeof (TCHAR));
				ext[0] = '.';
				ext[1] = '1';
			} else {
				int len = _tcslen (tmpname);
				tmpname[len] = '.';
				tmpname[len + 1] = '1';
				tmpname[len + 2] = 0;
			}
			zn2 = parent->child;
			continue;
		}
		zn2 = zn2->sibling;
	}

	fullpath[0] = 0;
	recurparent (fullpath, parent, false);
	_tcscat (fullpath, FSDB_DIR_SEPARATOR_S);
	_tcscat (fullpath, tmpname);
#ifdef ZFILE_DEBUG
	write_log (_T("znode_alloc vol='%s' parent='%s' name='%s'\n"), parent->volume->root.name, parent->name, name);
#endif
	zn->fullname = my_strdup (fullpath);
	zn->name = my_strdup (tmpname);
	zn->volume = parent->volume;
	zn->volume->last->next = zn;
	zn->prev = zn->volume->last;
	zn->volume->last = zn;
	return zn;
}

static struct znode *znode_alloc_child (struct znode *parent, const TCHAR *name)
{
	struct znode *zn = znode_alloc (parent, name);

	if (!parent->child) {
		parent->child = zn;
	} else {
		struct znode *pn = parent->child;
		while (pn->sibling)
			pn = pn->sibling;
		pn->sibling = zn;
	}
	zn->parent = parent;
	return zn;
}
static struct znode *znode_alloc_sibling (struct znode *sibling, const TCHAR *name)
{
	struct znode *zn = znode_alloc (sibling->parent, name);

	if (!sibling->sibling) {
		sibling->sibling = zn;
	} else {
		struct znode *pn = sibling->sibling;
		while (pn->sibling)
			pn = pn->sibling;
		pn->sibling = zn;
	}
	zn->parent = sibling->parent;
	return zn;
}

static void zvolume_addtolist (struct zvolume *zv)
{
	if (!zv)
		return;
	if (!zvolume_list) {
		zvolume_list = zv;
	} else {
		struct zvolume *v = zvolume_list;
		while (v->next)
			v = v->next;
		v->next = zv;
	}
}

static struct zvolume *zvolume_alloc_2 (const TCHAR *name, struct zfile *z, unsigned int id, void *handle, const TCHAR *volname)
{
	struct zvolume *zv = xcalloc (struct zvolume, 1);
	struct znode *root;
	uae_s64 pos;
	int i;

	root = &zv->root;
	zv->last = root;
	zv->archive = z;
	zv->handle = handle;
	zv->id = id;
	zv->blocks = 4;
	if (z)
		zv->zfdmask = z->zfdmask;
	root->volume = zv;
	root->type = ZNODE_DIR;
	i = 0;
	if (name[0] != '/' && name[0] != '\\' && _tcsncmp(name, _T(".\\"), 2) != 0 && _tcsncmp(name, _T("..\\"), 3) != 0) {
		if (_tcschr (name, ':') == 0) {
			for (i = _tcslen (name) - 1; i > 0; i--) {
				if (name[i] == FSDB_DIR_SEPARATOR) {
					i++;
					break;
				}
			}
		}
	}
	root->name = my_strdup (name + i);
	root->fullname = my_strdup (name);
#ifdef ZFILE_DEBUG
	write_log (_T("created zvolume: '%s' (%s)\n"), root->name, root->fullname);
#endif
	if (volname)
		zv->volumename = my_strdup (volname);
	if (z) {
		pos = zfile_ftell (z);
		zfile_fseek (z, 0, SEEK_END);
		zv->archivesize = zfile_ftell (z);
		zfile_fseek (z, pos, SEEK_SET);
	}
	return zv;
}
struct zvolume *zvolume_alloc (struct zfile *z, unsigned int id, void *handle, const TCHAR *volumename)
{
	return zvolume_alloc_2 (zfile_getname (z), z, id, handle, volumename);
}

struct zvolume *zvolume_alloc_nofile (const TCHAR *name, unsigned int id, void *handle, const TCHAR *volumename)
{
	return zvolume_alloc_2 (name, NULL, id, handle, volumename);
}

struct zvolume *zvolume_alloc_empty (struct zvolume *prev, const TCHAR *name)
{
	struct zvolume *zv = zvolume_alloc_2(name, 0, 0, 0, NULL);
	if (!zv)
		return NULL;
	if (prev)
		zv->zfdmask = prev->zfdmask;
	return zv;
}

static struct zvolume *get_zvolume (const TCHAR *path)
{
	struct zvolume *zv = zvolume_list;
	while (zv) {
		TCHAR *s = zfile_getname (zv->archive);
		if (!s)
			s = zv->root.name;
		if (_tcslen (path) >= _tcslen (s) && !memcmp (path, s, _tcslen (s) * sizeof (TCHAR)))
			return zv;
		zv = zv->next;
	}
	return NULL;
}

static struct zvolume *zfile_fopen_archive_ext (struct znode *parent, struct zfile *zf, int flags)
{
	struct zvolume *zv = NULL;
	TCHAR *name = zfile_getname (zf);
	TCHAR *ext;
	uae_u8 header[7];

	if (!name)
		return NULL;

	memset (header, 0, sizeof (header));
	zfile_fseek (zf, 0, SEEK_SET);
	zfile_fread (header, sizeof (header), 1, zf);
	zfile_fseek (zf, 0, SEEK_SET);

	ext = _tcsrchr (name, '.');
	if (ext != NULL) {
		ext++;
		if (flags & ZFD_ARCHIVE) {
#ifdef A_LHA
			if (strcasecmp (ext, _T("lha")) == 0 || strcasecmp (ext, _T("lzh")) == 0)
				zv = archive_directory_lha (zf);
#endif
#ifdef A_ZIP
			if (strcasecmp (ext, _T("zip")) == 0)
				zv = archive_directory_zip (zf);
#endif
#ifdef A_7Z
			if (strcasecmp (ext, _T("7z")) == 0)
				zv = archive_directory_7z (zf);
#endif
#ifdef A_LZX
			if (strcasecmp (ext, _T("lzx")) == 0)
				zv = archive_directory_lzx (zf);
#endif
#ifdef A_RAR
			if (strcasecmp (ext, _T("rar")) == 0)
				zv = archive_directory_rar (zf);
#endif
			if (strcasecmp (ext, _T("tar")) == 0)
				zv = archive_directory_tar (zf);
		}
		if (flags & ZFD_ADF) {
			if (strcasecmp (ext, _T("adf")) == 0 && !memcmp (header, "DOS", 3))
				zv = archive_directory_adf (parent, zf);
		}
		if (flags & ZFD_HD) {
			if (strcasecmp (ext, _T("hdf")) == 0)  {
				if (!memcmp (header, "RDSK", 4))
					zv = archive_directory_rdb (zf);
				else
					zv = archive_directory_adf (parent, zf);
			}
		}
	}
	return zv;
}

static struct znode *get_znode (struct zvolume *zv, const TCHAR *ppath, int);


static void zfile_fopen_archive_recurse2 (struct zvolume *zv, struct znode *zn, int flags)
{
	struct zvolume *zvnew;
	struct znode *zndir;
	TCHAR tmp[MAX_DPATH];

	_stprintf (tmp, _T("%s.DIR"), zn->fullname + _tcslen (zv->root.name) + 1);
	zndir = get_znode (zv, tmp, true);
	if (!zndir) {
		struct zarchive_info zai = { 0 };
		zvnew = zvolume_alloc_empty (zv, tmp);
		zvnew->parentz = zn;
		zai.name = tmp;
		zai.tv.tv_sec = zn->mtime.tv_sec;
		zai.tv.tv_usec = zn->mtime.tv_usec;
		zai.comment = zv->volumename;
		if (zn->flags < 0)
			zai.flags = zn->flags;
		zndir = zvolume_adddir_abs (zv, &zai);
		zndir->type = ZNODE_VDIR;
		zndir->vfile = zn;
		zndir->vchild = zvnew;
		zvnew->parent = zv;
		zndir->offset = zn->offset;
		zndir->offset2 = zn->offset2;
	}
}

static int zfile_fopen_archive_recurse (struct zvolume *zv, int flags)
{
	struct znode *zn;
	int i, added;

	added = 0;
	zn = zv->root.child;
	while (zn) {
		int done = 0;
		struct zfile *z;
		TCHAR *ext = _tcsrchr (zn->name, '.');
		if (ext && !zn->vchild && zn->type == ZNODE_FILE) {
			for (i = 0; !done && archive_extensions[i]; i++) {
				if (!strcasecmp (ext + 1, archive_extensions[i])) {
					zfile_fopen_archive_recurse2 (zv, zn, flags);
					done = 1;
				}
			}
		}
		if (!done) {
			z = archive_getzfile (zn, zv->method, 0);
			if (z && iszip (z))
				zfile_fopen_archive_recurse2 (zv, zn, flags);
		}
		zn = zn->next;
	}
	return 0;
}

static struct zvolume *prepare_recursive_volume (struct zvolume *zv, const TCHAR *path, int flags)
{
	struct zfile *zf = NULL;
	struct zvolume *zvnew = NULL;
	int done = 0;

#ifdef ZFILE_DEBUG
	write_log (_T("unpacking '%s'\n"), path);
#endif
	zf = zfile_open_archive (path, 0);
	if (!zf)
		goto end;
	zvnew = zfile_fopen_archive_ext (zv->parentz, zf, flags);
	if (!zvnew && !(flags & ZFD_NORECURSE)) {
#if 1
		zvnew = archive_directory_plain (zf);
		if (zvnew) {
			zfile_fopen_archive_recurse (zvnew, flags);
			done = 1;
		}
#else
		int rc;
		int index;
		struct zfile *zf2, *zf3;
		TCHAR oldname[MAX_DPATH];
		_tcscpy (oldname, zf->name);
		index = 0;
		for (;;) {
			zf3 = zfile_dup (zf);
			if (!zf3)
				break;
			zf2 = zuncompress (&zv->root, zf3, 0, ZFD_ALL, &rc, index);
			if (zf2) {
				zvnew = archive_directory_plain (zf2);
				if (zvnew) {
					zvnew->parent = zv->parent;
					zfile_fopen_archive_recurse (zvnew);
					done = 1;
				}
			} else {
				zfile_fclose (zf3);
				if (rc <= 0)
					break;
			}
			index++;
			break; // TODO
		}
#endif
	} else if (zvnew) {
		zvnew->parent = zv->parent;
		zfile_fopen_archive_recurse (zvnew, flags);
		done = 1;
	}
	if (!done)
		goto end;
	zfile_fclose_archive (zv);
	return zvnew;
end:
	write_log (_T("unpack '%s' failed\n"), path);
	zfile_fclose_archive (zvnew);
	zfile_fclose (zf);
	return NULL;
}

static struct znode *get_znode (struct zvolume *zv, const TCHAR *ppath, int recurse)
{
	struct znode *zn;
	TCHAR path[MAX_DPATH], zpath[MAX_DPATH];

	if (!zv)
		return NULL;
	_tcscpy (path, ppath);
	zn = &zv->root;
	while (zn) {
		zpath[0] = 0;
		recurparent (zpath, zn, recurse);
		if (zn->type == ZNODE_FILE) {
			if (!_tcsicmp (zpath, path))
				return zn;
		} else {
			size_t len = _tcslen (zpath);
			if (_tcslen (path) >= len && (path[len] == 0 || path[len] == FSDB_DIR_SEPARATOR) && !_tcsnicmp (zpath, path, len)) {
				if (path[len] == 0)
					return zn;
				if (zn->vchild) {
					/* jump to separate tree, recursive archives */
					struct zvolume *zvdeep = zn->vchild;
					if (zvdeep->archive == NULL) {
						TCHAR newpath[MAX_DPATH];
						newpath[0] = 0;
						recurparent (newpath, zn, recurse);
#ifdef ZFILE_DEBUG
						write_log (_T("'%s'\n"), newpath);
#endif
						zvdeep = prepare_recursive_volume (zvdeep, newpath, ZFD_ALL);
						if (!zvdeep) {
							write_log (_T("failed to unpack '%s'\n"), newpath);
							return NULL;
						}
						/* replace dummy empty volume with real volume */
						zn->vchild = zvdeep;
						zvdeep->parentz = zn;
					}
					zn = zvdeep->root.child;
				} else {
					zn = zn->child;
				}
				continue;
			}
		}
		zn = zn->sibling;
	}
	return NULL;
}

static void addvolumesize (struct zvolume *zv, uae_s64 size)
{
	unsigned int blocks = (size + 511) / 512;

	if (blocks == 0)
		blocks++;
	while (zv) {
		zv->blocks += blocks;
		zv->size += size;
		zv = zv->parent;
	}
}

struct znode *znode_adddir (struct znode *parent, const TCHAR *name, struct zarchive_info *zai)
{
	struct znode *zn = NULL;
	TCHAR path[MAX_DPATH];

	path[0] = 0;
	recurparent (path, parent, false);
	_tcscat (path, FSDB_DIR_SEPARATOR_S);
	_tcscat (path, name);
	zn = get_znode (parent->volume, path, false);
	if (zn)
		return zn;
	zn = znode_alloc_child (parent, name);
	zn->mtime.tv_sec = zai->tv.tv_sec;
	zn->mtime.tv_usec = zai->tv.tv_usec;
	zn->type = ZNODE_DIR;
	if (zai->comment)
		zn->comment = my_strdup (zai->comment);
	if (zai->flags < 0)
		zn->flags = zai->flags;
	addvolumesize (parent->volume, 0);
	return zn;
}

struct znode *zvolume_adddir_abs (struct zvolume *zv, struct zarchive_info *zai)
{
	struct znode *zn2;
	TCHAR *path = my_strdup (zai->name);
	TCHAR *p, *p2;
	int i;

	if (_tcslen (path) > 0) {
		/* remove possible trailing / or \ */
		TCHAR last;
		last = path[_tcslen (path) - 1];
		if (last == '/' || last == '\\')
			path[_tcslen (path) - 1] = 0;
	}
	zn2 = &zv->root;
	p = p2 = path;
	for (i = 0; path[i]; i++) {
		if (path[i] == '/' || path[i] == '\\') {
			path[i] = 0;
			zn2 = znode_adddir (zn2, p, zai);
			path[i] = FSDB_DIR_SEPARATOR;
			p = p2 = &path[i + 1];
		}
	}
	return znode_adddir (zn2, p, zai);
}

struct znode *zvolume_addfile_abs (struct zvolume *zv, struct zarchive_info *zai)
{
	struct znode *zn = NULL, *zn2;
	int i;
	TCHAR *path = my_strdup (zai->name);
	TCHAR *p, *p2;

	zn2 = &zv->root;
	p = p2 = path;
	for (i = 0; path[i]; i++) {
		if (path[i] == '/' || path[i] == '\\') {
			path[i] = 0;
			zn2 = znode_adddir (zn2, p, zai);
			path[i] = FSDB_DIR_SEPARATOR;
			p = p2 = &path[i + 1];
		}
	}
	if (p2) {
		zn = znode_alloc_child (zn2, p2);
		zn->size = zai->size;
		zn->type = ZNODE_FILE;
		zn->mtime.tv_sec = zai->tv.tv_sec;
		zn->mtime.tv_usec = zai->tv.tv_usec;
		if (zai->comment)
			zn->comment = my_strdup (zai->comment);
		zn->flags = zai->flags;
		addvolumesize (zn->volume, zai->size);
	}
	xfree (path);
	return zn;
}


void zfile_fclose_archive (struct zvolume *zv)
{
	struct znode *zn;
	struct zvolume *v;

	if (!zv)
		return;
	zn = &zv->root;
	while (zn) {
		struct znode *zn2 = zn->next;
		if (zn->vchild)
			zfile_fclose_archive (zn->vchild);
		xfree (zn->comment);
		xfree (zn->fullname);
		xfree (zn->name);
		zfile_fclose (zn->f);
		memset (zn, 0, sizeof (struct znode));
		if (zn != &zv->root)
			xfree (zn);
		zn = zn2;
	}
	archive_access_close (zv->handle, zv->id);
	if (zvolume_list == zv) {
		zvolume_list = zvolume_list->next;
	} else {
		v = zvolume_list;
		while (v) {
			if (v->next == zv) {
				v->next = zv->next;
				break;
			}
			v = v->next;
		}
	}
	xfree (zv);
}

struct zdirectory {
	TCHAR *parentpath;
	struct znode *first;
	struct znode *n;
	bool doclose;
	struct zvolume *zv;
	int cnt;
	int offset;
	TCHAR **filenames;
};

void zfile_closedir_archive (struct zdirectory *zd)
{
	if (!zd)
		return;
	zfile_fclose_archive (zd->zv);
	xfree (zd->parentpath);
	xfree (zd->filenames);
	xfree (zd);
}

uae_s64 zfile_lseek_archive (struct zfile *d, uae_s64 offset, int whence)
{
	uae_s64 old = zfile_ftell (d);
	if (old < 0 || zfile_fseek (d, offset, whence))
		return -1;
	return old;
}
uae_s64 zfile_fsize_archive (struct zfile *d)
{
	return zfile_size (d);
}

unsigned int zfile_read_archive (struct zfile *d, void *b, unsigned int size)
{
	return zfile_fread (b, 1, size, d);
}

struct zfile *zfile_open_archive (const TCHAR *path, int flags)
{
	struct zvolume *zv = get_zvolume (path);
	struct znode *zn = get_znode (zv, path, true);
	struct zfile *z;

	if (!zn)
		return 0;
	if (zn->f) {
		zfile_fseek (zn->f, 0, SEEK_SET);
		return zn->f;
	}
	if (zn->vfile)
		zn = zn->vfile;
	z = archive_getzfile (zn, zn->volume->id, 0);
	if (z)
		zfile_fseek (z, 0, SEEK_SET);
	zn->f = z;
	return zn->f;
}

#ifdef _CONSOLE
static TCHAR *zerror;
#define WRITE_LOG_BUF_SIZE 4096
void zfile_seterror (const TCHAR *format, ...)
{
	int count;
	if (!zerror) {
		TCHAR buffer[WRITE_LOG_BUF_SIZE];
		va_list parms;
		va_start (parms, format);
		count = _vsntprintf (buffer, WRITE_LOG_BUF_SIZE - 1, format, parms);
		zerror = my_strdup (buffer);
		va_end (parms);
	}
}
TCHAR *zfile_geterror (void)
{
	return zerror;
}
#else
void zfile_seterror (const TCHAR *format, ...)
{
}
#endif