 /*
  * UAE - The Un*x Amiga Emulator
  *
  * OS emulation prototypes
  *
  * Copyright 1996 Bernd Schmidt
  */

STATIC_INLINE char *raddr(uaecptr p)
{
<<<<<<< HEAD
    return p == 0 ? NULL : (char *)get_real_address(p);
=======
    return p == 0 ? NULL : (char *)get_real_address (p);
>>>>>>> p-uae/v2.1.0
}

extern void gfxlib_install(void);

/* graphics.library */

extern int GFX_WritePixel(uaecptr rp, int x, int y);

