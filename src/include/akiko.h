

#define AKIKO_BASE 0xb80000
#define AKIKO_BASE_END 0xb80100 /* ?? */

extern void akiko_reset (void);
extern int akiko_init (void);
extern void akiko_free (void);

extern int cd32_enabled;

extern void akiko_entergui (void);
extern void akiko_exitgui (void);
extern void AKIKO_hsync_handler (void);
#if 0
extern uae_u32 akiko_lget (uaecptr addr) REGPARAM;
extern uae_u32 akiko_wget (uaecptr addr) REGPARAM;
extern uae_u32 akiko_bget (uaecptr addr) REGPARAM;

extern void akiko_bput (uaecptr addr, uae_u32 value) REGPARAM;
extern void akiko_wput (uaecptr addr, uae_u32 value) REGPARAM;
extern void akiko_lput (uaecptr addr, uae_u32 value) REGPARAM;
#endif 
extern uae_u32 REGPARAM3 akiko_lget (uaecptr) REGPARAM;
extern uae_u32 REGPARAM3 akiko_wget (uaecptr) REGPARAM;
extern void REGPARAM3 akiko_lput (uaecptr, uae_u32) REGPARAM;
extern void REGPARAM3 akiko_wput (uaecptr, uae_u32) REGPARAM;
extern void REGPARAM3 akiko_bput (uaecptr, uae_u32) REGPARAM;


extern void akiko_mute (int);

extern uae_u8 *extendedkickmemory;

extern void rethink_akiko (void);

