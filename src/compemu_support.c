<<<<<<< HEAD
=======

>>>>>>> p-uae/v2.1.0
#define writemem_special writemem
#define readmem_special  readmem

#define USE_MATCHSTATE 0
<<<<<<< HEAD
#define setzflg_uses_bsf 0
#include "sysconfig.h"
#include "sysdeps.h"
=======
#include "sysconfig.h"
#include "sysdeps.h"

#if defined(JIT)

>>>>>>> p-uae/v2.1.0
#include "options.h"
#include "events.h"
#include "include/memory.h"
#include "custom.h"
#include "newcpu.h"
#include "comptbl.h"
#include "compemu.h"
#include "uae_endian.h"

<<<<<<< HEAD
// %%% BRIAN KING WAS HERE %%%
extern int canbang;

compop_func *compfunctbl[65536];
compop_func *nfcompfunctbl[65536];
#ifdef NOFLAGS_SUPPORT
cpuop_func *nfcpufunctbl[65536];
=======
#define NATMEM_OFFSETX (uae_u32)NATMEM_OFFSET

// %%% BRIAN KING WAS HERE %%%
extern int canbang;
#include <sys/mman.h>
extern void jit_abort(const TCHAR*,...);
compop_func *compfunctbl[65536];
compop_func *nfcompfunctbl[65536];
#ifdef NOFLAGS_SUPPORT
compop_func *nfcpufunctbl[65536];
>>>>>>> p-uae/v2.1.0
#endif
uae_u8* comp_pc_p;

uae_u8* start_pc_p;
uae_u32 start_pc;
uae_u32 current_block_pc_p;
uae_u32 current_block_start_target;
uae_u32 needed_flags;
static uae_u32 next_pc_p;
static uae_u32 taken_pc_p;
static int     branch_cc;
int segvcount=0;
int soft_flush_count=0;
int hard_flush_count=0;
int compile_count=0;
int checksum_count=0;
static uae_u8* current_compile_p=NULL;
static uae_u8* max_compile_start;
uae_u8* compiled_code=NULL;
static uae_s32 reg_alloc_run;

<<<<<<< HEAD
static int align_loops = 32;// Align the start of loops
static int align_jumps = 32;// Align the start of jumps

       void *pushall_call_handler;
static void *popall_do_nothing;
static void *popall_exec_nostats;
static void *popall_execute_normal;
static void *popall_cache_miss;
static void *popall_recompile_block;
static void *popall_check_checksum;

static uae_u8 *popallspace;
=======
static int		lazy_flush		= 1;	// Flag: lazy translation cache invalidation
static int		avoid_fpu		= 1;	// Flag: compile FPU instructions ?
static int		have_cmov		= 0;	// target has CMOV instructions ?
static int		have_rat_stall		= 1;	// target has partial register stalls ?
const int		tune_alignment		= 1;	// Tune code alignments for running CPU ?
const int		tune_nop_fillers	= 1;	// Tune no-op fillers for architecture

static int		setzflg_uses_bsf	= 0;	// setzflg virtual instruction can use native BSF instruction correctly?
static int align_loops = 32;// Align the start of loops
static int align_jumps = 32;// Align the start of jumps

void* pushall_call_handler=NULL;
static void* popall_do_nothing=NULL;
static void* popall_exec_nostats=NULL;
static void* popall_execute_normal=NULL;
static void* popall_cache_miss=NULL;
static void* popall_recompile_block=NULL;
static void* popall_check_checksum=NULL;

extern uae_u32 oink;
extern unsigned long foink3;
extern unsigned long foink;
>>>>>>> p-uae/v2.1.0

/* The 68k only ever executes from even addresses. So right now, we
   waste half the entries in this array
   UPDATE: We now use those entries to store the start of the linked
   lists that we maintain for each hash result. */
cacheline cache_tags[TAGSIZE];
<<<<<<< HEAD
static int letit;
blockinfo* hold_bi[MAX_HOLD_BI];
blockinfo* active;
blockinfo* dormant;
=======
static int letit=0;
static blockinfo* hold_bi[MAX_HOLD_BI];
static blockinfo* active;
static blockinfo* dormant;
>>>>>>> p-uae/v2.1.0

op_properties prop[65536];

#ifdef NOFLAGS_SUPPORT
/* 68040 */
<<<<<<< HEAD
extern const struct cputbl op_smalltbl_0_nf[];
=======
extern const struct comptbl op_smalltbl_0_nf[];
>>>>>>> p-uae/v2.1.0
#endif
extern const struct comptbl op_smalltbl_0_comp_nf[];
extern const struct comptbl op_smalltbl_0_comp_ff[];
#ifdef NOFLAGS_SUPPORT
/* 68020 + 68881 */
extern const struct cputbl op_smalltbl_1_nf[];
/* 68020 */
extern const struct cputbl op_smalltbl_2_nf[];
/* 68010 */
extern const struct cputbl op_smalltbl_3_nf[];
/* 68000 */
extern const struct cputbl op_smalltbl_4_nf[];
<<<<<<< HEAD
#ifdef CPUEMU_5
/* 68000 slow but compatible.  */
extern const struct cputbl op_smalltbl_5_nf[];
#endif
#endif

static void flush_icache_hard(int n);
=======
/* 68000 slow but compatible.  */
extern const struct cputbl op_smalltbl_5_nf[];
#endif

static void flush_icache_hard(uae_u32 ptr, int n);
>>>>>>> p-uae/v2.1.0



bigstate live;
<<<<<<< HEAD
smallstate empty_ss;
smallstate default_ss;
=======
static smallstate empty_ss;
static smallstate default_ss;
>>>>>>> p-uae/v2.1.0
static int optlev;

static int writereg(int r, int size);
static void unlock(int r);
static void setlock(int r);
static int readreg_specific(int r, int size, int spec);
static int writereg_specific(int r, int size, int spec);
static void prepare_for_call_1(void);
static void prepare_for_call_2(void);
static void align_target(uae_u32 a);

<<<<<<< HEAD
=======
static uae_s32 nextused[VREGS];

static uae_u8 *popallspace;

>>>>>>> p-uae/v2.1.0
#ifdef NATMEM_OFFSET
# ifndef WIN32
struct sigaction *saved_handler;
# endif
#endif

uae_u32 m68k_pc_offset;

<<<<<<< HEAD
/* Some arithmetic ooperations can be optimized away if the operands
=======
/* Some arithmetic operations can be optimized away if the operands
>>>>>>> p-uae/v2.1.0
   are known to be constant. But that's only a good idea when the
   side effects they would have on the flags are not important. This
   variable indicates whether we need the side effects or not
*/
uae_u32 needflags=0;

/* Flag handling is complicated.

   x86 instructions create flags, which quite often are exactly what we
   want. So at times, the "68k" flags are actually in the x86 flags.

   Then again, sometimes we do x86 instructions that clobber the x86
   flags, but don't represent a corresponding m68k instruction. In that
   case, we have to save them.

   We used to save them to the stack, but now store them back directly
   into the regflags.cznv of the traditional emulation. Thus some odd
   names.

   So flags can be in either of two places (used to be three; boy were
   things complicated back then!); And either place can contain either
   valid flags or invalid trash (and on the stack, there was also the
   option of "nothing at all", now gone). A couple of variables keep
   track of the respective states.

   To make things worse, we might or might not be interested in the flags.
   by default, we are, but a call to dont_care_flags can change that
   until the next call to live_flags. If we are not, pretty much whatever
   is in the register and/or the native flags is seen as valid.
*/


STATIC_INLINE blockinfo* get_blockinfo(uae_u32 cl)
{
    return cache_tags[cl+1].bi;
}

STATIC_INLINE blockinfo* get_blockinfo_addr(void* addr)
{
    blockinfo*  bi=get_blockinfo(cacheline(addr));

    while (bi) {
	if (bi->pc_p==addr)
	    return bi;
	bi=bi->next_same_cl;
    }
    return NULL;
}


/*******************************************************************
 * All sorts of list related functions for all of the lists        *
 *******************************************************************/

STATIC_INLINE void remove_from_cl_list(blockinfo* bi)
{
    uae_u32 cl=cacheline(bi->pc_p);

    if (bi->prev_same_cl_p)
	*(bi->prev_same_cl_p)=bi->next_same_cl;
    if (bi->next_same_cl)
	bi->next_same_cl->prev_same_cl_p=bi->prev_same_cl_p;
    if (cache_tags[cl+1].bi)
	cache_tags[cl].handler=cache_tags[cl+1].bi->handler_to_use;
    else
<<<<<<< HEAD
	cache_tags[cl].handler=popall_execute_normal;
=======
	cache_tags[cl].handler=(cpuop_func*)popall_execute_normal;
>>>>>>> p-uae/v2.1.0
}

STATIC_INLINE void remove_from_list(blockinfo* bi)
{
    if (bi->prev_p)
	*(bi->prev_p)=bi->next;
    if (bi->next)
	bi->next->prev_p=bi->prev_p;
}

STATIC_INLINE void remove_from_lists(blockinfo* bi)
{
    remove_from_list(bi);
    remove_from_cl_list(bi);
}

STATIC_INLINE void add_to_cl_list(blockinfo* bi)
{
    uae_u32 cl=cacheline(bi->pc_p);

    if (cache_tags[cl+1].bi)
	cache_tags[cl+1].bi->prev_same_cl_p=&(bi->next_same_cl);
    bi->next_same_cl=cache_tags[cl+1].bi;

    cache_tags[cl+1].bi=bi;
    bi->prev_same_cl_p=&(cache_tags[cl+1].bi);

    cache_tags[cl].handler=bi->handler_to_use;
}

STATIC_INLINE void raise_in_cl_list(blockinfo* bi)
{
    remove_from_cl_list(bi);
    add_to_cl_list(bi);
}

STATIC_INLINE void add_to_active(blockinfo* bi)
{
    if (active)
	active->prev_p=&(bi->next);
    bi->next=active;

    active=bi;
    bi->prev_p=&active;
}

STATIC_INLINE void add_to_dormant(blockinfo* bi)
{
    if (dormant)
	dormant->prev_p=&(bi->next);
    bi->next=dormant;

    dormant=bi;
    bi->prev_p=&dormant;
}

STATIC_INLINE void remove_dep(dependency* d)
{
    if (d->prev_p)
	*(d->prev_p)=d->next;
    if (d->next)
	d->next->prev_p=d->prev_p;
    d->prev_p=NULL;
    d->next=NULL;
}

/* This block's code is about to be thrown away, so it no longer
   depends on anything else */
STATIC_INLINE void remove_deps(blockinfo* bi)
{
    remove_dep(&(bi->dep[0]));
    remove_dep(&(bi->dep[1]));
}

STATIC_INLINE void adjust_jmpdep(dependency* d, void* a)
{
<<<<<<< HEAD
    *(d->jmp_off) = (uae_uintptr)a - ((uae_uintptr)d->jmp_off+4);
=======
    *(d->jmp_off)=(uae_u32)a-((uae_u32)d->jmp_off+4);
>>>>>>> p-uae/v2.1.0
}

/********************************************************************
 * Soft flush handling support functions                            *
 ********************************************************************/

STATIC_INLINE void set_dhtu(blockinfo* bi, void* dh)
{
<<<<<<< HEAD
    //printf("bi is %p\n",bi);
    if (dh!=bi->direct_handler_to_use) {
	dependency* x=bi->deplist;
	//printf("bi->deplist=%p\n",bi->deplist);
	while (x) {
	    //printf("x is %p\n",x);
	    //printf("x->next is %p\n",x->next);
	    //printf("x->prev_p is %p\n",x->prev_p);

	    if (x->jmp_off) {
		adjust_jmpdep(x,dh);
	    }
	    x=x->next;
	}
	bi->direct_handler_to_use=dh;
=======
	//write_log ("JIT: bi is %p\n",bi);
    if (dh!=bi->direct_handler_to_use) {
		dependency* x=bi->deplist;
		//write_log ("JIT: bi->deplist=%p\n",bi->deplist);
		while (x) {
			//write_log ("JIT: x is %p\n",x);
			//write_log ("JIT: x->next is %p\n",x->next);
			//write_log ("JIT: x->prev_p is %p\n",x->prev_p);

			if (x->jmp_off) {
				adjust_jmpdep(x,dh);
			}
			x=x->next;
		}
		bi->direct_handler_to_use=(cpuop_func*)dh;
>>>>>>> p-uae/v2.1.0
    }
}

STATIC_INLINE void invalidate_block(blockinfo* bi)
{
    int i;

    bi->optlevel=0;
    bi->count=currprefs.optcount[0]-1;
    bi->handler=NULL;
<<<<<<< HEAD
    bi->handler_to_use=popall_execute_normal;
=======
    bi->handler_to_use=(cpuop_func*)popall_execute_normal;
>>>>>>> p-uae/v2.1.0
    bi->direct_handler=NULL;
    set_dhtu(bi,bi->direct_pen);
    bi->needed_flags=0xff;

    for (i=0;i<2;i++) {
<<<<<<< HEAD
	bi->dep[i].jmp_off=NULL;
	bi->dep[i].target=NULL;
=======
		bi->dep[i].jmp_off=NULL;
		bi->dep[i].target=NULL;
>>>>>>> p-uae/v2.1.0
    }
    remove_deps(bi);
}

<<<<<<< HEAD
STATIC_INLINE void create_jmpdep(blockinfo* bi, int i, uae_u32* jmpaddr, uae_uintptr target)
=======
STATIC_INLINE void create_jmpdep(blockinfo* bi, int i, uae_u32* jmpaddr, uae_u32 target)
>>>>>>> p-uae/v2.1.0
{
    blockinfo*  tbi=get_blockinfo_addr((void*)target);

    Dif(!tbi) {
<<<<<<< HEAD
	write_log ("JIT: Could not create jmpdep!\n");
	abort();
=======
		jit_abort ("JIT: Could not create jmpdep!\n");
>>>>>>> p-uae/v2.1.0
    }
    bi->dep[i].jmp_off=jmpaddr;
    bi->dep[i].target=tbi;
    bi->dep[i].next=tbi->deplist;
    if (bi->dep[i].next)
<<<<<<< HEAD
	bi->dep[i].next->prev_p=&(bi->dep[i].next);
=======
		bi->dep[i].next->prev_p=&(bi->dep[i].next);
>>>>>>> p-uae/v2.1.0
    bi->dep[i].prev_p=&(tbi->deplist);
    tbi->deplist=&(bi->dep[i]);
}

STATIC_INLINE void big_to_small_state(bigstate* b, smallstate* s)
{
    int i;
    int count=0;

    for (i=0;i<N_REGS;i++) {
	s->nat[i].validsize=0;
	s->nat[i].dirtysize=0;
	if (b->nat[i].nholds) {
	    int index=b->nat[i].nholds-1;
	    int r=b->nat[i].holds[index];
	    s->nat[i].holds=r;
	    s->nat[i].validsize=b->state[r].validsize;
	    s->nat[i].dirtysize=b->state[r].dirtysize;
	    count++;
	}
    }
<<<<<<< HEAD
    write_log ("JIT: count=%d\n", count);
    for (i=0;i<N_REGS;i++) {  // FIXME --- don't do dirty yet
	s->nat[i].dirtysize=0;
=======
	write_log ("JIT: count=%d\n",count);
    for (i=0;i<N_REGS;i++) {  // FIXME --- don't do dirty yet
		s->nat[i].dirtysize=0;
>>>>>>> p-uae/v2.1.0
    }
}

STATIC_INLINE void attached_state(blockinfo* bi)
{
    bi->havestate=1;
    if (bi->direct_handler_to_use==bi->direct_handler)
<<<<<<< HEAD
	set_dhtu(bi,bi->direct_pen);
=======
		set_dhtu(bi,bi->direct_pen);
>>>>>>> p-uae/v2.1.0
    bi->direct_handler=bi->direct_pen;
    bi->status=BI_TARGETTED;
}

STATIC_INLINE blockinfo* get_blockinfo_addr_new(void* addr, int setstate)
{
    blockinfo*  bi=get_blockinfo_addr(addr);
    int i;

<<<<<<< HEAD
    if (!bi) {
	for (i=0;i<MAX_HOLD_BI && !bi;i++) {
	    if (hold_bi[i]) {
		bi=hold_bi[i];
		hold_bi[i]=NULL;
		bi->pc_p=addr;
		invalidate_block(bi);
		add_to_active(bi);
		add_to_cl_list(bi);
	    }
	}
    }
    if (!bi) {
	write_log ("JIT: Looking for blockinfo, can't find free one\n");
	abort();
=======
#if USE_OPTIMIZER
    if (reg_alloc_run)
		return NULL;
#endif
    if (!bi) {
	for (i=0;i<MAX_HOLD_BI && !bi;i++) {
	    if (hold_bi[i]) {
			uae_u32 cl=cacheline(addr);

			bi=hold_bi[i];
			hold_bi[i]=NULL;
			bi->pc_p=(uae_u8*)addr;
			invalidate_block(bi);
			add_to_active(bi);
			add_to_cl_list(bi);

		    }
		}
    }
    if (!bi) {
		jit_abort ("JIT: Looking for blockinfo, can't find free one\n");
>>>>>>> p-uae/v2.1.0
    }

#if USE_MATCHSTATE
    if (setstate &&
<<<<<<< HEAD
	!bi->havestate) {
	big_to_small_state(&live,&(bi->env));
	attached_state(bi);
=======
		!bi->havestate) {
			big_to_small_state(&live,&(bi->env));
			attached_state(bi);
>>>>>>> p-uae/v2.1.0
    }
#endif
    return bi;
}

static void prepare_block(blockinfo* bi);

STATIC_INLINE void alloc_blockinfos(void)
{
    int i;
    blockinfo* bi;

    for (i=0;i<MAX_HOLD_BI;i++) {
<<<<<<< HEAD
	if (hold_bi[i])
	    return;
	bi=hold_bi[i]=(blockinfo*)current_compile_p;
	current_compile_p+=sizeof(blockinfo);

	prepare_block(bi);
    }
}


/*
 * Translation cache management
 */

static void free_cache (void)
{
    if (compiled_code) {
	flush_icache_hard (6);
	cache_free (compiled_code);
	compiled_code = 0;

	write_log ("JIT: Deallocated translation cache.\n");
    }
}

static void alloc_cache (void)
{
    if (currprefs.cachesize == 0)
	return;

    while (!compiled_code && currprefs.cachesize) {
	compiled_code = cache_alloc (currprefs.cachesize * 1024);
	if (!compiled_code)
	    currprefs.cachesize /= 2;
    }
    if (compiled_code) {
	max_compile_start = compiled_code + currprefs.cachesize * 1024 - BYTES_PER_INST;
	current_compile_p = compiled_code;
    }
    if (currprefs.cachesize != changed_prefs.cachesize)
	changed_prefs.cachesize = currprefs.cachesize;

    write_log ("JIT: Allocated %d KB translation cache.\n", currprefs.cachesize);
}

void set_cache_state (int enabled)
{
    if (enabled != letit)
	flush_icache_hard (77);
    letit = enabled;
}

=======
		if (hold_bi[i])
		    return;
		bi=hold_bi[i]=(blockinfo*)current_compile_p;
		current_compile_p+=sizeof(blockinfo);

		prepare_block(bi);
    }
}

>>>>>>> p-uae/v2.1.0
/********************************************************************
 * Preferences handling. This is just a convenient place to put it  *
 ********************************************************************/
extern int have_done_picasso;

<<<<<<< HEAD
void check_prefs_changed_comp (void)
{
    currprefs.comptrustbyte  = changed_prefs.comptrustbyte;
    currprefs.comptrustword  = changed_prefs.comptrustword;
    currprefs.comptrustlong  = changed_prefs.comptrustlong;
    currprefs.comptrustnaddr = changed_prefs.comptrustnaddr;
    currprefs.compnf         = changed_prefs.compnf;
    currprefs.comp_hardflush = changed_prefs.comp_hardflush;
    currprefs.comp_constjump = changed_prefs.comp_constjump;
    currprefs.comp_oldsegv   = changed_prefs.comp_oldsegv;
    currprefs.compfpu        = changed_prefs.compfpu;

    if (currprefs.cachesize != changed_prefs.cachesize) {
	currprefs.cachesize = changed_prefs.cachesize;
	free_cache ();
	alloc_cache ();
    }

    if ((!canbang || !currprefs.cachesize) && currprefs.comptrustbyte != 1) {
	// Set all of these to indirect when canbang == 0
	// Basically, set the  compforcesettings option...
	currprefs.comptrustbyte = 1;
	currprefs.comptrustword = 1;
	currprefs.comptrustlong = 1;
	currprefs.comptrustnaddr= 1;
	currprefs.compforcesettings = 1;

	changed_prefs.comptrustbyte = 1;
	changed_prefs.comptrustword = 1;
	changed_prefs.comptrustlong = 1;
	changed_prefs.comptrustnaddr= 1;
	changed_prefs.compforcesettings = 1;

	if (currprefs.cachesize)
	    write_log ("JIT: Reverting to \"indirect\" access, because canbang is zero!\n");
    }
}

/********************************************************************
=======
int check_prefs_changed_comp (void)
{
    int changed = 0;
    static int cachesize_prev, comptrust_prev, canbang_prev;

    if (currprefs.comptrustbyte != changed_prefs.comptrustbyte ||
        currprefs.comptrustword != changed_prefs.comptrustword ||
        currprefs.comptrustlong != changed_prefs.comptrustlong ||
        currprefs.comptrustnaddr!= changed_prefs.comptrustnaddr ||
        currprefs.compnf != changed_prefs.compnf ||
        currprefs.comp_hardflush != changed_prefs.comp_hardflush ||
        currprefs.comp_constjump != changed_prefs.comp_constjump ||
        currprefs.comp_oldsegv != changed_prefs.comp_oldsegv ||
        currprefs.compfpu != changed_prefs.compfpu ||
        currprefs.fpu_strict != changed_prefs.fpu_strict)
        changed = 1;

    currprefs.comptrustbyte = changed_prefs.comptrustbyte;
    currprefs.comptrustword = changed_prefs.comptrustword;
    currprefs.comptrustlong = changed_prefs.comptrustlong;
    currprefs.comptrustnaddr= changed_prefs.comptrustnaddr;
    currprefs.compnf = changed_prefs.compnf;
    currprefs.comp_hardflush = changed_prefs.comp_hardflush;
    currprefs.comp_constjump = changed_prefs.comp_constjump;
    currprefs.comp_oldsegv = changed_prefs.comp_oldsegv;
    currprefs.compfpu = changed_prefs.compfpu;
    currprefs.fpu_strict = changed_prefs.fpu_strict;

    if (currprefs.cachesize != changed_prefs.cachesize) {
        if (currprefs.cachesize && !changed_prefs.cachesize) {
            cachesize_prev = currprefs.cachesize;
            comptrust_prev = currprefs.comptrustbyte;
            canbang_prev = canbang;
        } else if (!currprefs.cachesize && changed_prefs.cachesize == cachesize_prev) {
            changed_prefs.comptrustbyte = currprefs.comptrustbyte = comptrust_prev;
            changed_prefs.comptrustword = currprefs.comptrustword = comptrust_prev;
            changed_prefs.comptrustlong = currprefs.comptrustlong = comptrust_prev;
            changed_prefs.comptrustnaddr = currprefs.comptrustnaddr = comptrust_prev;
        }
        currprefs.cachesize = changed_prefs.cachesize;
        alloc_cache();
        changed = 1;
    }
    if (!candirect)
        canbang = 0;

    // Turn off illegal-mem logging when using JIT...
    if(currprefs.cachesize)
        currprefs.illegal_mem = changed_prefs.illegal_mem;// = 0;

    currprefs.comp_midopt = changed_prefs.comp_midopt;
    currprefs.comp_lowopt = changed_prefs.comp_lowopt;

    if ((!canbang || !currprefs.cachesize) && currprefs.comptrustbyte != 1) {
        // Set all of these to indirect when canbang == 0
        // Basically, set the compforcesettings option...
        currprefs.comptrustbyte = 1;
        currprefs.comptrustword = 1;
        currprefs.comptrustlong = 1;
        currprefs.comptrustnaddr= 1;

        changed_prefs.comptrustbyte = 1;
        changed_prefs.comptrustword = 1;
        changed_prefs.comptrustlong = 1;
        changed_prefs.comptrustnaddr= 1;

        changed = 1;

		if (currprefs.cachesize)
			write_log ("JIT: Reverting to \"indirect\" access, because canbang is zero!\n");
    }

    if (changed)
		write_log ("JIT: cache=%d. b=%d w=%d l=%d fpu=%d nf=%d const=%d hard=%d\n",
            currprefs.cachesize,
            currprefs.comptrustbyte, currprefs.comptrustword, currprefs.comptrustlong,
            currprefs.compfpu, currprefs.compnf, currprefs.comp_constjump, currprefs.comp_hardflush);

#if 0
    if (!currprefs.compforcesettings) {
	int stop=0;
	if (currprefs.comptrustbyte!=0 && currprefs.comptrustbyte!=3)
		stop = 1, write_log ("JIT: comptrustbyte is not 'direct' or 'afterpic'\n");
	if (currprefs.comptrustword!=0 && currprefs.comptrustword!=3)
		stop = 1, write_log ("JIT: comptrustword is not 'direct' or 'afterpic'\n");
	if (currprefs.comptrustlong!=0 && currprefs.comptrustlong!=3)
		stop = 1, write_log ("JIT: comptrustlong is not 'direct' or 'afterpic'\n");
	if (currprefs.comptrustnaddr!=0 && currprefs.comptrustnaddr!=3)
		stop = 1, write_log ("JIT: comptrustnaddr is not 'direct' or 'afterpic'\n");
	if (currprefs.compnf!=1)
		stop = 1, write_log ("JIT: compnf is not 'yes'\n");
	if (currprefs.cachesize<1024)
		stop = 1, write_log ("JIT: cachesize is less than 1024\n");
	if (currprefs.comp_hardflush)
		stop = 1, write_log ("JIT: comp_flushmode is 'hard'\n");
	if (!canbang)
		stop = 1, write_log ("JIT: Cannot use most direct memory access,\n"
				"     and unable to recover from failed guess!\n");
	if (stop) {
	    gui_message("JIT: Configuration problems were detected!\n"
		      "JIT: These will adversely affect performance, and should\n"
		      "JIT: not be used. For more info, please see README.JIT-tuning\n"
		      "JIT: in the UAE documentation directory. You can force\n"
		      "JIT: your settings to be used by setting\n"
		      "JIT:      'compforcesettings=yes'\n"
		      "JIT: in your config file\n");
	    exit(1);
	}
    }
#endif
	return changed;
}

/********************************************************************
 * Get the optimizer stuff                                          *
 ********************************************************************/

//#include "compemu_optimizer.c"
#include "compemu_optimizer_x86.c"

/********************************************************************
>>>>>>> p-uae/v2.1.0
 * Functions to emit data into memory, and other general support    *
 ********************************************************************/

static uae_u8* target;

<<<<<<< HEAD
=======
static  void emit_init(void)
{
}

>>>>>>> p-uae/v2.1.0
STATIC_INLINE void emit_byte(uae_u8 x)
{
    *target++=x;
}

STATIC_INLINE void emit_word(uae_u16 x)
{
    *((uae_u16*)target)=x;
    target+=2;
}

STATIC_INLINE void emit_long(uae_u32 x)
{
    *((uae_u32*)target)=x;
    target+=4;
}

<<<<<<< HEAD
STATIC_INLINE void emit_block (const uae_u8 *block, uae_u32 blocklen)
{
    memcpy ((uae_u8 *) target, block, blocklen);
    target += blocklen;
}

STATIC_INLINE uae_u32 reverse32 (uae_u32 oldv)
{
    return bswap_32 (oldv);
}

void set_target(uae_u8* t)
{
=======
STATIC_INLINE uae_u32 reverse32 (uae_u32 oldv)
{
    return ((oldv>>24)&0xff) | ((oldv>>8)&0xff00) |
		((oldv<<8)&0xff0000) | ((oldv<<24)&0xff000000);
}


void set_target(uae_u8* t)
{
    lopt_emit_all();
>>>>>>> p-uae/v2.1.0
    target=t;
}

STATIC_INLINE uae_u8* get_target_noopt(void)
{
    return target;
}

STATIC_INLINE uae_u8* get_target(void)
{
<<<<<<< HEAD
=======
    lopt_emit_all();
>>>>>>> p-uae/v2.1.0
    return get_target_noopt();
}


/********************************************************************
 * Getting the information about the target CPU                     *
 ********************************************************************/

#include "compemu_raw_x86.c"


/********************************************************************
 * Flags status handling. EMIT TIME!                                *
 ********************************************************************/

static void bt_l_ri_noclobber(R4 r, IMM i);

static void make_flags_live_internal(void)
{
    if (live.flags_in_flags==VALID)
<<<<<<< HEAD
	return;
    Dif (live.flags_on_stack==TRASH) {
	write_log ("JIT: Want flags, got something on stack, but it is TRASH\n");
	abort();
    }
    if (live.flags_on_stack==VALID) {
	int tmp;
	tmp=readreg_specific(FLAGTMP,4,FLAG_NREG2);
	raw_reg_to_flags(tmp);
	unlock(tmp);

	live.flags_in_flags=VALID;
	return;
    }
    write_log ("JIT: Huh? live.flags_in_flags=%d, live.flags_on_stack=%d, but need to make live\n",
	       live.flags_in_flags, live.flags_on_stack);
    abort();
=======
		return;
    Dif (live.flags_on_stack==TRASH) {
		jit_abort ("JIT: Want flags, got something on stack, but it is TRASH\n");
    }
    if (live.flags_on_stack==VALID) {
		int tmp;
		tmp=readreg_specific(FLAGTMP,4,FLAG_NREG2);
		raw_reg_to_flags(tmp);
		unlock(tmp);

		live.flags_in_flags=VALID;
		return;
    }
	jit_abort ("JIT: Huh? live.flags_in_flags=%d, live.flags_on_stack=%d, but need to make live\n",
	       live.flags_in_flags, live.flags_on_stack);
>>>>>>> p-uae/v2.1.0
}

static void flags_to_stack(void)
{
    if (live.flags_on_stack==VALID)
<<<<<<< HEAD
	return;
    if (!live.flags_are_important) {
	live.flags_on_stack=VALID;
	return;
    }
    Dif (live.flags_in_flags!=VALID)
	abort();
    else  {
	int tmp;
	tmp=writereg_specific(FLAGTMP,4,FLAG_NREG1);
	raw_flags_to_reg(tmp);
	unlock(tmp);
=======
		return;
    if (!live.flags_are_important) {
		live.flags_on_stack=VALID;
		return;
    }
    Dif (live.flags_in_flags!=VALID)
		jit_abort ("flags_to_stack != VALID");
    else  {
		int tmp;
		tmp=writereg_specific(FLAGTMP,4,FLAG_NREG1);
		raw_flags_to_reg(tmp);
		unlock(tmp);
>>>>>>> p-uae/v2.1.0
    }
    live.flags_on_stack=VALID;
}

STATIC_INLINE void clobber_flags(void)
{
    if (live.flags_in_flags==VALID && live.flags_on_stack!=VALID)
<<<<<<< HEAD
	flags_to_stack();
=======
		flags_to_stack();
>>>>>>> p-uae/v2.1.0
    live.flags_in_flags=TRASH;
}

/* Prepare for leaving the compiled stuff */
STATIC_INLINE void flush_flags(void)
{
    flags_to_stack();
    return;
}

<<<<<<< HEAD
static int touchcnt;
=======
int touchcnt;
>>>>>>> p-uae/v2.1.0

/********************************************************************
 * register allocation per block logging                            *
 ********************************************************************/

static uae_s8 vstate[VREGS];
static uae_s8 nstate[N_REGS];

#define L_UNKNOWN -127
#define L_UNAVAIL -1
#define L_NEEDED -2
#define L_UNNEEDED -3

STATIC_INLINE void log_startblock(void)
{
    int i;
    for (i=0;i<VREGS;i++)
<<<<<<< HEAD
	vstate[i]=L_UNKNOWN;
    for (i=0;i<N_REGS;i++)
	nstate[i]=L_UNKNOWN;
=======
		vstate[i]=L_UNKNOWN;
    for (i=0;i<N_REGS;i++)
		nstate[i]=L_UNKNOWN;
>>>>>>> p-uae/v2.1.0
}

STATIC_INLINE void log_isused(int n)
{
    if (nstate[n]==L_UNKNOWN)
<<<<<<< HEAD
	nstate[n]=L_UNAVAIL;
=======
		nstate[n]=L_UNAVAIL;
>>>>>>> p-uae/v2.1.0
}

STATIC_INLINE void log_isreg(int n, int r)
{
    if (nstate[n]==L_UNKNOWN)
<<<<<<< HEAD
	nstate[n]=r;
    if (vstate[r]==L_UNKNOWN)
	vstate[r]=L_NEEDED;
=======
		nstate[n]=r;
    if (vstate[r]==L_UNKNOWN)
		vstate[r]=L_NEEDED;
>>>>>>> p-uae/v2.1.0
}

STATIC_INLINE void log_clobberreg(int r)
{
    if (vstate[r]==L_UNKNOWN)
<<<<<<< HEAD
	vstate[r]=L_UNNEEDED;
=======
		vstate[r]=L_UNNEEDED;
>>>>>>> p-uae/v2.1.0
}

/* This ends all possibility of clever register allocation */

STATIC_INLINE void log_flush(void)
{
    int i;
    for (i=0;i<VREGS;i++)
<<<<<<< HEAD
	if (vstate[i]==L_UNKNOWN)
	    vstate[i]=L_NEEDED;
    for (i=0;i<N_REGS;i++)
	if (nstate[i]==L_UNKNOWN)
	    nstate[i]=L_UNAVAIL;
=======
		if (vstate[i]==L_UNKNOWN)
		    vstate[i]=L_NEEDED;
    for (i=0;i<N_REGS;i++)
		if (nstate[i]==L_UNKNOWN)
		    nstate[i]=L_UNAVAIL;
>>>>>>> p-uae/v2.1.0
}

STATIC_INLINE void log_dump(void)
{
    int i;

    return;

<<<<<<< HEAD
    write_log("----------------------\n");
    for (i=0;i<N_REGS;i++) {
	switch(nstate[i]) {
	 case L_UNKNOWN: write_log("Nat %d : UNKNOWN\n",i); break;
	 case L_UNAVAIL: write_log("Nat %d : UNAVAIL\n",i); break;
	 default:        write_log("Nat %d : %d\n",i,nstate[i]); break;
	}
    }
    for (i=0;i<VREGS;i++) {
	if (vstate[i]==L_UNNEEDED)
	    write_log("Virt %d: UNNEEDED\n",i);
=======
	write_log ("----------------------\n");
    for (i=0;i<N_REGS;i++) {
		switch(nstate[i]) {
		case L_UNKNOWN: write_log ("Nat %d : UNKNOWN\n",i); break;
		case L_UNAVAIL: write_log ("Nat %d : UNAVAIL\n",i); break;
		default:        write_log ("Nat %d : %d\n",i,nstate[i]); break;
		}
    }
    for (i=0;i<VREGS;i++) {
		if (vstate[i]==L_UNNEEDED)
			write_log ("Virt %d: UNNEEDED\n",i);
>>>>>>> p-uae/v2.1.0
    }
}

/********************************************************************
 * register status handling. EMIT TIME!                             *
 ********************************************************************/

STATIC_INLINE void set_status(int r, int status)
{
    if (status==ISCONST)
<<<<<<< HEAD
	log_clobberreg(r);
    live.state[r].status=status;
}

=======
		log_clobberreg(r);
    live.state[r].status=status;
}


>>>>>>> p-uae/v2.1.0
STATIC_INLINE int isinreg(int r)
{
    return live.state[r].status==CLEAN || live.state[r].status==DIRTY;
}

STATIC_INLINE void adjust_nreg(int r, uae_u32 val)
{
    if (!val)
<<<<<<< HEAD
	return;
=======
		return;
>>>>>>> p-uae/v2.1.0
    raw_lea_l_brr(r,r,val);
}

static  void tomem(int r)
{
    int rr=live.state[r].realreg;

    if (isinreg(r)) {
<<<<<<< HEAD
	if (live.state[r].val &&
	    live.nat[rr].nholds==1 &&
	    !live.nat[rr].locked) {
	    // printf("RemovingA offset %x from reg %d (%d) at %p\n",
	    //   live.state[r].val,r,rr,target);
	    adjust_nreg(rr,live.state[r].val);
	    live.state[r].val=0;
	    live.state[r].dirtysize=4;
	    set_status(r,DIRTY);
	}
    }

    if (live.state[r].status==DIRTY) {
	switch (live.state[r].dirtysize) {
	 case 1: raw_mov_b_mr((uae_uintptr)live.state[r].mem,rr); break;
	 case 2: raw_mov_w_mr((uae_uintptr)live.state[r].mem,rr); break;
	 case 4: raw_mov_l_mr((uae_uintptr)live.state[r].mem,rr); break;
	 default: abort();
	}
	set_status(r,CLEAN);
	live.state[r].dirtysize=0;
=======
		if (live.state[r].val &&
		    live.nat[rr].nholds==1 &&
		    !live.nat[rr].locked) {
		    // write_log ("JIT: RemovingA offset %x from reg %d (%d) at %p\n",
		    //   live.state[r].val,r,rr,target);
		    adjust_nreg(rr,live.state[r].val);
		    live.state[r].val=0;
		    live.state[r].dirtysize=4;
		    set_status(r,DIRTY);
		}
    }

    if (live.state[r].status==DIRTY) {
		switch (live.state[r].dirtysize) {
		case 1: raw_mov_b_mr((uae_u32)live.state[r].mem,rr); break;
		case 2: raw_mov_w_mr((uae_u32)live.state[r].mem,rr); break;
		case 4: raw_mov_l_mr((uae_u32)live.state[r].mem,rr); break;
		default: abort();
		}
		set_status(r,CLEAN);
		live.state[r].dirtysize=0;
>>>>>>> p-uae/v2.1.0
    }
}

STATIC_INLINE int isconst(int r)
{
    return live.state[r].status==ISCONST;
}

int is_const(int r)
{
    return isconst(r);
}

STATIC_INLINE void writeback_const(int r)
{
    if (!isconst(r))
<<<<<<< HEAD
	return;
    Dif (live.state[r].needflush==NF_HANDLER) {
	write_log ("JIT: Trying to write back constant NF_HANDLER!\n");
	abort();
    }

    raw_mov_l_mi((uae_uintptr)live.state[r].mem,live.state[r].val);
=======
		return;
    Dif (live.state[r].needflush==NF_HANDLER) {
		jit_abort ("JIT: Trying to write back constant NF_HANDLER!\n");
    }

    raw_mov_l_mi((uae_u32)live.state[r].mem,live.state[r].val);
>>>>>>> p-uae/v2.1.0
    live.state[r].val=0;
    set_status(r,INMEM);
}

STATIC_INLINE void tomem_c(int r)
{
    if (isconst(r)) {
<<<<<<< HEAD
	writeback_const(r);
    }
    else
	tomem(r);
=======
		writeback_const(r);
    }
    else
		tomem(r);
>>>>>>> p-uae/v2.1.0
}

static  void evict(int r)
{
    int rr;

    if (!isinreg(r))
<<<<<<< HEAD
	return;
=======
		return;
>>>>>>> p-uae/v2.1.0
    tomem(r);
    rr=live.state[r].realreg;

    Dif (live.nat[rr].locked &&
<<<<<<< HEAD
	live.nat[rr].nholds==1) {
	write_log ("JIT: register %d in nreg %d is locked!\n", r, live.state[r].realreg);
	abort();
=======
		live.nat[rr].nholds==1) {
			jit_abort ("JIT: register %d in nreg %d is locked!\n",r,live.state[r].realreg);
>>>>>>> p-uae/v2.1.0
    }

    live.nat[rr].nholds--;
    if (live.nat[rr].nholds!=live.state[r].realind) { /* Was not last */
<<<<<<< HEAD
	int topreg=live.nat[rr].holds[live.nat[rr].nholds];
	int thisind=live.state[r].realind;
	live.nat[rr].holds[thisind]=topreg;
	live.state[topreg].realind=thisind;
=======
		int topreg=live.nat[rr].holds[live.nat[rr].nholds];
		int thisind=live.state[r].realind;
		live.nat[rr].holds[thisind]=topreg;
		live.state[topreg].realind=thisind;
>>>>>>> p-uae/v2.1.0
    }
    live.state[r].realreg=-1;
    set_status(r,INMEM);
}

STATIC_INLINE void free_nreg(int r)
{
    int i=live.nat[r].nholds;

    while (i) {
<<<<<<< HEAD
	int vr;

	--i;
	vr=live.nat[r].holds[i];
	evict(vr);
    }
    Dif (live.nat[r].nholds!=0) {
	write_log ("JIT: Failed to free nreg %d, nholds is %d\n", r, live.nat[r].nholds);
	abort();
=======
		int vr;

		--i;
		vr=live.nat[r].holds[i];
		evict(vr);
    }
    Dif (live.nat[r].nholds!=0) {
		jit_abort ("JIT: Failed to free nreg %d, nholds is %d\n",r,live.nat[r].nholds);
>>>>>>> p-uae/v2.1.0
    }
}

/* Use with care! */
STATIC_INLINE void isclean(int r)
{
    if (!isinreg(r))
<<<<<<< HEAD
	return;
=======
		return;
>>>>>>> p-uae/v2.1.0
    live.state[r].validsize=4;
    live.state[r].dirtysize=0;
    live.state[r].val=0;
    set_status(r,CLEAN);
}

STATIC_INLINE void disassociate(int r)
{
    isclean(r);
    evict(r);
}

STATIC_INLINE void set_const(int r, uae_u32 val)
{
    disassociate(r);
    live.state[r].val=val;
    set_status(r,ISCONST);
}

STATIC_INLINE uae_u32 get_offset(int r)
{
    return live.state[r].val;
}

static  int alloc_reg_hinted(int r, int size, int willclobber, int hint)
{
    int bestreg;
    uae_s32 when;
    int i;
    uae_s32 badness=0; /* to shut up gcc */
    bestreg=-1;
    when=2000000000;

    for (i=N_REGS;i--;) {
<<<<<<< HEAD
	badness=live.nat[i].touched;
	if (live.nat[i].nholds==0)
	    badness=0;
	if (i==hint)
	    badness-=200000000;
	if (!live.nat[i].locked && badness<when) {
	    if ((size==1 && live.nat[i].canbyte) ||
		(size==2 && live.nat[i].canword) ||
		(size==4)) {
		bestreg=i;
		when=badness;
		if (live.nat[i].nholds==0 && hint<0)
		    break;
		if (i==hint)
		    break;
	    }
	}
    }
    Dif (bestreg==-1)
	abort();

    if (live.nat[bestreg].nholds>0) {
	free_nreg(bestreg);
    }
    if (isinreg(r)) {
	int rr=live.state[r].realreg;
	/* This will happen if we read a partially dirty register at a
	   bigger size */
	Dif (willclobber || live.state[r].validsize>=size)
	    abort();
	Dif (live.nat[rr].nholds!=1)
	    abort();
	if (size==4 && live.state[r].validsize==2) {
	    log_isused(bestreg);
	    raw_mov_l_rm(bestreg,(uae_uintptr)live.state[r].mem);
	    raw_bswap_32(bestreg);
	    raw_zero_extend_16_rr(rr,rr);
	    raw_zero_extend_16_rr(bestreg,bestreg);
	    raw_bswap_32(bestreg);
	    raw_lea_l_brr_indexed(rr,rr,bestreg,1,0);
	    live.state[r].validsize=4;
	    live.nat[rr].touched=touchcnt++;
	    return rr;
	}
	if (live.state[r].validsize==1) {
	    /* Nothing yet */
	}
	evict(r);
    }

    if (!willclobber) {
	if (live.state[r].status!=UNDEF) {
	    if (isconst(r)) {
		raw_mov_l_ri(bestreg,live.state[r].val);
		live.state[r].val=0;
		live.state[r].dirtysize=4;
		set_status(r,DIRTY);
		log_isused(bestreg);
	    }
	    else {
		if (r==FLAGTMP)
		    raw_load_flagreg(bestreg,r);
		else if (r==FLAGX)
		    raw_load_flagx(bestreg,r);
		else {
		    raw_mov_l_rm(bestreg,(uae_uintptr)live.state[r].mem);
		}
		live.state[r].dirtysize=0;
		set_status(r,CLEAN);
		log_isreg(bestreg,r);
	    }
	}
	else {
	    live.state[r].val=0;
	    live.state[r].dirtysize=0;
	    set_status(r,CLEAN);
	    log_isused(bestreg);
	}
	live.state[r].validsize=4;
    }
    else { /* this is the easiest way, but not optimal. FIXME! */
	/* Now it's trickier, but hopefully still OK */
	if (!isconst(r) || size==4) {
	    live.state[r].validsize=size;
	    live.state[r].dirtysize=size;
	    live.state[r].val=0;
	    set_status(r,DIRTY);
	    if (size==4)
		log_isused(bestreg);
	    else
		log_isreg(bestreg,r);
	}
	else {
	    if (live.state[r].status!=UNDEF)
		raw_mov_l_ri(bestreg,live.state[r].val);
	    live.state[r].val=0;
	    live.state[r].validsize=4;
	    live.state[r].dirtysize=4;
	    set_status(r,DIRTY);
	    log_isused(bestreg);
	}
=======
		badness=live.nat[i].touched;
		if (live.nat[i].nholds==0)
		    badness=0;
		if (i==hint)
		    badness-=200000000;
		if (!live.nat[i].locked && badness<when) {
		    if ((size==1 && live.nat[i].canbyte) ||
				(size==2 && live.nat[i].canword) ||
				(size==4)) {
					bestreg=i;
					when=badness;
					if (live.nat[i].nholds==0 && hint<0)
					    break;
					if (i==hint)
					    break;
		    }
		}
    }
    Dif (bestreg==-1)
		jit_abort ("alloc_reg_hinted bestreg=-1");

    if (live.nat[bestreg].nholds>0) {
		free_nreg(bestreg);
    }
    if (isinreg(r)) {
		int rr=live.state[r].realreg;
		/* This will happen if we read a partially dirty register at a
		   bigger size */
		Dif (willclobber || live.state[r].validsize>=size)
			jit_abort ("willclobber || live.state[r].validsize>=size");
		Dif (live.nat[rr].nholds!=1)
			jit_abort ("live.nat[rr].nholds!=1");
		if (size==4 && live.state[r].validsize==2) {
		    log_isused(bestreg);
		    raw_mov_l_rm(bestreg,(uae_u32)live.state[r].mem);
		    raw_bswap_32(bestreg);
		    raw_zero_extend_16_rr(rr,rr);
		    raw_zero_extend_16_rr(bestreg,bestreg);
		    raw_bswap_32(bestreg);
		    raw_lea_l_rr_indexed(rr,rr,bestreg);
		    live.state[r].validsize=4;
		    live.nat[rr].touched=touchcnt++;
		    return rr;
		}
		if (live.state[r].validsize==1) {
		    /* Nothing yet */
		}
		evict(r);
    }

    if (!willclobber) {
		if (live.state[r].status!=UNDEF) {
		    if (isconst(r)) {
				raw_mov_l_ri(bestreg,live.state[r].val);
				live.state[r].val=0;
				live.state[r].dirtysize=4;
				set_status(r,DIRTY);
				log_isused(bestreg);
		    }
		    else {
				if (r==FLAGTMP)
				    raw_load_flagreg(bestreg,r);
				else if (r==FLAGX)
				    raw_load_flagx(bestreg,r);
				else {
				    raw_mov_l_rm(bestreg,(uae_u32)live.state[r].mem);
				}
				live.state[r].dirtysize=0;
				set_status(r,CLEAN);
				log_isreg(bestreg,r);
		    }
		}
		else {
		    live.state[r].val=0;
		    live.state[r].dirtysize=0;
		    set_status(r,CLEAN);
		    log_isused(bestreg);
		}
		live.state[r].validsize=4;
    }
    else { /* this is the easiest way, but not optimal. FIXME! */
		/* Now it's trickier, but hopefully still OK */
		if (!isconst(r) || size==4) {
		    live.state[r].validsize=size;
		    live.state[r].dirtysize=size;
		    live.state[r].val=0;
		    set_status(r,DIRTY);
		    if (size==4)
				log_isused(bestreg);
		    else
				log_isreg(bestreg,r);
		}
		else {
		    if (live.state[r].status!=UNDEF)
				raw_mov_l_ri(bestreg,live.state[r].val);
		    live.state[r].val=0;
		    live.state[r].validsize=4;
		    live.state[r].dirtysize=4;
		    set_status(r,DIRTY);
		    log_isused(bestreg);
		}
>>>>>>> p-uae/v2.1.0
    }
    live.state[r].realreg=bestreg;
    live.state[r].realind=live.nat[bestreg].nholds;
    live.nat[bestreg].touched=touchcnt++;
    live.nat[bestreg].holds[live.nat[bestreg].nholds]=r;
    live.nat[bestreg].nholds++;

    return bestreg;
}

static  int alloc_reg(int r, int size, int willclobber)
{
    return alloc_reg_hinted(r,size,willclobber,-1);
}

static  void unlock(int r)
{
    Dif (!live.nat[r].locked)
<<<<<<< HEAD
	abort();
=======
		jit_abort ("unlock %d not locked", r);
>>>>>>> p-uae/v2.1.0
    live.nat[r].locked--;
}

static  void setlock(int r)
{
    live.nat[r].locked++;
}


static void mov_nregs(int d, int s)
{
<<<<<<< HEAD
=======
    int ns=live.nat[s].nholds;
>>>>>>> p-uae/v2.1.0
    int nd=live.nat[d].nholds;
    int i;

    if (s==d)
<<<<<<< HEAD
	return;

    if (nd>0)
	free_nreg(d);
=======
		return;

    if (nd>0)
		free_nreg(d);
>>>>>>> p-uae/v2.1.0

    raw_mov_l_rr(d,s);
    log_isused(d);

    for (i=0;i<live.nat[s].nholds;i++) {
<<<<<<< HEAD
	int vs=live.nat[s].holds[i];

	live.state[vs].realreg=d;
	live.state[vs].realind=i;
	live.nat[d].holds[i]=vs;
=======
		int vs=live.nat[s].holds[i];

		live.state[vs].realreg=d;
		live.state[vs].realind=i;
		live.nat[d].holds[i]=vs;
>>>>>>> p-uae/v2.1.0
    }
    live.nat[d].nholds=live.nat[s].nholds;

    live.nat[s].nholds=0;
}


STATIC_INLINE void make_exclusive(int r, int size, int spec)
{
    reg_status oldstate;
    int rr=live.state[r].realreg;
    int nr;
    int nind;
    int ndirt=0;
    int i;

    if (!isinreg(r))
<<<<<<< HEAD
	return;
    if (live.nat[rr].nholds==1)
	return;
    for (i=0;i<live.nat[rr].nholds;i++) {
	int vr=live.nat[rr].holds[i];
	if (vr!=r &&
	    (live.state[vr].status==DIRTY || live.state[vr].val))
	    ndirt++;
    }
    if (!ndirt && size<live.state[r].validsize && !live.nat[rr].locked) {
	/* Everything else is clean, so let's keep this register */
	for (i=0;i<live.nat[rr].nholds;i++) {
	    int vr=live.nat[rr].holds[i];
	    if (vr!=r) {
		evict(vr);
		i--; /* Try that index again! */
	    }
	}
	Dif (live.nat[rr].nholds!=1) {
	    write_log ("JIT: natreg %d holds %d vregs, %d not exclusive\n",
		       rr, live.nat[rr].nholds, r);
	    abort();
	}
	return;
=======
		return;
    if (live.nat[rr].nholds==1)
		return;
    for (i=0;i<live.nat[rr].nholds;i++) {
		int vr=live.nat[rr].holds[i];
		if (vr!=r &&
		    (live.state[vr].status==DIRTY || live.state[vr].val))
		    ndirt++;
    }
    if (!ndirt && size<live.state[r].validsize && !live.nat[rr].locked) {
		/* Everything else is clean, so let's keep this register */
		for (i=0;i<live.nat[rr].nholds;i++) {
		    int vr=live.nat[rr].holds[i];
		    if (vr!=r) {
				evict(vr);
			i--; /* Try that index again! */
		    }
		}
		Dif (live.nat[rr].nholds!=1) {
			jit_abort ("JIT: natreg %d holds %d vregs, %d not exclusive\n",
		       rr, live.nat[rr].nholds, r);
		}
		return;
>>>>>>> p-uae/v2.1.0
    }

    /* We have to split the register */
    oldstate=live.state[r];

    setlock(rr); /* Make sure this doesn't go away */
    /* Forget about r being in the register rr */
    disassociate(r);
    /* Get a new register, that we will clobber completely */
    if (oldstate.status==DIRTY) {
<<<<<<< HEAD
	/* If dirtysize is <4, we need a register that can handle the
	   eventual smaller memory store! Thanks to Quake68k for exposing
	   this detail ;-) */
	nr=alloc_reg_hinted(r,oldstate.dirtysize,1,spec);
    }
    else {
	nr=alloc_reg_hinted(r,4,1,spec);
=======
		/* If dirtysize is <4, we need a register that can handle the
		   eventual smaller memory store! Thanks to Quake68k for exposing
		   this detail ;-) */
		nr=alloc_reg_hinted(r,oldstate.dirtysize,1,spec);
    }
    else {
		nr=alloc_reg_hinted(r,4,1,spec);
>>>>>>> p-uae/v2.1.0
    }
    nind=live.state[r].realind;
    live.state[r]=oldstate;   /* Keep all the old state info */
    live.state[r].realreg=nr;
    live.state[r].realind=nind;

    if (size<live.state[r].validsize) {
<<<<<<< HEAD
	if (live.state[r].val) {
=======
		if (live.state[r].val) {
>>>>>>> p-uae/v2.1.0
	    /* Might as well compensate for the offset now */
	    raw_lea_l_brr(nr,rr,oldstate.val);
	    live.state[r].val=0;
	    live.state[r].dirtysize=4;
	    set_status(r,DIRTY);
	}
	else
	    raw_mov_l_rr(nr,rr);  /* Make another copy */
    }
    unlock(rr);
}

STATIC_INLINE void add_offset(int r, uae_u32 off)
{
    live.state[r].val+=off;
}

STATIC_INLINE void remove_offset(int r, int spec)
{
    int rr;

    if (isconst(r))
<<<<<<< HEAD
	return;
    if (live.state[r].val==0)
	return;
    if (isinreg(r) && live.state[r].validsize<4)
	evict(r);

    if (!isinreg(r))
	alloc_reg_hinted(r,4,0,spec);

    Dif (live.state[r].validsize!=4) {
	write_log ("JIT: Validsize=%d in remove_offset\n", live.state[r].validsize);
	abort();
=======
		return;
    if (live.state[r].val==0)
		return;
    if (isinreg(r) && live.state[r].validsize<4)
		evict(r);

    if (!isinreg(r))
		alloc_reg_hinted(r,4,0,spec);

    Dif (live.state[r].validsize!=4) {
		jit_abort ("JIT: Validsize=%d in remove_offset\n",live.state[r].validsize);
>>>>>>> p-uae/v2.1.0
    }
    make_exclusive(r,0,-1);
    /* make_exclusive might have done the job already */
    if (live.state[r].val==0)
<<<<<<< HEAD
	return;
=======
		return;
>>>>>>> p-uae/v2.1.0

    rr=live.state[r].realreg;

    if (live.nat[rr].nholds==1) {
<<<<<<< HEAD
	//printf("RemovingB offset %x from reg %d (%d) at %p\n",
	//       live.state[r].val,r,rr,target);
	adjust_nreg(rr,live.state[r].val);
	live.state[r].dirtysize=4;
	live.state[r].val=0;
	set_status(r,DIRTY);
	return;
    }
    write_log ("JIT: Failed in remove_offset\n");
    abort();
=======
		//write_log ("JIT: RemovingB offset %x from reg %d (%d) at %p\n",
		//       live.state[r].val,r,rr,target);
		adjust_nreg(rr,live.state[r].val);
		live.state[r].dirtysize=4;
		live.state[r].val=0;
		set_status(r,DIRTY);
		return;
    }
	jit_abort ("JIT: Failed in remove_offset\n");
>>>>>>> p-uae/v2.1.0
}

STATIC_INLINE void remove_all_offsets(void)
{
    int i;

    for (i=0;i<VREGS;i++)
<<<<<<< HEAD
	remove_offset(i,-1);
=======
		remove_offset(i,-1);
>>>>>>> p-uae/v2.1.0
}

STATIC_INLINE int readreg_general(int r, int size, int spec, int can_offset)
{
    int n;
    int answer=-1;

    if (live.state[r].status==UNDEF) {
<<<<<<< HEAD
      write_log ("JIT: WARNING: Unexpected read of undefined register %d\n", r);
    }
    if (!can_offset)
	remove_offset(r,spec);

    if (isinreg(r) && live.state[r].validsize>=size) {
	n=live.state[r].realreg;
	switch(size) {
	 case 1:
	    if (live.nat[n].canbyte || spec>=0) {
		answer=n;
	    }
	    break;
	 case 2:
	    if (live.nat[n].canword || spec>=0) {
		answer=n;
	    }
	    break;
	 case 4:
	    answer=n;
	    break;
	 default: abort();
	}
	if (answer<0)
	    evict(r);
=======
		write_log ("JIT: WARNING: Unexpected read of undefined register %d\n",r);
    }
    if (!can_offset)
		remove_offset(r,spec);

    if (isinreg(r) && live.state[r].validsize>=size) {
		n=live.state[r].realreg;
		switch(size) {
		case 1:
		    if (live.nat[n].canbyte || spec>=0) {
				answer=n;
		    }
		    break;
		case 2:
	    	if (live.nat[n].canword || spec>=0) {
				answer=n;
	    	}
	    	break;
	 	case 4:
	    	answer=n;
	    	break;
	 	default: abort();
		}
		if (answer<0)
		    evict(r);
>>>>>>> p-uae/v2.1.0
    }
    /* either the value was in memory to start with, or it was evicted and
       is in memory now */
    if (answer<0) {
<<<<<<< HEAD
	answer=alloc_reg_hinted(r,spec>=0?4:size,0,spec);
    }

    if (spec>=0 && spec!=answer) {
	/* Too bad */
	mov_nregs(spec,answer);
	answer=spec;
=======
		answer=alloc_reg_hinted(r,spec>=0?4:size,0,spec);
    }

    if (spec>=0 && spec!=answer) {
		/* Too bad */
		mov_nregs(spec,answer);
		answer=spec;
>>>>>>> p-uae/v2.1.0
    }
    live.nat[answer].locked++;
    live.nat[answer].touched=touchcnt++;
    return answer;
}



static int readreg(int r, int size)
{
    return readreg_general(r,size,-1,0);
}

static int readreg_specific(int r, int size, int spec)
{
    return readreg_general(r,size,spec,0);
}

static int readreg_offset(int r, int size)
{
    return readreg_general(r,size,-1,1);
}


STATIC_INLINE int writereg_general(int r, int size, int spec)
{
    int n;
    int answer=-1;

    if (size<4) {
<<<<<<< HEAD
	remove_offset(r,spec);
=======
		remove_offset(r,spec);
>>>>>>> p-uae/v2.1.0
    }

    make_exclusive(r,size,spec);
    if (isinreg(r)) {
<<<<<<< HEAD
	int nvsize=size>live.state[r].validsize?size:live.state[r].validsize;
	int ndsize=size>live.state[r].dirtysize?size:live.state[r].dirtysize;
	n=live.state[r].realreg;

	Dif (live.nat[n].nholds!=1)
	    abort();
	switch(size) {
	 case 1:
	    if (live.nat[n].canbyte || spec>=0) {
		live.state[r].dirtysize=ndsize;
		live.state[r].validsize=nvsize;
		answer=n;
	    }
	    break;
	 case 2:
	    if (live.nat[n].canword || spec>=0) {
		live.state[r].dirtysize=ndsize;
		live.state[r].validsize=nvsize;
		answer=n;
	    }
	    break;
	 case 4:
	    live.state[r].dirtysize=ndsize;
	    live.state[r].validsize=nvsize;
	    answer=n;
	    break;
	 default: abort();
	}
	if (answer<0)
	    evict(r);
    }
    /* either the value was in memory to start with, or it was evicted and
       is in memory now */
    if (answer<0) {
	answer=alloc_reg_hinted(r,size,1,spec);
    }
    if (spec>=0 && spec!=answer) {
	mov_nregs(spec,answer);
	answer=spec;
    }
    if (live.state[r].status==UNDEF)
	live.state[r].validsize=4;
    live.state[r].dirtysize=size>live.state[r].dirtysize?size:live.state[r].dirtysize;
    live.state[r].validsize=size>live.state[r].validsize?size:live.state[r].validsize;
=======
		int nvsize=size>live.state[r].validsize?size:live.state[r].validsize;
		int ndsize=size>live.state[r].dirtysize?size:live.state[r].dirtysize;
		n=live.state[r].realreg;

		Dif (live.nat[n].nholds!=1)
			jit_abort ("live.nat[%d].nholds!=1", n);
		switch(size) {
		case 1:
		    if (live.nat[n].canbyte || spec>=0) {
				live.state[r].dirtysize=ndsize;
				live.state[r].validsize=nvsize;
				answer=n;
		    }
		    break;
	 	case 2:
	    	if (live.nat[n].canword || spec>=0) {
				live.state[r].dirtysize=ndsize;
				live.state[r].validsize=nvsize;
				answer=n;
	    	}
	    	break;
		case 4:
			live.state[r].dirtysize=ndsize;
			live.state[r].validsize=nvsize;
			answer=n;
			break;
		default: abort();
		}
		if (answer<0)
			evict(r);
	}
	/* either the value was in memory to start with, or it was evicted and
	is in memory now */
	if (answer<0) {
		answer=alloc_reg_hinted(r,size,1,spec);
	}
	if (spec>=0 && spec!=answer) {
		mov_nregs(spec,answer);
		answer=spec;
	}
	if (live.state[r].status==UNDEF)
		live.state[r].validsize=4;
	live.state[r].dirtysize=size>live.state[r].dirtysize?size:live.state[r].dirtysize;
	live.state[r].validsize=size>live.state[r].validsize?size:live.state[r].validsize;
>>>>>>> p-uae/v2.1.0

    live.nat[answer].locked++;
    live.nat[answer].touched=touchcnt++;
    if (size==4) {
<<<<<<< HEAD
	live.state[r].val=0;
    }
    else {
	Dif (live.state[r].val) {
	    write_log ("JIT: Problem with val\n");
	    abort();
	}
=======
		live.state[r].val=0;
    }
    else {
		Dif (live.state[r].val) {
			jit_abort ("JIT: Problem with val\n");
		}
>>>>>>> p-uae/v2.1.0
    }
    set_status(r,DIRTY);
    return answer;
}

static int writereg(int r, int size)
{
    return writereg_general(r,size,-1);
}

static int writereg_specific(int r, int size, int spec)
{
    return writereg_general(r,size,spec);
}

STATIC_INLINE int rmw_general(int r, int wsize, int rsize, int spec)
{
    int n;
    int answer=-1;

    if (live.state[r].status==UNDEF) {
<<<<<<< HEAD
      write_log ("JIT: WARNING: Unexpected read of undefined register %d\n", r);
=======
		write_log ("JIT: WARNING: Unexpected read of undefined register %d\n",r);
>>>>>>> p-uae/v2.1.0
    }
    remove_offset(r,spec);
    make_exclusive(r,0,spec);

    Dif (wsize<rsize) {
<<<<<<< HEAD
	write_log ("JIT: Cannot handle wsize<rsize in rmw_general()\n");
	abort();
    }
    if (isinreg(r) && live.state[r].validsize>=rsize) {
	n=live.state[r].realreg;
	Dif (live.nat[n].nholds!=1)
	    abort();

	switch(rsize) {
	 case 1:
	    if (live.nat[n].canbyte || spec>=0) {
		answer=n;
	    }
	    break;
	 case 2:
	    if (live.nat[n].canword || spec>=0) {
		answer=n;
	    }
	    break;
	 case 4:
	    answer=n;
	    break;
	 default: abort();
	}
	if (answer<0)
	    evict(r);
    }
    /* either the value was in memory to start with, or it was evicted and
       is in memory now */
    if (answer<0) {
	answer=alloc_reg_hinted(r,spec>=0?4:rsize,0,spec);
    }

    if (spec>=0 && spec!=answer) {
	/* Too bad */
	mov_nregs(spec,answer);
	answer=spec;
    }
    if (wsize>live.state[r].dirtysize)
	live.state[r].dirtysize=wsize;
    if (wsize>live.state[r].validsize)
	live.state[r].validsize=wsize;
=======
		jit_abort ("JIT: Cannot handle wsize<rsize in rmw_general()\n");
    }
    if (isinreg(r) && live.state[r].validsize>=rsize) {
		n=live.state[r].realreg;
		Dif (live.nat[n].nholds!=1)
			jit_abort ("live.nat[n].nholds!=1", n);

		switch(rsize) {
		case 1:
			if (live.nat[n].canbyte || spec>=0) {
				answer=n;
			}
			break;
		case 2:
			if (live.nat[n].canword || spec>=0) {
				answer=n;
			}
			break;
		case 4:
			answer=n;
			break;
		default: abort();
		}
		if (answer<0)
			evict(r);
    }
	/* either the value was in memory to start with, or it was evicted and
	is in memory now */
    if (answer<0) {
		answer=alloc_reg_hinted(r,spec>=0?4:rsize,0,spec);
    }

    if (spec>=0 && spec!=answer) {
		/* Too bad */
		mov_nregs(spec,answer);
		answer=spec;
    }
    if (wsize>live.state[r].dirtysize)
		live.state[r].dirtysize=wsize;
    if (wsize>live.state[r].validsize)
		live.state[r].validsize=wsize;
>>>>>>> p-uae/v2.1.0
    set_status(r,DIRTY);

    live.nat[answer].locked++;
    live.nat[answer].touched=touchcnt++;

    Dif (live.state[r].val) {
<<<<<<< HEAD
	write_log ("JIT: Problem with val(rmw)\n");
	abort();
=======
		jit_abort ("JIT: Problem with val(rmw)\n");
>>>>>>> p-uae/v2.1.0
    }
    return answer;
}

static int rmw(int r, int wsize, int rsize)
{
    return rmw_general(r,wsize,rsize,-1);
}

static int rmw_specific(int r, int wsize, int rsize, int spec)
{
    return rmw_general(r,wsize,rsize,spec);
}


/* needed for restoring the carry flag on non-P6 cores */
static void bt_l_ri_noclobber(R4 r, IMM i)
{
    int size=4;
    if (i<16)
<<<<<<< HEAD
	size=2;
=======
		size=2;
>>>>>>> p-uae/v2.1.0
    r=readreg(r,size);
    raw_bt_l_ri(r,i);
    unlock(r);
}

/********************************************************************
 * FPU register status handling. EMIT TIME!                         *
 ********************************************************************/

static  void f_tomem(int r)
{
    if (live.fate[r].status==DIRTY) {
#if USE_LONG_DOUBLE
<<<<<<< HEAD
	raw_fmov_ext_mr((uae_uintptr)live.fate[r].mem,live.fate[r].realreg);
#else
	raw_fmov_mr((uae_uintptr)live.fate[r].mem,live.fate[r].realreg);
#endif
	live.fate[r].status=CLEAN;
=======
		raw_fmov_ext_mr((uae_u32)live.fate[r].mem,live.fate[r].realreg);
#else
		raw_fmov_mr((uae_u32)live.fate[r].mem,live.fate[r].realreg);
#endif
		live.fate[r].status=CLEAN;
>>>>>>> p-uae/v2.1.0
    }
}

static  void f_tomem_drop(int r)
{
    if (live.fate[r].status==DIRTY) {
#if USE_LONG_DOUBLE
<<<<<<< HEAD
	raw_fmov_ext_mr_drop((uae_uintptr)live.fate[r].mem,live.fate[r].realreg);
#else
	raw_fmov_mr_drop((uae_uintptr)live.fate[r].mem,live.fate[r].realreg);
#endif
	live.fate[r].status=INMEM;
=======
		raw_fmov_ext_mr_drop((uae_u32)live.fate[r].mem,live.fate[r].realreg);
#else
		raw_fmov_mr_drop((uae_u32)live.fate[r].mem,live.fate[r].realreg);
#endif
		live.fate[r].status=INMEM;
>>>>>>> p-uae/v2.1.0
    }
}


STATIC_INLINE int f_isinreg(int r)
{
    return live.fate[r].status==CLEAN || live.fate[r].status==DIRTY;
}

static void f_evict(int r)
{
    int rr;

    if (!f_isinreg(r))
<<<<<<< HEAD
	return;
    rr=live.fate[r].realreg;
    if (live.fat[rr].nholds==1)
	f_tomem_drop(r);
    else
	f_tomem(r);

    Dif (live.fat[rr].locked &&
	live.fat[rr].nholds==1) {
	write_log ("JIT: FPU register %d in nreg %d is locked!\n", r, live.fate[r].realreg);
	abort();
=======
		return;
    rr=live.fate[r].realreg;
    if (live.fat[rr].nholds==1)
		f_tomem_drop(r);
    else
		f_tomem(r);

	Dif (live.fat[rr].locked && live.fat[rr].nholds==1) {
		jit_abort ("JIT: FPU register %d in nreg %d is locked!\n",r,live.fate[r].realreg);
>>>>>>> p-uae/v2.1.0
    }

    live.fat[rr].nholds--;
    if (live.fat[rr].nholds!=live.fate[r].realind) { /* Was not last */
<<<<<<< HEAD
	int topreg=live.fat[rr].holds[live.fat[rr].nholds];
	int thisind=live.fate[r].realind;
	live.fat[rr].holds[thisind]=topreg;
	live.fate[topreg].realind=thisind;
=======
		int topreg=live.fat[rr].holds[live.fat[rr].nholds];
		int thisind=live.fate[r].realind;
		live.fat[rr].holds[thisind]=topreg;
		live.fate[topreg].realind=thisind;
>>>>>>> p-uae/v2.1.0
    }
    live.fate[r].status=INMEM;
    live.fate[r].realreg=-1;
}

STATIC_INLINE void f_free_nreg(int r)
{
    int i=live.fat[r].nholds;

    while (i) {
<<<<<<< HEAD
	int vr;

	--i;
	vr=live.fat[r].holds[i];
	f_evict(vr);
    }
    Dif (live.fat[r].nholds!=0) {
	write_log ("JIT: Failed to free nreg %d, nholds is %d\n", r, live.fat[r].nholds);
	abort();
=======
		int vr;

		--i;
		vr=live.fat[r].holds[i];
		f_evict(vr);
    }
    Dif (live.fat[r].nholds!=0) {
		jit_abort ("JIT: Failed to free nreg %d, nholds is %d\n",r,live.fat[r].nholds);
>>>>>>> p-uae/v2.1.0
    }
}


/* Use with care! */
STATIC_INLINE void f_isclean(int r)
{
    if (!f_isinreg(r))
<<<<<<< HEAD
	return;
=======
		return;
>>>>>>> p-uae/v2.1.0
    live.fate[r].status=CLEAN;
}

STATIC_INLINE void f_disassociate(int r)
{
    f_isclean(r);
    f_evict(r);
}



static  int f_alloc_reg(int r, int willclobber)
{
    int bestreg;
    uae_s32 when;
    int i;
    uae_s32 badness;
    bestreg=-1;
    when=2000000000;
    for (i=N_FREGS;i--;) {
<<<<<<< HEAD
	badness=live.fat[i].touched;
	if (live.fat[i].nholds==0)
	    badness=0;

	if (!live.fat[i].locked && badness<when) {
	    bestreg=i;
	    when=badness;
	    if (live.fat[i].nholds==0)
		break;
	}
    }
    Dif (bestreg==-1)
	abort();

    if (live.fat[bestreg].nholds>0) {
	f_free_nreg(bestreg);
    }
    if (f_isinreg(r)) {
	f_evict(r);
    }

    if (!willclobber) {
	if (live.fate[r].status!=UNDEF) {
#if USE_LONG_DOUBLE
	    raw_fmov_ext_rm(bestreg,(uae_uintptr)live.fate[r].mem);
#else
	    raw_fmov_rm(bestreg,(uae_uintptr)live.fate[r].mem);
#endif
	}
	live.fate[r].status=CLEAN;
    }
    else {
	live.fate[r].status=DIRTY;
=======
		badness=live.fat[i].touched;
		if (live.fat[i].nholds==0)
		    badness=0;

		if (!live.fat[i].locked && badness<when) {
		    bestreg=i;
		    when=badness;
		    if (live.fat[i].nholds==0)
				break;
		}
    }
    Dif (bestreg==-1)
		abort();

    if (live.fat[bestreg].nholds>0) {
		f_free_nreg(bestreg);
    }
    if (f_isinreg(r)) {
		f_evict(r);
    }

    if (!willclobber) {
		if (live.fate[r].status!=UNDEF) {
#if USE_LONG_DOUBLE
		    raw_fmov_ext_rm(bestreg,(uae_u32)live.fate[r].mem);
#else
		    raw_fmov_rm(bestreg,(uae_u32)live.fate[r].mem);
#endif
		}
		live.fate[r].status=CLEAN;
    }
    else {
		live.fate[r].status=DIRTY;
>>>>>>> p-uae/v2.1.0
    }
    live.fate[r].realreg=bestreg;
    live.fate[r].realind=live.fat[bestreg].nholds;
    live.fat[bestreg].touched=touchcnt++;
    live.fat[bestreg].holds[live.fat[bestreg].nholds]=r;
    live.fat[bestreg].nholds++;

    return bestreg;
}

static  void f_unlock(int r)
{
    Dif (!live.fat[r].locked)
<<<<<<< HEAD
	abort();
=======
		jit_abort ("unlock %d", r);
>>>>>>> p-uae/v2.1.0
    live.fat[r].locked--;
}

static  void f_setlock(int r)
{
    live.fat[r].locked++;
}

<<<<<<< HEAD
static __inline__ int f_readreg(int r)
=======
STATIC_INLINE int f_readreg(int r)
>>>>>>> p-uae/v2.1.0
{
    int n;
    int answer=-1;

    if (f_isinreg(r)) {
<<<<<<< HEAD
	n=live.fate[r].realreg;
	answer=n;
    }
    /* either the value was in memory to start with, or it was evicted and
       is in memory now */
    if (answer<0)
	answer=f_alloc_reg(r,0);
=======
		n=live.fate[r].realreg;
		answer=n;
    }
    /* either the value was in memory to start with, or it was evicted and
    is in memory now */
    if (answer<0)
		answer=f_alloc_reg(r,0);
>>>>>>> p-uae/v2.1.0

    live.fat[answer].locked++;
    live.fat[answer].touched=touchcnt++;
    return answer;
}

<<<<<<< HEAD
static __inline__ void f_make_exclusive(int r, int clobber)
=======
STATIC_INLINE void f_make_exclusive(int r, int clobber)
>>>>>>> p-uae/v2.1.0
{
    freg_status oldstate;
    int rr=live.fate[r].realreg;
    int nr;
    int nind;
    int ndirt=0;
    int i;

    if (!f_isinreg(r))
<<<<<<< HEAD
	return;
    if (live.fat[rr].nholds==1)
	return;
    for (i=0;i<live.fat[rr].nholds;i++) {
	int vr=live.fat[rr].holds[i];
	if (vr!=r && live.fate[vr].status==DIRTY)
	    ndirt++;
    }
    if (!ndirt && !live.fat[rr].locked) {
	/* Everything else is clean, so let's keep this register */
	for (i=0;i<live.fat[rr].nholds;i++) {
	    int vr=live.fat[rr].holds[i];
	    if (vr!=r) {
		f_evict(vr);
		i--; /* Try that index again! */
	    }
	}
	Dif (live.fat[rr].nholds!=1) {
	    write_log ("JIT: realreg %d holds %d (", rr, live.fat[rr].nholds);
	    for (i=0;i<live.fat[rr].nholds;i++) {
		write_log (" %d(%d,%d)", live.fat[rr].holds[i],
					 live.fate[live.fat[rr].holds[i]].realreg,
					 live.fate[live.fat[rr].holds[i]].realind);
	    }
	    write_log ("\n");
	    abort();
	}
	return;
=======
		return;
    if (live.fat[rr].nholds==1)
		return;
    for (i=0;i<live.fat[rr].nholds;i++) {
		int vr=live.fat[rr].holds[i];
		if (vr!=r && live.fate[vr].status==DIRTY)
		    ndirt++;
    }
    if (!ndirt && !live.fat[rr].locked) {
		/* Everything else is clean, so let's keep this register */
		for (i=0;i<live.fat[rr].nholds;i++) {
		    int vr=live.fat[rr].holds[i];
		    if (vr!=r) {
				f_evict(vr);
				i--; /* Try that index again! */
		    }
		}
		Dif (live.fat[rr].nholds!=1) {
			write_log ("JIT: realreg %d holds %d (",rr,live.fat[rr].nholds);
		    for (i=0;i<live.fat[rr].nholds;i++) {
				write_log ("JIT: %d(%d,%d)",live.fat[rr].holds[i],
					 live.fate[live.fat[rr].holds[i]].realreg,
					 live.fate[live.fat[rr].holds[i]].realind);
		    }
			write_log ("\n");
			jit_abort ("x");
		}
		return;
>>>>>>> p-uae/v2.1.0
    }

    /* We have to split the register */
    oldstate=live.fate[r];

    f_setlock(rr); /* Make sure this doesn't go away */
    /* Forget about r being in the register rr */
    f_disassociate(r);
    /* Get a new register, that we will clobber completely */
    nr=f_alloc_reg(r,1);
    nind=live.fate[r].realind;
    if (!clobber)
<<<<<<< HEAD
	raw_fmov_rr(nr,rr);  /* Make another copy */
=======
		raw_fmov_rr(nr,rr);  /* Make another copy */
>>>>>>> p-uae/v2.1.0
    live.fate[r]=oldstate;   /* Keep all the old state info */
    live.fate[r].realreg=nr;
    live.fate[r].realind=nind;
    f_unlock(rr);
}


<<<<<<< HEAD
static __inline__ int f_writereg(int r)
=======
STATIC_INLINE int f_writereg(int r)
>>>>>>> p-uae/v2.1.0
{
    int n;
    int answer=-1;

    f_make_exclusive(r,1);
    if (f_isinreg(r)) {
<<<<<<< HEAD
	n=live.fate[r].realreg;
	answer=n;
    }
    if (answer<0) {
	answer=f_alloc_reg(r,1);
=======
		n=live.fate[r].realreg;
		answer=n;
    }
    if (answer<0) {
		answer=f_alloc_reg(r,1);
>>>>>>> p-uae/v2.1.0
    }
    live.fate[r].status=DIRTY;
    live.fat[answer].locked++;
    live.fat[answer].touched=touchcnt++;
    return answer;
}

static int f_rmw(int r)
{
    int n;

    f_make_exclusive(r,0);
    if (f_isinreg(r)) {
<<<<<<< HEAD
	n=live.fate[r].realreg;
    }
    else
	n=f_alloc_reg(r,0);
=======
		n=live.fate[r].realreg;
    }
    else
		n=f_alloc_reg(r,0);
>>>>>>> p-uae/v2.1.0
    live.fate[r].status=DIRTY;
    live.fat[n].locked++;
    live.fat[n].touched=touchcnt++;
    return n;
}

static void fflags_into_flags_internal(uae_u32 tmp)
{
    int r;

    clobber_flags();
    r=f_readreg(FP_RESULT);
    raw_fflags_into_flags(r);
    f_unlock(r);
}




/********************************************************************
 * CPU functions exposed to gencomp. Both CREATE and EMIT time      *
 ********************************************************************/

/*
 *  RULES FOR HANDLING REGISTERS:
 *
 *  * In the function headers, order the parameters
 *     - 1st registers written to
 *     - 2nd read/modify/write registers
 *     - 3rd registers read from
 *  * Before calling raw_*, you must call readreg, writereg or rmw for
 *    each register
 *  * The order for this is
 *     - 1st call remove_offset for all registers written to with size<4
 *     - 2nd call readreg for all registers read without offset
 *     - 3rd call rmw for all rmw registers
 *     - 4th call readreg_offset for all registers that can handle offsets
 *     - 5th call get_offset for all the registers from the previous step
 *     - 6th call writereg for all written-to registers
 *     - 7th call raw_*
 *     - 8th unlock all registers that were locked
 */

MIDFUNC(0,live_flags,(void))
{
    live.flags_on_stack=TRASH;
    live.flags_in_flags=VALID;
    live.flags_are_important=1;
}
MENDFUNC(0,live_flags,(void))

MIDFUNC(0,dont_care_flags,(void))
{
    live.flags_are_important=0;
}
MENDFUNC(0,dont_care_flags,(void))

<<<<<<< HEAD
/*
 * Copy m68k C flag into m68k X flag
 *
 * FIXME: This needs to be moved into the machdep
 * part of the source because it depends on what bit
 * is used to hold X.
 */
MIDFUNC(0,duplicate_carry,(void))
{
    evict(FLAGX);
    make_flags_live_internal();
    COMPCALL(setcc_m)((uae_uintptr)live.state[FLAGX].mem + 1, 2);
}
MENDFUNC(0,duplicate_carry,(void))

/*
 * Set host C flag from m68k X flag.
 *
 * FIXME: This needs to be moved into the machdep
 * part of the source because it depends on what bit
 * is used to hold X.
 */
MIDFUNC(0,restore_carry,(void))
{
    if (!have_rat_stall) { /* Not a P6 core, i.e. no partial stalls */
	bt_l_ri_noclobber(FLAGX, 8);
=======

	/*
	 * Copy m68k C flag into m68k X flag
	 *
	 * FIXME: This needs to be moved into the machdep
	 * part of the source because it depends on what bit
	 * is used to hold X.
	 */
	MIDFUNC(0,duplicate_carry,(void))
{
    evict(FLAGX);
    make_flags_live_internal();
    COMPCALL(setcc_m)((uae_u32)live.state[FLAGX].mem + 1,2);
}
MENDFUNC(0,duplicate_carry,(void))

	/*
	 * Set host C flag from m68k X flag.
	 *
	 * FIXME: This needs to be moved into the machdep
	 * part of the source because it depends on what bit
	 * is used to hold X.
	 */
	MIDFUNC(0,restore_carry,(void))
{
    if (!have_rat_stall) { /* Not a P6 core, i.e. no partial stalls */
		bt_l_ri_noclobber(FLAGX, 8);
>>>>>>> p-uae/v2.1.0
    }
    else {  /* Avoid the stall the above creates.
	       This is slow on non-P6, though.
	    */
<<<<<<< HEAD
	COMPCALL(rol_w_ri(FLAGX, 8));
	isclean(FLAGX);
	/* Why is the above faster than the below? */
	//raw_rol_b_mi((uae_uintptr)live.state[FLAGX].mem,8);
=======
		COMPCALL(rol_w_ri(FLAGX, 8));
		isclean(FLAGX);
		/* Why is the above faster than the below? */
		//raw_rol_b_mi((uae_u32)live.state[FLAGX].mem,8);
>>>>>>> p-uae/v2.1.0
    }
}
MENDFUNC(0,restore_carry,(void))

<<<<<<< HEAD
MIDFUNC(0,start_needflags,(void))
=======
	MIDFUNC(0,start_needflags,(void))
>>>>>>> p-uae/v2.1.0
{
    needflags=1;
}
MENDFUNC(0,start_needflags,(void))

<<<<<<< HEAD
MIDFUNC(0,end_needflags,(void))
=======
	MIDFUNC(0,end_needflags,(void))
>>>>>>> p-uae/v2.1.0
{
    needflags=0;
}
MENDFUNC(0,end_needflags,(void))

<<<<<<< HEAD
MIDFUNC(0,make_flags_live,(void))
=======
	MIDFUNC(0,make_flags_live,(void))
>>>>>>> p-uae/v2.1.0
{
    make_flags_live_internal();
}
MENDFUNC(0,make_flags_live,(void))

<<<<<<< HEAD
MIDFUNC(1,fflags_into_flags,(W2 tmp))
=======
	MIDFUNC(1,fflags_into_flags,(W2 tmp))
>>>>>>> p-uae/v2.1.0
{
    clobber_flags();
    fflags_into_flags_internal(tmp);
}
MENDFUNC(1,fflags_into_flags,(W2 tmp))


<<<<<<< HEAD
MIDFUNC(2,bt_l_ri,(R4 r, IMM i)) /* This is defined as only affecting C */
{
    int size=4;
    if (i<16)
	size=2;
=======
	MIDFUNC(2,bt_l_ri,(R4 r, IMM i)) /* This is defined as only affecting C */
{
    int size=4;
    if (i<16)
		size=2;
>>>>>>> p-uae/v2.1.0
    CLOBBER_BT;
    r=readreg(r,size);
    raw_bt_l_ri(r,i);
    unlock(r);
}
MENDFUNC(2,bt_l_ri,(R4 r, IMM i)) /* This is defined as only affecting C */

<<<<<<< HEAD
MIDFUNC(2,bt_l_rr,(R4 r, R4 b)) /* This is defined as only affecting C */
=======
	MIDFUNC(2,bt_l_rr,(R4 r, R4 b)) /* This is defined as only affecting C */
>>>>>>> p-uae/v2.1.0
{
    CLOBBER_BT;
    r=readreg(r,4);
    b=readreg(b,4);
    raw_bt_l_rr(r,b);
    unlock(r);
    unlock(b);
}
MENDFUNC(2,bt_l_rr,(R4 r, R4 b)) /* This is defined as only affecting C */

MIDFUNC(2,btc_l_ri,(RW4 r, IMM i))
{
    int size=4;
    if (i<16)
<<<<<<< HEAD
	size=2;
=======
		size=2;
>>>>>>> p-uae/v2.1.0
    CLOBBER_BT;
    r=rmw(r,size,size);
    raw_btc_l_ri(r,i);
    unlock(r);
}
MENDFUNC(2,btc_l_ri,(RW4 r, IMM i))

MIDFUNC(2,btc_l_rr,(RW4 r, R4 b))
{
    CLOBBER_BT;
    b=readreg(b,4);
    r=rmw(r,4,4);
    raw_btc_l_rr(r,b);
    unlock(r);
    unlock(b);
}
MENDFUNC(2,btc_l_rr,(RW4 r, R4 b))


MIDFUNC(2,btr_l_ri,(RW4 r, IMM i))
{
    int size=4;
    if (i<16)
	size=2;
    CLOBBER_BT;
    r=rmw(r,size,size);
    raw_btr_l_ri(r,i);
    unlock(r);
}
MENDFUNC(2,btr_l_ri,(RW4 r, IMM i))

MIDFUNC(2,btr_l_rr,(RW4 r, R4 b))
{
    CLOBBER_BT;
    b=readreg(b,4);
    r=rmw(r,4,4);
    raw_btr_l_rr(r,b);
    unlock(r);
    unlock(b);
}
MENDFUNC(2,btr_l_rr,(RW4 r, R4 b))


MIDFUNC(2,bts_l_ri,(RW4 r, IMM i))
{
    int size=4;
    if (i<16)
<<<<<<< HEAD
	size=2;
=======
		size=2;
>>>>>>> p-uae/v2.1.0
    CLOBBER_BT;
    r=rmw(r,size,size);
    raw_bts_l_ri(r,i);
    unlock(r);
}
MENDFUNC(2,bts_l_ri,(RW4 r, IMM i))

MIDFUNC(2,bts_l_rr,(RW4 r, R4 b))
{
    CLOBBER_BT;
    b=readreg(b,4);
    r=rmw(r,4,4);
    raw_bts_l_rr(r,b);
    unlock(r);
    unlock(b);
}
MENDFUNC(2,bts_l_rr,(RW4 r, R4 b))

MIDFUNC(2,mov_l_rm,(W4 d, IMM s))
{
    CLOBBER_MOV;
    d=writereg(d,4);
    raw_mov_l_rm(d,s);
    unlock(d);
}
MENDFUNC(2,mov_l_rm,(W4 d, IMM s))


MIDFUNC(1,call_r,(R4 r)) /* Clobbering is implicit */
{
    r=readreg(r,4);
    raw_call_r(r);
    unlock(r);
}
MENDFUNC(1,call_r,(R4 r)) /* Clobbering is implicit */

MIDFUNC(2,sub_l_mi,(IMM d, IMM s))
{
    CLOBBER_SUB;
    raw_sub_l_mi(d,s) ;
}
MENDFUNC(2,sub_l_mi,(IMM d, IMM s))

MIDFUNC(2,mov_l_mi,(IMM d, IMM s))
{
    CLOBBER_MOV;
    raw_mov_l_mi(d,s) ;
}
MENDFUNC(2,mov_l_mi,(IMM d, IMM s))

MIDFUNC(2,mov_w_mi,(IMM d, IMM s))
{
    CLOBBER_MOV;
    raw_mov_w_mi(d,s) ;
}
MENDFUNC(2,mov_w_mi,(IMM d, IMM s))

MIDFUNC(2,mov_b_mi,(IMM d, IMM s))
{
    CLOBBER_MOV;
    raw_mov_b_mi(d,s) ;
}
MENDFUNC(2,mov_b_mi,(IMM d, IMM s))

MIDFUNC(2,rol_b_ri,(RW1 r, IMM i))
{
<<<<<<< HEAD
		if (!i && !needflags)
=======
	if (!i && !needflags)
>>>>>>> p-uae/v2.1.0
		return;
    CLOBBER_ROL;
    r=rmw(r,1,1);
    raw_rol_b_ri(r,i);
    unlock(r);
}
MENDFUNC(2,rol_b_ri,(RW1 r, IMM i))

MIDFUNC(2,rol_w_ri,(RW2 r, IMM i))
{
<<<<<<< HEAD
		if (!i && !needflags)
=======
	if (!i && !needflags)
>>>>>>> p-uae/v2.1.0
		return;
    CLOBBER_ROL;
    r=rmw(r,2,2);
    raw_rol_w_ri(r,i);
    unlock(r);
}
MENDFUNC(2,rol_w_ri,(RW2 r, IMM i))

MIDFUNC(2,rol_l_ri,(RW4 r, IMM i))
{
		if (!i && !needflags)
		return;
    CLOBBER_ROL;
    r=rmw(r,4,4);
    raw_rol_l_ri(r,i);
    unlock(r);
}
MENDFUNC(2,rol_l_ri,(RW4 r, IMM i))

MIDFUNC(2,rol_l_rr,(RW4 d, R1 r))
{
    if (isconst(r)) {
<<<<<<< HEAD
	COMPCALL(rol_l_ri)(d,(uae_u8)live.state[r].val);
	return;
=======
		COMPCALL(rol_l_ri)(d,(uae_u8)live.state[r].val);
		return;
>>>>>>> p-uae/v2.1.0
    }
    CLOBBER_ROL;
    r=readreg_specific(r,1,SHIFTCOUNT_NREG);
    d=rmw(d,4,4);
    Dif (r!=1) {
<<<<<<< HEAD
	write_log ("JIT: Illegal register %d in raw_rol_b\n", r);
	abort();
=======
		jit_abort ("JIT: Illegal register %d in raw_rol_b\n",r);
>>>>>>> p-uae/v2.1.0
    }
    raw_rol_l_rr(d,r) ;
    unlock(r);
    unlock(d);
}
MENDFUNC(2,rol_l_rr,(RW4 d, R1 r))

MIDFUNC(2,rol_w_rr,(RW2 d, R1 r))
{ /* Can only do this with r==1, i.e. cl */

    if (isconst(r)) {
<<<<<<< HEAD
	COMPCALL(rol_w_ri)(d,(uae_u8)live.state[r].val);
	return;
=======
		COMPCALL(rol_w_ri)(d,(uae_u8)live.state[r].val);
		return;
>>>>>>> p-uae/v2.1.0
    }
    CLOBBER_ROL;
    r=readreg_specific(r,1,SHIFTCOUNT_NREG);
    d=rmw(d,2,2);
    Dif (r!=1) {
<<<<<<< HEAD
	write_log ("JIT: Illegal register %d in raw_rol_b\n", r);
	abort();
=======
		jit_abort ("JIT: Illegal register %d in raw_rol_b\n",r);
>>>>>>> p-uae/v2.1.0
    }
    raw_rol_w_rr(d,r) ;
    unlock(r);
    unlock(d);
}
MENDFUNC(2,rol_w_rr,(RW2 d, R1 r))

MIDFUNC(2,rol_b_rr,(RW1 d, R1 r))
{ /* Can only do this with r==1, i.e. cl */

    if (isconst(r)) {
<<<<<<< HEAD
	COMPCALL(rol_b_ri)(d,(uae_u8)live.state[r].val);
	return;
=======
		COMPCALL(rol_b_ri)(d,(uae_u8)live.state[r].val);
		return;
>>>>>>> p-uae/v2.1.0
    }

    CLOBBER_ROL;
    r=readreg_specific(r,1,SHIFTCOUNT_NREG);
    d=rmw(d,1,1);
    Dif (r!=1) {
<<<<<<< HEAD
	write_log ("JIT: Illegal register %d in raw_rol_b\n", r);
	abort();
=======
		jit_abort ("JIT: Illegal register %d in raw_rol_b\n",r);
>>>>>>> p-uae/v2.1.0
    }
    raw_rol_b_rr(d,r) ;
    unlock(r);
    unlock(d);
}
MENDFUNC(2,rol_b_rr,(RW1 d, R1 r))


MIDFUNC(2,shll_l_rr,(RW4 d, R1 r))
{
    if (isconst(r)) {
<<<<<<< HEAD
	COMPCALL(shll_l_ri)(d,(uae_u8)live.state[r].val);
	return;
=======
		COMPCALL(shll_l_ri)(d,(uae_u8)live.state[r].val);
		return;
>>>>>>> p-uae/v2.1.0
    }
    CLOBBER_SHLL;
    r=readreg_specific(r,1,SHIFTCOUNT_NREG);
    d=rmw(d,4,4);
    Dif (r!=1) {
<<<<<<< HEAD
	write_log ("JIT: Illegal register %d in raw_rol_b\n", r);
	abort();
=======
		jit_abort ("JIT: Illegal register %d in raw_rol_b\n",r);
>>>>>>> p-uae/v2.1.0
    }
    raw_shll_l_rr(d,r) ;
    unlock(r);
    unlock(d);
}
MENDFUNC(2,shll_l_rr,(RW4 d, R1 r))

MIDFUNC(2,shll_w_rr,(RW2 d, R1 r))
{ /* Can only do this with r==1, i.e. cl */

    if (isconst(r)) {
<<<<<<< HEAD
	COMPCALL(shll_w_ri)(d,(uae_u8)live.state[r].val);
	return;
=======
		COMPCALL(shll_w_ri)(d,(uae_u8)live.state[r].val);
		return;
>>>>>>> p-uae/v2.1.0
    }
    CLOBBER_SHLL;
    r=readreg_specific(r,1,SHIFTCOUNT_NREG);
    d=rmw(d,2,2);
    Dif (r!=1) {
<<<<<<< HEAD
	write_log ("JIT: Illegal register %d in raw_shll_b\n", r);
	abort();
=======
		jit_abort ("JIT: Illegal register %d in raw_shll_b\n",r);
>>>>>>> p-uae/v2.1.0
    }
    raw_shll_w_rr(d,r) ;
    unlock(r);
    unlock(d);
}
MENDFUNC(2,shll_w_rr,(RW2 d, R1 r))

MIDFUNC(2,shll_b_rr,(RW1 d, R1 r))
{ /* Can only do this with r==1, i.e. cl */

    if (isconst(r)) {
<<<<<<< HEAD
	COMPCALL(shll_b_ri)(d,(uae_u8)live.state[r].val);
	return;
=======
		COMPCALL(shll_b_ri)(d,(uae_u8)live.state[r].val);
		return;
>>>>>>> p-uae/v2.1.0
    }

    CLOBBER_SHLL;
    r=readreg_specific(r,1,SHIFTCOUNT_NREG);
    d=rmw(d,1,1);
    Dif (r!=1) {
<<<<<<< HEAD
	write_log ("JIT: Illegal register %d in raw_shll_b\n", r);
	abort();
=======
		jit_abort ("JIT: Illegal register %d in raw_shll_b\n",r);
>>>>>>> p-uae/v2.1.0
    }
    raw_shll_b_rr(d,r) ;
    unlock(r);
    unlock(d);
}
MENDFUNC(2,shll_b_rr,(RW1 d, R1 r))


MIDFUNC(2,ror_b_ri,(R1 r, IMM i))
{
<<<<<<< HEAD
		if (!i && !needflags)
=======
	if (!i && !needflags)
>>>>>>> p-uae/v2.1.0
		return;
    CLOBBER_ROR;
    r=rmw(r,1,1);
    raw_ror_b_ri(r,i);
    unlock(r);
}
MENDFUNC(2,ror_b_ri,(R1 r, IMM i))

MIDFUNC(2,ror_w_ri,(R2 r, IMM i))
{
<<<<<<< HEAD
		if (!i && !needflags)
=======
	if (!i && !needflags)
>>>>>>> p-uae/v2.1.0
		return;
    CLOBBER_ROR;
    r=rmw(r,2,2);
    raw_ror_w_ri(r,i);
    unlock(r);
}
MENDFUNC(2,ror_w_ri,(R2 r, IMM i))

MIDFUNC(2,ror_l_ri,(R4 r, IMM i))
{
<<<<<<< HEAD
		if (!i && !needflags)
=======
	if (!i && !needflags)
>>>>>>> p-uae/v2.1.0
		return;
    CLOBBER_ROR;
    r=rmw(r,4,4);
    raw_ror_l_ri(r,i);
    unlock(r);
}
MENDFUNC(2,ror_l_ri,(R4 r, IMM i))

MIDFUNC(2,ror_l_rr,(R4 d, R1 r))
{
    if (isconst(r)) {
<<<<<<< HEAD
	COMPCALL(ror_l_ri)(d,(uae_u8)live.state[r].val);
	return;
=======
		COMPCALL(ror_l_ri)(d,(uae_u8)live.state[r].val);
		return;
>>>>>>> p-uae/v2.1.0
    }
    CLOBBER_ROR;
    r=readreg_specific(r,1,SHIFTCOUNT_NREG);
    d=rmw(d,4,4);
    raw_ror_l_rr(d,r) ;
    unlock(r);
    unlock(d);
}
MENDFUNC(2,ror_l_rr,(R4 d, R1 r))

MIDFUNC(2,ror_w_rr,(R2 d, R1 r))
{
    if (isconst(r)) {
<<<<<<< HEAD
	COMPCALL(ror_w_ri)(d,(uae_u8)live.state[r].val);
	return;
=======
		COMPCALL(ror_w_ri)(d,(uae_u8)live.state[r].val);
		return;
>>>>>>> p-uae/v2.1.0
    }
    CLOBBER_ROR;
    r=readreg_specific(r,1,SHIFTCOUNT_NREG);
    d=rmw(d,2,2);
    raw_ror_w_rr(d,r) ;
    unlock(r);
    unlock(d);
}
MENDFUNC(2,ror_w_rr,(R2 d, R1 r))

MIDFUNC(2,ror_b_rr,(R1 d, R1 r))
{
    if (isconst(r)) {
<<<<<<< HEAD
	COMPCALL(ror_b_ri)(d,(uae_u8)live.state[r].val);
	return;
=======
		COMPCALL(ror_b_ri)(d,(uae_u8)live.state[r].val);
		return;
>>>>>>> p-uae/v2.1.0
    }

    CLOBBER_ROR;
    r=readreg_specific(r,1,SHIFTCOUNT_NREG);
    d=rmw(d,1,1);
    raw_ror_b_rr(d,r) ;
    unlock(r);
    unlock(d);
}
MENDFUNC(2,ror_b_rr,(R1 d, R1 r))

MIDFUNC(2,shrl_l_rr,(RW4 d, R1 r))
{
    if (isconst(r)) {
<<<<<<< HEAD
	COMPCALL(shrl_l_ri)(d,(uae_u8)live.state[r].val);
	return;
=======
		COMPCALL(shrl_l_ri)(d,(uae_u8)live.state[r].val);
		return;
>>>>>>> p-uae/v2.1.0
    }
    CLOBBER_SHRL;
    r=readreg_specific(r,1,SHIFTCOUNT_NREG);
    d=rmw(d,4,4);
    Dif (r!=1) {
<<<<<<< HEAD
	write_log ("JIT: Illegal register %d in raw_rol_b\n", r);
	abort();
=======
		jit_abort ("JIT: Illegal register %d in raw_rol_b\n",r);
>>>>>>> p-uae/v2.1.0
    }
    raw_shrl_l_rr(d,r) ;
    unlock(r);
    unlock(d);
}
MENDFUNC(2,shrl_l_rr,(RW4 d, R1 r))

MIDFUNC(2,shrl_w_rr,(RW2 d, R1 r))
{ /* Can only do this with r==1, i.e. cl */

    if (isconst(r)) {
<<<<<<< HEAD
	COMPCALL(shrl_w_ri)(d,(uae_u8)live.state[r].val);
	return;
=======
		COMPCALL(shrl_w_ri)(d,(uae_u8)live.state[r].val);
		return;
>>>>>>> p-uae/v2.1.0
    }
    CLOBBER_SHRL;
    r=readreg_specific(r,1,SHIFTCOUNT_NREG);
    d=rmw(d,2,2);
    Dif (r!=1) {
<<<<<<< HEAD
	write_log ("JIT: Illegal register %d in raw_shrl_b\n", r);
	abort();
=======
		jit_abort ("JIT: Illegal register %d in raw_shrl_b\n",r);
>>>>>>> p-uae/v2.1.0
    }
    raw_shrl_w_rr(d,r) ;
    unlock(r);
    unlock(d);
}
MENDFUNC(2,shrl_w_rr,(RW2 d, R1 r))

MIDFUNC(2,shrl_b_rr,(RW1 d, R1 r))
{ /* Can only do this with r==1, i.e. cl */

    if (isconst(r)) {
<<<<<<< HEAD
	COMPCALL(shrl_b_ri)(d,(uae_u8)live.state[r].val);
	return;
=======
		COMPCALL(shrl_b_ri)(d,(uae_u8)live.state[r].val);
		return;
>>>>>>> p-uae/v2.1.0
    }

    CLOBBER_SHRL;
    r=readreg_specific(r,1,SHIFTCOUNT_NREG);
    d=rmw(d,1,1);
    Dif (r!=1) {
<<<<<<< HEAD
	write_log ("JIT: Illegal register %d in raw_shrl_b\n", r);
	abort();
=======
		jit_abort ("JIT: Illegal register %d in raw_shrl_b\n",r);
>>>>>>> p-uae/v2.1.0
    }
    raw_shrl_b_rr(d,r) ;
    unlock(r);
    unlock(d);
}
MENDFUNC(2,shrl_b_rr,(RW1 d, R1 r))

<<<<<<< HEAD


MIDFUNC(2,shll_l_ri,(RW4 r, IMM i))
{
    if (!i && !needflags)
	return;
    if (isconst(r) && !needflags) {
	live.state[r].val<<=i;
	return;
=======
MIDFUNC(2,shll_l_ri,(RW4 r, IMM i))
{
    if (!i && !needflags)
		return;
    if (isconst(r) && !needflags) {
		live.state[r].val<<=i;
		return;
>>>>>>> p-uae/v2.1.0
    }
    CLOBBER_SHLL;
    r=rmw(r,4,4);
    raw_shll_l_ri(r,i);
    unlock(r);
}
MENDFUNC(2,shll_l_ri,(RW4 r, IMM i))

MIDFUNC(2,shll_w_ri,(RW2 r, IMM i))
{
    if (!i && !needflags)
<<<<<<< HEAD
	return;
=======
		return;
>>>>>>> p-uae/v2.1.0
    CLOBBER_SHLL;
    r=rmw(r,2,2);
    raw_shll_w_ri(r,i);
    unlock(r);
}
MENDFUNC(2,shll_w_ri,(RW2 r, IMM i))

MIDFUNC(2,shll_b_ri,(RW1 r, IMM i))
{
    if (!i && !needflags)
<<<<<<< HEAD
	return;
=======
		return;
>>>>>>> p-uae/v2.1.0
    CLOBBER_SHLL;
    r=rmw(r,1,1);
    raw_shll_b_ri(r,i);
    unlock(r);
}
MENDFUNC(2,shll_b_ri,(RW1 r, IMM i))

MIDFUNC(2,shrl_l_ri,(RW4 r, IMM i))
{
    if (!i && !needflags)
<<<<<<< HEAD
	return;
    if (isconst(r) && !needflags) {
	live.state[r].val>>=i;
	return;
=======
		return;
    if (isconst(r) && !needflags) {
		live.state[r].val>>=i;
		return;
>>>>>>> p-uae/v2.1.0
    }
    CLOBBER_SHRL;
    r=rmw(r,4,4);
    raw_shrl_l_ri(r,i);
    unlock(r);
}
MENDFUNC(2,shrl_l_ri,(RW4 r, IMM i))

MIDFUNC(2,shrl_w_ri,(RW2 r, IMM i))
{
    if (!i && !needflags)
<<<<<<< HEAD
	return;
=======
		return;
>>>>>>> p-uae/v2.1.0
    CLOBBER_SHRL;
    r=rmw(r,2,2);
    raw_shrl_w_ri(r,i);
    unlock(r);
}
MENDFUNC(2,shrl_w_ri,(RW2 r, IMM i))

MIDFUNC(2,shrl_b_ri,(RW1 r, IMM i))
{
    if (!i && !needflags)
<<<<<<< HEAD
	return;
=======
		return;
>>>>>>> p-uae/v2.1.0
    CLOBBER_SHRL;
    r=rmw(r,1,1);
    raw_shrl_b_ri(r,i);
    unlock(r);
}
MENDFUNC(2,shrl_b_ri,(RW1 r, IMM i))

MIDFUNC(2,shra_l_ri,(RW4 r, IMM i))
{
    if (!i && !needflags)
<<<<<<< HEAD
	return;
=======
		return;
>>>>>>> p-uae/v2.1.0
    CLOBBER_SHRA;
    r=rmw(r,4,4);
    raw_shra_l_ri(r,i);
    unlock(r);
}
MENDFUNC(2,shra_l_ri,(RW4 r, IMM i))

MIDFUNC(2,shra_w_ri,(RW2 r, IMM i))
{
    if (!i && !needflags)
<<<<<<< HEAD
	return;
=======
		return;
>>>>>>> p-uae/v2.1.0
    CLOBBER_SHRA;
    r=rmw(r,2,2);
    raw_shra_w_ri(r,i);
    unlock(r);
}
MENDFUNC(2,shra_w_ri,(RW2 r, IMM i))

<<<<<<< HEAD
MIDFUNC(2,shra_b_ri,(RW1 r, IMM i))
{
    if (!i && !needflags)
	return;
=======
	MIDFUNC(2,shra_b_ri,(RW1 r, IMM i))
{
    if (!i && !needflags)
		return;
>>>>>>> p-uae/v2.1.0
    CLOBBER_SHRA;
    r=rmw(r,1,1);
    raw_shra_b_ri(r,i);
    unlock(r);
}
MENDFUNC(2,shra_b_ri,(RW1 r, IMM i))

<<<<<<< HEAD
MIDFUNC(2,shra_l_rr,(RW4 d, R1 r))
{
    if (isconst(r)) {
	COMPCALL(shra_l_ri)(d,(uae_u8)live.state[r].val);
	return;
=======
	MIDFUNC(2,shra_l_rr,(RW4 d, R1 r))
{
    if (isconst(r)) {
		COMPCALL(shra_l_ri)(d,(uae_u8)live.state[r].val);
		return;
>>>>>>> p-uae/v2.1.0
    }
    CLOBBER_SHRA;
    r=readreg_specific(r,1,SHIFTCOUNT_NREG);
    d=rmw(d,4,4);
    Dif (r!=1) {
<<<<<<< HEAD
	write_log ("JIT: Illegal register %d in raw_rol_b\n", r);
	abort();
=======
		jit_abort ("JIT: Illegal register %d in raw_rol_b\n",r);
>>>>>>> p-uae/v2.1.0
    }
    raw_shra_l_rr(d,r) ;
    unlock(r);
    unlock(d);
}
MENDFUNC(2,shra_l_rr,(RW4 d, R1 r))

MIDFUNC(2,shra_w_rr,(RW2 d, R1 r))
{ /* Can only do this with r==1, i.e. cl */

    if (isconst(r)) {
<<<<<<< HEAD
	COMPCALL(shra_w_ri)(d,(uae_u8)live.state[r].val);
	return;
=======
		COMPCALL(shra_w_ri)(d,(uae_u8)live.state[r].val);
		return;
>>>>>>> p-uae/v2.1.0
    }
    CLOBBER_SHRA;
    r=readreg_specific(r,1,SHIFTCOUNT_NREG);
    d=rmw(d,2,2);
    Dif (r!=1) {
<<<<<<< HEAD
	write_log ("JIT: Illegal register %d in raw_shra_b\n", r);
	abort();
=======
		jit_abort ("JIT: Illegal register %d in raw_shra_b\n",r);
>>>>>>> p-uae/v2.1.0
    }
    raw_shra_w_rr(d,r) ;
    unlock(r);
    unlock(d);
}
MENDFUNC(2,shra_w_rr,(RW2 d, R1 r))

<<<<<<< HEAD
MIDFUNC(2,shra_b_rr,(RW1 d, R1 r))
{ /* Can only do this with r==1, i.e. cl */

    if (isconst(r)) {
	COMPCALL(shra_b_ri)(d,(uae_u8)live.state[r].val);
	return;
=======
	MIDFUNC(2,shra_b_rr,(RW1 d, R1 r))
{ /* Can only do this with r==1, i.e. cl */

    if (isconst(r)) {
		COMPCALL(shra_b_ri)(d,(uae_u8)live.state[r].val);
		return;
>>>>>>> p-uae/v2.1.0
    }

    CLOBBER_SHRA;
    r=readreg_specific(r,1,SHIFTCOUNT_NREG);
    d=rmw(d,1,1);
    Dif (r!=1) {
<<<<<<< HEAD
	write_log ("JIT: Illegal register %d in raw_shra_b\n", r);
	abort();
=======
		jit_abort ("JIT: Illegal register %d in raw_shra_b\n",r);
>>>>>>> p-uae/v2.1.0
    }
    raw_shra_b_rr(d,r) ;
    unlock(r);
    unlock(d);
}
MENDFUNC(2,shra_b_rr,(RW1 d, R1 r))

<<<<<<< HEAD

=======
>>>>>>> p-uae/v2.1.0
MIDFUNC(2,setcc,(W1 d, IMM cc))
{
    CLOBBER_SETCC;
    d=writereg(d,1);
    raw_setcc(d,cc);
    unlock(d);
}
MENDFUNC(2,setcc,(W1 d, IMM cc))

MIDFUNC(2,setcc_m,(IMM d, IMM cc))
{
    CLOBBER_SETCC;
    raw_setcc_m(d,cc);
}
MENDFUNC(2,setcc_m,(IMM d, IMM cc))

<<<<<<< HEAD
MIDFUNC(3,cmov_l_rr,(RW4 d, R4 s, IMM cc))
{
    if (d==s)
	return;
=======
MIDFUNC(3,cmov_b_rr,(RW1 d, R1 s, IMM cc))
{
    if (d==s)
		return;
    CLOBBER_CMOV;
    s=readreg(s,1);
    d=rmw(d,1,1);
    raw_cmov_b_rr(d,s,cc);
    unlock(s);
    unlock(d);
}
MENDFUNC(3,cmov_b_rr,(RW1 d, R1 s, IMM cc))

	MIDFUNC(3,cmov_w_rr,(RW2 d, R2 s, IMM cc))
{
    if (d==s)
		return;
    CLOBBER_CMOV;
    s=readreg(s,2);
    d=rmw(d,2,2);
    raw_cmov_w_rr(d,s,cc);
    unlock(s);
    unlock(d);
}
MENDFUNC(3,cmov_w_rr,(RW2 d, R2 s, IMM cc))

	MIDFUNC(3,cmov_l_rr,(RW4 d, R4 s, IMM cc))
{
    if (d==s)
		return;
>>>>>>> p-uae/v2.1.0
    CLOBBER_CMOV;
    s=readreg(s,4);
    d=rmw(d,4,4);
    raw_cmov_l_rr(d,s,cc);
    unlock(s);
    unlock(d);
}
MENDFUNC(3,cmov_l_rr,(RW4 d, R4 s, IMM cc))

<<<<<<< HEAD
MIDFUNC(1,setzflg_l,(RW4 r))
=======
	MIDFUNC(1,setzflg_l,(RW4 r))
>>>>>>> p-uae/v2.1.0
{
	if (setzflg_uses_bsf) {
		CLOBBER_BSF;
		r=rmw(r,4,4);
		raw_bsf_l_rr(r,r);
		unlock(r);
	}
	else {
		Dif (live.flags_in_flags!=VALID) {
<<<<<<< HEAD
			write_log ("JIT: setzflg() wanted flags in native flags, they are %d\n",
				   live.flags_in_flags);
			abort();
		}
		r=readreg(r,4);
		{
		int f=writereg(S11,4);
		int t=writereg(S12,4);
		raw_flags_set_zero(f,r,t);
		unlock(f);
		unlock(r);
		unlock(t);
=======
			jit_abort ("JIT: setzflg() wanted flags in native flags, they are %d\n",
				   live.flags_in_flags);
		}
		r=readreg(r,4);
		{
			int f=writereg(S11,4);
			int t=writereg(S12,4);
			raw_flags_set_zero(f,r,t);
			unlock(f);
			unlock(r);
			unlock(t);
>>>>>>> p-uae/v2.1.0
		}
	}
}
MENDFUNC(1,setzflg_l,(RW4 r))

<<<<<<< HEAD
MIDFUNC(3,cmov_l_rm,(RW4 d, IMM s, IMM cc))
=======
	MIDFUNC(3,cmov_l_rm,(RW4 d, IMM s, IMM cc))
>>>>>>> p-uae/v2.1.0
{
    CLOBBER_CMOV;
    d=rmw(d,4,4);
    raw_cmov_l_rm(d,s,cc);
    unlock(d);
}
MENDFUNC(3,cmov_l_rm,(RW4 d, IMM s, IMM cc))

MIDFUNC(2,bsf_l_rr,(W4 d, R4 s))
{
    CLOBBER_BSF;
    s=readreg(s,4);
    d=writereg(d,4);
    raw_bsf_l_rr(d,s);
    unlock(s);
    unlock(d);
}
MENDFUNC(2,bsf_l_rr,(W4 d, R4 s))

MIDFUNC(2,imul_32_32,(RW4 d, R4 s))
{
    CLOBBER_MUL;
    s=readreg(s,4);
    d=rmw(d,4,4);
    raw_imul_32_32(d,s);
    unlock(s);
    unlock(d);
}
MENDFUNC(2,imul_32_32,(RW4 d, R4 s))

MIDFUNC(2,imul_64_32,(RW4 d, RW4 s))
{
    CLOBBER_MUL;
    s=rmw_specific(s,4,4,MUL_NREG2);
    d=rmw_specific(d,4,4,MUL_NREG1);
    raw_imul_64_32(d,s);
    unlock(s);
    unlock(d);
}
MENDFUNC(2,imul_64_32,(RW4 d, RW4 s))

MIDFUNC(2,mul_64_32,(RW4 d, RW4 s))
{
    CLOBBER_MUL;
    s=rmw_specific(s,4,4,MUL_NREG2);
    d=rmw_specific(d,4,4,MUL_NREG1);
    raw_mul_64_32(d,s);
    unlock(s);
    unlock(d);
}
MENDFUNC(2,mul_64_32,(RW4 d, RW4 s))

<<<<<<< HEAD
MIDFUNC(2,mul_32_32,(RW4 d, R4 s))
{
    CLOBBER_MUL;
    s=readreg(s,4);
    d=rmw(d,4,4);
    raw_mul_32_32(d,s);
    unlock(s);
    unlock(d);
}
MENDFUNC(2,mul_32_32,(RW4 d, R4 s))

=======
>>>>>>> p-uae/v2.1.0
MIDFUNC(2,sign_extend_16_rr,(W4 d, R2 s))
{
    int isrmw;

    if (isconst(s)) {
	set_const(d,(uae_s32)(uae_s16)live.state[s].val);
	return;
    }

    CLOBBER_SE16;
    isrmw=(s==d);
    if (!isrmw) {
	s=readreg(s,2);
	d=writereg(d,4);
    }
    else {  /* If we try to lock this twice, with different sizes, we
	       are int trouble! */
	s=d=rmw(s,4,2);
    }
    raw_sign_extend_16_rr(d,s);
    if (!isrmw) {
	unlock(d);
	unlock(s);
    }
    else {
	unlock(s);
    }
}
MENDFUNC(2,sign_extend_16_rr,(W4 d, R2 s))

MIDFUNC(2,sign_extend_8_rr,(W4 d, R1 s))
{
    int isrmw;

    if (isconst(s)) {
	set_const(d,(uae_s32)(uae_s8)live.state[s].val);
	return;
    }

    isrmw=(s==d);
    CLOBBER_SE8;
    if (!isrmw) {
	s=readreg(s,1);
	d=writereg(d,4);
    }
    else {  /* If we try to lock this twice, with different sizes, we
	       are int trouble! */
	s=d=rmw(s,4,1);
    }

    raw_sign_extend_8_rr(d,s);

    if (!isrmw) {
	unlock(d);
	unlock(s);
    }
    else {
	unlock(s);
    }
}
MENDFUNC(2,sign_extend_8_rr,(W4 d, R1 s))

<<<<<<< HEAD

=======
>>>>>>> p-uae/v2.1.0
MIDFUNC(2,zero_extend_16_rr,(W4 d, R2 s))
{
    int isrmw;

    if (isconst(s)) {
	set_const(d,(uae_u32)(uae_u16)live.state[s].val);
	return;
    }

    isrmw=(s==d);
    CLOBBER_ZE16;
    if (!isrmw) {
	s=readreg(s,2);
	d=writereg(d,4);
    }
    else {  /* If we try to lock this twice, with different sizes, we
	       are int trouble! */
	s=d=rmw(s,4,2);
    }
    raw_zero_extend_16_rr(d,s);
    if (!isrmw) {
	unlock(d);
	unlock(s);
    }
    else {
	unlock(s);
    }
}
MENDFUNC(2,zero_extend_16_rr,(W4 d, R2 s))

MIDFUNC(2,zero_extend_8_rr,(W4 d, R1 s))
{
    int isrmw;
    if (isconst(s)) {
	set_const(d,(uae_u32)(uae_u8)live.state[s].val);
	return;
    }

    isrmw=(s==d);
    CLOBBER_ZE8;
    if (!isrmw) {
	s=readreg(s,1);
	d=writereg(d,4);
    }
    else {  /* If we try to lock this twice, with different sizes, we
	       are int trouble! */
	s=d=rmw(s,4,1);
    }

    raw_zero_extend_8_rr(d,s);

    if (!isrmw) {
	unlock(d);
	unlock(s);
    }
    else {
	unlock(s);
    }
}
MENDFUNC(2,zero_extend_8_rr,(W4 d, R1 s))

MIDFUNC(2,mov_b_rr,(W1 d, R1 s))
{
    if (d==s)
	return;
    if (isconst(s)) {
	COMPCALL(mov_b_ri)(d,(uae_u8)live.state[s].val);
	return;
    }

    CLOBBER_MOV;
    s=readreg(s,1);
    d=writereg(d,1);
    raw_mov_b_rr(d,s);
    unlock(d);
    unlock(s);
}
MENDFUNC(2,mov_b_rr,(W1 d, R1 s))

MIDFUNC(2,mov_w_rr,(W2 d, R2 s))
{
    if (d==s)
<<<<<<< HEAD
	return;
    if (isconst(s)) {
	COMPCALL(mov_w_ri)(d,(uae_u16)live.state[s].val);
	return;
=======
		return;
    if (isconst(s)) {
		COMPCALL(mov_w_ri)(d,(uae_u16)live.state[s].val);
		return;
>>>>>>> p-uae/v2.1.0
    }

    CLOBBER_MOV;
    s=readreg(s,2);
    d=writereg(d,2);
    raw_mov_w_rr(d,s);
    unlock(d);
    unlock(s);
}
MENDFUNC(2,mov_w_rr,(W2 d, R2 s))

<<<<<<< HEAD

MIDFUNC(4,mov_l_rrm_indexed,(W4 d,R4 baser, R4 index, IMM factor))
=======
	MIDFUNC(3,mov_l_rrm_indexed,(W4 d,R4 baser, R4 index))
>>>>>>> p-uae/v2.1.0
{
    CLOBBER_MOV;
    baser=readreg(baser,4);
    index=readreg(index,4);
    d=writereg(d,4);

<<<<<<< HEAD
    raw_mov_l_rrm_indexed(d,baser,index,factor);
=======
    raw_mov_l_rrm_indexed(d,baser,index);
>>>>>>> p-uae/v2.1.0
    unlock(d);
    unlock(baser);
    unlock(index);
}
<<<<<<< HEAD
MENDFUNC(4,mov_l_rrm_indexed,(W4 d,R4 baser, R4 index, IMM factor))

MIDFUNC(4,mov_w_rrm_indexed,(W2 d, R4 baser, R4 index, IMM factor))
=======
MENDFUNC(3,mov_l_rrm_indexed,(W4 d,R4 baser, R4 index))

MIDFUNC(3,mov_w_rrm_indexed,(W2 d, R4 baser, R4 index))
>>>>>>> p-uae/v2.1.0
{
    CLOBBER_MOV;
    baser=readreg(baser,4);
    index=readreg(index,4);
    d=writereg(d,2);

<<<<<<< HEAD
    raw_mov_w_rrm_indexed(d,baser,index,factor);
=======
    raw_mov_w_rrm_indexed(d,baser,index);
>>>>>>> p-uae/v2.1.0
    unlock(d);
    unlock(baser);
    unlock(index);
}
<<<<<<< HEAD
MENDFUNC(4,mov_w_rrm_indexed,(W2 d, R4 baser, R4 index, IMM factor))

MIDFUNC(4,mov_b_rrm_indexed,(W1 d, R4 baser, R4 index, IMM factor))
=======
MENDFUNC(3,mov_w_rrm_indexed,(W2 d, R4 baser, R4 index))

MIDFUNC(3,mov_b_rrm_indexed,(W1 d, R4 baser, R4 index))
>>>>>>> p-uae/v2.1.0
{
    CLOBBER_MOV;
    baser=readreg(baser,4);
    index=readreg(index,4);
    d=writereg(d,1);

<<<<<<< HEAD
    raw_mov_b_rrm_indexed(d,baser,index,factor);
=======
    raw_mov_b_rrm_indexed(d,baser,index);
>>>>>>> p-uae/v2.1.0

    unlock(d);
    unlock(baser);
    unlock(index);
}
<<<<<<< HEAD
MENDFUNC(4,mov_b_rrm_indexed,(W1 d, R4 baser, R4 index, IMM factor))


MIDFUNC(4,mov_l_mrr_indexed,(R4 baser, R4 index, IMM factor, R4 s))
=======
MENDFUNC(3,mov_b_rrm_indexed,(W1 d, R4 baser, R4 index))

MIDFUNC(3,mov_l_mrr_indexed,(R4 baser, R4 index, R4 s))
>>>>>>> p-uae/v2.1.0
{
    CLOBBER_MOV;
    baser=readreg(baser,4);
    index=readreg(index,4);
    s=readreg(s,4);

    Dif (baser==s || index==s)
<<<<<<< HEAD
	abort();


    raw_mov_l_mrr_indexed(baser,index,factor,s);
=======
		jit_abort ("mov_l_mrr_indexed");

    raw_mov_l_mrr_indexed(baser,index,s);
>>>>>>> p-uae/v2.1.0
    unlock(s);
    unlock(baser);
    unlock(index);
}
<<<<<<< HEAD
MENDFUNC(4,mov_l_mrr_indexed,(R4 baser, R4 index, IMM factor, R4 s))

MIDFUNC(4,mov_w_mrr_indexed,(R4 baser, R4 index, IMM factor, R2 s))
=======
MENDFUNC(3,mov_l_mrr_indexed,(R4 baser, R4 index, R4 s))

MIDFUNC(3,mov_w_mrr_indexed,(R4 baser, R4 index, R2 s))
>>>>>>> p-uae/v2.1.0
{
    CLOBBER_MOV;
    baser=readreg(baser,4);
    index=readreg(index,4);
    s=readreg(s,2);

<<<<<<< HEAD
    raw_mov_w_mrr_indexed(baser,index,factor,s);
=======
    raw_mov_w_mrr_indexed(baser,index,s);
>>>>>>> p-uae/v2.1.0
    unlock(s);
    unlock(baser);
    unlock(index);
}
<<<<<<< HEAD
MENDFUNC(4,mov_w_mrr_indexed,(R4 baser, R4 index, IMM factor, R2 s))

MIDFUNC(4,mov_b_mrr_indexed,(R4 baser, R4 index, IMM factor, R1 s))
=======
MENDFUNC(3,mov_w_mrr_indexed,(R4 baser, R4 index, R2 s))

	MIDFUNC(3,mov_b_mrr_indexed,(R4 baser, R4 index, R1 s))
>>>>>>> p-uae/v2.1.0
{
    CLOBBER_MOV;
    s=readreg(s,1);
    baser=readreg(baser,4);
    index=readreg(index,4);

<<<<<<< HEAD
    raw_mov_b_mrr_indexed(baser,index,factor,s);
=======
    raw_mov_b_mrr_indexed(baser,index,s);
>>>>>>> p-uae/v2.1.0
    unlock(s);
    unlock(baser);
    unlock(index);
}
<<<<<<< HEAD
MENDFUNC(4,mov_b_mrr_indexed,(R4 baser, R4 index, IMM factor, R1 s))


MIDFUNC(5,mov_l_bmrr_indexed,(IMM base, R4 baser, R4 index, IMM factor, R4 s))
{
    int basereg=baser;
    int indexreg=index;

    CLOBBER_MOV;
    s=readreg(s,4);
    baser=readreg_offset(baser,4);
    index=readreg_offset(index,4);

    base+=get_offset(basereg);
    base+=factor*get_offset(indexreg);

    raw_mov_l_bmrr_indexed(base,baser,index,factor,s);
    unlock(s);
    unlock(baser);
    unlock(index);
}
MENDFUNC(5,mov_l_bmrr_indexed,(IMM base, R4 baser, R4 index, IMM factor, R4 s))

MIDFUNC(5,mov_w_bmrr_indexed,(IMM base, R4 baser, R4 index, IMM factor, R2 s))
{
    int basereg=baser;
    int indexreg=index;

    CLOBBER_MOV;
    s=readreg(s,2);
    baser=readreg_offset(baser,4);
    index=readreg_offset(index,4);

    base+=get_offset(basereg);
    base+=factor*get_offset(indexreg);

    raw_mov_w_bmrr_indexed(base,baser,index,factor,s);
    unlock(s);
    unlock(baser);
    unlock(index);
}
MENDFUNC(5,mov_w_bmrr_indexed,(IMM base, R4 baser, R4 index, IMM factor, R2 s))

MIDFUNC(5,mov_b_bmrr_indexed,(IMM base, R4 baser, R4 index, IMM factor, R1 s))
{
    int basereg=baser;
    int indexreg=index;

    CLOBBER_MOV;
    s=readreg(s,1);
    baser=readreg_offset(baser,4);
    index=readreg_offset(index,4);

    base+=get_offset(basereg);
    base+=factor*get_offset(indexreg);

    raw_mov_b_bmrr_indexed(base,baser,index,factor,s);
    unlock(s);
    unlock(baser);
    unlock(index);
}
MENDFUNC(5,mov_b_bmrr_indexed,(IMM base, R4 baser, R4 index, IMM factor, R1 s))



/* Read a long from base+baser+factor*index */
MIDFUNC(5,mov_l_brrm_indexed,(W4 d, IMM base, R4 baser, R4 index, IMM factor))
{
    int basereg=baser;
    int indexreg=index;

    CLOBBER_MOV;
    baser=readreg_offset(baser,4);
    index=readreg_offset(index,4);
    base+=get_offset(basereg);
    base+=factor*get_offset(indexreg);
    d=writereg(d,4);
    raw_mov_l_brrm_indexed(d,base,baser,index,factor);
    unlock(d);
    unlock(baser);
    unlock(index);
}
MENDFUNC(5,mov_l_brrm_indexed,(W4 d, IMM base, R4 baser, R4 index, IMM factor))


MIDFUNC(5,mov_w_brrm_indexed,(W2 d, IMM base, R4 baser, R4 index, IMM factor))
{
    int basereg=baser;
    int indexreg=index;

    CLOBBER_MOV;
    remove_offset(d,-1);
    baser=readreg_offset(baser,4);
    index=readreg_offset(index,4);
    base+=get_offset(basereg);
    base+=factor*get_offset(indexreg);
    d=writereg(d,2);
    raw_mov_w_brrm_indexed(d,base,baser,index,factor);
    unlock(d);
    unlock(baser);
    unlock(index);
}
MENDFUNC(5,mov_w_brrm_indexed,(W2 d, IMM base, R4 baser, R4 index, IMM factor))


MIDFUNC(5,mov_b_brrm_indexed,(W1 d, IMM base, R4 baser, R4 index, IMM factor))
{
    int basereg=baser;
    int indexreg=index;

    CLOBBER_MOV;
    remove_offset(d,-1);
    baser=readreg_offset(baser,4);
    index=readreg_offset(index,4);
    base+=get_offset(basereg);
    base+=factor*get_offset(indexreg);
    d=writereg(d,1);
    raw_mov_b_brrm_indexed(d,base,baser,index,factor);
    unlock(d);
    unlock(baser);
    unlock(index);
}
MENDFUNC(5,mov_b_brrm_indexed,(W1 d, IMM base, R4 baser, R4 index, IMM factor))

/* Read a long from base+factor*index */
MIDFUNC(4,mov_l_rm_indexed,(W4 d, IMM base, R4 index, IMM factor))
=======
MENDFUNC(3,mov_b_mrr_indexed,(R4 baser, R4 index, R1 s))

/* Read a long from base+4*index */
MIDFUNC(3,mov_l_rm_indexed,(W4 d, IMM base, R4 index))
>>>>>>> p-uae/v2.1.0
{
    int indexreg=index;

    if (isconst(index)) {
<<<<<<< HEAD
	COMPCALL(mov_l_rm)(d,base+factor*live.state[index].val);
=======
	COMPCALL(mov_l_rm)(d,base+4*live.state[index].val);
>>>>>>> p-uae/v2.1.0
	return;
    }

    CLOBBER_MOV;
    index=readreg_offset(index,4);
<<<<<<< HEAD
    base+=get_offset(indexreg)*factor;
    d=writereg(d,4);

    raw_mov_l_rm_indexed(d,base,index,factor);
    unlock(index);
    unlock(d);
}
MENDFUNC(4,mov_l_rm_indexed,(W4 d, IMM base, R4 index, IMM factor))

=======
    base+=get_offset(indexreg)*4;
    d=writereg(d,4);

    raw_mov_l_rm_indexed(d,base,index);
    unlock(index);
    unlock(d);
}
MENDFUNC(3,mov_l_rm_indexed,(W4 d, IMM base, R4 index))
>>>>>>> p-uae/v2.1.0

/* read the long at the address contained in s+offset and store in d */
MIDFUNC(3,mov_l_rR,(W4 d, R4 s, IMM offset))
{
    if (isconst(s)) {
	COMPCALL(mov_l_rm)(d,live.state[s].val+offset);
	return;
    }
    CLOBBER_MOV;
    s=readreg(s,4);
    d=writereg(d,4);

    raw_mov_l_rR(d,s,offset);
    unlock(d);
    unlock(s);
}
MENDFUNC(3,mov_l_rR,(W4 d, R4 s, IMM offset))

/* read the word at the address contained in s+offset and store in d */
MIDFUNC(3,mov_w_rR,(W2 d, R4 s, IMM offset))
{
    if (isconst(s)) {
	COMPCALL(mov_w_rm)(d,live.state[s].val+offset);
	return;
    }
    CLOBBER_MOV;
    s=readreg(s,4);
    d=writereg(d,2);

    raw_mov_w_rR(d,s,offset);
    unlock(d);
    unlock(s);
}
MENDFUNC(3,mov_w_rR,(W2 d, R4 s, IMM offset))

/* read the word at the address contained in s+offset and store in d */
MIDFUNC(3,mov_b_rR,(W1 d, R4 s, IMM offset))
{
    if (isconst(s)) {
	COMPCALL(mov_b_rm)(d,live.state[s].val+offset);
	return;
    }
    CLOBBER_MOV;
    s=readreg(s,4);
    d=writereg(d,1);

    raw_mov_b_rR(d,s,offset);
    unlock(d);
    unlock(s);
}
MENDFUNC(3,mov_b_rR,(W1 d, R4 s, IMM offset))

/* read the long at the address contained in s+offset and store in d */
MIDFUNC(3,mov_l_brR,(W4 d, R4 s, IMM offset))
{
    int sreg=s;
    if (isconst(s)) {
	COMPCALL(mov_l_rm)(d,live.state[s].val+offset);
	return;
    }
    CLOBBER_MOV;
    s=readreg_offset(s,4);
    offset+=get_offset(sreg);
    d=writereg(d,4);

    raw_mov_l_brR(d,s,offset);
    unlock(d);
    unlock(s);
}
MENDFUNC(3,mov_l_brR,(W4 d, R4 s, IMM offset))

/* read the word at the address contained in s+offset and store in d */
MIDFUNC(3,mov_w_brR,(W2 d, R4 s, IMM offset))
{
    int sreg=s;
    if (isconst(s)) {
	COMPCALL(mov_w_rm)(d,live.state[s].val+offset);
	return;
    }
    CLOBBER_MOV;
    remove_offset(d,-1);
    s=readreg_offset(s,4);
    offset+=get_offset(sreg);
    d=writereg(d,2);

    raw_mov_w_brR(d,s,offset);
    unlock(d);
    unlock(s);
}
MENDFUNC(3,mov_w_brR,(W2 d, R4 s, IMM offset))

/* read the word at the address contained in s+offset and store in d */
MIDFUNC(3,mov_b_brR,(W1 d, R4 s, IMM offset))
{
    int sreg=s;
    if (isconst(s)) {
	COMPCALL(mov_b_rm)(d,live.state[s].val+offset);
	return;
    }
    CLOBBER_MOV;
    remove_offset(d,-1);
    s=readreg_offset(s,4);
    offset+=get_offset(sreg);
    d=writereg(d,1);

    raw_mov_b_brR(d,s,offset);
    unlock(d);
    unlock(s);
}
MENDFUNC(3,mov_b_brR,(W1 d, R4 s, IMM offset))

MIDFUNC(3,mov_l_Ri,(R4 d, IMM i, IMM offset))
{
    int dreg=d;
    if (isconst(d)) {
	COMPCALL(mov_l_mi)(live.state[d].val+offset,i);
	return;
    }

    CLOBBER_MOV;
    d=readreg_offset(d,4);
    offset+=get_offset(dreg);
    raw_mov_l_Ri(d,i,offset);
    unlock(d);
}
MENDFUNC(3,mov_l_Ri,(R4 d, IMM i, IMM offset))

MIDFUNC(3,mov_w_Ri,(R4 d, IMM i, IMM offset))
{
    int dreg=d;
    if (isconst(d)) {
	COMPCALL(mov_w_mi)(live.state[d].val+offset,i);
	return;
    }

    CLOBBER_MOV;
    d=readreg_offset(d,4);
    offset+=get_offset(dreg);
    raw_mov_w_Ri(d,i,offset);
    unlock(d);
}
MENDFUNC(3,mov_w_Ri,(R4 d, IMM i, IMM offset))

MIDFUNC(3,mov_b_Ri,(R4 d, IMM i, IMM offset))
{
    int dreg=d;
    if (isconst(d)) {
	COMPCALL(mov_b_mi)(live.state[d].val+offset,i);
	return;
    }

    CLOBBER_MOV;
    d=readreg_offset(d,4);
    offset+=get_offset(dreg);
    raw_mov_b_Ri(d,i,offset);
    unlock(d);
}
MENDFUNC(3,mov_b_Ri,(R4 d, IMM i, IMM offset))

     /* Warning! OFFSET is byte sized only! */
MIDFUNC(3,mov_l_Rr,(R4 d, R4 s, IMM offset))
{
    if (isconst(d)) {
	COMPCALL(mov_l_mr)(live.state[d].val+offset,s);
	return;
    }
    if (isconst(s)) {
	COMPCALL(mov_l_Ri)(d,live.state[s].val,offset);
	return;
    }

    CLOBBER_MOV;
    s=readreg(s,4);
    d=readreg(d,4);

    raw_mov_l_Rr(d,s,offset);
    unlock(d);
    unlock(s);
}
MENDFUNC(3,mov_l_Rr,(R4 d, R4 s, IMM offset))

MIDFUNC(3,mov_w_Rr,(R4 d, R2 s, IMM offset))
{
    if (isconst(d)) {
	COMPCALL(mov_w_mr)(live.state[d].val+offset,s);
	return;
    }
    if (isconst(s)) {
	COMPCALL(mov_w_Ri)(d,(uae_u16)live.state[s].val,offset);
	return;
    }

    CLOBBER_MOV;
    s=readreg(s,2);
    d=readreg(d,4);
    raw_mov_w_Rr(d,s,offset);
    unlock(d);
    unlock(s);
}
MENDFUNC(3,mov_w_Rr,(R4 d, R2 s, IMM offset))

MIDFUNC(3,mov_b_Rr,(R4 d, R1 s, IMM offset))
{
    if (isconst(d)) {
	COMPCALL(mov_b_mr)(live.state[d].val+offset,s);
	return;
    }
    if (isconst(s)) {
	COMPCALL(mov_b_Ri)(d,(uae_u8)live.state[s].val,offset);
	return;
    }

    CLOBBER_MOV;
    s=readreg(s,1);
    d=readreg(d,4);
    raw_mov_b_Rr(d,s,offset);
    unlock(d);
    unlock(s);
}
MENDFUNC(3,mov_b_Rr,(R4 d, R1 s, IMM offset))

MIDFUNC(3,lea_l_brr,(W4 d, R4 s, IMM offset))
{
    if (isconst(s)) {
	COMPCALL(mov_l_ri)(d,live.state[s].val+offset);
	return;
    }
#if USE_OFFSET
    if (d==s) {
	add_offset(d,offset);
	return;
    }
#endif
    CLOBBER_LEA;
    s=readreg(s,4);
    d=writereg(d,4);
    raw_lea_l_brr(d,s,offset);
    unlock(d);
    unlock(s);
}
MENDFUNC(3,lea_l_brr,(W4 d, R4 s, IMM offset))

MIDFUNC(5,lea_l_brr_indexed,(W4 d, R4 s, R4 index, IMM factor, IMM offset))
{
<<<<<<< HEAD
    if (!offset) {
	COMPCALL(lea_l_rr_indexed)(d,s,index,factor);
	return;
    }
=======
>>>>>>> p-uae/v2.1.0
    CLOBBER_LEA;
    s=readreg(s,4);
    index=readreg(index,4);
    d=writereg(d,4);

    raw_lea_l_brr_indexed(d,s,index,factor,offset);
    unlock(d);
    unlock(index);
    unlock(s);
}
MENDFUNC(5,lea_l_brr_indexed,(W4 d, R4 s, R4 index, IMM factor, IMM offset))

<<<<<<< HEAD
MIDFUNC(4,lea_l_rr_indexed,(W4 d, R4 s, R4 index, IMM factor))
{
    CLOBBER_LEA;
    s=readreg(s,4);
    index=readreg(index,4);
    d=writereg(d,4);

    raw_lea_l_rr_indexed(d,s,index,factor);
    unlock(d);
    unlock(index);
    unlock(s);
}
MENDFUNC(4,lea_l_rr_indexed,(W4 d, R4 s, R4 index, IMM factor))

=======
>>>>>>> p-uae/v2.1.0
/* write d to the long at the address contained in s+offset */
MIDFUNC(3,mov_l_bRr,(R4 d, R4 s, IMM offset))
{
    int dreg=d;
    if (isconst(d)) {
	COMPCALL(mov_l_mr)(live.state[d].val+offset,s);
	return;
    }

    CLOBBER_MOV;
    s=readreg(s,4);
    d=readreg_offset(d,4);
    offset+=get_offset(dreg);

    raw_mov_l_bRr(d,s,offset);
    unlock(d);
    unlock(s);
}
MENDFUNC(3,mov_l_bRr,(R4 d, R4 s, IMM offset))

/* write the word at the address contained in s+offset and store in d */
MIDFUNC(3,mov_w_bRr,(R4 d, R2 s, IMM offset))
{
    int dreg=d;

    if (isconst(d)) {
	COMPCALL(mov_w_mr)(live.state[d].val+offset,s);
	return;
    }

    CLOBBER_MOV;
    s=readreg(s,2);
    d=readreg_offset(d,4);
    offset+=get_offset(dreg);
    raw_mov_w_bRr(d,s,offset);
    unlock(d);
    unlock(s);
}
MENDFUNC(3,mov_w_bRr,(R4 d, R2 s, IMM offset))

MIDFUNC(3,mov_b_bRr,(R4 d, R1 s, IMM offset))
{
    int dreg=d;
    if (isconst(d)) {
	COMPCALL(mov_b_mr)(live.state[d].val+offset,s);
	return;
    }

    CLOBBER_MOV;
    s=readreg(s,1);
    d=readreg_offset(d,4);
    offset+=get_offset(dreg);
    raw_mov_b_bRr(d,s,offset);
    unlock(d);
    unlock(s);
}
MENDFUNC(3,mov_b_bRr,(R4 d, R1 s, IMM offset))

MIDFUNC(1,gen_bswap_32,(RW4 r))
{
<<<<<<< HEAD
=======
    int reg=r;

>>>>>>> p-uae/v2.1.0
    if (isconst(r)) {
	uae_u32 oldv=live.state[r].val;
	live.state[r].val=reverse32(oldv);
	return;
    }

    CLOBBER_SW32;
    r=rmw(r,4,4);
    raw_bswap_32(r);
    unlock(r);
}
MENDFUNC(1,gen_bswap_32,(RW4 r))

MIDFUNC(1,gen_bswap_16,(RW2 r))
{
    if (isconst(r)) {
	uae_u32 oldv=live.state[r].val;
	live.state[r].val=((oldv>>8)&0xff) | ((oldv<<8)&0xff00) |
	    (oldv&0xffff0000);
	return;
    }

    CLOBBER_SW16;
    r=rmw(r,2,2);

    raw_bswap_16(r);
    unlock(r);
}
MENDFUNC(1,gen_bswap_16,(RW2 r))



MIDFUNC(2,mov_l_rr,(W4 d, R4 s))
{
    int olds;

    if (d==s) { /* How pointless! */
	return;
    }
    if (isconst(s)) {
	COMPCALL(mov_l_ri)(d,live.state[s].val);
	return;
    }
#if USE_ALIAS
    olds=s;
    disassociate(d);
    s=readreg_offset(s,4);
    live.state[d].realreg=s;
    live.state[d].realind=live.nat[s].nholds;
    live.state[d].val=live.state[olds].val;
    live.state[d].validsize=4;
    live.state[d].dirtysize=4;
    set_status(d,DIRTY);

    live.nat[s].holds[live.nat[s].nholds]=d;
    live.nat[s].nholds++;
    log_clobberreg(d);

<<<<<<< HEAD
    /* printf("Added %d to nreg %d(%d), now holds %d regs\n",
=======
	/* write_log ("JIT: Added %d to nreg %d(%d), now holds %d regs\n",
>>>>>>> p-uae/v2.1.0
       d,s,live.state[d].realind,live.nat[s].nholds); */
    unlock(s);
#else
    CLOBBER_MOV;
    s=readreg(s,4);
    d=writereg(d,4);

    raw_mov_l_rr(d,s);
    unlock(d);
    unlock(s);
#endif
}
MENDFUNC(2,mov_l_rr,(W4 d, R4 s))

MIDFUNC(2,mov_l_mr,(IMM d, R4 s))
{
    if (isconst(s)) {
	COMPCALL(mov_l_mi)(d,live.state[s].val);
	return;
    }
    CLOBBER_MOV;
    s=readreg(s,4);

    raw_mov_l_mr(d,s);
    unlock(s);
}
MENDFUNC(2,mov_l_mr,(IMM d, R4 s))


MIDFUNC(2,mov_w_mr,(IMM d, R2 s))
{
    if (isconst(s)) {
	COMPCALL(mov_w_mi)(d,(uae_u16)live.state[s].val);
	return;
    }
    CLOBBER_MOV;
    s=readreg(s,2);

    raw_mov_w_mr(d,s);
    unlock(s);
}
MENDFUNC(2,mov_w_mr,(IMM d, R2 s))

MIDFUNC(2,mov_w_rm,(W2 d, IMM s))
{
    CLOBBER_MOV;
    d=writereg(d,2);

    raw_mov_w_rm(d,s);
    unlock(d);
}
MENDFUNC(2,mov_w_rm,(W2 d, IMM s))

MIDFUNC(2,mov_b_mr,(IMM d, R1 s))
{
    if (isconst(s)) {
	COMPCALL(mov_b_mi)(d,(uae_u8)live.state[s].val);
	return;
    }

    CLOBBER_MOV;
    s=readreg(s,1);

    raw_mov_b_mr(d,s);
    unlock(s);
}
MENDFUNC(2,mov_b_mr,(IMM d, R1 s))

MIDFUNC(2,mov_b_rm,(W1 d, IMM s))
{
    CLOBBER_MOV;
    d=writereg(d,1);

    raw_mov_b_rm(d,s);
    unlock(d);
}
MENDFUNC(2,mov_b_rm,(W1 d, IMM s))

MIDFUNC(2,mov_l_ri,(W4 d, IMM s))
{
    set_const(d,s);
    return;
}
MENDFUNC(2,mov_l_ri,(W4 d, IMM s))

MIDFUNC(2,mov_w_ri,(W2 d, IMM s))
{
    CLOBBER_MOV;
    d=writereg(d,2);

    raw_mov_w_ri(d,s);
    unlock(d);
}
MENDFUNC(2,mov_w_ri,(W2 d, IMM s))

MIDFUNC(2,mov_b_ri,(W1 d, IMM s))
{
    CLOBBER_MOV;
    d=writereg(d,1);

    raw_mov_b_ri(d,s);
    unlock(d);
}
MENDFUNC(2,mov_b_ri,(W1 d, IMM s))


MIDFUNC(2,add_l_mi,(IMM d, IMM s))
{
    CLOBBER_ADD;
    raw_add_l_mi(d,s) ;
}
MENDFUNC(2,add_l_mi,(IMM d, IMM s))

MIDFUNC(2,add_w_mi,(IMM d, IMM s))
{
    CLOBBER_ADD;
    raw_add_w_mi(d,s) ;
}
MENDFUNC(2,add_w_mi,(IMM d, IMM s))

MIDFUNC(2,add_b_mi,(IMM d, IMM s))
{
    CLOBBER_ADD;
    raw_add_b_mi(d,s) ;
}
MENDFUNC(2,add_b_mi,(IMM d, IMM s))


MIDFUNC(2,test_l_ri,(R4 d, IMM i))
{
    CLOBBER_TEST;
    d=readreg(d,4);

    raw_test_l_ri(d,i);
    unlock(d);
}
MENDFUNC(2,test_l_ri,(R4 d, IMM i))

MIDFUNC(2,test_l_rr,(R4 d, R4 s))
{
    CLOBBER_TEST;
    d=readreg(d,4);
    s=readreg(s,4);

    raw_test_l_rr(d,s);;
    unlock(d);
    unlock(s);
}
MENDFUNC(2,test_l_rr,(R4 d, R4 s))

MIDFUNC(2,test_w_rr,(R2 d, R2 s))
{
    CLOBBER_TEST;
    d=readreg(d,2);
    s=readreg(s,2);

    raw_test_w_rr(d,s);
    unlock(d);
    unlock(s);
}
MENDFUNC(2,test_w_rr,(R2 d, R2 s))

MIDFUNC(2,test_b_rr,(R1 d, R1 s))
{
    CLOBBER_TEST;
    d=readreg(d,1);
    s=readreg(s,1);

    raw_test_b_rr(d,s);
    unlock(d);
    unlock(s);
}
MENDFUNC(2,test_b_rr,(R1 d, R1 s))

<<<<<<< HEAD

=======
>>>>>>> p-uae/v2.1.0
MIDFUNC(2,and_l_ri,(RW4 d, IMM i))
{
    if (isconst (d) && ! needflags) {
	live.state[d].val &= i;
	return;
    }

    CLOBBER_AND;
    d=rmw(d,4,4);

    raw_and_l_ri(d,i);
    unlock(d);
}
MENDFUNC(2,and_l_ri,(RW4 d, IMM i))

MIDFUNC(2,and_l,(RW4 d, R4 s))
{
    CLOBBER_AND;
    s=readreg(s,4);
    d=rmw(d,4,4);

    raw_and_l(d,s);
    unlock(d);
    unlock(s);
}
MENDFUNC(2,and_l,(RW4 d, R4 s))

MIDFUNC(2,and_w,(RW2 d, R2 s))
{
    CLOBBER_AND;
    s=readreg(s,2);
    d=rmw(d,2,2);

    raw_and_w(d,s);
    unlock(d);
    unlock(s);
}
MENDFUNC(2,and_w,(RW2 d, R2 s))

<<<<<<< HEAD
MIDFUNC(2,and_b,(RW1 d, R1 s))
=======
	MIDFUNC(2,and_b,(RW1 d, R1 s))
>>>>>>> p-uae/v2.1.0
{
    CLOBBER_AND;
    s=readreg(s,1);
    d=rmw(d,1,1);

    raw_and_b(d,s);
    unlock(d);
    unlock(s);
}
MENDFUNC(2,and_b,(RW1 d, R1 s))

<<<<<<< HEAD
MIDFUNC(2,or_l_ri,(RW4 d, IMM i))
{
    if (isconst(d) && !needflags) {
	live.state[d].val|=i;
	return;
=======
	MIDFUNC(2,or_l_ri,(RW4 d, IMM i))
{
    if (isconst(d) && !needflags) {
		live.state[d].val|=i;
		return;
>>>>>>> p-uae/v2.1.0
    }
    CLOBBER_OR;
    d=rmw(d,4,4);

    raw_or_l_ri(d,i);
    unlock(d);
}
MENDFUNC(2,or_l_ri,(RW4 d, IMM i))

<<<<<<< HEAD
MIDFUNC(2,or_l,(RW4 d, R4 s))
{
    if (isconst(d) && isconst(s) && !needflags) {
	live.state[d].val|=live.state[s].val;
	return;
=======
	MIDFUNC(2,or_l,(RW4 d, R4 s))
{
    if (isconst(d) && isconst(s) && !needflags) {
		live.state[d].val|=live.state[s].val;
		return;
>>>>>>> p-uae/v2.1.0
    }
    CLOBBER_OR;
    s=readreg(s,4);
    d=rmw(d,4,4);

    raw_or_l(d,s);
    unlock(d);
    unlock(s);
}
MENDFUNC(2,or_l,(RW4 d, R4 s))

<<<<<<< HEAD
MIDFUNC(2,or_w,(RW2 d, R2 s))
=======
	MIDFUNC(2,or_w,(RW2 d, R2 s))
>>>>>>> p-uae/v2.1.0
{
    CLOBBER_OR;
    s=readreg(s,2);
    d=rmw(d,2,2);

    raw_or_w(d,s);
    unlock(d);
    unlock(s);
}
MENDFUNC(2,or_w,(RW2 d, R2 s))

<<<<<<< HEAD
MIDFUNC(2,or_b,(RW1 d, R1 s))
=======
	MIDFUNC(2,or_b,(RW1 d, R1 s))
>>>>>>> p-uae/v2.1.0
{
    CLOBBER_OR;
    s=readreg(s,1);
    d=rmw(d,1,1);

    raw_or_b(d,s);
    unlock(d);
    unlock(s);
}
MENDFUNC(2,or_b,(RW1 d, R1 s))

<<<<<<< HEAD
MIDFUNC(2,adc_l,(RW4 d, R4 s))
=======
	MIDFUNC(2,adc_l,(RW4 d, R4 s))
>>>>>>> p-uae/v2.1.0
{
    CLOBBER_ADC;
    s=readreg(s,4);
    d=rmw(d,4,4);

    raw_adc_l(d,s);

    unlock(d);
    unlock(s);
}
MENDFUNC(2,adc_l,(RW4 d, R4 s))

<<<<<<< HEAD
MIDFUNC(2,adc_w,(RW2 d, R2 s))
=======
	MIDFUNC(2,adc_w,(RW2 d, R2 s))
>>>>>>> p-uae/v2.1.0
{
    CLOBBER_ADC;
    s=readreg(s,2);
    d=rmw(d,2,2);

    raw_adc_w(d,s);
    unlock(d);
    unlock(s);
}
MENDFUNC(2,adc_w,(RW2 d, R2 s))

<<<<<<< HEAD
MIDFUNC(2,adc_b,(RW1 d, R1 s))
=======
	MIDFUNC(2,adc_b,(RW1 d, R1 s))
>>>>>>> p-uae/v2.1.0
{
    CLOBBER_ADC;
    s=readreg(s,1);
    d=rmw(d,1,1);

    raw_adc_b(d,s);
    unlock(d);
    unlock(s);
}
MENDFUNC(2,adc_b,(RW1 d, R1 s))

<<<<<<< HEAD
MIDFUNC(2,add_l,(RW4 d, R4 s))
{
    if (isconst(s)) {
	COMPCALL(add_l_ri)(d,live.state[s].val);
	return;
=======
	MIDFUNC(2,add_l,(RW4 d, R4 s))
{
    if (isconst(s)) {
		COMPCALL(add_l_ri)(d,live.state[s].val);
		return;
>>>>>>> p-uae/v2.1.0
    }

    CLOBBER_ADD;
    s=readreg(s,4);
    d=rmw(d,4,4);

    raw_add_l(d,s);

    unlock(d);
    unlock(s);
}
MENDFUNC(2,add_l,(RW4 d, R4 s))

MIDFUNC(2,add_w,(RW2 d, R2 s))
{
    if (isconst(s)) {
<<<<<<< HEAD
	COMPCALL(add_w_ri)(d,(uae_u16)live.state[s].val);
	return;
=======
		COMPCALL(add_w_ri)(d,(uae_u16)live.state[s].val);
		return;
>>>>>>> p-uae/v2.1.0
    }

    CLOBBER_ADD;
    s=readreg(s,2);
    d=rmw(d,2,2);

    raw_add_w(d,s);
    unlock(d);
    unlock(s);
}
MENDFUNC(2,add_w,(RW2 d, R2 s))

MIDFUNC(2,add_b,(RW1 d, R1 s))
{
    if (isconst(s)) {
<<<<<<< HEAD
	COMPCALL(add_b_ri)(d,(uae_u8)live.state[s].val);
	return;
=======
		COMPCALL(add_b_ri)(d,(uae_u8)live.state[s].val);
		return;
>>>>>>> p-uae/v2.1.0
    }

    CLOBBER_ADD;
    s=readreg(s,1);
    d=rmw(d,1,1);

    raw_add_b(d,s);
    unlock(d);
    unlock(s);
}
MENDFUNC(2,add_b,(RW1 d, R1 s))

<<<<<<< HEAD
MIDFUNC(2,sub_l_ri,(RW4 d, IMM i))
{
    if (!i && !needflags)
	return;
    if (isconst(d) && !needflags) {
	live.state[d].val-=i;
	return;
    }
#if USE_OFFSET
    if (!needflags) {
	add_offset(d,-(signed)i);
	return;
=======
	MIDFUNC(2,sub_l_ri,(RW4 d, IMM i))
{
    if (!i && !needflags)
		return;
    if (isconst(d) && !needflags) {
		live.state[d].val-=i;
		return;
    }
#if USE_OFFSET
    if (!needflags) {
		add_offset(d,-(signed)i);
		return;
>>>>>>> p-uae/v2.1.0
    }
#endif

    CLOBBER_SUB;
    d=rmw(d,4,4);

    raw_sub_l_ri(d,i);
    unlock(d);
}
MENDFUNC(2,sub_l_ri,(RW4 d, IMM i))

<<<<<<< HEAD
MIDFUNC(2,sub_w_ri,(RW2 d, IMM i))
{
    if (!i && !needflags)
	return;
=======
	MIDFUNC(2,sub_w_ri,(RW2 d, IMM i))
{
    if (!i && !needflags)
		return;
>>>>>>> p-uae/v2.1.0

    CLOBBER_SUB;
    d=rmw(d,2,2);

    raw_sub_w_ri(d,i);
    unlock(d);
}
MENDFUNC(2,sub_w_ri,(RW2 d, IMM i))

<<<<<<< HEAD
MIDFUNC(2,sub_b_ri,(RW1 d, IMM i))
{
    if (!i && !needflags)
	return;
=======
	MIDFUNC(2,sub_b_ri,(RW1 d, IMM i))
{
    if (!i && !needflags)
		return;
>>>>>>> p-uae/v2.1.0

    CLOBBER_SUB;
    d=rmw(d,1,1);

    raw_sub_b_ri(d,i);

    unlock(d);
}
MENDFUNC(2,sub_b_ri,(RW1 d, IMM i))

<<<<<<< HEAD
MIDFUNC(2,add_l_ri,(RW4 d, IMM i))
{
    if (!i && !needflags)
	return;
    if (isconst(d) && !needflags) {
	live.state[d].val+=i;
	return;
    }
#if USE_OFFSET
    if (!needflags) {
	add_offset(d,i);
	return;
=======
	MIDFUNC(2,add_l_ri,(RW4 d, IMM i))
{
    if (!i && !needflags)
		return;
    if (isconst(d) && !needflags) {
		live.state[d].val+=i;
		return;
    }
#if USE_OFFSET
    if (!needflags) {
		add_offset(d,i);
		return;
>>>>>>> p-uae/v2.1.0
    }
#endif
    CLOBBER_ADD;
    d=rmw(d,4,4);
    raw_add_l_ri(d,i);
    unlock(d);
}
MENDFUNC(2,add_l_ri,(RW4 d, IMM i))

<<<<<<< HEAD
MIDFUNC(2,add_w_ri,(RW2 d, IMM i))
{
    if (!i && !needflags)
	return;
=======
	MIDFUNC(2,add_w_ri,(RW2 d, IMM i))
{
    if (!i && !needflags)
		return;
>>>>>>> p-uae/v2.1.0

    CLOBBER_ADD;
    d=rmw(d,2,2);

    raw_add_w_ri(d,i);
    unlock(d);
}
MENDFUNC(2,add_w_ri,(RW2 d, IMM i))

<<<<<<< HEAD
MIDFUNC(2,add_b_ri,(RW1 d, IMM i))
{
    if (!i && !needflags)
	return;
=======
	MIDFUNC(2,add_b_ri,(RW1 d, IMM i))
{
    if (!i && !needflags)
		return;
>>>>>>> p-uae/v2.1.0

    CLOBBER_ADD;
    d=rmw(d,1,1);

    raw_add_b_ri(d,i);

    unlock(d);
}
MENDFUNC(2,add_b_ri,(RW1 d, IMM i))

<<<<<<< HEAD
MIDFUNC(2,sbb_l,(RW4 d, R4 s))
=======
	MIDFUNC(2,sbb_l,(RW4 d, R4 s))
>>>>>>> p-uae/v2.1.0
{
    CLOBBER_SBB;
    s=readreg(s,4);
    d=rmw(d,4,4);

    raw_sbb_l(d,s);
    unlock(d);
    unlock(s);
}
MENDFUNC(2,sbb_l,(RW4 d, R4 s))

<<<<<<< HEAD
MIDFUNC(2,sbb_w,(RW2 d, R2 s))
=======
	MIDFUNC(2,sbb_w,(RW2 d, R2 s))
>>>>>>> p-uae/v2.1.0
{
    CLOBBER_SBB;
    s=readreg(s,2);
    d=rmw(d,2,2);

    raw_sbb_w(d,s);
    unlock(d);
    unlock(s);
}
MENDFUNC(2,sbb_w,(RW2 d, R2 s))

<<<<<<< HEAD
MIDFUNC(2,sbb_b,(RW1 d, R1 s))
=======
	MIDFUNC(2,sbb_b,(RW1 d, R1 s))
>>>>>>> p-uae/v2.1.0
{
    CLOBBER_SBB;
    s=readreg(s,1);
    d=rmw(d,1,1);

    raw_sbb_b(d,s);
    unlock(d);
    unlock(s);
}
MENDFUNC(2,sbb_b,(RW1 d, R1 s))

<<<<<<< HEAD
MIDFUNC(2,sub_l,(RW4 d, R4 s))
{
    if (isconst(s)) {
	COMPCALL(sub_l_ri)(d,live.state[s].val);
	return;
=======
	MIDFUNC(2,sub_l,(RW4 d, R4 s))
{
    if (isconst(s)) {
		COMPCALL(sub_l_ri)(d,live.state[s].val);
		return;
>>>>>>> p-uae/v2.1.0
    }

    CLOBBER_SUB;
    s=readreg(s,4);
    d=rmw(d,4,4);

    raw_sub_l(d,s);
    unlock(d);
    unlock(s);
}
MENDFUNC(2,sub_l,(RW4 d, R4 s))

<<<<<<< HEAD
MIDFUNC(2,sub_w,(RW2 d, R2 s))
{
    if (isconst(s)) {
	COMPCALL(sub_w_ri)(d,(uae_u16)live.state[s].val);
	return;
=======
	MIDFUNC(2,sub_w,(RW2 d, R2 s))
{
    if (isconst(s)) {
		COMPCALL(sub_w_ri)(d,(uae_u16)live.state[s].val);
		return;
>>>>>>> p-uae/v2.1.0
    }

    CLOBBER_SUB;
    s=readreg(s,2);
    d=rmw(d,2,2);

    raw_sub_w(d,s);
    unlock(d);
    unlock(s);
}
MENDFUNC(2,sub_w,(RW2 d, R2 s))

<<<<<<< HEAD
MIDFUNC(2,sub_b,(RW1 d, R1 s))
{
    if (isconst(s)) {
	COMPCALL(sub_b_ri)(d,(uae_u8)live.state[s].val);
	return;
=======
	MIDFUNC(2,sub_b,(RW1 d, R1 s))
{
    if (isconst(s)) {
		COMPCALL(sub_b_ri)(d,(uae_u8)live.state[s].val);
		return;
>>>>>>> p-uae/v2.1.0
    }

    CLOBBER_SUB;
    s=readreg(s,1);
    d=rmw(d,1,1);

    raw_sub_b(d,s);
    unlock(d);
    unlock(s);
}
MENDFUNC(2,sub_b,(RW1 d, R1 s))

<<<<<<< HEAD
MIDFUNC(2,cmp_l,(R4 d, R4 s))
=======
	MIDFUNC(2,cmp_l,(R4 d, R4 s))
>>>>>>> p-uae/v2.1.0
{
    CLOBBER_CMP;
    s=readreg(s,4);
    d=readreg(d,4);

    raw_cmp_l(d,s);
    unlock(d);
    unlock(s);
}
MENDFUNC(2,cmp_l,(R4 d, R4 s))

MIDFUNC(2,cmp_l_ri,(R4 r, IMM i))
{
    CLOBBER_CMP;
    r=readreg(r,4);

    raw_cmp_l_ri(r,i);
    unlock(r);
}
MENDFUNC(2,cmp_l_ri,(R4 r, IMM i))

MIDFUNC(2,cmp_w,(R2 d, R2 s))
{
    CLOBBER_CMP;
    s=readreg(s,2);
    d=readreg(d,2);

    raw_cmp_w(d,s);
    unlock(d);
    unlock(s);
}
MENDFUNC(2,cmp_w,(R2 d, R2 s))

MIDFUNC(2,cmp_b,(R1 d, R1 s))
{
    CLOBBER_CMP;
    s=readreg(s,1);
    d=readreg(d,1);

    raw_cmp_b(d,s);
    unlock(d);
    unlock(s);
}
MENDFUNC(2,cmp_b,(R1 d, R1 s))


MIDFUNC(2,xor_l,(RW4 d, R4 s))
{
    CLOBBER_XOR;
    s=readreg(s,4);
    d=rmw(d,4,4);

    raw_xor_l(d,s);
    unlock(d);
    unlock(s);
}
MENDFUNC(2,xor_l,(RW4 d, R4 s))

MIDFUNC(2,xor_w,(RW2 d, R2 s))
{
    CLOBBER_XOR;
    s=readreg(s,2);
    d=rmw(d,2,2);

    raw_xor_w(d,s);
    unlock(d);
    unlock(s);
}
MENDFUNC(2,xor_w,(RW2 d, R2 s))

MIDFUNC(2,xor_b,(RW1 d, R1 s))
{
    CLOBBER_XOR;
    s=readreg(s,1);
    d=rmw(d,1,1);

    raw_xor_b(d,s);
    unlock(d);
    unlock(s);
}
MENDFUNC(2,xor_b,(RW1 d, R1 s))

MIDFUNC(5,call_r_11,(W4 out1, R4 r, R4 in1, IMM osize, IMM isize))
{
    clobber_flags();
    remove_all_offsets();
    if (osize==4) {
	if (out1!=in1 && out1!=r) {
	    COMPCALL(forget_about)(out1);
	}
    }
    else {
	tomem_c(out1);
    }

    in1=readreg_specific(in1,isize,REG_PAR1);
    r=readreg(r,4);
    prepare_for_call_1();  /* This should ensure that there won't be
			      any need for swapping nregs in prepare_for_call_2
			   */
#if USE_NORMAL_CALLING_CONVENTION
    raw_push_l_r(in1);
#endif
    unlock(in1);
    unlock(r);

    prepare_for_call_2();
    raw_call_r(r);

#if USE_NORMAL_CALLING_CONVENTION
    raw_inc_sp(4);
#endif


    live.nat[REG_RESULT].holds[0]=out1;
    live.nat[REG_RESULT].nholds=1;
    live.nat[REG_RESULT].touched=touchcnt++;

    live.state[out1].realreg=REG_RESULT;
    live.state[out1].realind=0;
    live.state[out1].val=0;
    live.state[out1].validsize=osize;
    live.state[out1].dirtysize=osize;
    set_status(out1,DIRTY);
}
MENDFUNC(5,call_r_11,(W4 out1, R4 r, R4 in1, IMM osize, IMM isize))

MIDFUNC(5,call_r_02,(R4 r, R4 in1, R4 in2, IMM isize1, IMM isize2))
{
    clobber_flags();
    remove_all_offsets();
    in1=readreg_specific(in1,isize1,REG_PAR1);
    in2=readreg_specific(in2,isize2,REG_PAR2);
    r=readreg(r,4);
    prepare_for_call_1();  /* This should ensure that there won't be
			      any need for swapping nregs in prepare_for_call_2
			   */
#if USE_NORMAL_CALLING_CONVENTION
    raw_push_l_r(in2);
    raw_push_l_r(in1);
#endif
    unlock(r);
    unlock(in1);
    unlock(in2);
    prepare_for_call_2();
    raw_call_r(r);
#if USE_NORMAL_CALLING_CONVENTION
    raw_inc_sp(8);
#endif
}
MENDFUNC(5,call_r_02,(R4 r, R4 in1, R4 in2, IMM isize1, IMM isize2))

<<<<<<< HEAD

=======
>>>>>>> p-uae/v2.1.0
MIDFUNC(1,forget_about,(W4 r))
{
    if (isinreg(r))
	disassociate(r);
    live.state[r].val=0;
    set_status(r,UNDEF);
}
MENDFUNC(1,forget_about,(W4 r))

MIDFUNC(0,nop,(void))
{
    raw_nop();
}
MENDFUNC(0,nop,(void))

<<<<<<< HEAD

=======
>>>>>>> p-uae/v2.1.0
MIDFUNC(1,f_forget_about,(FW r))
{
    if (f_isinreg(r))
	f_disassociate(r);
    live.fate[r].status=UNDEF;
}
MENDFUNC(1,f_forget_about,(FW r))

MIDFUNC(1,fmov_pi,(FW r))
{
    r=f_writereg(r);
    raw_fmov_pi(r);
    f_unlock(r);
}
MENDFUNC(1,fmov_pi,(FW r))

MIDFUNC(1,fmov_log10_2,(FW r))
{
    r=f_writereg(r);
    raw_fmov_log10_2(r);
    f_unlock(r);
}
MENDFUNC(1,fmov_log10_2,(FW r))

MIDFUNC(1,fmov_log2_e,(FW r))
{
    r=f_writereg(r);
    raw_fmov_log2_e(r);
    f_unlock(r);
}
MENDFUNC(1,fmov_log2_e,(FW r))

MIDFUNC(1,fmov_loge_2,(FW r))
{
    r=f_writereg(r);
    raw_fmov_loge_2(r);
    f_unlock(r);
}
MENDFUNC(1,fmov_loge_2,(FW r))

MIDFUNC(1,fmov_1,(FW r))
{
    r=f_writereg(r);
    raw_fmov_1(r);
    f_unlock(r);
}
MENDFUNC(1,fmov_1,(FW r))

MIDFUNC(1,fmov_0,(FW r))
{
    r=f_writereg(r);
    raw_fmov_0(r);
    f_unlock(r);
}
MENDFUNC(1,fmov_0,(FW r))

MIDFUNC(2,fmov_rm,(FW r, MEMR m))
{
    r=f_writereg(r);
    raw_fmov_rm(r,m);
    f_unlock(r);
}
MENDFUNC(2,fmov_rm,(FW r, MEMR m))

MIDFUNC(2,fmovi_rm,(FW r, MEMR m))
{
    r=f_writereg(r);
    raw_fmovi_rm(r,m);
    f_unlock(r);
}
MENDFUNC(2,fmovi_rm,(FW r, MEMR m))

<<<<<<< HEAD
MIDFUNC(2,fmovi_mr,(MEMW m, FR r))
{
    r=f_readreg(r);
    raw_fmovi_mr(m,r);
    f_unlock(r);
}
MENDFUNC(2,fmovi_mr,(MEMW m, FR r))
=======
MIDFUNC(3,fmovi_mrb,(MEMW m, FR r, double *bounds))
{
    r=f_readreg(r);
    raw_fmovi_mrb(m,r,bounds);
    f_unlock(r);
}
MENDFUNC(3,fmovi_mrb,(MEMW m, FR r, double *bounds))
>>>>>>> p-uae/v2.1.0

MIDFUNC(2,fmovs_rm,(FW r, MEMR m))
{
    r=f_writereg(r);
    raw_fmovs_rm(r,m);
    f_unlock(r);
}
MENDFUNC(2,fmovs_rm,(FW r, MEMR m))

MIDFUNC(2,fmovs_mr,(MEMW m, FR r))
{
    r=f_readreg(r);
    raw_fmovs_mr(m,r);
    f_unlock(r);
}
MENDFUNC(2,fmovs_mr,(MEMW m, FR r))

<<<<<<< HEAD
=======
MIDFUNC(1,fcuts_r,(FRW r))
{
    r=f_rmw(r);
    raw_fcuts_r(r);
    f_unlock(r);
}
MENDFUNC(1,fcuts_r,(FRW r))

MIDFUNC(1,fcut_r,(FRW r))
{
    r=f_rmw(r);
    raw_fcut_r(r);
    f_unlock(r);
}
MENDFUNC(1,fcut_r,(FRW r))

MIDFUNC(2,fmovl_ri,(FW r, IMMS i))
{
    r=f_writereg(r);
    raw_fmovl_ri(r,i);
    f_unlock(r);
}
MENDFUNC(2,fmovl_ri,(FW r, IMMS i))

MIDFUNC(2,fmovs_ri,(FW r, IMM i))
{
    r=f_writereg(r);
    raw_fmovs_ri(r,i);
    f_unlock(r);
}
MENDFUNC(2,fmovs_ri,(FW r, IMM i))

MIDFUNC(3,fmov_ri,(FW r, IMM i1, IMM i2))
{
    r=f_writereg(r);
    raw_fmov_ri(r,i1,i2);
    f_unlock(r);
}
MENDFUNC(3,fmov_ri,(FW r, IMM i1, IMM i2))

MIDFUNC(4,fmov_ext_ri,(FW r, IMM i1, IMM i2, IMM i3))
{
    r=f_writereg(r);
    raw_fmov_ext_ri(r,i1,i2,i3);
    f_unlock(r);
}
MENDFUNC(4,fmov_ext_ri,(FW r, IMM i1, IMM i2, IMM i3))

>>>>>>> p-uae/v2.1.0
MIDFUNC(2,fmov_ext_mr,(MEMW m, FR r))
{
    r=f_readreg(r);
    raw_fmov_ext_mr(m,r);
    f_unlock(r);
}
MENDFUNC(2,fmov_ext_mr,(MEMW m, FR r))

MIDFUNC(2,fmov_mr,(MEMW m, FR r))
{
    r=f_readreg(r);
    raw_fmov_mr(m,r);
    f_unlock(r);
}
MENDFUNC(2,fmov_mr,(MEMW m, FR r))

MIDFUNC(2,fmov_ext_rm,(FW r, MEMR m))
{
    r=f_writereg(r);
    raw_fmov_ext_rm(r,m);
    f_unlock(r);
}
MENDFUNC(2,fmov_ext_rm,(FW r, MEMR m))

MIDFUNC(2,fmov_rr,(FW d, FR s))
{
    if (d==s) { /* How pointless! */
	return;
    }
#if USE_F_ALIAS
    f_disassociate(d);
    s=f_readreg(s);
    live.fate[d].realreg=s;
    live.fate[d].realind=live.fat[s].nholds;
    live.fate[d].status=DIRTY;
    live.fat[s].holds[live.fat[s].nholds]=d;
    live.fat[s].nholds++;
    f_unlock(s);
#else
    s=f_readreg(s);
    d=f_writereg(d);
    raw_fmov_rr(d,s);
    f_unlock(s);
    f_unlock(d);
#endif
}
MENDFUNC(2,fmov_rr,(FW d, FR s))

MIDFUNC(2,fldcw_m_indexed,(R4 index, IMM base))
{
    index=readreg(index,4);

    raw_fldcw_m_indexed(index,base);
    unlock(index);
}
MENDFUNC(2,fldcw_m_indexed,(R4 index, IMM base))

MIDFUNC(1,ftst_r,(FR r))
{
    r=f_readreg(r);
    raw_ftst_r(r);
    f_unlock(r);
}
MENDFUNC(1,ftst_r,(FR r))

MIDFUNC(0,dont_care_fflags,(void))
{
    f_disassociate(FP_RESULT);
}
MENDFUNC(0,dont_care_fflags,(void))

MIDFUNC(2,fsqrt_rr,(FW d, FR s))
{
    s=f_readreg(s);
    d=f_writereg(d);
    raw_fsqrt_rr(d,s);
    f_unlock(s);
    f_unlock(d);
}
MENDFUNC(2,fsqrt_rr,(FW d, FR s))

MIDFUNC(2,fabs_rr,(FW d, FR s))
{
    s=f_readreg(s);
    d=f_writereg(d);
    raw_fabs_rr(d,s);
    f_unlock(s);
    f_unlock(d);
}
MENDFUNC(2,fabs_rr,(FW d, FR s))

<<<<<<< HEAD
=======
MIDFUNC(2,frndint_rr,(FW d, FR s))
{
    s=f_readreg(s);
    d=f_writereg(d);
    raw_frndint_rr(d,s);
    f_unlock(s);
    f_unlock(d);
}
MENDFUNC(2,frndint_rr,(FW d, FR s))

MIDFUNC(2,fgetexp_rr,(FW d, FR s))
{
    s=f_readreg(s);
    d=f_writereg(d);
    raw_fgetexp_rr(d,s);
    f_unlock(s);
    f_unlock(d);
}
MENDFUNC(2,fgetexp_rr,(FW d, FR s))

MIDFUNC(2,fgetman_rr,(FW d, FR s))
{
    s=f_readreg(s);
    d=f_writereg(d);
    raw_fgetman_rr(d,s);
    f_unlock(s);
    f_unlock(d);
}
MENDFUNC(2,fgetman_rr,(FW d, FR s))

>>>>>>> p-uae/v2.1.0
MIDFUNC(2,fsin_rr,(FW d, FR s))
{
    s=f_readreg(s);
    d=f_writereg(d);
    raw_fsin_rr(d,s);
    f_unlock(s);
    f_unlock(d);
}
MENDFUNC(2,fsin_rr,(FW d, FR s))

MIDFUNC(2,fcos_rr,(FW d, FR s))
{
    s=f_readreg(s);
    d=f_writereg(d);
    raw_fcos_rr(d,s);
    f_unlock(s);
    f_unlock(d);
}
MENDFUNC(2,fcos_rr,(FW d, FR s))

<<<<<<< HEAD
=======
MIDFUNC(2,ftan_rr,(FW d, FR s))
{
    s=f_readreg(s);
    d=f_writereg(d);
    raw_ftan_rr(d,s);
    f_unlock(s);
    f_unlock(d);
}
MENDFUNC(2,ftan_rr,(FW d, FR s))

MIDFUNC(3,fsincos_rr,(FW d, FW c, FR s))
{
    s=f_readreg(s);  /* s for source */
    d=f_writereg(d); /* d for sine   */
    c=f_writereg(c); /* c for cosine */
    raw_fsincos_rr(d,c,s);
    f_unlock(s);
    f_unlock(d);
    f_unlock(c);
}
MENDFUNC(3,fsincos_rr,(FW d, FW c, FR s))

MIDFUNC(2,fscale_rr,(FRW d, FR s))
{
    s=f_readreg(s);
    d=f_rmw(d);
    raw_fscale_rr(d,s);
    f_unlock(s);
    f_unlock(d);
}
MENDFUNC(2,fscale_rr,(FRW d, FR s))

>>>>>>> p-uae/v2.1.0
MIDFUNC(2,ftwotox_rr,(FW d, FR s))
{
    s=f_readreg(s);
    d=f_writereg(d);
    raw_ftwotox_rr(d,s);
    f_unlock(s);
    f_unlock(d);
}
MENDFUNC(2,ftwotox_rr,(FW d, FR s))

MIDFUNC(2,fetox_rr,(FW d, FR s))
{
    s=f_readreg(s);
    d=f_writereg(d);
    raw_fetox_rr(d,s);
    f_unlock(s);
    f_unlock(d);
}
MENDFUNC(2,fetox_rr,(FW d, FR s))

<<<<<<< HEAD
MIDFUNC(2,frndint_rr,(FW d, FR s))
{
    s=f_readreg(s);
    d=f_writereg(d);
    raw_frndint_rr(d,s);
    f_unlock(s);
    f_unlock(d);
}
MENDFUNC(2,frndint_rr,(FW d, FR s))
=======
MIDFUNC(2,fetoxM1_rr,(FW d, FR s))
{
    s=f_readreg(s);
    d=f_writereg(d);
    raw_fetoxM1_rr(d,s);
    f_unlock(s);
    f_unlock(d);
}
MENDFUNC(2,fetoxM1_rr,(FW d, FR s))

MIDFUNC(2,ftentox_rr,(FW d, FR s))
{
    s=f_readreg(s);
    d=f_writereg(d);
    raw_ftentox_rr(d,s);
    f_unlock(s);
    f_unlock(d);
}
MENDFUNC(2,ftentox_rr,(FW d, FR s))
>>>>>>> p-uae/v2.1.0

MIDFUNC(2,flog2_rr,(FW d, FR s))
{
    s=f_readreg(s);
    d=f_writereg(d);
    raw_flog2_rr(d,s);
    f_unlock(s);
    f_unlock(d);
}
MENDFUNC(2,flog2_rr,(FW d, FR s))

<<<<<<< HEAD
=======
MIDFUNC(2,flogN_rr,(FW d, FR s))
{
    s=f_readreg(s);
    d=f_writereg(d);
    raw_flogN_rr(d,s);
    f_unlock(s);
    f_unlock(d);
}
MENDFUNC(2,flogN_rr,(FW d, FR s))

MIDFUNC(2,flogNP1_rr,(FW d, FR s))
{
    s=f_readreg(s);
    d=f_writereg(d);
    raw_flogNP1_rr(d,s);
    f_unlock(s);
    f_unlock(d);
}
MENDFUNC(2,flogNP1_rr,(FW d, FR s))

MIDFUNC(2,flog10_rr,(FW d, FR s))
{
    s=f_readreg(s);
    d=f_writereg(d);
    raw_flog10_rr(d,s);
    f_unlock(s);
    f_unlock(d);
}
MENDFUNC(2,flog10_rr,(FW d, FR s))

MIDFUNC(2,fasin_rr,(FW d, FR s))
{
    s=f_readreg(s);
    d=f_writereg(d);
    raw_fasin_rr(d,s);
    f_unlock(s);
    f_unlock(d);
}
MENDFUNC(2,fasin_rr,(FW d, FR s))

MIDFUNC(2,facos_rr,(FW d, FR s))
{
    s=f_readreg(s);
    d=f_writereg(d);
    raw_facos_rr(d,s);
    f_unlock(s);
    f_unlock(d);
}
MENDFUNC(2,facos_rr,(FW d, FR s))

MIDFUNC(2,fatan_rr,(FW d, FR s))
{
    s=f_readreg(s);
    d=f_writereg(d);
    raw_fatan_rr(d,s);
    f_unlock(s);
    f_unlock(d);
}
MENDFUNC(2,fatan_rr,(FW d, FR s))

MIDFUNC(2,fatanh_rr,(FW d, FR s))
{
    s=f_readreg(s);
    d=f_writereg(d);
    raw_fatanh_rr(d,s);
    f_unlock(s);
    f_unlock(d);
}
MENDFUNC(2,fatanh_rr,(FW d, FR s))

MIDFUNC(2,fsinh_rr,(FW d, FR s))
{
    s=f_readreg(s);
    d=f_writereg(d);
    raw_fsinh_rr(d,s);
    f_unlock(s);
    f_unlock(d);
}
MENDFUNC(2,fsinh_rr,(FW d, FR s))

MIDFUNC(2,fcosh_rr,(FW d, FR s))
{
    s=f_readreg(s);
    d=f_writereg(d);
    raw_fcosh_rr(d,s);
    f_unlock(s);
    f_unlock(d);
}
MENDFUNC(2,fcosh_rr,(FW d, FR s))

MIDFUNC(2,ftanh_rr,(FW d, FR s))
{
    s=f_readreg(s);
    d=f_writereg(d);
    raw_ftanh_rr(d,s);
    f_unlock(s);
    f_unlock(d);
}
MENDFUNC(2,ftanh_rr,(FW d, FR s))

>>>>>>> p-uae/v2.1.0
MIDFUNC(2,fneg_rr,(FW d, FR s))
{
    s=f_readreg(s);
    d=f_writereg(d);
    raw_fneg_rr(d,s);
    f_unlock(s);
    f_unlock(d);
}
MENDFUNC(2,fneg_rr,(FW d, FR s))

MIDFUNC(2,fadd_rr,(FRW d, FR s))
{
    s=f_readreg(s);
    d=f_rmw(d);
    raw_fadd_rr(d,s);
    f_unlock(s);
    f_unlock(d);
}
MENDFUNC(2,fadd_rr,(FRW d, FR s))

MIDFUNC(2,fsub_rr,(FRW d, FR s))
{
    s=f_readreg(s);
    d=f_rmw(d);
    raw_fsub_rr(d,s);
    f_unlock(s);
    f_unlock(d);
}
MENDFUNC(2,fsub_rr,(FRW d, FR s))

MIDFUNC(2,fcmp_rr,(FR d, FR s))
{
    d=f_readreg(d);
    s=f_readreg(s);
    raw_fcmp_rr(d,s);
    f_unlock(s);
    f_unlock(d);
}
MENDFUNC(2,fcmp_rr,(FR d, FR s))

MIDFUNC(2,fdiv_rr,(FRW d, FR s))
{
    s=f_readreg(s);
    d=f_rmw(d);
    raw_fdiv_rr(d,s);
    f_unlock(s);
    f_unlock(d);
}
MENDFUNC(2,fdiv_rr,(FRW d, FR s))

MIDFUNC(2,frem_rr,(FRW d, FR s))
{
    s=f_readreg(s);
    d=f_rmw(d);
    raw_frem_rr(d,s);
    f_unlock(s);
    f_unlock(d);
}
MENDFUNC(2,frem_rr,(FRW d, FR s))

MIDFUNC(2,frem1_rr,(FRW d, FR s))
{
    s=f_readreg(s);
    d=f_rmw(d);
    raw_frem1_rr(d,s);
    f_unlock(s);
    f_unlock(d);
}
MENDFUNC(2,frem1_rr,(FRW d, FR s))

MIDFUNC(2,fmul_rr,(FRW d, FR s))
{
    s=f_readreg(s);
    d=f_rmw(d);
    raw_fmul_rr(d,s);
    f_unlock(s);
    f_unlock(d);
}
MENDFUNC(2,fmul_rr,(FRW d, FR s))

<<<<<<< HEAD

=======
>>>>>>> p-uae/v2.1.0
/********************************************************************
 * Support functions exposed to gencomp. CREATE time                *
 ********************************************************************/

int kill_rodent(int r)
{
    return KILLTHERAT &&
<<<<<<< HEAD
        have_rat_stall &&
	(live.state[r].status==INMEM ||
	 live.state[r].status==CLEAN ||
	 live.state[r].status==ISCONST ||
	 live.state[r].dirtysize==4);
=======
		have_rat_stall &&
		(live.state[r].status==INMEM ||
	 	live.state[r].status==CLEAN ||
	 	live.state[r].status==ISCONST ||
	 	live.state[r].dirtysize==4);
>>>>>>> p-uae/v2.1.0
}

uae_u32 get_const(int r)
{
<<<<<<< HEAD
	Dif (!isconst(r)) {
	    write_log ("JIT: Register %d should be constant, but isn't\n", r);
	    abort();
=======
#if USE_OPTIMIZER
    if (!reg_alloc_run)
#endif
		Dif (!isconst(r)) {
			jit_abort ("JIT: Register %d should be constant, but isn't\n",r);
>>>>>>> p-uae/v2.1.0
	}
    return live.state[r].val;
}

void sync_m68k_pc(void)
{
    if (m68k_pc_offset) {
<<<<<<< HEAD
	add_l_ri(PC_P,m68k_pc_offset);
	comp_pc_p+=m68k_pc_offset;
	m68k_pc_offset=0;
=======
		add_l_ri(PC_P,m68k_pc_offset);
		comp_pc_p+=m68k_pc_offset;
		m68k_pc_offset=0;
>>>>>>> p-uae/v2.1.0
    }
}

/********************************************************************
 * Support functions exposed to newcpu                              *
 ********************************************************************/

uae_u32 scratch[VREGS];
<<<<<<< HEAD
fptype  fscratch[VFREGS];
=======
#ifdef FPUEMU
fptype  fscratch[VFREGS];
#endif
>>>>>>> p-uae/v2.1.0

void init_comp(void)
{
    int i;
<<<<<<< HEAD
    uae_u8* cb = (uae_u8 *)can_byte;
    uae_u8* cw = (uae_u8 *)can_word;
    uae_u8* au = (uae_u8 *)always_used;

    for (i=0;i<VREGS;i++) {
	live.state[i].realreg=-1;
	live.state[i].needflush=NF_SCRATCH;
	live.state[i].val=0;
	set_status(i,UNDEF);
    }

    for (i=0;i<VFREGS;i++) {
	live.fate[i].status=UNDEF;
	live.fate[i].realreg=-1;
	live.fate[i].needflush=NF_SCRATCH;
    }

    for (i=0;i<VREGS;i++) {
	if (i<16) { /* First 16 registers map to 68k registers */
	    live.state[i].mem=((uae_u32*)&regs)+i;
	    live.state[i].needflush=NF_TOMEM;
	    set_status(i,INMEM);
	}
	else
	    live.state[i].mem=scratch+i;
    }
    live.state[PC_P].mem=(uae_u32*)&(regs.pc_p);
    live.state[PC_P].needflush=NF_TOMEM;
    set_const(PC_P,(uae_uintptr)comp_pc_p);

    live.state[FLAGX].mem=&(regs.ccrflags.x);
    live.state[FLAGX].needflush=NF_TOMEM;
    set_status(FLAGX,INMEM);

    live.state[FLAGTMP].mem=&(regs.ccrflags.cznv);
=======
    uae_u8* cb=can_byte;
    uae_u8* cw=can_word;
    uae_u8* au=always_used;

    for (i=0;i<VREGS;i++) {
		live.state[i].realreg=-1;
		live.state[i].needflush=NF_SCRATCH;
		live.state[i].val=0;
		set_status(i,UNDEF);
    }

    for (i=0;i<VFREGS;i++) {
		live.fate[i].status=UNDEF;
		live.fate[i].realreg=-1;
		live.fate[i].needflush=NF_SCRATCH;
    }

    for (i=0;i<VREGS;i++) {
		if (i<16) { /* First 16 registers map to 68k registers */
		    live.state[i].mem=((uae_u32*)&regs)+i;
		    live.state[i].needflush=NF_TOMEM;
		    set_status(i,INMEM);
		}
		else
		    live.state[i].mem=scratch+i;
    }
    live.state[PC_P].mem=(uae_u32*)&(regs.pc_p);
    live.state[PC_P].needflush=NF_TOMEM;
    set_const(PC_P,(uae_u32)comp_pc_p);

    live.state[FLAGX].mem=&(regflags.x);
    live.state[FLAGX].needflush=NF_TOMEM;
    set_status(FLAGX,INMEM);

    live.state[FLAGTMP].mem=&(regflags.cznv);
>>>>>>> p-uae/v2.1.0
    live.state[FLAGTMP].needflush=NF_TOMEM;
    set_status(FLAGTMP,INMEM);

    live.state[NEXT_HANDLER].needflush=NF_HANDLER;
    set_status(NEXT_HANDLER,UNDEF);

<<<<<<< HEAD
    for (i=0;i<VFREGS;i++) {
	if (i<8) { /* First 8 registers map to 68k FPU registers */
	    live.fate[i].mem=(uae_u32*)(((fptype*)regs.fp)+i);
	    live.fate[i].needflush=NF_TOMEM;
	    live.fate[i].status=INMEM;
	}
	else if (i==FP_RESULT) {
	    live.fate[i].mem=(uae_u32*)(&regs.fp_result);
	    live.fate[i].needflush=NF_TOMEM;
	    live.fate[i].status=INMEM;
	}
	else
	    live.fate[i].mem=(uae_u32*)(fscratch+i);
    }


    for (i=0;i<N_REGS;i++) {
	live.nat[i].touched=0;
	live.nat[i].nholds=0;
	live.nat[i].locked=0;
	if (*cb==i) {
	    live.nat[i].canbyte=1; cb++;
	} else live.nat[i].canbyte=0;
	if (*cw==i) {
	    live.nat[i].canword=1; cw++;
	} else live.nat[i].canword=0;
	if (*au==i) {
	    live.nat[i].locked=1; au++;
	}
    }

    for (i=0;i<N_FREGS;i++) {
	live.fat[i].touched=0;
	live.fat[i].nholds=0;
	live.fat[i].locked=0;
=======
#ifdef FPUEMU
    for (i=0;i<VFREGS;i++) {
		if (i<8) { /* First 8 registers map to 68k FPU registers */
		    live.fate[i].mem=(uae_u32*)(((fptype*)regs.fp)+i);
		    live.fate[i].needflush=NF_TOMEM;
		    live.fate[i].status=INMEM;
		}
		else if (i==FP_RESULT) {
		    live.fate[i].mem=(uae_u32*)(&regs.fp_result);
		    live.fate[i].needflush=NF_TOMEM;
		    live.fate[i].status=INMEM;
		}
		else
		    live.fate[i].mem=(uae_u32*)(fscratch+i);
    }
#endif

    for (i=0;i<N_REGS;i++) {
		live.nat[i].touched=0;
		live.nat[i].nholds=0;
		live.nat[i].locked=0;
		if (*cb==i) {
		    live.nat[i].canbyte=1; cb++;
		} else live.nat[i].canbyte=0;
		if (*cw==i) {
		    live.nat[i].canword=1; cw++;
		} else live.nat[i].canword=0;
		if (*au==i) {
		    live.nat[i].locked=1; au++;
		}
    }

    for (i=0;i<N_FREGS;i++) {
		live.fat[i].touched=0;
		live.fat[i].nholds=0;
		live.fat[i].locked=0;
>>>>>>> p-uae/v2.1.0
    }

    touchcnt=1;
    m68k_pc_offset=0;
    live.flags_in_flags=TRASH;
    live.flags_on_stack=VALID;
    live.flags_are_important=1;

    raw_fp_init();
}

<<<<<<< HEAD

=======
>>>>>>> p-uae/v2.1.0
static void vinton(int i, uae_s8* vton, int depth)
{
    int n;
    int rr;

    Dif (vton[i]==-1) {
<<<<<<< HEAD
	write_log ("JIT: Asked to load register %d, but nowhere to go\n", i);
	abort();
    }
    n=vton[i];
    Dif (live.nat[n].nholds>1)
	abort();
    if (live.nat[n].nholds && depth<N_REGS) {
	vinton(live.nat[n].holds[0],vton,depth+1);
    }
    if (!isinreg(i))
	return;  /* Oops --- got rid of that one in the recursive calls */
    rr=live.state[i].realreg;
    if (rr!=n)
	mov_nregs(n,rr);
=======
		jit_abort ("JIT: Asked to load register %d, but nowhere to go\n",i);
    }
    n=vton[i];
    Dif (live.nat[n].nholds>1)
		jit_abort ("vinton");
    if (live.nat[n].nholds && depth<N_REGS) {
		vinton(live.nat[n].holds[0],vton,depth+1);
    }
    if (!isinreg(i))
		return;  /* Oops --- got rid of that one in the recursive calls */
    rr=live.state[i].realreg;
    if (rr!=n)
		mov_nregs(n,rr);
>>>>>>> p-uae/v2.1.0
}

#if USE_MATCHSTATE
/* This is going to be, amongst other things, a more elaborate version of
   flush() */
<<<<<<< HEAD
static __inline__ void match_states(smallstate* s)
=======
STATIC_INLINE void match_states(smallstate* s)
>>>>>>> p-uae/v2.1.0
{
    uae_s8 vton[VREGS];
    uae_s8 ndone[N_REGS];
    int i;
    int again=0;

    for (i=0;i<VREGS;i++)
<<<<<<< HEAD
	vton[i]=-1;

    for (i=0;i<N_REGS;i++)
	if (s->nat[i].validsize)
	    vton[s->nat[i].holds]=i;
=======
		vton[i]=-1;

    for (i=0;i<N_REGS;i++)
		if (s->nat[i].validsize)
		    vton[s->nat[i].holds]=i;
>>>>>>> p-uae/v2.1.0

    flush_flags(); /* low level */
    sync_m68k_pc(); /* mid level */

    /* We don't do FREGS yet, so this is raw flush() code */
    for (i=0;i<VFREGS;i++) {
<<<<<<< HEAD
	if (live.fate[i].needflush==NF_SCRATCH ||
	    live.fate[i].status==CLEAN) {
	    f_disassociate(i);
	}
    }
    for (i=0;i<VFREGS;i++) {
	if (live.fate[i].needflush==NF_TOMEM &&
	    live.fate[i].status==DIRTY) {
	    f_evict(i);
	}
=======
		if (live.fate[i].needflush==NF_SCRATCH ||
		    live.fate[i].status==CLEAN) {
			    f_disassociate(i);
		}
    }
    for (i=0;i<VFREGS;i++) {
		if (live.fate[i].needflush==NF_TOMEM &&
		    live.fate[i].status==DIRTY) {
			    f_evict(i);
		}
>>>>>>> p-uae/v2.1.0
    }
    raw_fp_cleanup_drop();

    /* Now comes the fun part. First, we need to remove all offsets */
    for (i=0;i<VREGS;i++)
<<<<<<< HEAD
	if (!isconst(i) && live.state[i].val)
	    remove_offset(i,-1);

    /* Next, we evict everything that does not end up in registers,
       write back overly dirty registers, and write back constants */
    for (i=0;i<VREGS;i++) {
	switch (live.state[i].status) {
	 case ISCONST:
	    if (i!=PC_P)
		writeback_const(i);
	    break;
	 case DIRTY:
	    if (vton[i]==-1) {
		evict(i);
		break;
	    }
	    if (live.state[i].dirtysize>s->nat[vton[i]].dirtysize)
		tomem(i);
	    /* Fall-through! */
	 case CLEAN:
	    if (vton[i]==-1 ||
		live.state[i].validsize<s->nat[vton[i]].validsize)
		evict(i);
	    else
		make_exclusive(i,0,-1);
	    break;
	 case INMEM:
	    break;
	 case UNDEF:
	    break;
	 default:
	    write_log ("JIT: Weird status: %d\n", live.state[i].status);
	    abort();
	}
=======
		if (!isconst(i) && live.state[i].val)
		    remove_offset(i,-1);

	/* Next, we evict everything that does not end up in registers,
	write back overly dirty registers, and write back constants */
	for (i=0;i<VREGS;i++) {
		switch (live.state[i].status) {
		case ISCONST:
			if (i!=PC_P)
				writeback_const(i);
			break;
		case DIRTY:
			if (vton[i]==-1) {
				evict(i);
				break;
			}
			if (live.state[i].dirtysize>s->nat[vton[i]].dirtysize)
				tomem(i);
			/* Fall-through! */
		case CLEAN:
			if (vton[i]==-1 ||
				live.state[i].validsize<s->nat[vton[i]].validsize)
				evict(i);
			else
				make_exclusive(i,0,-1);
			break;
		case INMEM:
			break;
		case UNDEF:
			break;
		default:
			write_log ("JIT: Weird status: %d\n", live.state[i].status);
	    	abort();
		}
>>>>>>> p-uae/v2.1.0
    }

    /* Quick consistency check */
    for (i=0;i<VREGS;i++) {
<<<<<<< HEAD
	if (isinreg(i)) {
	    int n=live.state[i].realreg;

	    if (live.nat[n].nholds!=1) {
		write_log ("JIT: Register %d isn't alone in nreg %d\n", i, n);
		abort();
	    }
	    if (vton[i]==-1) {
		write_log ("JIT: Register %d is still in register, shouldn't be\n", i);
		abort();
	    }
	}
=======
		if (isinreg(i)) {
		    int n=live.state[i].realreg;

		    if (live.nat[n].nholds!=1) {
				write_log ("JIT: Register %d isn't alone in nreg %d\n", i, n);
				abort();
		    }
		    if (vton[i]==-1) {
				write_log ("JIT: Register %d is still in register, shouldn't be\n", i);
				abort();
		    }
		}
>>>>>>> p-uae/v2.1.0
    }

    /* Now we need to shuffle things around so the VREGs are in the
       right N_REGs. */
    for (i=0;i<VREGS;i++) {
<<<<<<< HEAD
	if (isinreg(i) && vton[i]!=live.state[i].realreg)
	    vinton(i,vton,0);
=======
		if (isinreg(i) && vton[i]!=live.state[i].realreg)
		    vinton(i,vton,0);
>>>>>>> p-uae/v2.1.0
    }

    /* And now we may need to load some registers from memory */
    for (i=0;i<VREGS;i++) {
<<<<<<< HEAD
	int n=vton[i];
	if (n==-1) {
	    Dif (isinreg(i)) {
		write_log ("JIT: Register %d unexpectedly in nreg %d\n",
			   i, live.state[i].realreg);
		abort();
	    }
	}
	else {
	    switch(live.state[i].status) {
	     case CLEAN:
	     case DIRTY:
		Dif (n!=live.state[i].realreg)
		    abort();
		break;
	     case INMEM:
		Dif (live.nat[n].nholds) {
		    write_log ("JIT: natreg %d holds %d vregs, should be empty\n",
			       n, live.nat[n].nholds);
		}
		raw_mov_l_rm(n,(uae_uintptr)live.state[i].mem);
		live.state[i].validsize=4;
		live.state[i].dirtysize=0;
		live.state[i].realreg=n;
		live.state[i].realind=0;
		live.state[i].val=0;
		live.state[i].is_swapped=0;
		live.nat[n].nholds=1;
		live.nat[n].holds[0]=i;

		set_status(i,CLEAN);
		break;
	     case ISCONST:
		if (i!=PC_P) {
		    write_log ("JIT: Got constant in matchstate for reg %d. Bad!\n", i);
		    abort();
		}
		break;
	     case UNDEF:
		break;
	    }
	}
    }

    /* One last consistency check, and adjusting the states in live
       to those in s */
    for (i=0;i<VREGS;i++) {
	int n=vton[i];
	switch(live.state[i].status) {
	 case INMEM:
	    if (n!=-1)
		abort();
	    break;
	 case ISCONST:
	    if (i!=PC_P)
		abort();
	    break;
	 case CLEAN:
	 case DIRTY:
	    if (n==-1)
		abort();
	    if (live.state[i].dirtysize>s->nat[n].dirtysize)
		abort;
	    if (live.state[i].validsize<s->nat[n].validsize)
		abort;
	    live.state[i].dirtysize=s->nat[n].dirtysize;
	    live.state[i].validsize=s->nat[n].validsize;
	    if (live.state[i].dirtysize)
		set_status(i,DIRTY);
	    break;
	 case UNDEF:
	    break;
	}
	if (n!=-1)
	    live.nat[n].touched=touchcnt++;
    }
}
#else
static __inline__ void match_states(smallstate* s)
=======
		int n=vton[i];
		if (n==-1) {
		    Dif (isinreg(i)) {
				write_log ("JIT: Register %d unexpectedly in nreg %d\n", i, live.state[i].realreg);
			abort();
	    }
	}
		else {
		    switch(live.state[i].status) {
			case CLEAN:
		    case DIRTY:
				Dif (n!=live.state[i].realreg)
				    abort();
				break;
		    case INMEM:
				Dif (live.nat[n].nholds) {
					write_log ("JIT: natreg %d holds %d vregs, should be empty\n", n, live.nat[n].nholds);
				}
				raw_mov_l_rm(n,(uae_u32)live.state[i].mem);
				live.state[i].validsize=4;
				live.state[i].dirtysize=0;
				live.state[i].realreg=n;
				live.state[i].realind=0;
				live.state[i].val=0;
				live.state[i].is_swapped=0;
				live.nat[n].nholds=1;
				live.nat[n].holds[0]=i;

				set_status(i,CLEAN);
				break;
			case ISCONST:
				if (i!=PC_P) {
					write_log ("JIT: Got constant in matchstate for reg %d. Bad!\n", i);
				    abort();
				}
				break;
	     	case UNDEF:
				break;
	    	}
	}
    }

	/* One last consistency check, and adjusting the states in live
	to those in s */
	for (i=0;i<VREGS;i++) {
		int n=vton[i];
		switch(live.state[i].status) {
		case INMEM:
			if (n!=-1)
				abort();
			break;
		case ISCONST:
			if (i!=PC_P)
				abort();
			break;
		case CLEAN:
		case DIRTY:
			if (n==-1)
				abort();
			if (live.state[i].dirtysize>s->nat[n].dirtysize)
				abort;
			if (live.state[i].validsize<s->nat[n].validsize)
				abort;
			live.state[i].dirtysize=s->nat[n].dirtysize;
			live.state[i].validsize=s->nat[n].validsize;
			if (live.state[i].dirtysize)
				set_status(i,DIRTY);
			break;
		case UNDEF:
			break;
		}
		if (n!=-1)
			live.nat[n].touched=touchcnt++;
	}
}
#else
STATIC_INLINE void match_states(smallstate* s)
>>>>>>> p-uae/v2.1.0
{
    flush(1);
}
#endif

/* Only do this if you really mean it! The next call should be to init!*/
void flush(int save_regs)
{
    int i;

    log_flush();
    flush_flags(); /* low level */
    sync_m68k_pc(); /* mid level */

<<<<<<< HEAD
    if (save_regs) {
	for (i=0;i<VFREGS;i++) {
	    if (live.fate[i].needflush==NF_SCRATCH ||
		live.fate[i].status==CLEAN) {
		f_disassociate(i);
	    }
	}
	for (i=0;i<VREGS;i++) {
	    if (live.state[i].needflush==NF_TOMEM) {
		switch(live.state[i].status) {
		 case INMEM:
		    if (live.state[i].val) {
			raw_add_l_mi((uae_uintptr)live.state[i].mem,live.state[i].val);
			live.state[i].val=0;
		    }
		    break;
		 case CLEAN:
		 case DIRTY:
		    remove_offset(i,-1); tomem(i); break;
		 case ISCONST:
		    if (i!=PC_P)
			writeback_const(i);
		    break;
		 default: break;
		}
		Dif (live.state[i].val && i!=PC_P) {
		    write_log ("JIT: Register %d still has val %x\n",
			       i, live.state[i].val);
		}
	    }
	}
	for (i=0;i<VFREGS;i++) {
	    if (live.fate[i].needflush==NF_TOMEM &&
		live.fate[i].status==DIRTY) {
		f_evict(i);
	    }
	}
	raw_fp_cleanup_drop();
    }
    if (needflags) {
	write_log ("JIT: Warning! flush with needflags=1!\n");
    }
=======
	if (save_regs) {
		for (i=0;i<VFREGS;i++) {
			if (live.fate[i].needflush==NF_SCRATCH ||
				live.fate[i].status==CLEAN) {
					f_disassociate(i);
			}
		}
		for (i=0;i<VREGS;i++) {
			if (live.state[i].needflush==NF_TOMEM) {
				switch(live.state[i].status) {
				case INMEM:
					if (live.state[i].val) {
						raw_add_l_mi((uae_u32)live.state[i].mem,live.state[i].val);
						live.state[i].val=0;
					}
					break;
				case CLEAN:
				case DIRTY:
					remove_offset(i,-1); tomem(i); break;
				case ISCONST:
					if (i!=PC_P)
						writeback_const(i);
					break;
				default: break;
				}
				Dif (live.state[i].val && i!=PC_P) {
					write_log ("JIT: Register %d still has val %x\n", i, live.state[i].val);
				}
		    }
		}
		for (i=0;i<VFREGS;i++) {
			if (live.fate[i].needflush==NF_TOMEM &&
				live.fate[i].status==DIRTY) {
					f_evict(i);
			}
		}
		raw_fp_cleanup_drop();
	}
	if (needflags) {
		write_log ("JIT: Warning! flush with needflags=1!\n");
    }

    lopt_emit_all();
>>>>>>> p-uae/v2.1.0
}

static void flush_keepflags(void)
{
    int i;

    for (i=0;i<VFREGS;i++) {
<<<<<<< HEAD
	if (live.fate[i].needflush==NF_SCRATCH ||
	    live.fate[i].status==CLEAN) {
	    f_disassociate(i);
	}
    }
    for (i=0;i<VREGS;i++) {
	if (live.state[i].needflush==NF_TOMEM) {
	    switch(live.state[i].status) {
	     case INMEM:
		/* Can't adjust the offset here --- that needs "add" */
		break;
	     case CLEAN:
	     case DIRTY:
		remove_offset(i,-1); tomem(i); break;
	     case ISCONST:
		if (i!=PC_P)
		    writeback_const(i);
		break;
	     default: break;
	    }
	}
    }
    for (i=0;i<VFREGS;i++) {
	if (live.fate[i].needflush==NF_TOMEM &&
	    live.fate[i].status==DIRTY) {
	    f_evict(i);
	}
    }
    raw_fp_cleanup_drop();
=======
		if (live.fate[i].needflush==NF_SCRATCH ||
		    live.fate[i].status==CLEAN) {
			    f_disassociate(i);
		}
    }
	for (i=0;i<VREGS;i++) {
		if (live.state[i].needflush==NF_TOMEM) {
			switch(live.state[i].status) {
			case INMEM:
				/* Can't adjust the offset here --- that needs "add" */
				break;
			case CLEAN:
			case DIRTY:
				remove_offset(i,-1); tomem(i); break;
			case ISCONST:
				if (i!=PC_P)
					writeback_const(i);
				break;
			default: break;
			}
		}
	}
	for (i=0;i<VFREGS;i++) {
		if (live.fate[i].needflush==NF_TOMEM &&
			live.fate[i].status==DIRTY) {
				f_evict(i);
		}
	}
    raw_fp_cleanup_drop();
    lopt_emit_all();
>>>>>>> p-uae/v2.1.0
}

void freescratch(void)
{
    int i;
    for (i=0;i<N_REGS;i++)
<<<<<<< HEAD
	if (live.nat[i].locked && i!=4)
	    write_log ("JIT: Warning! %d is locked\n", i);

    for (i=0;i<VREGS;i++)
	if (live.state[i].needflush==NF_SCRATCH) {
	    forget_about(i);
	}

    for (i=0;i<VFREGS;i++)
	if (live.fate[i].needflush==NF_SCRATCH) {
	    f_forget_about(i);
	}
=======
		if (live.nat[i].locked && i!=4)
			write_log ("JIT: Warning! %d is locked\n", i);

    for (i=0;i<VREGS;i++)
		if (live.state[i].needflush==NF_SCRATCH) {
		    forget_about(i);
		}

	    for (i=0;i<VFREGS;i++)
			if (live.fate[i].needflush==NF_SCRATCH) {
			    f_forget_about(i);
			}
>>>>>>> p-uae/v2.1.0
}

/********************************************************************
 * Support functions, internal                                      *
 ********************************************************************/


static void align_target (uae_u32 a)
{
<<<<<<< HEAD
    if (!a)
	return;

    if (1)
	raw_emit_nop_filler (a - (((uae_uintptr) target) & (a - 1)));
    else {
	/* Fill with NOPs --- makes debugging with gdb easier */
	while ((uae_uintptr) target & (a - 1))
	    *target++=0x90;
    }
}

extern uae_u8* kickmemory;
STATIC_INLINE int isinrom (uae_uintptr addr)
{
    return (addr >= (uae_uintptr) kickmemory &&
	    addr <  (uae_uintptr) kickmemory + 8 * 65536);
=======
    lopt_emit_all();
	/* Fill with NOPs --- makes debugging with gdb easier */
    while ((uae_u32)target&(a-1))
	    *target++=0x90;
    }

extern uae_u8* kickmemory;
STATIC_INLINE int isinrom(uae_u32 addr)
{
    return (addr>=(uae_u32)kickmemory &&
	    addr<(uae_u32)kickmemory+8*65536);
>>>>>>> p-uae/v2.1.0
}

static void flush_all(void)
{
    int i;

<<<<<<< HEAD
    log_flush();
    for (i=0;i<VREGS;i++)
	if (live.state[i].status==DIRTY) {
	    if (!call_saved[live.state[i].realreg]) {
		tomem(i);
	    }
	}
    for (i=0;i<VFREGS;i++)
	if (f_isinreg(i))
	    f_evict(i);
    raw_fp_cleanup_drop();
=======
	log_flush();
	for (i=0;i<VREGS;i++)
		if (live.state[i].status==DIRTY) {
			if (!call_saved[live.state[i].realreg]) {
				tomem(i);
			}
		}
		for (i=0;i<VFREGS;i++)
			if (f_isinreg(i))
				f_evict(i);
		raw_fp_cleanup_drop();
>>>>>>> p-uae/v2.1.0
}

/* Make sure all registers that will get clobbered by a call are
   save and sound in memory */
static void prepare_for_call_1(void)
{
    flush_all();  /* If there are registers that don't get clobbered,
<<<<<<< HEAD
		   * we should be a bit more selective here */
=======
				   * we should be a bit more selective here */
>>>>>>> p-uae/v2.1.0
}

/* We will call a C routine in a moment. That will clobber all registers,
   so we need to disassociate everything */
static void prepare_for_call_2(void)
{
    int i;
<<<<<<< HEAD
    for (i=0;i<N_REGS;i++)
	if (!call_saved[i] && live.nat[i].nholds>0)
	    free_nreg(i);

    for (i=0;i<N_FREGS;i++)
	if (live.fat[i].nholds>0)
	    f_free_nreg(i);

    live.flags_in_flags=TRASH;  /* Note: We assume we already rescued the
				   flags at the very start of the call_r
				   functions! */
=======
	for (i=0;i<N_REGS;i++)
		if (!call_saved[i] && live.nat[i].nholds>0)
			free_nreg(i);

	for (i=0;i<N_FREGS;i++)
		if (live.fat[i].nholds>0)
			f_free_nreg(i);

	live.flags_in_flags=TRASH;  /* Note: We assume we already rescued the
								flags at the very start of the call_r
								functions! */
>>>>>>> p-uae/v2.1.0
}


/********************************************************************
 * Memory access and related functions, CREATE time                 *
 ********************************************************************/

void register_branch (uae_u32 not_taken, uae_u32 taken, uae_u8 cond)
{
    next_pc_p = not_taken;
    taken_pc_p = taken;
    branch_cc = cond;
}

<<<<<<< HEAD
static uae_uintptr get_handler_address (uae_uintptr addr)
{
    blockinfo *bi = get_blockinfo_addr_new ((void *) addr, 0);

    return (uae_uintptr) &(bi->direct_handler_to_use);
}

static uae_uintptr get_handler (uae_uintptr addr)
{
    blockinfo *bi = get_blockinfo_addr_new ((void *) addr, 0);

    return (uae_uintptr) bi->direct_handler_to_use;
}

static void load_handler (int reg, uae_uintptr addr)
=======
static uae_u32 get_handler_address(uae_u32 addr)
{
    uae_u32 cl=cacheline(addr);
    blockinfo *bi = get_blockinfo_addr_new ((void *) addr, 0);

#if USE_OPTIMIZER
    if (!bi && reg_alloc_run)
		return 0;
#endif
    return (uae_u32)&(bi->direct_handler_to_use);
}

static uae_u32 get_handler(uae_u32 addr)
{
    uae_u32 cl=cacheline(addr);
    blockinfo *bi = get_blockinfo_addr_new ((void *) addr, 0);

#if USE_OPTIMIZER
    if (!bi && reg_alloc_run)
		return 0;
#endif
    return (uae_u32)bi->direct_handler_to_use;
}

static void load_handler(int reg, uae_u32 addr)
>>>>>>> p-uae/v2.1.0
{
    mov_l_rm (reg, get_handler_address (addr));
}

/* This version assumes that it is writing *real* memory, and *will* fail
 *  if that assumption is wrong! No branches, no second chances, just
 *  straight go-for-it attitude */
<<<<<<< HEAD
STATIC_INLINE void writemem_real (int address, int source, int offset, int size, int tmp, int clobber)
=======

static void writemem_real(int address, int source, int offset, int size, int tmp, int clobber)
>>>>>>> p-uae/v2.1.0
{
    int f = tmp;

#ifdef NATMEM_OFFSET
<<<<<<< HEAD
    if (canbang) {  /* Woohoo! go directly at the memory! */
	if (clobber)
	    f = source;
	switch (size) {
	 case 1: mov_b_bRr (address, source, NATMEM_OFFSET); break;
	 case 2: mov_w_rr (f, source); gen_bswap_16 (f); mov_w_bRr (address, f, NATMEM_OFFSET); break;
	 case 4: mov_l_rr (f, source); gen_bswap_32 (f); mov_l_bRr (address, f, NATMEM_OFFSET); break;
	}
	forget_about (tmp);
	forget_about (f);
	return;
    }
#endif

    mov_l_rr (f, address);
    shrl_l_ri (f, 16);   /* The index into the baseaddr table */
    mov_l_rm_indexed (f, (uae_uintptr) baseaddr, f, 4);

    if (address == source && size > 1) { /* IBrowse does this! */
	add_l (f, address); /* f now has the final address */
	switch (size) {
	 case 2: gen_bswap_16 (source); mov_w_Rr (f, source, 0); gen_bswap_16 (source); break;
	 case 4: gen_bswap_32 (source); mov_l_Rr (f, source, 0); gen_bswap_32 (source); break;
	}
    } else {
	/* f now holds the offset */
	switch (size) {
	 case 1: mov_b_mrr_indexed (address,f,1,source); break;
	 case 2: gen_bswap_16 (source); mov_w_mrr_indexed (address, f, 1, source); gen_bswap_16 (source); break;
	 case 4: gen_bswap_32 (source); mov_l_mrr_indexed (address, f, 1, source); gen_bswap_32 (source); break;
	}
    }
=======
	if (canbang) {  /* Woohoo! go directly at the memory! */
		if (clobber)
			f=source;
		switch(size) {
		case 1: mov_b_bRr(address,source,NATMEM_OFFSETX); break;
		case 2: mov_w_rr(f,source); gen_bswap_16(f); mov_w_bRr(address,f,NATMEM_OFFSETX); break;
		case 4: mov_l_rr(f,source); gen_bswap_32(f); mov_l_bRr(address,f,NATMEM_OFFSETX); break;
		}
		forget_about(tmp);
		forget_about(f);
		return;
	}
#endif

	mov_l_rr(f,address);
	shrl_l_ri(f,16);  /* The index into the baseaddr table */
	mov_l_rm_indexed(f,(uae_u32)(baseaddr),f);

	if (address==source) { /* IBrowse does this! */
		if (size > 1) {
			add_l(f,address); /* f now holds the final address */
			switch (size) {
			case 2: gen_bswap_16(source); mov_w_Rr(f,source,0);
				gen_bswap_16(source); return;
			case 4: gen_bswap_32(source); mov_l_Rr(f,source,0);
				gen_bswap_32(source); return;
			}
		}
	}
	switch (size) { /* f now holds the offset */
	case 1: mov_b_mrr_indexed(address,f,source); break;
	case 2: gen_bswap_16(source); mov_w_mrr_indexed(address,f,source);
		gen_bswap_16(source); break;	   /* base, index, source */
	case 4: gen_bswap_32(source); mov_l_mrr_indexed(address,f,source);
		gen_bswap_32(source); break;
	}
>>>>>>> p-uae/v2.1.0
}

STATIC_INLINE void writemem (int address, int source, int offset, int size, int tmp)
{
    int f = tmp;

    mov_l_rr (f, address);
    shrl_l_ri (f, 16);   /* The index into the mem bank table */
<<<<<<< HEAD
    mov_l_rm_indexed (f, (uae_uintptr) mem_banks, f, 4);
=======
    mov_l_rm_indexed(f,(uae_u32)mem_banks,f);
>>>>>>> p-uae/v2.1.0
    /* Now f holds a pointer to the actual membank */
    mov_l_rR (f, f, offset);
    /* Now f holds the address of the b/w/lput function */
    call_r_02 (f, address, source, 4, size);
    forget_about (tmp);
}

void writebyte (int address, int source, int tmp)
{
<<<<<<< HEAD
    int distrust;
    switch (currprefs.comptrustbyte) {
     default:
     case 0: distrust = 0; break;
     case 1: distrust = 1; break;
     case 2: distrust = ((start_pc & 0xF80000) == 0xF80000); break;
     case 3: distrust = !have_done_picasso; break;
    }

    if ((special_mem & SPECIAL_MEM_WRITE) || distrust)
	writemem_special (address, source, 20, 1, tmp);
    else
	writemem_real (address, source, 20, 1, tmp, 0);
}

void writeword_general (int address, int source, int tmp, int clobber)
{
    int distrust;
    switch (currprefs.comptrustword) {
     default:
     case 0: distrust = 0; break;
     case 1: distrust = 1; break;
     case 2: distrust = ((start_pc & 0xF80000) == 0xF80000); break;
     case 3: distrust = !have_done_picasso; break;
    }

    if ((special_mem & SPECIAL_MEM_WRITE) || distrust)
	writemem_special (address, source, 16, 2, tmp);
    else
	writemem_real (address, source, 16, 2, tmp, clobber);
}

void writelong_general (int address, int source, int tmp, int clobber)
{
    int distrust;
    switch (currprefs.comptrustlong) {
     default:
=======
	int  distrust;
	switch (currprefs.comptrustbyte) {
	case 0: distrust=0; break;
	case 1: distrust=1; break;
	case 2: distrust=((start_pc&0xF80000)==0xF80000); break;
	case 3: distrust=!have_done_picasso; break;
	default: abort();
	}

	if ((special_mem&S_WRITE) || distrust)
		writemem_special(address,source,20,1,tmp);
	else
		writemem_real(address,source,20,1,tmp,0);
}

STATIC_INLINE void writeword_general(int address, int source, int tmp,
	int clobber)
{
	int  distrust;
	switch (currprefs.comptrustword) {
	case 0: distrust=0; break;
	case 1: distrust=1; break;
	case 2: distrust=((start_pc&0xF80000)==0xF80000); break;
	case 3: distrust=!have_done_picasso; break;
	default: abort();
	}

	if ((special_mem&S_WRITE) || distrust)
		writemem_special(address,source,16,2,tmp);
	else
		writemem_real(address,source,16,2,tmp,clobber);
}

void writeword_clobber(int address, int source, int tmp)
{
    writeword_general(address,source,tmp,1);
}

void writeword(int address, int source, int tmp)
{
    writeword_general(address,source,tmp,0);
}

STATIC_INLINE void writelong_general(int address, int source, int tmp,
					 int clobber)
{
    int distrust;
    switch (currprefs.comptrustlong) {
>>>>>>> p-uae/v2.1.0
     case 0: distrust = 0; break;
     case 1: distrust = 1; break;
     case 2: distrust = ((start_pc & 0xF80000) == 0xF80000); break;
     case 3: distrust = !have_done_picasso; break;
<<<<<<< HEAD
    }

    if ((special_mem & SPECIAL_MEM_WRITE) || distrust)
=======
     default: abort();
    }

    if ((special_mem&S_WRITE) || distrust)
>>>>>>> p-uae/v2.1.0
	writemem_special (address, source, 12, 4, tmp);
    else
	writemem_real (address, source, 12, 4, tmp, clobber);
}

<<<<<<< HEAD
/* This version assumes that it is reading *real* memory, and *will* fail
 *  if that assumption is wrong! No branches, no second chances, just
 *  straight go-for-it attitude */
STATIC_INLINE void readmem_real (int address, int dest, int offset, int size, int tmp)
=======
void writelong_clobber(int address, int source, int tmp)
{
    writelong_general(address,source,tmp,1);
}

void writelong(int address, int source, int tmp)
{
    writelong_general(address,source,tmp,0);
}



/* This version assumes that it is reading *real* memory, and *will* fail
 *  if that assumption is wrong! No branches, no second chances, just
 *  straight go-for-it attitude */

static void readmem_real(int address, int dest, int offset, int size, int tmp)
>>>>>>> p-uae/v2.1.0
{
    int f = tmp;

    if (size == 4 && address != dest)
	f = dest;

#ifdef NATMEM_OFFSET
    if (canbang) {  /* Woohoo! go directly at the memory! */
	switch (size) {
<<<<<<< HEAD
	 case 1: mov_b_brR (dest, address, NATMEM_OFFSET); break;
	 case 2: mov_w_brR (dest, address, NATMEM_OFFSET); gen_bswap_16 (dest); break;
	 case 4: mov_l_brR (dest, address, NATMEM_OFFSET); gen_bswap_32 (dest); break;
=======
	 case 1: mov_b_brR(dest,address,NATMEM_OFFSETX); break;
	 case 2: mov_w_brR(dest,address,NATMEM_OFFSETX); gen_bswap_16(dest); break;
	 case 4: mov_l_brR(dest,address,NATMEM_OFFSETX); gen_bswap_32(dest); break;
>>>>>>> p-uae/v2.1.0
	}
	forget_about (tmp);
	return;
    }
#endif

    mov_l_rr (f, address);
    shrl_l_ri (f, 16);   /* The index into the baseaddr table */
<<<<<<< HEAD
    mov_l_rm_indexed (f, (uae_uintptr) baseaddr, f, 4);
    /* f now holds the offset */

    switch(size) {
     case 1: mov_b_rrm_indexed (dest, address, f, 1); break;
     case 2: mov_w_rrm_indexed (dest, address, f, 1); gen_bswap_16 (dest); break;
     case 4: mov_l_rrm_indexed (dest, address, f, 1); gen_bswap_32 (dest); break;
=======
    mov_l_rm_indexed(f,(uae_u32)baseaddr,f);
    /* f now holds the offset */

    switch(size) {
     case 1: mov_b_rrm_indexed(dest,address,f); break;
     case 2: mov_w_rrm_indexed(dest,address,f); gen_bswap_16(dest); break;
     case 4: mov_l_rrm_indexed(dest,address,f); gen_bswap_32(dest); break;
>>>>>>> p-uae/v2.1.0
    }
    forget_about (tmp);
}

<<<<<<< HEAD
=======


>>>>>>> p-uae/v2.1.0
STATIC_INLINE void readmem (int address, int dest, int offset, int size, int tmp)
{
    int f = tmp;

    mov_l_rr (f,address);
    shrl_l_ri (f,16);   /* The index into the mem bank table */
<<<<<<< HEAD
    mov_l_rm_indexed (f, (uae_uintptr) mem_banks, f, 4);
=======
    mov_l_rm_indexed(f,(uae_u32)mem_banks,f);
>>>>>>> p-uae/v2.1.0
    /* Now f holds a pointer to the actual membank */
    mov_l_rR (f, f, offset);
    /* Now f holds the address of the b/w/lget function */
    call_r_11 (dest, f, address, size, 4);
    forget_about (tmp);
}

void readbyte (int address, int dest, int tmp)
{
    int distrust;
    switch (currprefs.comptrustbyte) {
<<<<<<< HEAD
     default:
=======
>>>>>>> p-uae/v2.1.0
     case 0: distrust = 0; break;
     case 1: distrust = 1; break;
     case 2: distrust = ((start_pc & 0xF80000) == 0xF80000); break;
     case 3: distrust = !have_done_picasso; break;
<<<<<<< HEAD
    }

    if ((special_mem & SPECIAL_MEM_READ) || distrust)
=======
     default: abort();
    }

    if ((special_mem&S_READ) || distrust)
>>>>>>> p-uae/v2.1.0
	readmem_special (address, dest, 8, 1,tmp);
    else
	readmem_real (address, dest, 8, 1, tmp);
}

void readword (int address, int dest, int tmp)
{
    int distrust;
    switch (currprefs.comptrustword) {
<<<<<<< HEAD
     default:
=======
>>>>>>> p-uae/v2.1.0
     case 0: distrust = 0; break;
     case 1: distrust = 1; break;
     case 2: distrust = ((start_pc & 0xF80000) == 0xF80000); break;
     case 3: distrust = !have_done_picasso; break;
<<<<<<< HEAD
    }

    if ((special_mem & SPECIAL_MEM_READ) || distrust)
=======
     default: abort();
    }

    if ((special_mem&S_READ) || distrust)
>>>>>>> p-uae/v2.1.0
	readmem_special (address, dest, 4, 2, tmp);
    else
	readmem_real (address, dest, 4, 2, tmp);
}

void readlong (int address, int dest, int tmp)
{
    int distrust;
    switch (currprefs.comptrustlong) {
<<<<<<< HEAD
     default:
=======
>>>>>>> p-uae/v2.1.0
     case 0: distrust = 0; break;
     case 1: distrust = 1; break;
     case 2: distrust = ((start_pc & 0xF80000) == 0xF80000); break;
     case 3: distrust = !have_done_picasso; break;
<<<<<<< HEAD
    }

    if ((special_mem & SPECIAL_MEM_READ) || distrust)
=======
     default: abort();
    }

    if ((special_mem&S_READ) || distrust)
>>>>>>> p-uae/v2.1.0
	readmem_special (address, dest, 0, 4,tmp);
    else
	readmem_real (address, dest, 0, 4, tmp);
}

<<<<<<< HEAD
=======


>>>>>>> p-uae/v2.1.0
/* This one might appear a bit odd... */
STATIC_INLINE void get_n_addr_old (int address, int dest, int tmp)
{
    readmem (address, dest, 24, 4, tmp);
}

STATIC_INLINE void get_n_addr_real (int address, int dest, int tmp)
{
    int f = tmp;
    if (address != dest)
	f = dest;

#ifdef NATMEM_OFFSET
    if (canbang) {
<<<<<<< HEAD
	lea_l_brr (dest, address, NATMEM_OFFSET);
=======
	lea_l_brr(dest,address,NATMEM_OFFSETX);
>>>>>>> p-uae/v2.1.0
	forget_about (tmp);
	return;
    }
#endif
    mov_l_rr (f, address);
    mov_l_rr (dest, address); // gb-- nop if dest==address
    shrl_l_ri (f, 16);
<<<<<<< HEAD
    mov_l_rm_indexed (f, (uae_uintptr) baseaddr, f, 4);
=======
    mov_l_rm_indexed(f,(uae_u32)baseaddr,f);
>>>>>>> p-uae/v2.1.0
    add_l (dest,f);
    forget_about (tmp);
}

void get_n_addr (int address, int dest, int tmp)
{
    int distrust;
    switch (currprefs.comptrustnaddr) {
<<<<<<< HEAD
     default:
=======
>>>>>>> p-uae/v2.1.0
     case 0: distrust = 0; break;
     case 1: distrust = 1; break;
     case 2: distrust = ((start_pc & 0xF80000) == 0xF80000); break;
     case 3: distrust = !have_done_picasso; break;
<<<<<<< HEAD
=======
     default: abort();
>>>>>>> p-uae/v2.1.0
    }

    if (special_mem || distrust)
	get_n_addr_old (address,dest,tmp);
    else
	get_n_addr_real (address,dest,tmp);
}

void get_n_addr_jmp (int address, int dest, int tmp)
{
#if 0 /* For this, we need to get the same address as the rest of UAE
	 would --- otherwise we end up translating everything twice */
    get_n_addr (address, dest, tmp);
#else
    int f = tmp;
    if (address != dest)
	f = dest;
    mov_l_rr (f, address);
    shrl_l_ri (f, 16);   /* The index into the baseaddr bank table */
<<<<<<< HEAD
    mov_l_rm_indexed (dest, (uae_uintptr) baseaddr, f, 4);
=======
    mov_l_rm_indexed(dest,(uae_u32)baseaddr,f);
>>>>>>> p-uae/v2.1.0
    add_l (dest, address);
    and_l_ri (dest, ~1);
    forget_about (tmp);
#endif
}

<<<<<<< HEAD

/* base is a register, but dp is an actual value.
   target is a register, as is tmp */
=======
/* base, target and tmp are registers, but dp is the actual opcode extension word */
>>>>>>> p-uae/v2.1.0
void calc_disp_ea_020(int base, uae_u32 dp, int target, int tmp)
{
    int reg = (dp >> 12) & 15;
    int regd_shift=(dp >> 9) & 3;

    if (dp & 0x100) {
	int ignorebase=(dp&0x80);
	int ignorereg=(dp&0x40);
	int addbase=0;
	int outer=0;

	if ((dp & 0x30) == 0x20) addbase = (uae_s32)(uae_s16)comp_get_iword((m68k_pc_offset+=2)-2);
	if ((dp & 0x30) == 0x30) addbase = comp_get_ilong((m68k_pc_offset+=4)-4);

	if ((dp & 0x3) == 0x2) outer = (uae_s32)(uae_s16)comp_get_iword((m68k_pc_offset+=2)-2);
	if ((dp & 0x3) == 0x3) outer = comp_get_ilong((m68k_pc_offset+=4)-4);

	if ((dp & 0x4) == 0) {  /* add regd *before* the get_long */
	    if (!ignorereg) {
		if ((dp & 0x800) == 0)
		    sign_extend_16_rr(target,reg);
		else
		    mov_l_rr(target,reg);
		shll_l_ri(target,regd_shift);
	    }
	    else
		mov_l_ri(target,0);

	    /* target is now regd */
	    if (!ignorebase)
		add_l(target,base);
	    add_l_ri(target,addbase);
	    if (dp&0x03) readlong(target,target,tmp);
	} else { /* do the getlong first, then add regd */
	    if (!ignorebase) {
		mov_l_rr(target,base);
		add_l_ri(target,addbase);
	    }
	    else
		mov_l_ri(target,addbase);
	    if (dp&0x03) readlong(target,target,tmp);

	    if (!ignorereg) {
		if ((dp & 0x800) == 0)
		    sign_extend_16_rr(tmp,reg);
		else
		    mov_l_rr(tmp,reg);
		shll_l_ri(tmp,regd_shift);
		/* tmp is now regd */
		add_l(target,tmp);
	    }
	}
	add_l_ri(target,outer);
    }
    else { /* 68000 version */
	if ((dp & 0x800) == 0) { /* Sign extend */
	    sign_extend_16_rr(target,reg);
<<<<<<< HEAD
	    lea_l_brr_indexed(target,base,target,1<<regd_shift,(uae_s32)((uae_s8)dp));
	}
	else {
	    lea_l_brr_indexed(target,base,reg,1<<regd_shift,(uae_s32)((uae_s8)dp));
=======
	    lea_l_brr_indexed(target,base,target,regd_shift,(uae_s32)(uae_s8)dp);
	}
	else {
	    lea_l_brr_indexed(target,base,reg,regd_shift,(uae_s32)(uae_s8)dp);
>>>>>>> p-uae/v2.1.0
	}
    }
    forget_about(tmp);
}

STATIC_INLINE unsigned int cft_map (unsigned int f)
{
    return ((f >> 8) & 255) | ((f & 255) << 8);
}

<<<<<<< HEAD
extern unsigned long op_illg (uae_u32 opcode, struct regstruct *regs) REGPARAM;
=======
void set_cache_state(int enabled)
{
    if (enabled!=letit)
	flush_icache_hard(0, 3);
    letit=enabled;
}

int get_cache_state(void)
{
    return letit;
}

uae_u32 get_jitted_size(void)
{
    if (compiled_code)
	return current_compile_p-compiled_code;
    return 0;
}

void alloc_cache(void)
{
    if (compiled_code) {
		flush_icache_hard(0, 3);
		cache_free(compiled_code);
    }
#ifdef NATMEM_OFFSET
    if (canbang) {
# ifndef _WIN32
		/* Set up signal handler to catch illegal natmem accesses */
		struct sigaction act;
		act.sa_sigaction = vec;

		sigemptyset (&act.sa_mask);
		act.sa_flags = SA_SIGINFO;
		sigaction (SIGSEGV, &act, NULL);
# endif
	/* Cache for generating illegal natmem access handler. */
#endif
    if (veccode == NULL)
		veccode = cache_alloc (256);
#ifdef NATMEM_OFFSET
	if (!veccode) {
	    canbang = 0;
	    sigaction (SIGSEGV, saved_handler, 0);
	} else
	    write_log ("JIT: Enabled direct memory access.\n");
    }
#endif
    if (popallspace == NULL)
		popallspace = cache_alloc (1024);
    compiled_code = NULL;
    if (currprefs.cachesize == 0)
		return;

    while (!compiled_code && currprefs.cachesize) {
		compiled_code=cache_alloc(currprefs.cachesize*1024);
		if (!compiled_code)
		    currprefs.cachesize/=2;
    }
    if (compiled_code) {
		max_compile_start=compiled_code+currprefs.cachesize*1024-BYTES_PER_INST;
		current_compile_p=compiled_code;
    }
}
>>>>>>> p-uae/v2.1.0

static void calc_checksum(blockinfo* bi, uae_u32* c1, uae_u32* c2)
{
    uae_u32 k1=0;
    uae_u32 k2=0;
    uae_s32 len=bi->len;
<<<<<<< HEAD
    uae_uintptr tmp=bi->min_pcp;
=======
    uae_u32 tmp=bi->min_pcp;
>>>>>>> p-uae/v2.1.0
    uae_u32* pos;

    len+=(tmp&3);
    tmp&=(~3);
    pos=(uae_u32*)tmp;

    if (len<0 || len>MAX_CHECKSUM_LEN) {
<<<<<<< HEAD
	*c1=0;
	*c2=0;
    }
    else {
	while (len>0) {
	    k1+=*pos;
	    k2^=*pos;
	    pos++;
	    len-=4;
	}
	*c1=k1;
	*c2=k2;
    }
}

=======
		*c1=0;
		*c2=0;
    }
    else {
		while (len>0) {
		    k1+=*pos;
		    k2^=*pos;
		    pos++;
		    len-=4;
		}
		*c1=k1;
		*c2=k2;
    }
}

static void show_checksum(blockinfo* bi)
{
    uae_u32 k1=0;
    uae_u32 k2=0;
    uae_s32 len=bi->len;
    uae_u32 tmp=(uae_u32)bi->pc_p;
    uae_u32* pos;

    len+=(tmp&3);
    tmp&=(~3);
    pos=(uae_u32*)tmp;

    if (len<0 || len>MAX_CHECKSUM_LEN) {
		return;
    }
    else {
		while (len>0) {
			write_log ("%08x ",*pos);
		    pos++;
		    len-=4;
		}
		write_log (" bla\n");
    }
}


>>>>>>> p-uae/v2.1.0
int check_for_cache_miss(void)
{
    blockinfo* bi=get_blockinfo_addr(regs.pc_p);

    if (bi) {
<<<<<<< HEAD
	int cl=cacheline(regs.pc_p);
	if (bi!=cache_tags[cl+1].bi) {
	    raise_in_cl_list(bi);
	    return 1;
	}
=======
		int cl=cacheline(regs.pc_p);
		if (bi!=cache_tags[cl+1].bi) {
		    raise_in_cl_list(bi);
		    return 1;
		}
>>>>>>> p-uae/v2.1.0
    }
    return 0;
}


static void recompile_block(void)
{
    /* An existing block's countdown code has expired. We need to make
       sure that execute_normal doesn't refuse to recompile due to a
       perceived cache miss... */
    blockinfo*  bi=get_blockinfo_addr(regs.pc_p);

    Dif (!bi)
<<<<<<< HEAD
	abort();
=======
		jit_abort ("recompile_block");
>>>>>>> p-uae/v2.1.0
    raise_in_cl_list(bi);
    execute_normal();
    return;
}

static void cache_miss(void)
{
    blockinfo*  bi=get_blockinfo_addr(regs.pc_p);
    uae_u32     cl=cacheline(regs.pc_p);
    blockinfo*  bi2=get_blockinfo(cl);

    if (!bi) {
<<<<<<< HEAD
	execute_normal(); /* Compile this block now */
	return;
    }
    Dif (!bi2 || bi==bi2) {
	write_log ("JIT: Unexplained cache miss %p %p\n", bi, bi2);
	abort();
=======
		execute_normal(); /* Compile this block now */
		return;
    }
    Dif (!bi2 || bi==bi2) {
		jit_abort ("Unexplained cache miss %p %p\n",bi,bi2);
>>>>>>> p-uae/v2.1.0
    }
    raise_in_cl_list(bi);
    return;
}

static void check_checksum(void)
{
    blockinfo*  bi=get_blockinfo_addr(regs.pc_p);
    uae_u32     cl=cacheline(regs.pc_p);
    blockinfo*  bi2=get_blockinfo(cl);

    uae_u32     c1,c2;

    checksum_count++;
    /* These are not the droids you are looking for...  */
    if (!bi) {
<<<<<<< HEAD
	/* Whoever is the primary target is in a dormant state, but
	   calling it was accidental, and we should just compile this
	   new block */
	execute_normal();
	return;
    }
    if (bi!=bi2) {
	/* The block was hit accidentally, but it does exist. Cache miss */
	cache_miss();
	return;
    }

    if (bi->c1 || bi->c2)
	calc_checksum(bi,&c1,&c2);
    else {
	c1=c2=1;  /* Make sure it doesn't match */
    }
    if (c1==bi->c1 && c2==bi->c2) {
	/* This block is still OK. So we reactivate. Of course, that
	   means we have to move it into the needs-to-be-flushed list */
	bi->handler_to_use=bi->handler;
	set_dhtu(bi,bi->direct_handler);

	/*	printf("reactivate %p/%p (%x %x/%x %x)\n",bi,bi->pc_p,
		c1,c2,bi->c1,bi->c2);*/
	remove_from_list(bi);
	add_to_active(bi);
	raise_in_cl_list(bi);
    }
    else {
	/* This block actually changed. We need to invalidate it,
	   and set it up to be recompiled */
	/* printf("discard %p/%p (%x %x/%x %x)\n",bi,bi->pc_p,
	   c1,c2,bi->c1,bi->c2); */
	invalidate_block(bi);
	raise_in_cl_list(bi);
	execute_normal();
    }
}

static void free_popalls (void)
{
    if (popallspace) {
	cache_free (popallspace);
	popallspace = 0;
    }
    popall_exec_nostats    = 0;
    popall_execute_normal  = 0;
    popall_cache_miss      = 0;
    popall_recompile_block = 0;
    popall_do_nothing      = 0;
    popall_check_checksum  = 0;
    pushall_call_handler   = 0;
}

static void create_popalls (void)
{
    popallspace = cache_alloc (1024);

    if (popallspace) {
=======
		/* Whoever is the primary target is in a dormant state, but
		calling it was accidental, and we should just compile this
		new block */
		execute_normal();
		return;
    }
    if (bi!=bi2) {
		/* The block was hit accidentally, but it does exist. Cache miss */
		cache_miss();
		return;
    }

    if (bi->c1 || bi->c2)
		calc_checksum(bi,&c1,&c2);
    else {
		c1=c2=1;  /* Make sure it doesn't match */
    }
    if (c1==bi->c1 && c2==bi->c2) {
		/* This block is still OK. So we reactivate. Of course, that
		means we have to move it into the needs-to-be-flushed list */
		bi->handler_to_use=bi->handler;
		set_dhtu(bi,bi->direct_handler);

		/*	write_log ("JIT: reactivate %p/%p (%x %x/%x %x)\n",bi,bi->pc_p,
		c1,c2,bi->c1,bi->c2);*/
		remove_from_list(bi);
		add_to_active(bi);
		raise_in_cl_list(bi);
    }
    else {
		/* This block actually changed. We need to invalidate it,
	   and set it up to be recompiled */
		/* write_log ("JIT: discard %p/%p (%x %x/%x %x)\n",bi,bi->pc_p,
	   c1,c2,bi->c1,bi->c2); */
		invalidate_block(bi);
		raise_in_cl_list(bi);
		execute_normal();
    }
}


STATIC_INLINE void create_popalls(void)
{
>>>>>>> p-uae/v2.1.0
	int i, r;

	current_compile_p = popallspace;
	set_target (current_compile_p);
<<<<<<< HEAD

#if USE_PUSH_POP
	/* If we can't use gcc inline assembly, we need to pop some
	 * registers before jumping back to the various get-out routines.
	 * This generates the code for it.
	 */
	popall_do_nothing = current_compile_p;
	for (i = 0; i < N_REGS; i++) {
	    if (need_to_preserve[i])
		raw_pop_l_r (i);
	}
	raw_jmp ((uae_uintptr) do_nothing);
	align_target (align_jumps);
=======
#if USE_PUSH_POP
	/* If we can't use gcc inline assembly, we need to pop some
    registers before jumping back to the various get-out routines.
    This generates the code for it.
	*/
	popall_do_nothing = current_compile_p;
	for (i = 0; i < N_REGS; i++) {
	    if (need_to_preserve[i])
			raw_pop_l_r (i);
	}
  	raw_jmp((uae_u32)do_nothing);
  	align_target(32);
>>>>>>> p-uae/v2.1.0

	popall_execute_normal = get_target ();
	for (i = 0; i < N_REGS; i++) {
	    if (need_to_preserve[i])
<<<<<<< HEAD
		raw_pop_l_r (i);
	}
	raw_jmp ((uae_uintptr) execute_normal);
	align_target (align_jumps);
=======
			raw_pop_l_r (i);
	}
  	raw_jmp((uae_u32)execute_normal);
  	align_target(32);
>>>>>>> p-uae/v2.1.0

	popall_cache_miss = get_target ();
	for (i = 0; i < N_REGS; i++) {
	    if (need_to_preserve[i])
		raw_pop_l_r (i);
	}
<<<<<<< HEAD
	raw_jmp ((uae_uintptr) cache_miss);
	align_target (align_jumps);
=======
  raw_jmp((uae_u32)cache_miss);
  align_target(32);
>>>>>>> p-uae/v2.1.0

	popall_recompile_block = get_target ();
	for (i = 0; i < N_REGS; i++) {
	    if (need_to_preserve[i])
		raw_pop_l_r (i);
	}
<<<<<<< HEAD
	raw_jmp ((uae_uintptr) recompile_block);
	align_target (align_jumps);
=======
  raw_jmp((uae_u32)recompile_block);
  align_target(32);
>>>>>>> p-uae/v2.1.0

	popall_exec_nostats = get_target ();
	for (i = 0; i < N_REGS; i++) {
	    if (need_to_preserve[i])
		raw_pop_l_r (i);
	}
<<<<<<< HEAD
	raw_jmp ((uae_uintptr) exec_nostats);
	align_target (align_jumps);
=======
  raw_jmp((uae_u32)exec_nostats);
  align_target(32);
>>>>>>> p-uae/v2.1.0

	popall_check_checksum = get_target ();
	for (i = 0; i < N_REGS; i++) {
	    if (need_to_preserve[i])
		raw_pop_l_r (i);
	}
<<<<<<< HEAD
	raw_jmp ((uae_uintptr) check_checksum);
	align_target (align_jumps);
=======
  raw_jmp((uae_u32)check_checksum);
  align_target(32);
>>>>>>> p-uae/v2.1.0

	current_compile_p=get_target();
#else
	popall_exec_nostats    = exec_nostats;
	popall_execute_normal  = execute_normal;
	popall_cache_miss      = cache_miss;
	popall_recompile_block = recompile_block;
	popall_do_nothing      = do_nothing;
	popall_check_checksum  = check_checksum;
#endif

	/* And now, the code to do the matching pushes and then jump
<<<<<<< HEAD
	 * into a handler routine.
	 */
=======
     into a handler routine */
>>>>>>> p-uae/v2.1.0
	pushall_call_handler = get_target ();
#if USE_PUSH_POP
	for (i = N_REGS; i--;) {
	    if (need_to_preserve[i])
		raw_push_l_r (i);
	}
#endif
	r = REG_PC_TMP;
<<<<<<< HEAD
	raw_mov_l_rm (r, (uae_uintptr) &regs.pc_p);
	raw_and_l_ri (r, TAGMASK);
	raw_jmp_m_indexed ((uae_uintptr) cache_tags, r, 4);
    }
=======
  raw_mov_l_rm(r,(uae_u32)&regs.pc_p);
	raw_and_l_ri (r, TAGMASK);
  raw_jmp_m_indexed((uae_u32)cache_tags,r,4);
>>>>>>> p-uae/v2.1.0
}

STATIC_INLINE void reset_lists (void)
{
    int i;

    for (i = 0; i < MAX_HOLD_BI; i++)
	hold_bi[i] = NULL;
    active  = NULL;
    dormant = NULL;
}

static void prepare_block(blockinfo* bi)
{
    int i;

    set_target(current_compile_p);
<<<<<<< HEAD
    align_target (align_jumps);
    bi->direct_pen=(void*)get_target();
    raw_mov_l_rm(0,(uae_uintptr)&(bi->pc_p));
    raw_mov_l_mr((uae_uintptr)&regs.pc_p, 0);
    raw_jmp((uae_uintptr)popall_execute_normal);

    align_target (align_jumps);
    bi->direct_pcc=(void*)get_target();
    raw_mov_l_rm(0,(uae_uintptr)&(bi->pc_p));
    raw_mov_l_mr((uae_uintptr)&regs.pc_p, 0);
    raw_jmp((uae_uintptr)popall_check_checksum);

=======
    align_target(32);
    bi->direct_pen=(cpuop_func*)get_target();
    raw_mov_l_rm(0,(uae_u32)&(bi->pc_p));
    raw_mov_l_mr((uae_u32)&regs.pc_p,0);
    raw_jmp((uae_u32)popall_execute_normal);

    align_target(32);
    bi->direct_pcc=(cpuop_func*)get_target();
    raw_mov_l_rm(0,(uae_u32)&(bi->pc_p));
    raw_mov_l_mr((uae_u32)&regs.pc_p,0);
    raw_jmp((uae_u32)popall_check_checksum);

    align_target(32);
>>>>>>> p-uae/v2.1.0
    current_compile_p=get_target();

    bi->deplist=NULL;
    for (i=0;i<2;i++) {
	bi->dep[i].prev_p=NULL;
	bi->dep[i].next=NULL;
    }
    bi->env=default_ss;
    bi->status=BI_NEW;
    bi->havestate=0;
    //bi->env=empty_ss;
}

void compemu_reset(void)
{
    set_cache_state(0);
}

void build_comp (void)
{
    int i;
<<<<<<< HEAD
=======
    int jumpcount=0;
>>>>>>> p-uae/v2.1.0
    unsigned long opcode;
    const struct comptbl *tbl   = op_smalltbl_0_comp_ff;
    const struct comptbl *nftbl = op_smalltbl_0_comp_nf;
    int count;
#ifdef NOFLAGS_SUPPORT
<<<<<<< HEAD
    struct cputbl *nfctbl = (currprefs.cpu_level >= 4 ? op_smalltbl_0_nf
			   : currprefs.cpu_level == 3 ? op_smalltbl_1_nf
			   : currprefs.cpu_level == 2 ? op_smalltbl_2_nf
			   : currprefs.cpu_level == 1 ? op_smalltbl_3_nf
# ifdef CPUEMU_5
			   : currprefs.cpu_compatible ? op_smalltbl_5_nf
# endif
			   : op_smalltbl_4_nf);
#endif
    raw_init_cpu ();

    write_log ("JIT: Building compiler function table.\n");
=======
    struct comptbl *nfctbl = (currprefs.cpu_level >= 5 ? op_smalltbl_0_nf
			     : currprefs.cpu_level == 4 ? op_smalltbl_1_nf
			     : (currprefs.cpu_level == 2 || currprefs.cpu_level == 3) ? op_smalltbl_2_nf
			     : currprefs.cpu_level == 1 ? op_smalltbl_3_nf
			     : ! currprefs.cpu_compatible ? op_smalltbl_4_nf
			     : op_smalltbl_5_nf);
#endif
    raw_init_cpu ();
#ifdef NATMEM_OFFSET
    write_log ("JIT: Setting signal handler\n");
#ifndef _WIN32
    signal(SIGSEGV,vec);
#endif
#endif
    write_log ("JIT: Building Compiler function table\n");
>>>>>>> p-uae/v2.1.0
    for (opcode = 0; opcode < 65536; opcode++) {
#ifdef NOFLAGS_SUPPORT
	nfcpufunctbl[opcode] = op_illg;
#endif
	compfunctbl[opcode] = NULL;
	nfcompfunctbl[opcode] = NULL;
	prop[opcode].use_flags = 0x1f;
	prop[opcode].set_flags = 0x1f;
	prop[opcode].is_jump=1;
    }

    for (i = 0; tbl[i].opcode < 65536; i++) {
<<<<<<< HEAD
	int isjmp=(tbl[i].specific&1);
	int isaddx=(tbl[i].specific&8);
	int iscjmp=(tbl[i].specific&16);

	prop[tbl[i].opcode].is_jump=isjmp;
	prop[tbl[i].opcode].is_const_jump=iscjmp;
	prop[tbl[i].opcode].is_addx=isaddx;
	compfunctbl[tbl[i].opcode] = tbl[i].handler;
    }
    for (i = 0; nftbl[i].opcode < 65536; i++) {
	nfcompfunctbl[nftbl[i].opcode] = nftbl[i].handler;
#ifdef NOFLAGS_SUPPORT
	nfcpufunctbl[nftbl[i].opcode] = nfctbl[i].handler;
=======
		int isjmp=(tbl[i].specific&1);
		int isaddx=(tbl[i].specific&8);
		int iscjmp=(tbl[i].specific&16);

		prop[tbl[i].opcode].is_jump=isjmp;
		prop[tbl[i].opcode].is_const_jump=iscjmp;
		prop[tbl[i].opcode].is_addx=isaddx;
		compfunctbl[tbl[i].opcode] = tbl[i].handler;
    }
    for (i = 0; nftbl[i].opcode < 65536; i++) {
		nfcompfunctbl[nftbl[i].opcode] = nftbl[i].handler;
#ifdef NOFLAGS_SUPPORT
		nfcpufunctbl[nftbl[i].opcode] = nfctbl[i].handler;
>>>>>>> p-uae/v2.1.0
#endif
    }

#ifdef NOFLAGS_SUPPORT
    for (i = 0; nfctbl[i].handler; i++) {
<<<<<<< HEAD
	nfcpufunctbl[nfctbl[i].opcode] = nfctbl[i].handler;
=======
		nfcpufunctbl[nfctbl[i].opcode] = nfctbl[i].handler;
>>>>>>> p-uae/v2.1.0
    }
#endif

    for (opcode = 0; opcode < 65536; opcode++) {
<<<<<<< HEAD
	compop_func *f;
	compop_func *nff;
#ifdef NOFLAGS_SUPPORT
	cpuop_func *nfcf;
#endif
	int isjmp,isaddx,iscjmp;

	if (table68k[opcode].mnemo == i_ILLG || table68k[opcode].clev > currprefs.cpu_level)
	    continue;

	if (table68k[opcode].handler != -1) {
	    f = compfunctbl[table68k[opcode].handler];
	    nff = nfcompfunctbl[table68k[opcode].handler];
#ifdef NOFLAGS_SUPPORT
	    nfcf = nfcpufunctbl[table68k[opcode].handler];
#endif
	    isjmp=prop[table68k[opcode].handler].is_jump;
	    iscjmp=prop[table68k[opcode].handler].is_const_jump;
	    isaddx=prop[table68k[opcode].handler].is_addx;
	    prop[opcode].is_jump=isjmp;
	    prop[opcode].is_const_jump=iscjmp;
	    prop[opcode].is_addx=isaddx;
	    compfunctbl[opcode] = f;
	    nfcompfunctbl[opcode] = nff;
#ifdef NOFLAGS_SUPPORT
	    Dif (nfcf == op_illg)
		abort();
	    nfcpufunctbl[opcode] = nfcf;
#endif
	}
	prop[opcode].set_flags =table68k[opcode].flagdead;
	prop[opcode].use_flags =table68k[opcode].flaglive;
	/* Unconditional jumps don't evaluate condition codes, so they
	   don't actually use any flags themselves */
	if (prop[opcode].is_const_jump)
	    prop[opcode].use_flags=0;
    }
#ifdef NOFLAGS_SUPPORT
    for (i = 0; nfctbl[i].handler != NULL; i++) {
	if (nfctbl[i].specific)
	    nfcpufunctbl[tbl[i].opcode] = nfctbl[i].handler;
=======
		compop_func *f;
		compop_func *nff;
#ifdef NOFLAGS_SUPPORT
		compop_func *nfcf;
#endif
		int isjmp,isaddx,iscjmp;
		int lvl;

		lvl = (currprefs.cpu_model - 68000) / 10;
		if (lvl > 4)
		    lvl--;
		if (table68k[opcode].mnemo == i_ILLG || table68k[opcode].clev > lvl)
		    continue;

		if (table68k[opcode].handler != -1) {
		    f = compfunctbl[table68k[opcode].handler];
		    nff = nfcompfunctbl[table68k[opcode].handler];
#ifdef NOFLAGS_SUPPORT
		    nfcf = nfcpufunctbl[table68k[opcode].handler];
#endif
		    isjmp=prop[table68k[opcode].handler].is_jump;
		    iscjmp=prop[table68k[opcode].handler].is_const_jump;
		    isaddx=prop[table68k[opcode].handler].is_addx;
		    prop[opcode].is_jump=isjmp;
		    prop[opcode].is_const_jump=iscjmp;
		    prop[opcode].is_addx=isaddx;
		    compfunctbl[opcode] = f;
		    nfcompfunctbl[opcode] = nff;
#ifdef NOFLAGS_SUPPORT
		    Dif (nfcf == op_illg)
				abort();
		    nfcpufunctbl[opcode] = nfcf;
#endif
		}
		prop[opcode].set_flags =table68k[opcode].flagdead;
		prop[opcode].use_flags =table68k[opcode].flaglive;
		/* Unconditional jumps don't evaluate condition codes, so they
		don't actually use any flags themselves */
		if (prop[opcode].is_const_jump)
		    prop[opcode].use_flags=0;
    }
#ifdef NOFLAGS_SUPPORT
    for (i = 0; nfctbl[i].handler != NULL; i++) {
		if (nfctbl[i].specific)
		    nfcpufunctbl[tbl[i].opcode] = nfctbl[i].handler;
>>>>>>> p-uae/v2.1.0
    }
#endif

    count=0;
    for (opcode = 0; opcode < 65536; opcode++) {
<<<<<<< HEAD
	if (compfunctbl[opcode])
	    count++;
    }
    write_log ("JIT: Supposedly %d compileable opcodes.\n", count);

#ifdef NATMEM_OFFSET
    if (canbang) {
# ifndef _WIN32
	/* Set up signal handler to catch illegal natmem accesses */
	struct sigaction act;
	act.sa_sigaction = vec;

	sigemptyset (&act.sa_mask);
	act.sa_flags = SA_SIGINFO;
	sigaction (SIGSEGV, &act, NULL);
# endif
	/* Cache for generating illegal natmem access handler. */
	veccode = cache_alloc (256);

	if (!veccode) {
	    canbang = 0;
	    sigaction (SIGSEGV, saved_handler, 0);
	} else
	    write_log ("JIT: Enabled direct memory access.\n");
    }
#endif
=======
		if (compfunctbl[opcode])
		    count++;
    }
	write_log ("JIT: Supposedly %d compileable opcodes!\n",count);
>>>>>>> p-uae/v2.1.0

    /* Initialise state */
    alloc_cache ();
    create_popalls ();
    reset_lists ();

    for (i=0;i<TAGSIZE;i+=2) {
<<<<<<< HEAD
	cache_tags[i].handler=(void*)popall_execute_normal;
	cache_tags[i+1].bi=NULL;
=======
		cache_tags[i].handler=(cpuop_func*)popall_execute_normal;
		cache_tags[i+1].bi=NULL;
>>>>>>> p-uae/v2.1.0
    }
    compemu_reset ();

    for (i=0;i<N_REGS;i++) {
<<<<<<< HEAD
	empty_ss.nat[i].holds=-1;
	empty_ss.nat[i].validsize=0;
	empty_ss.nat[i].dirtysize=0;
    }
    default_ss=empty_ss;
}

static void flush_icache_hard(int n)
{
    uae_u32 i;
=======
		empty_ss.nat[i].holds=-1;
		empty_ss.nat[i].validsize=0;
		empty_ss.nat[i].dirtysize=0;
    }
    default_ss=empty_ss;
#if 0
    default_ss.nat[6].holds=11;
    default_ss.nat[6].validsize=4;
    default_ss.nat[5].holds=12;
    default_ss.nat[5].validsize=4;
#endif
}


static void flush_icache_hard(uae_u32 ptr, int n)
{
>>>>>>> p-uae/v2.1.0
    blockinfo* bi;

    hard_flush_count++;
#if 0
<<<<<<< HEAD
    printf("Flush Icache_hard(%d/%x/%p), %u instruction bytes\n",
=======
	write_log ("JIT: Flush Icache_hard(%d/%x/%p), %u instruction bytes\n",
>>>>>>> p-uae/v2.1.0
	   n,regs.pc,regs.pc_p,current_compile_p-compiled_code);
#endif
    bi=active;
    while(bi) {
<<<<<<< HEAD
	cache_tags[cacheline(bi->pc_p)].handler=(void*)popall_execute_normal;
	cache_tags[cacheline(bi->pc_p)+1].bi=NULL;
	bi=bi->next;
    }
    bi=dormant;
    while(bi) {
	cache_tags[cacheline(bi->pc_p)].handler=(void*)popall_execute_normal;
	cache_tags[cacheline(bi->pc_p)+1].bi=NULL;
	bi=bi->next;
=======
		cache_tags[cacheline(bi->pc_p)].handler=(cpuop_func*)popall_execute_normal;
		cache_tags[cacheline(bi->pc_p)+1].bi=NULL;
		bi=bi->next;
    }
    bi=dormant;
    while(bi) {
		cache_tags[cacheline(bi->pc_p)].handler=(cpuop_func*)popall_execute_normal;
		cache_tags[cacheline(bi->pc_p)+1].bi=NULL;
		bi=bi->next;
>>>>>>> p-uae/v2.1.0
    }

    reset_lists();
    if (!compiled_code)
<<<<<<< HEAD
	return;
    current_compile_p=compiled_code;
    set_special (&regs,0); /* To get out of compiled code */
=======
		return;
    current_compile_p=compiled_code;
    set_special (0); /* To get out of compiled code */
>>>>>>> p-uae/v2.1.0
}


/* "Soft flushing" --- instead of actually throwing everything away,
   we simply mark everything as "needs to be checked".
*/

<<<<<<< HEAD
void flush_icache(int n)
{
    uae_u32 i;
=======
void flush_icache (uaecptr ptr, int n)
{
>>>>>>> p-uae/v2.1.0
    blockinfo* bi;
    blockinfo* bi2;

    if (currprefs.comp_hardflush) {
<<<<<<< HEAD
	flush_icache_hard(n);
	return;
    }
    soft_flush_count++;
    if (!active)
	return;

    bi=active;
    while (bi) {
	uae_u32 cl=cacheline(bi->pc_p);
	if (!bi->handler) {
	    /* invalidated block */
	    if (bi==cache_tags[cl+1].bi)
		cache_tags[cl].handler=popall_execute_normal;
	    bi->handler_to_use=popall_execute_normal;
	    set_dhtu(bi,bi->direct_pen);
	}
	else {
	    if (bi==cache_tags[cl+1].bi)
		cache_tags[cl].handler=popall_check_checksum;
	    bi->handler_to_use=popall_check_checksum;
	    set_dhtu(bi,bi->direct_pcc);
	}
	bi2=bi;
	bi=bi->next;
=======
		flush_icache_hard(ptr, n);
		return;
    }
    soft_flush_count++;
    if (!active)
		return;

    bi=active;
    while (bi) {
		uae_u32 cl=cacheline(bi->pc_p);
		if (!bi->handler) {
		    /* invalidated block */
		    if (bi==cache_tags[cl+1].bi)
				cache_tags[cl].handler=(cpuop_func*)popall_execute_normal;
		    bi->handler_to_use=(cpuop_func*)popall_execute_normal;
		    set_dhtu(bi,bi->direct_pen);
		} else {
		    if (bi==cache_tags[cl+1].bi)
				cache_tags[cl].handler=(cpuop_func*)popall_check_checksum;
		    bi->handler_to_use=(cpuop_func*)popall_check_checksum;
		    set_dhtu(bi,bi->direct_pcc);
		}
		bi2=bi;
		bi=bi->next;
>>>>>>> p-uae/v2.1.0
    }
    /* bi2 is now the last entry in the active list */
    bi2->next=dormant;
    if (dormant)
<<<<<<< HEAD
	dormant->prev_p=&(bi2->next);
=======
		dormant->prev_p=&(bi2->next);
>>>>>>> p-uae/v2.1.0

    dormant=active;
    active->prev_p=&dormant;
    active=NULL;
}

<<<<<<< HEAD
int failure;

void compile_block (const cpu_history *pc_hist, int blocklen, int totcycles)
{
    if (letit && compiled_code && currprefs.cpu_level>=2) {

	/* OK, here we need to 'compile' a block */
	int i;
	int r;
	int was_comp=0;
	uae_u8 liveflags[MAXRUN+1];

	uae_uintptr max_pcp=(uae_uintptr)pc_hist[0].location;
	uae_uintptr min_pcp=max_pcp;
	uae_u32 cl=cacheline(pc_hist[0].location);
	void* specflags=(void*)&regs.spcflags;
	blockinfo* bi=NULL;
	blockinfo* bi2;
	int extra_len=0;

	compile_count++;
	if (current_compile_p>=max_compile_start)
	    flush_icache_hard(7);

	alloc_blockinfos();

	bi=get_blockinfo_addr_new(pc_hist[0].location,0);
	bi2=get_blockinfo(cl);

	optlev=bi->optlevel;
	if (bi->handler) {
	    Dif (bi!=bi2) {
		/* I don't think it can happen anymore. Shouldn't, in
		   any case. So let's make sure... */
		write_log ("WOOOWOO count=%d, ol=%d %p %p\n",
			   bi->count, bi->optlevel, bi->handler_to_use,
			   cache_tags[cl].handler);
		abort();
	    }

	    Dif (bi->count!=-1 && bi->status!=BI_TARGETTED) {
		/* What the heck? We are not supposed to be here! */
		abort();
	    }
	}
	if (bi->count==-1) {
	    optlev++;
	    while (!currprefs.optcount[optlev])
		optlev++;
	    bi->count=currprefs.optcount[optlev]-1;
	}
	current_block_pc_p=(uae_uintptr)pc_hist[0].location;

	remove_deps(bi); /* We are about to create new code */
	bi->optlevel=optlev;
	bi->pc_p=(uae_u8*)pc_hist[0].location;

	liveflags[blocklen]=0x1f; /* All flags needed afterwards */
	i=blocklen;
	while (i--) {
	    uae_u16* currpcp=pc_hist[i].location;
	    int op=cft_map(*currpcp);

	    if ((uae_uintptr) currpcp < min_pcp)
		min_pcp = (uae_uintptr) currpcp;
	    if ((uae_uintptr) currpcp > max_pcp)
		max_pcp = (uae_uintptr) currpcp;

	    if (currprefs.compnf) {
		liveflags[i]=((liveflags[i+1]&
			       (~prop[op].set_flags))|
			      prop[op].use_flags);
		if (prop[op].is_addx && (liveflags[i+1]&FLAG_Z)==0)
		    liveflags[i]&= ~FLAG_Z;
	    }
	    else {
		liveflags[i]=0x1f;
	    }
	}

	bi->needed_flags=liveflags[0];

#if 1
	/* This is the non-direct handler */
	align_target(32);
	set_target(get_target()+1);
	align_target(16);
	/* Now aligned at n*32+16 */
#else
	align_target (align_loops);
#endif

	bi->handler=
	    bi->handler_to_use=(void*)get_target();
	raw_cmp_l_mi((uae_uintptr)&regs.pc_p,(uae_uintptr)pc_hist[0].location);
	raw_jnz((uae_uintptr)popall_cache_miss);
	/* This was 16 bytes on the x86, so now aligned on (n+1)*32 */

	was_comp=0;
=======

static void catastrophe(void)
{
	jit_abort ("catastprophe");
}

int failure;


void compile_block(cpu_history* pc_hist, int blocklen, int totcycles)
{
    if (letit && compiled_code && currprefs.cpu_model>=68020) {

		/* OK, here we need to 'compile' a block */
		int i;
		int r;
		int was_comp=0;
		uae_u8 liveflags[MAXRUN+1];
		uae_u32 max_pcp=(uae_u32)pc_hist[0].location;
		uae_u32 min_pcp=max_pcp;
		uae_u32 cl=cacheline(pc_hist[0].location);
		void* specflags=(void*)&regs.spcflags;
		blockinfo* bi=NULL;
		blockinfo* bi2;
		int extra_len=0;

		compile_count++;
		if (current_compile_p>=max_compile_start)
			flush_icache_hard(0, 3);

		alloc_blockinfos();

		bi=get_blockinfo_addr_new(pc_hist[0].location,0);
		bi2=get_blockinfo(cl);

		optlev=bi->optlevel;
		if (bi->handler) {
			Dif (bi!=bi2) {
				/* I don't think it can happen anymore. Shouldn't, in
				any case. So let's make sure... */
				jit_abort ("JIT: WOOOWOO count=%d, ol=%d %p %p\n",
					bi->count, bi->optlevel, bi->handler_to_use,
					cache_tags[cl].handler);
			}

	    Dif (bi->count!=-1 && bi->status!=BI_TARGETTED) {
		/* What the heck? We are not supposed to be here! */
		jit_abort ("BI_TARGETTED");
	    }
		}
		if (bi->count==-1) {
		    optlev++;
		    while (!currprefs.optcount[optlev])
				optlev++;
		    bi->count=currprefs.optcount[optlev]-1;
		}
		current_block_pc_p=(uae_u32)pc_hist[0].location;

		remove_deps(bi); /* We are about to create new code */
		bi->optlevel=optlev;
		bi->pc_p=(uae_u8*)pc_hist[0].location;

		liveflags[blocklen]=0x1f; /* All flags needed afterwards */
		i=blocklen;
		while (i--) {
		    uae_u16* currpcp=pc_hist[i].location;
		    int op=cft_map(*currpcp);

		    if ((uae_u32)currpcp<min_pcp)
				min_pcp=(uae_u32)currpcp;
		    if ((uae_u32)currpcp>max_pcp)
				max_pcp=(uae_u32)currpcp;

		    if (currprefs.compnf) {
				liveflags[i]=((liveflags[i+1]&
			       (~prop[op].set_flags))|
			      prop[op].use_flags);
			if (prop[op].is_addx && (liveflags[i+1]&FLAG_Z)==0)
			    liveflags[i]&= ~FLAG_Z;
		    }
		    else {
			liveflags[i]=0x1f;
		    }
		}

		bi->needed_flags=liveflags[0];

		/* This is the non-direct handler */
		align_target(32);
		set_target(get_target()+1);
		align_target(16);
		/* Now aligned at n*32+16 */

		bi->handler=
		    bi->handler_to_use=(cpuop_func*)get_target();
		raw_cmp_l_mi((uae_u32)&regs.pc_p,(uae_u32)pc_hist[0].location);
		raw_jnz((uae_u32)popall_cache_miss);
		/* This was 16 bytes on the x86, so now aligned on (n+1)*32 */

		was_comp=0;
>>>>>>> p-uae/v2.1.0

#if USE_MATCHSTATE
	comp_pc_p=(uae_u8*)pc_hist[0].location;
	init_comp();
	match_states(&(bi->env));
	was_comp=1;
#endif

<<<<<<< HEAD
	bi->direct_handler=(void*)get_target();
	set_dhtu(bi,bi->direct_handler);
	current_block_start_target=(uae_uintptr)get_target();

	if (bi->count>=0) { /* Need to generate countdown code */
	    raw_mov_l_mi((uae_uintptr)&regs.pc_p,(uae_uintptr)pc_hist[0].location);
	    raw_sub_l_mi((uae_uintptr)&(bi->count),1);
	    raw_jl((uae_uintptr)popall_recompile_block);
	}
	if (optlev==0) { /* No need to actually translate */
	    /* Execute normally without keeping stats */
	    raw_mov_l_mi((uae_uintptr)&regs.pc_p,(uae_uintptr)pc_hist[0].location);
	    raw_jmp((uae_uintptr)popall_exec_nostats);
=======
	bi->direct_handler=(cpuop_func*)get_target();
	set_dhtu(bi,bi->direct_handler);
	current_block_start_target=(uae_u32)get_target();

	if (bi->count>=0) { /* Need to generate countdown code */
	    raw_mov_l_mi((uae_u32)&regs.pc_p,(uae_u32)pc_hist[0].location);
	    raw_sub_l_mi((uae_u32)&(bi->count),1);
	    raw_jl((uae_u32)popall_recompile_block);
	}
	if (optlev==0) { /* No need to actually translate */
	    /* Execute normally without keeping stats */
	    raw_mov_l_mi((uae_u32)&regs.pc_p,(uae_u32)pc_hist[0].location);
	    raw_jmp((uae_u32)popall_exec_nostats);
>>>>>>> p-uae/v2.1.0
	}
	else {
	    reg_alloc_run=0;
	    next_pc_p=0;
	    taken_pc_p=0;
	    branch_cc=0;

	    log_startblock();
	    for (i=0;i<blocklen &&
		     get_target_noopt()<max_compile_start;i++) {
		cpuop_func **cputbl;
		compop_func **comptbl;
		uae_u16 opcode;

		opcode=cft_map((uae_u16)*pc_hist[i].location);
		special_mem=pc_hist[i].specmem;
		needed_flags=(liveflags[i+1] & prop[opcode].set_flags);
		if (!needed_flags && currprefs.compnf) {
#ifdef NOFLAGS_SUPPORT
		    cputbl=nfcpufunctbl;
#else
		    cputbl=cpufunctbl;
#endif
		    comptbl=nfcompfunctbl;
		}
		else {
		    cputbl=cpufunctbl;
		    comptbl=compfunctbl;
		}

		if (comptbl[opcode] && optlev>1) {
		    failure=0;
		    if (!was_comp) {
			comp_pc_p=(uae_u8*)pc_hist[i].location;
			init_comp();
		    }
		    was_comp++;

		    comptbl[opcode](opcode);
		    freescratch();
		    if (!(liveflags[i+1] & FLAG_CZNV)) {
			/* We can forget about flags */
			dont_care_flags();
		    }
#if INDIVIDUAL_INST
		    flush(1);
		    nop();
		    flush(1);
		    was_comp=0;
#endif
		}
		else
		    failure=1;
		if (failure) {
		    if (was_comp) {
			flush(1);
			was_comp=0;
		    }
		    raw_mov_l_ri(REG_PAR1,(uae_u32)opcode);
<<<<<<< HEAD
		    raw_mov_l_ri(REG_PAR2,(uae_uintptr)&regs);
=======
		    raw_mov_l_ri(REG_PAR2,(uae_u32)&regs);
>>>>>>> p-uae/v2.1.0
#if USE_NORMAL_CALLING_CONVENTION
		    raw_push_l_r(REG_PAR2);
		    raw_push_l_r(REG_PAR1);
#endif
<<<<<<< HEAD
		    raw_mov_l_mi((uae_uintptr)&regs.pc_p,
				 (uae_uintptr)pc_hist[i].location);
		    raw_call((uae_uintptr)cputbl[opcode]);

#if USE_NORMAL_CALLING_CONVENTION
		    raw_inc_sp(8);
#endif
=======
		    raw_mov_l_mi((uae_u32)&regs.pc_p,
				 (uae_u32)pc_hist[i].location);
		    raw_call((uae_u32)cputbl[opcode]);
		    //raw_add_l_mi((uae_u32)&oink,1); // FIXME
#if USE_NORMAL_CALLING_CONVENTION
		    raw_inc_sp(8);
#endif
		    /*if (needed_flags)
			raw_mov_l_mi((uae_u32)&foink3,(uae_u32)opcode+65536);
		    else
			raw_mov_l_mi((uae_u32)&foink3,(uae_u32)opcode);
		     */
>>>>>>> p-uae/v2.1.0

		    if (i<blocklen-1) {
			uae_s8* branchadd;

<<<<<<< HEAD
			raw_mov_l_rm(0,(uae_uintptr)specflags);
			raw_test_l_rr(0,0);
			raw_jz_b_oponly();
			branchadd = (uae_s8 *)get_target();
			emit_byte(0);
			raw_sub_l_mi((uae_uintptr)&countdown,scaled_cycles(totcycles));
			raw_jmp((uae_uintptr)popall_do_nothing);
			*branchadd=(uae_uintptr)get_target()-(uae_uintptr)branchadd-1;
=======
			raw_mov_l_rm(0,(uae_u32)specflags);
			raw_test_l_rr(0,0);
			raw_jz_b_oponly();
			branchadd=(uae_s8*)get_target();
			emit_byte(0);
			raw_sub_l_mi((uae_u32)&countdown,scaled_cycles(totcycles));
			raw_jmp((uae_u32)popall_do_nothing);
			*branchadd=(uae_u32)get_target()-(uae_u32)branchadd-1;
>>>>>>> p-uae/v2.1.0
		    }
		}
	    }
#if 0 /* This isn't completely kosher yet; It really needs to be
	 be integrated into a general inter-block-dependency scheme */
	    if (next_pc_p && taken_pc_p &&
		was_comp && taken_pc_p==current_block_pc_p) {
		blockinfo* bi1=get_blockinfo_addr_new((void*)next_pc_p,0);
		blockinfo* bi2=get_blockinfo_addr_new((void*)taken_pc_p,0);
		uae_u8 x=bi1->needed_flags;

		if (x==0xff || 1) {  /* To be on the safe side */
		    uae_u16* next=(uae_u16*)next_pc_p;
		    uae_u16 op=cft_map(*next);

		    x=0x1f;
		    x&=(~prop[op].set_flags);
		    x|=prop[op].use_flags;
		}

		x|=bi2->needed_flags;
		if (!(x & FLAG_CZNV)) {
		    /* We can forget about flags */
		    dont_care_flags();
		    extra_len+=2; /* The next instruction now is part of this
				     block */
		}

	    }
#endif

	    if (next_pc_p) { /* A branch was registered */
<<<<<<< HEAD
		uae_uintptr t1=next_pc_p;
		uae_uintptr t2=taken_pc_p;
=======
		uae_u32 t1=next_pc_p;
		uae_u32 t2=taken_pc_p;
>>>>>>> p-uae/v2.1.0
		int         cc=branch_cc;

		uae_u32* branchadd;
		uae_u32* tba;
		bigstate tmp;
		blockinfo* tbi;

		if (taken_pc_p<next_pc_p) {
		    /* backward branch. Optimize for the "taken" case ---
		       which means the raw_jcc should fall through when
		       the 68k branch is taken. */
		    t1=taken_pc_p;
		    t2=next_pc_p;
		    cc=branch_cc^1;
		}

#if !USE_MATCHSTATE
		flush_keepflags();
#endif
		tmp=live; /* ouch! This is big... */
		raw_jcc_l_oponly(cc);
		branchadd=(uae_u32*)get_target();
		emit_long(0);
		/* predicted outcome */
		tbi=get_blockinfo_addr_new((void*)t1,1);
		match_states(&(tbi->env));
		//flush(1); /* Can only get here if was_comp==1 */
<<<<<<< HEAD
		raw_sub_l_mi((uae_uintptr)&countdown,scaled_cycles(totcycles));
		raw_jcc_l_oponly(9);
		tba=(uae_u32*)get_target();
		emit_long(get_handler(t1)-((uae_uintptr)tba+4));
		raw_mov_l_mi((uae_uintptr)&regs.pc_p,t1);
		raw_jmp((uae_uintptr)popall_do_nothing);
		create_jmpdep(bi,0,tba,t1);

		align_target (align_jumps);
		/* not-predicted outcome */
		*branchadd=(uae_uintptr)get_target()-((uae_uintptr)branchadd+4);
=======
		raw_sub_l_mi((uae_u32)&countdown,scaled_cycles(totcycles));
		raw_jcc_l_oponly(9);
		tba=(uae_u32*)get_target();
		emit_long(get_handler(t1)-((uae_u32)tba+4));
		raw_mov_l_mi((uae_u32)&regs.pc_p,t1);
		raw_jmp((uae_u32)popall_do_nothing);
		create_jmpdep(bi,0,tba,t1);

		align_target(16);
		/* not-predicted outcome */
		*branchadd=(uae_u32)get_target()-((uae_u32)branchadd+4);
>>>>>>> p-uae/v2.1.0
		live=tmp; /* Ouch again */
		tbi=get_blockinfo_addr_new((void*)t2,1);
		match_states(&(tbi->env));

		//flush(1); /* Can only get here if was_comp==1 */
<<<<<<< HEAD
		raw_sub_l_mi((uae_uintptr)&countdown,scaled_cycles(totcycles));
		raw_jcc_l_oponly(9);
		tba=(uae_u32*)get_target();
		emit_long(get_handler(t2)-((uae_uintptr)tba+4));
		raw_mov_l_mi((uae_uintptr)&regs.pc_p,t2);
		raw_jmp((uae_uintptr)popall_do_nothing);
=======
		raw_sub_l_mi((uae_u32)&countdown,scaled_cycles(totcycles));
		raw_jcc_l_oponly(9);
		tba=(uae_u32*)get_target();
		emit_long(get_handler(t2)-((uae_u32)tba+4));
		raw_mov_l_mi((uae_u32)&regs.pc_p,t2);
		raw_jmp((uae_u32)popall_do_nothing);
>>>>>>> p-uae/v2.1.0
		create_jmpdep(bi,1,tba,t2);
	    }
	    else
	    {
		if (was_comp) {
		    flush(1);
		}

		/* Let's find out where next_handler is... */
		if (was_comp && isinreg(PC_P)) {
		    int r2;

		    r=live.state[PC_P].realreg;

		    if (r==0)
			r2=1;
		    else
			r2=0;

		    raw_and_l_ri(r,TAGMASK);
<<<<<<< HEAD
		    raw_mov_l_ri(r2,(uae_uintptr)popall_do_nothing);
		    raw_sub_l_mi((uae_uintptr)&countdown,scaled_cycles(totcycles));
		    raw_cmov_l_rm_indexed(r2,(uae_uintptr)cache_tags,r,4,9);
=======
		    raw_mov_l_ri(r2,(uae_u32)popall_do_nothing);
		    raw_sub_l_mi((uae_u32)&countdown,scaled_cycles(totcycles));
		    raw_cmov_l_rm_indexed(r2,(uae_u32)cache_tags,r,9);
>>>>>>> p-uae/v2.1.0
		    raw_jmp_r(r2);
		}
		else if (was_comp && isconst(PC_P)) {
		    uae_u32 v=live.state[PC_P].val;
		    uae_u32* tba;
		    blockinfo* tbi;

<<<<<<< HEAD
		    tbi=get_blockinfo_addr_new((void*)(uae_uintptr)v,1);
		    match_states(&(tbi->env));

		    raw_sub_l_mi((uae_uintptr)&countdown,scaled_cycles(totcycles));
		    raw_jcc_l_oponly(9);
		    tba=(uae_u32*)get_target();
		    emit_long(get_handler(v)-((uae_uintptr)tba+4));
		    raw_mov_l_mi((uae_uintptr)&regs.pc_p,v);
		    raw_jmp((uae_uintptr)popall_do_nothing);
=======
		    tbi=get_blockinfo_addr_new((void*)v,1);
		    match_states(&(tbi->env));

		    raw_sub_l_mi((uae_u32)&countdown,scaled_cycles(totcycles));
		    raw_jcc_l_oponly(9);
		    tba=(uae_u32*)get_target();
		    emit_long(get_handler(v)-((uae_u32)tba+4));
		    raw_mov_l_mi((uae_u32)&regs.pc_p,v);
		    raw_jmp((uae_u32)popall_do_nothing);
>>>>>>> p-uae/v2.1.0
		    create_jmpdep(bi,0,tba,v);
		}
		else {
		    int r2;

		    r=REG_PC_TMP;
<<<<<<< HEAD
		    raw_mov_l_rm(r,(uae_uintptr)&regs.pc_p);
=======
		    raw_mov_l_rm(r,(uae_u32)&regs.pc_p);
>>>>>>> p-uae/v2.1.0
		    if (r==0)
			r2=1;
		    else
			r2=0;

		    raw_and_l_ri(r,TAGMASK);
<<<<<<< HEAD
		    raw_mov_l_ri(r2,(uae_uintptr)popall_do_nothing);
		    raw_sub_l_mi((uae_uintptr)&countdown,scaled_cycles(totcycles));
		    raw_cmov_l_rm_indexed(r2,(uae_uintptr)cache_tags,r,4,9);
=======
		    raw_mov_l_ri(r2,(uae_u32)popall_do_nothing);
		    raw_sub_l_mi((uae_u32)&countdown,scaled_cycles(totcycles));
		    raw_cmov_l_rm_indexed(r2,(uae_u32)cache_tags,r,9);
>>>>>>> p-uae/v2.1.0
		    raw_jmp_r(r2);
		}
	    }
	}

	if (next_pc_p+extra_len>=max_pcp &&
	    next_pc_p+extra_len<max_pcp+LONGEST_68K_INST)
	    max_pcp=next_pc_p+extra_len;  /* extra_len covers flags magic */
	else
	    max_pcp+=LONGEST_68K_INST;
	bi->len=max_pcp-min_pcp;
	bi->min_pcp=min_pcp;

	remove_from_list(bi);
	if (isinrom(min_pcp) && isinrom(max_pcp))
	    add_to_dormant(bi); /* No need to checksum it on cache flush.
				   Please don't start changing ROMs in
				   flight! */
	else {
	    calc_checksum(bi,&(bi->c1),&(bi->c2));
	    add_to_active(bi);
	}

	log_dump();
<<<<<<< HEAD
	align_target (align_jumps);
	current_compile_p=get_target();

        raise_in_cl_list(bi);
        bi->nexthandler=current_compile_p;

        /* We will flush soon, anyway, so let's do it now */
        if (current_compile_p>=max_compile_start)
            flush_icache_hard(7);

        do_extra_cycles(totcycles); /* for the compilation time */
    }
}
=======
	align_target(32);
	current_compile_p=get_target();

	raise_in_cl_list(bi);
	bi->nexthandler=current_compile_p;

	/* We will flush soon, anyway, so let's do it now */
	if (current_compile_p>=max_compile_start)
	    flush_icache_hard(0, 3);

	do_extra_cycles(totcycles); /* for the compilation time */
    }
}

#endif
>>>>>>> p-uae/v2.1.0
