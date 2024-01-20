/*
 * UAE - The Un*x Amiga Emulator
 *
 * MC68000 emulation - machine dependent bits
 *
 * Copyright 1996 Bernd Schmidt
 */

struct flag_struct {
    unsigned int cznv;
    unsigned int x;
};

extern struct flag_struct regflags;

#define FLAGBIT_N       15
#define FLAGBIT_Z       14
#define FLAGBIT_C       8
#define FLAGBIT_V       0
#define FLAGBIT_X       8

#define FLAGVAL_N       (1 << FLAGBIT_N)
#define FLAGVAL_Z       (1 << FLAGBIT_Z)
#define FLAGVAL_C       (1 << FLAGBIT_C)
#define FLAGVAL_V       (1 << FLAGBIT_V)
#define FLAGVAL_X       (1 << FLAGBIT_X)

#define SET_ZFLG(flags, y)     ((flags)->cznv = ((flags)->cznv & ~FLAGVAL_Z) | (((flags, y) ? 1 : 0) << FLAGBIT_Z))
#define SET_CFLG(flags, y)     ((flags)->cznv = ((flags)->cznv & ~FLAGVAL_C) | (((flags, y) ? 1 : 0) << FLAGBIT_C))
#define SET_VFLG(flags, y)     ((flags)->cznv = ((flags)->cznv & ~FLAGVAL_V) | (((flags, y) ? 1 : 0) << FLAGBIT_V))
#define SET_NFLG(flags, y)     ((flags)->cznv = ((flags)->cznv & ~FLAGVAL_N) | (((flags, y) ? 1 : 0) << FLAGBIT_N))
#define SET_XFLG(flags, y)     ((flags)->x    = ((flags, y) ? 1 : 0) << FLAGBIT_X)

#define GET_ZFLG(flags)      (((flags)->cznv >> FLAGBIT_Z) & 1)
#define GET_CFLG(flags)      (((flags)->cznv >> FLAGBIT_C) & 1)
#define GET_VFLG(flags)      (((flags)->cznv >> FLAGBIT_V) & 1)
#define GET_NFLG(flags)      (((flags)->cznv >> FLAGBIT_N) & 1)
#define GET_XFLG(flags)      (((flags)->x    >> FLAGBIT_X) & 1)

#define CLEAR_CZNV(flags)    ((flags)->cznv  = 0)
#define GET_CZNV        ((flags)->cznv)
#define IOR_CZNV(X)     ((flags)->cznv |= (X))
#define SET_CZNV(X)     ((flags)->cznv  = (X))

#define COPY_CARRY(flags) ((flags)->x = (flags)->cznv)

static __inline__ int cctrue(int cc)
{
    uae_u32 cznv = (flags)->cznv;

    switch (cc) {
        case 0:  return 1;                                                              /*                              T  */
        case 1:  return 0;                                                              /*                              F  */
        case 2:  return (cznv & (FLAGVAL_C | FLAGVAL_Z)) == 0;                          /* !CFLG && !ZFLG               HI */
        case 3:  return (cznv & (FLAGVAL_C | FLAGVAL_Z)) != 0;                          /*  CFLG || ZFLG                LS */
        case 4:  return (cznv & FLAGVAL_C) == 0;                                        /* !CFLG                        CC */
        case 5:  return (cznv & FLAGVAL_C) != 0;                                        /*  CFLG                        CS */
        case 6:  return (cznv & FLAGVAL_Z) == 0;                                        /* !ZFLG                        NE */
        case 7:  return (cznv & FLAGVAL_Z) != 0;                                        /*  ZFLG                        EQ */
        case 8:  return (cznv & FLAGVAL_V) == 0;                                        /* !VFLG                        VC */
        case 9:  return (cznv & FLAGVAL_V) != 0;                                        /*  VFLG                        VS */
        case 10: return (cznv & FLAGVAL_N) == 0;                                        /* !NFLG                        PL */
        case 11: return (cznv & FLAGVAL_N) != 0;                                        /*  NFLG                        MI */
        case 12: return (((cznv << (FLAGBIT_N - FLAGBIT_V)) ^ cznv) & FLAGVAL_N) == 0;  /*  NFLG == VFLG                GE */
        case 13: return (((cznv << (FLAGBIT_N - FLAGBIT_V)) ^ cznv) & FLAGVAL_N) != 0;  /*  NFLG != VFLG                LT */
        case 14: cznv &= (FLAGVAL_N | FLAGVAL_Z | FLAGVAL_V);                           /* ZFLG && (NFLG == VFLG)       GT */
                 return (((cznv << (FLAGBIT_N - FLAGBIT_V)) ^ cznv) & (FLAGVAL_N | FLAGVAL_Z)) == 0;
        case 15: cznv &= (FLAGVAL_N | FLAGVAL_Z | FLAGVAL_V);                           /* ZFLG && (NFLG != VFLG)       LE */
                 return (((cznv << (FLAGBIT_N - FLAGBIT_V)) ^ cznv) & (FLAGVAL_N | FLAGVAL_Z)) != 0;
    }
    abort ();
    return 0;
}

