/*
 * UAE - The Un*x Amiga Emulator
 *
 * Config file handling
 * This still needs some thought before it's complete...
 *
 * Copyright 1998 Brian King, Bernd Schmidt
 * Copyright 2006 Richard Drummond
 * Copyright 2008 Mustafa Tufan
 */

#pragma once
#ifndef CFGFILE_H
#define CFGFILE_H

#include "options.h"
#include "zfile.h"

extern int cfgfile_yesno (const TCHAR *option, const TCHAR *value, const TCHAR *name, int *location);
extern int cfgfile_intval (const TCHAR *option, const TCHAR *value, const TCHAR *name, int *location, int scale);
extern int cfgfile_strval (const TCHAR *option, const TCHAR *value, const TCHAR *name, int *location, const TCHAR *table[], int more);
extern int cfgfile_string (const TCHAR *option, const TCHAR *value, const TCHAR *name, TCHAR *location, int maxsz);

#if 0
int cfgfile_yesno (const TCHAR *option, const TCHAR *value, const TCHAR *name, bool *location);
int cfgfile_intval (const TCHAR *option, const TCHAR *value, const TCHAR *name, int *location, int scale);
int cfgfile_strval (const TCHAR *option, const TCHAR *value, const TCHAR *name, int *location, const TCHAR *table[], int more);
int cfgfile_string (const TCHAR *option, const TCHAR *value, const TCHAR *name, TCHAR *location, int maxsz);
#endif

extern TCHAR *cfgfile_subst_path (const TCHAR *path, const TCHAR *subst, const TCHAR *file);
int cfgfile_load (struct uae_prefs *p, const TCHAR *filename, int *type, int ignorelink, int userconfig);
//extern int cfgfile_load (struct uae_prefs *, const char *filename, int *);

//extern int cfgfile_load (struct uae_prefs *p, const char *filename, int *type, int ignorelink, int userconfig);
extern int cfgfile_save (const struct uae_prefs *, const char *filename, int);
extern void cfgfile_parse_line (struct uae_prefs *p, char *, int);
extern int cfgfile_parse_option (struct uae_prefs *p, char *option, char *value, int);
extern int cfgfile_get_description (const char *filename, char *description, char *hostlink, char *hardwarelink, int *type);

#if 0
int cfgfile_save (struct uae_prefs *p, const TCHAR *filename, int);
void cfgfile_parse_line (struct uae_prefs *p, TCHAR *, int);
void cfgfile_parse_lines (struct uae_prefs *p, const TCHAR *, int);
int cfgfile_parse_option (struct uae_prefs *p, TCHAR *option, TCHAR *value, int);
int cfgfile_get_description (const TCHAR *filename, TCHAR *description, TCHAR *hostlink, TCHAR *hardwarelink, int *type);
#endif

extern void cfgfile_show_usage (void);
extern uae_u32 cfgfile_uaelib (int mode, uae_u32 name, uae_u32 dst, uae_u32 maxlen);
extern uae_u32 cfgfile_uaelib_modify (uae_u32 mode, uae_u32 parms, uae_u32 size, uae_u32 out, uae_u32 outsize);
extern uae_u32 cfgfile_modify (uae_u32 index, TCHAR *parms, uae_u32 size, TCHAR *out, uae_u32 outsize);
extern void cfgfile_addcfgparam (TCHAR *);
//extern int built_in_prefs (struct uae_prefs *p, int model, int config, int compa, int romcheck);
int built_in_chipset_prefs (struct uae_prefs *p);
extern int cmdlineparser (const TCHAR *s, TCHAR *outp[], int max);
extern int cfgfile_configuration_change (int);

extern void cfgfile_dwrite (FILE *f, const TCHAR *option, const TCHAR *format,...);
extern void cfgfile_target_write (FILE *f, const TCHAR *option, const TCHAR *format,...);
extern void cfgfile_target_dwrite (FILE *f, const TCHAR *option, const TCHAR *format,...);

extern void cfgfile_write_bool (FILE *f, const TCHAR *option, int b);
extern void cfgfile_dwrite_bool (FILE *f,const  TCHAR *option, int b);
extern void cfgfile_target_write_bool (FILE *f, const TCHAR *option, int b);
extern void cfgfile_target_dwrite_bool (FILE *f, const TCHAR *option, int b);

extern void cfgfile_write_str (FILE *f, const TCHAR *option, const TCHAR *value);
extern void cfgfile_dwrite_str (FILE *f, const TCHAR *option, const TCHAR *value);
extern void cfgfile_target_write_str (FILE *f, const TCHAR *option, const TCHAR *value);
extern void cfgfile_target_dwrite_str (FILE *f, const TCHAR *option, const TCHAR *value);
#if 0
void cfgfile_dwrite (struct zfile *, const TCHAR *option, const TCHAR *format,...);
void cfgfile_target_write (struct zfile *, const TCHAR *option, const TCHAR *format,...);
void cfgfile_target_dwrite (struct zfile *, const TCHAR *option, const TCHAR *format,...);
void cfgfile_write_bool (struct zfile *f, const TCHAR *option, bool b);
void cfgfile_dwrite_bool (struct zfile *f,const  TCHAR *option, bool b);
void cfgfile_target_write_bool (struct zfile *f, const TCHAR *option, bool b);
void cfgfile_target_dwrite_bool (struct zfile *f, const TCHAR *option, bool b);
void cfgfile_write_str (struct zfile *f, const TCHAR *option, const TCHAR *value);
void cfgfile_dwrite_str (struct zfile *f, const TCHAR *option, const TCHAR *value);
void cfgfile_target_write_str (struct zfile *f, const TCHAR *option, const TCHAR *value);
void cfgfile_target_dwrite_str (struct zfile *f, const TCHAR *option, const TCHAR *value);
#endif

//void cfgfile_save_options (struct zfile *f, struct uae_prefs *p, int type);
extern int cfgfile_yesno2 (const TCHAR *option, const TCHAR *value, const TCHAR *name, int *location);
extern int cfgfile_doubleval (const TCHAR *option, const TCHAR *value, const TCHAR *name, double *location);
extern int cfgfile_intval_unsigned (const TCHAR *option, const TCHAR *value, const TCHAR *name, unsigned int *location, int scale);
extern int cfgfile_strboolval (const TCHAR *option, const TCHAR *value, const TCHAR *name, bool *location, const TCHAR *table[], int more);
extern int cfgfile_path_mp (const TCHAR *option, const TCHAR *value, const TCHAR *name, TCHAR *location, int maxsz, struct multipath *mp);
//extern int cfgfile_path (const TCHAR *option, const TCHAR *value, const TCHAR *name, TCHAR *location, int maxsz);
extern int cfgfile_multipath (const TCHAR *option, const TCHAR *value, const TCHAR *name, struct multipath *mp);
extern int cfgfile_rom (const TCHAR *option, const TCHAR *value, const TCHAR *name, TCHAR *location, int maxsz);


#endif // CFGFILE_H

