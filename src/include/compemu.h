/*
 *  include/compemu.h - Public interface and definitions
 *
 *  Original 68040 JIT compiler for UAE, copyright 2000-2002 Bernd Meyer
 *
 *  Adaptation for Basilisk II and improvements, copyright 2000-2005
 *    Gwenole Beauchesne
 *
 *  Basilisk II (C) 1997-2008 Christian Bauer
 *
 * Updated to a unified interface (C) 2013 Sven Eden
 */

#pragma once
#ifndef COMPEMU_H
#define COMPEMU_H

//#include "flags_x86.h"

/** REMOVEME:
  * We have a unified type for pointer values
  * of the correct size already.
  * Include sysdeps.h instead and use uaecptr.
**/
#if 0
#ifdef __x86_64__
typedef uae_u64 uintptr;
#else
typedef uae_u32 uintptr;
#endif
#endif // 0

#include "sysdeps.h"

/* Now that we do block chaining, and also have linked lists on each tag,
   TAGMASK can be much smaller and still do its job. Saves several megs
   of memory! */
#define DISTRUST_CONSISTENT_MEM 0
#define TAGMASK 0x000fffff
#define TAGSIZE (TAGMASK+1)
#define MAXRUN 1024

extern uae_u8* start_pc_p;
extern uae_u32 start_pc;

#define cacheline(x) ((PTR_TO_UINT32(x)) & TAGMASK)

typedef struct {
  uae_u16* location;
  uae_u8  cycles;
  uae_u8  specmem;
  uae_u8  dummy2;
  uae_u8  dummy3;
} cpu_history;

struct blockinfo_t;

typedef union {
    cpuop_func* handler;
    struct blockinfo_t* bi;
} cacheline;

extern signed long pissoff;

#define USE_OPTIMIZER 0
#define USE_LOW_OPTIMIZER 0
#define USE_ALIAS 1
#define USE_F_ALIAS 1
#define USE_SOFT_FLUSH 1
#define USE_OFFSET 1
#define COMP_DEBUG 1

#if COMP_DEBUG
#define Dif(x) if (x)
#else
#define Dif(x) if (0)
#endif

#define SCALE 2
#define MAXCYCLES (1000 * CYCLE_UNIT)
#define MAXREGOPT 65536

#define BYTES_PER_INST 10240  /* paranoid ;-) */
#define LONGEST_68K_INST 16 /* The number of bytes the longest possible
			       68k instruction takes */
#define MAX_CHECKSUM_LEN 2048 /* The maximum size we calculate checksums
				 for. Anything larger will be flushed
				 unconditionally even with SOFT_FLUSH */
#define MAX_HOLD_BI 3  /* One for the current block, and up to two
			  for jump targets */

#define INDIVIDUAL_INST 0
#define FLAG_C    0x0010
#define FLAG_V    0x0008
#define FLAG_Z    0x0004
#define FLAG_N    0x0002
#define FLAG_X    0x0001
#define FLAG_CZNV (FLAG_C | FLAG_Z | FLAG_N | FLAG_V)
#define FLAG_ZNV  (FLAG_Z | FLAG_N | FLAG_V)

#define KILLTHERAT 1  /* Set to 1 to avoid some partial_rat_stalls */


/** REMOVEME:
  * X86_ASSEMBLY is nowhere set. And I tried it out, the outcome is not nice.
  * Same is with USE_PUSH_POP.
  * All those do is to set whether the pushall_* popall_ methods push/pop all
  * registers or not. If not, the executed functions do nothing about it anyway.
  * Further, with X86_ASSEMBLY DitherLine() in gfxutil.c is not compiled and is
  * the missing in the output.
  * Oh, and if it is defined, fpp.c:native_set_fpucw() adds a line
  * 	__asm__ ("fldcw %0" : : "m" (*&x87_cw));
  * which isn't needed anyway.
**/
/* Whether to preserve registers across calls to JIT compiled routines */
#if defined(X86_ASSEMBLY)
#define USE_PUSH_POP 0
#else
#define USE_PUSH_POP 1
#endif

#define N_REGS 8  /* really only 7, but they are numbered 0,1,2,3,5,6,7 */
#define N_FREGS 6 /* That leaves us two positions on the stack to play with */

/* Functions exposed to newcpu, or to what was moved from newcpu.c to
 * compemu_support.c */
void init_comp (void);
void flush (int save_regs);
void small_flush (int save_regs);
void set_target (uae_u8* t);
void freescratch (void);
void build_comp (void);
void set_cache_state (int enabled);
int get_cache_state (void);
uae_u32 get_jitted_size (void);
#ifdef JIT
void flush_icache (uaecptr ptr, int n);
#endif
void alloc_cache (void);
void compile_block (cpu_history *pc_hist, int blocklen, int totcyles);
int check_for_cache_miss (void);


#define scaled_cycles(x) (currprefs.m68k_speed<0?(((x)/SCALE)?(((x)/SCALE<MAXCYCLES?((x)/SCALE):MAXCYCLES)):1):(x))


extern uae_u32 needed_flags;
//extern cacheline cache_tags[];
extern uae_u8* comp_pc_p;
extern uae_u8* pushall_call_handler;

#define VREGS 32
#define VFREGS 16

#define INMEM 1
#define CLEAN 2
#define DIRTY 3
#define UNDEF 4
#define ISCONST 5

typedef struct {
  uae_u32* mem;
  uae_u32 val;
  uae_u8 is_swapped;
  uae_u8 status;
  uae_u8 realreg;
  uae_u8 realind; /* The index in the holds[] array */
  uae_u8 needflush;
  uae_u8 validsize;
  uae_u8 dirtysize;
  uae_u8 dummy;
} reg_status;

typedef struct {
  uae_u32* mem;
  double val;
  uae_u8 status;
  uae_u8 realreg;
  uae_u8 realind;
  uae_u8 needflush;
} freg_status;

typedef struct {
    uae_u8 use_flags;
    uae_u8 set_flags;
    uae_u8 is_jump;
    uae_u8 is_addx;
    uae_u8 is_const_jump;
} op_properties;

extern op_properties prop[65536];

STATIC_INLINE int end_block(uae_u16 opcode)
{
    return prop[opcode].is_jump ||
  (prop[opcode].is_const_jump
#ifdef JIT
  && !currprefs.comp_constjump
#endif
  );
}

#define PC_P 16
#define FLAGX 17
#define FLAGTMP 18
#define NEXT_HANDLER 19
#define S1 20
#define S2 21
#define S3 22
#define S4 23
#define S5 24
#define S6 25
#define S7 26
#define S8 27
#define S9 28
#define S10 29
#define S11 30
#define S12 31

#define FP_RESULT 8
#define FS1 9
#define FS2 10
#define FS3 11

typedef struct {
  uae_u32 touched;
  uae_s8 holds[VREGS];
  uae_u8 nholds;
  uae_u8 canbyte;
  uae_u8 canword;
  uae_u8 locked;
} n_status;

typedef struct {
    uae_s8 holds;
    uae_u8 validsize;
    uae_u8 dirtysize;
} n_smallstatus;

typedef struct {
  uae_u32 touched;
  uae_s8 holds[VFREGS];
  uae_u8 nholds;
  uae_u8 locked;
} fn_status;

/* For flag handling */
#define NADA 1
#define TRASH 2
#define VALID 3

/* needflush values */
#define NF_SCRATCH   0
#define NF_TOMEM     1
#define NF_HANDLER   2

typedef struct {
    /* Integer part */
    reg_status state[VREGS];
    n_status   nat[N_REGS];
    uae_u32 flags_on_stack;
    uae_u32 flags_in_flags;
    uae_u32 flags_are_important;
    /* FPU part */
    freg_status fate[VFREGS];
    fn_status   fat[N_FREGS];

    /* x86 FPU part */
    uae_s8 spos[N_FREGS];
    uae_s8 onstack[6];
    uae_s8 tos;
} bigstate;

typedef struct {
    /* Integer part */
    n_smallstatus  nat[N_REGS];
} smallstate;

//extern bigstate live;
extern int touchcnt;


#define IMM uae_s32
#define RR1  uae_u32
#define RR2  uae_u32
#define RR4  uae_u32
/*
  R1, R2, R4 collides with ARM registers defined in ucontext
#define R1  uae_u32
#define R2  uae_u32
#define R4  uae_u32
*/
#define W1  uae_u32
#define W2  uae_u32
#define W4  uae_u32
#define RW1 uae_u32
#define RW2 uae_u32
#define RW4 uae_u32
#define MEMR uae_u32
#define MEMW uae_u32
#define MEMRW uae_u32

#define FW   uae_u32
#define FR   uae_u32
#define FRW  uae_u32

#define MIDFUNC(nargs,func,args) void func args
#define MENDFUNC(nargs,func,args)
#define COMPCALL(func) func

#define LOWFUNC(flags,mem,nargs,func,args) STATIC_INLINE void func args
#define LENDFUNC(flags,mem,nargs,func,args)

#if USE_OPTIMIZER
#define REGALLOC_O 2
#define PEEPHOLE_O 3 /* Has to be >= REGALLOC */
#define DECLARE(func) extern void func; extern void do_##func
#else
#define REGALLOC_O 2000000
#define PEEPHOLE_O 2000000
#define DECLARE(func) extern void func
#endif

/* What we expose to the outside */
#define DECLARE_MIDFUNC(func) extern void func

#if defined(__x86_64__) || defined(__i386__)
#include "compemu_midfunc_x86.h"
#endif

#ifdef __arm__
#include "compemu_midfunc_arm.h"
#endif


#undef DECLARE_MIDFUNC

extern int failure;
#define FAIL(x) do { failure|=x; } while (0)

/* Convenience functions exposed to gencomp */
extern uae_u32 m68k_pc_offset;
void readbyte (int address, int dest, int tmp);
void readword (int address, int dest, int tmp);
void readlong (int address, int dest, int tmp);
void writebyte (int address, int source, int tmp);
void writeword (int address, int source, int tmp);
void writelong (int address, int source, int tmp);
void writeword_clobber (int address, int source, int tmp);
void writelong_clobber (int address, int source, int tmp);
void get_n_addr (int address, int dest, int tmp);
void get_n_addr_jmp (int address, int dest, int tmp);
void calc_disp_ea_020 (int base, uae_u32 dp, int target, int tmp);
int kill_rodent (int r);
void sync_m68k_pc (void);
uae_u32 get_const (int r);
int is_const (int r);
void register_branch (uae_u32 not_taken, uae_u32 taken, uae_u8 cond);
void empty_optimizer (void);

#define comp_get_ibyte(o) do_get_mem_byte((uae_u8 *)(comp_pc_p + (o) + 1))
#define comp_get_iword(o) do_get_mem_word((uae_u16 *)(comp_pc_p + (o)))
#define comp_get_ilong(o) do_get_mem_long((uae_u32 *)(comp_pc_p + (o)))

struct blockinfo_t;

typedef struct dep_t {
  uae_u32*            jmp_off;
  struct blockinfo_t* target;
  struct dep_t**      prev_p;
  struct dep_t*       next;
} dependency;

typedef struct blockinfo_t {
    uae_s32 count;
    cpuop_func* direct_handler_to_use;
    cpuop_func* handler_to_use;
    /* The direct handler does not check for the correct address */

    cpuop_func* handler;
    cpuop_func* direct_handler;

    cpuop_func* direct_pen;
    cpuop_func* direct_pcc;

    uae_u8* nexthandler;
    uae_u8* pc_p;

    uae_u32 c1;
    uae_u32 c2;
    uae_u32 len;

    struct blockinfo_t* next_same_cl;
    struct blockinfo_t** prev_same_cl_p;
    struct blockinfo_t* next;
    struct blockinfo_t** prev_p;

    uae_u32 min_pcp;
    uae_u8 optlevel;
    uae_u8 needed_flags;
    uae_u8 status;
    uae_u8 havestate;

    dependency  dep[2];  /* Holds things we depend on */
    dependency* deplist; /* List of things that depend on this */
    smallstate  env;
} blockinfo;

#define BI_NEW 0
#define BI_COUNTING 1
#define BI_TARGETTED 2

typedef struct {
    uae_u8 type;
    uae_u8 reg;
    uae_u32 next;
} regacc;

void execute_normal(void);
void exec_nostats(void);
void do_nothing(void);

void comp_fdbcc_opp (uae_u32 opcode, uae_u16 extra);
void comp_fscc_opp (uae_u32 opcode, uae_u16 extra);
void comp_ftrapcc_opp (uae_u32 opcode, uaecptr oldpc);
void comp_fbcc_opp (uae_u32 opcode);
void comp_fsave_opp (uae_u32 opcode);
void comp_frestore_opp (uae_u32 opcode);
void comp_fpp_opp (uae_u32 opcode, uae_u16 extra);

#endif // COMPEMU_H
