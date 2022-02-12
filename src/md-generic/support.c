 /*
  * UAE - The Un*x Amiga Emulator
  *
  * Miscellaneous machine dependent support functions and definitions
  *
  * Copyright 1996 Bernd Schmidt
  * Copyright 2004-2005 Richard Drummond
  */

#include "sysconfig.h"
#include "sysdeps.h"

#include "options.h"
#include "machdep/m68k.h"

<<<<<<< HEAD
void machdep_init (void)
{
=======
struct flag_struct regflags;

int machdep_init (void)
{
	return 1;
>>>>>>> p-uae/v2.1.0
}

/*
 * Handle processor-specific cfgfile options
 */
void machdep_save_options (FILE *f, const struct uae_prefs *p)
{
}

int machdep_parse_option (struct uae_prefs *p, const char *option, const char *value)
{
    return 0;
}

void machdep_default_options (struct uae_prefs *p)
{
}
