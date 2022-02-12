 /*
  * E-UAE - The portable Amiga Emulator
  *
  * Support for traps
  *
  * Copyright Richard Drummond 2005
  *
  * Based on code:
  * Copyright 1995, 1996 Bernd Schmidt
  * Copyright 1996 Ed Hanway
  */

#ifndef TRAPS_H
#define TRAPS_H

/*
 * Data passed to a trap handler
 */
<<<<<<< HEAD
typedef struct TrapContext
{
    struct regstruct regs;
} TrapContext;
=======
typedef struct TrapContext TrapContext;
>>>>>>> p-uae/v2.1.0


#define TRAPFLAG_NO_REGSAVE  1
#define TRAPFLAG_NO_RETVAL   2
#define TRAPFLAG_EXTRA_STACK 4
#define TRAPFLAG_DORET       8
<<<<<<< HEAD
=======
#define TRAPFLAG_UAERES     16
>>>>>>> p-uae/v2.1.0

/*
 * A function which handles a 68k trap
 */
<<<<<<< HEAD
typedef uae_u32 (*TrapHandler) (TrapContext *) REGPARAM;
=======
typedef uae_u32 (REGPARAM3 *TrapHandler) (TrapContext *) REGPARAM;
>>>>>>> p-uae/v2.1.0


/*
 * Interface with 68k interpreter
 */
<<<<<<< HEAD
extern void m68k_handle_trap (unsigned int trap_num, struct regstruct *) REGPARAM;

unsigned int define_trap (TrapHandler handler_func, int flags, const char *name);
=======
extern void m68k_handle_trap (unsigned int trap_num) REGPARAM;

unsigned int define_trap (TrapHandler handler_func, int flags, const TCHAR *name);
uaecptr find_trap (const TCHAR *name);
>>>>>>> p-uae/v2.1.0

/*
 * Call a 68k Library function from an extended trap
 */
extern uae_u32 CallLib (TrapContext *context, uaecptr library_base, uae_s16 func_offset);
<<<<<<< HEAD
=======
extern uae_u32 CallFunc (TrapContext *context, uaecptr func);
>>>>>>> p-uae/v2.1.0

/*
 * Initialization
 */
void init_traps (void);
void init_extended_traps (void);

<<<<<<< HEAD
=======
#define deftrap(f) define_trap((f), 0, "")
#define deftrap2(f, mode, str) define_trap((f), (mode), (str))
#define deftrapres(f, mode, str) define_trap((f), (mode | TRAPFLAG_UAERES), (str))

>>>>>>> p-uae/v2.1.0
#endif
