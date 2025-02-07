 /*
  * UAE - The Un*x Amiga Emulator
  *
  * Joystick, mouse and keyboard emulation prototypes and definitions
  *
  * Copyright 1995 Bernd Schmidt
  * Copyright 2001-2002 Toni Wilen
  */


#define IDTYPE_JOYSTICK 0
#define IDTYPE_MOUSE 1
#define IDTYPE_KEYBOARD 2
#define IDTYPE_INTERNALEVENT 3

struct inputdevice_functions {
    int           (*init)             (void);
    void          (*close)            (void);
    int           (*acquire)          (unsigned int, int);
    void          (*unacquire)        (unsigned int);
    void          (*read)             (void);
    unsigned int  (*get_num)          (void);
    const char* (*get_friendlyname)(unsigned int);
    const char* (*get_uniquename)(unsigned int);
    unsigned int  (*get_widget_num)   (unsigned int);
    int           (*get_widget_type)  (unsigned int, unsigned int, char *, uae_u32 *);
    int           (*get_widget_first) (unsigned int, int);
    unsigned int (*get_flags)(unsigned int);
};
extern struct inputdevice_functions idev[3];
extern struct inputdevice_functions inputdevicefunc_joystick;
extern struct inputdevice_functions inputdevicefunc_mouse;
extern struct inputdevice_functions inputdevicefunc_keyboard;
extern int pause_emulation;

struct uae_input_device_kbr_default {
    int scancode;
    int event;
};

#define IDEV_WIDGET_NONE 0
#define IDEV_WIDGET_BUTTON 1
#define IDEV_WIDGET_AXIS 2
#define IDEV_WIDGET_KEY 3

#define IDEV_MAPPED_AUTOFIRE_POSSIBLE 1
#define IDEV_MAPPED_AUTOFIRE_SET 2
#define IDEV_MAPPED_TOGGLE 4

#define ID_BUTTON_OFFSET 0
#define ID_BUTTON_TOTAL 32
#define ID_AXIS_OFFSET 32
#define ID_AXIS_TOTAL 32

 int inputdevice_iterate (int devnum, int num, TCHAR *name, int *af);
 int inputdevice_set_mapping (int devnum, int num, const TCHAR *name, TCHAR *custom, int flags, int sub);
 int inputdevice_get_mapped_name (int devnum, int num, int *pflags, TCHAR *name, TCHAR *custom, int sub);
 void inputdevice_copyconfig (const struct uae_prefs *src, struct uae_prefs *dst);
 void inputdevice_copy_single_config (struct uae_prefs *p, int src, int dst, int devnum);
 void inputdevice_swap_ports (struct uae_prefs *p, int devnum);
 void inputdevice_config_change (void);
 int inputdevice_config_change_test (void);
 int inputdevice_get_device_index (unsigned int devnum);
 const char *inputdevice_get_device_name (int type, int devnum);
 const TCHAR *inputdevice_get_device_unique_name (int type, int devnum);
 int inputdevice_get_device_status (int devnum);
 void inputdevice_set_device_status (int devnum, int enabled);
 int inputdevice_get_device_total (int type);
 int inputdevice_get_widget_num (int devnum);
 int inputdevice_get_widget_type (int devnum, int num, TCHAR *name);

 int input_get_default_mouse (struct uae_input_device *uid, int num, int port);
 int input_get_default_lightpen (struct uae_input_device *uid, int num, int port);
 int input_get_default_joystick (struct uae_input_device *uid, int num, int port, int cd32);
 int input_get_default_joystick_analog (struct uae_input_device *uid, int num, int port);
 int input_get_default_keyboard (int num);

#define DEFEVENT(A, B, C, D, E, F) INPUTEVENT_ ## A,
enum inputevents {
INPUTEVENT_ZERO,
#include "inputevents.def"
INPUTEVENT_END
};
#undef DEFEVENT

 void handle_cd32_joystick_cia (uae_u8, uae_u8);
 uae_u8 handle_parport_joystick (int port, uae_u8 pra, uae_u8 dra);
 uae_u8 handle_joystick_buttons (uae_u8);
 int getbuttonstate (int joy, int button);
 int getjoystate (int joy);

 int mousehack_alive (void);
 int mousehack_allowed (void);
#define MAGICMOUSE_BOTH 0
#define MAGICMOUSE_NATIVE_ONLY 1
#define MAGICMOUSE_HOST_ONLY 2

 int magicmouse_alive (void);
 int is_tablet (void);
 int inputdevice_is_tablet (void);
 void input_mousehack_status (int mode, uaecptr diminfo, uaecptr dispinfo, uaecptr vp, uae_u32 moffset);
 void input_mousehack_mouseoffset (uaecptr pointerprefs);
 int mousehack_alive (void);
 void setmouseactive (int);

 void setmousebuttonstateall (int mouse, uae_u32 buttonbits, uae_u32 buttonmask);
 void setjoybuttonstateall (int joy, uae_u32 buttonbits, uae_u32 buttonmask);
 void setjoybuttonstate (int joy, int button, int state);
 void setmousebuttonstate (int mouse, int button, int state);
 void setjoystickstate (int joy, int axle, int state, int max);
void setmousestate (int mouse, int axis, int data, int isabs);
 void inputdevice_updateconfig (struct uae_prefs *prefs);
 int getjoystickstate (int mouse);
void setmousestate (int mouse, int axis, int data, int isabs);
 int getmousestate (int mouse);
 void inputdevice_updateconfig (struct uae_prefs *prefs);
 void inputdevice_mergeconfig (struct uae_prefs *prefs);
 void inputdevice_devicechange (struct uae_prefs *prefs);

 int inputdevice_translatekeycode (int keyboard, int scancode, int state);
 void inputdevice_setkeytranslation (struct uae_input_device_kbr_default *trans);
 int handle_input_event (int nr, int state, int max, int autofire);
 void inputdevice_do_keyboard (int code, int state);
void inputdevice_release_all_keys (void);
 int inputdevice_iskeymapped (int keyboard, int scancode);
 int inputdevice_synccapslock (int, int*);
 void inputdevice_testrecord (int type, int num, int wtype, int wnum, int state);

#define INTERNALEVENT_KBRESET 1

void send_internalevent (int eventid);

extern uae_u16 potgo_value;
 uae_u16 POTGOR (void);
 void POTGO (uae_u16 v);
 uae_u16 POT0DAT (void);
 uae_u16 POT1DAT (void);
 void JOYTEST (uae_u16 v);
 uae_u16 JOY0DAT (void);
 uae_u16 JOY1DAT (void);

 void inputdevice_vsync (void);
 void inputdevice_hsync (void);
 void inputdevice_reset (void);

 void write_inputdevice_config (const struct uae_prefs *p, FILE *f);
 void read_inputdevice_config (struct uae_prefs *p, const char *option, const char *value);
 void reset_inputdevice_config (struct uae_prefs *pr);
 int inputdevice_joyport_config (struct uae_prefs *p, TCHAR *value, int portnum, int mode, int type);
 int inputdevice_getjoyportdevice (int jport);

 void inputdevice_init (void);
 void inputdevice_close (void);
 void inputdevice_default_prefs (struct uae_prefs *p);

 void inputdevice_acquire (int allmode);
 void inputdevice_unacquire (void);

 void indicator_leds (int num, int state);

 void warpmode (int mode);
 void pausemode (int mode);

 void inputdevice_add_inputcode (int code, int state);
 void inputdevice_handle_inputcode (void);

 void inputdevice_tablet (int x, int y, int z,
	      int pressure, uae_u32 buttonbits, int inproximity,
	      int ax, int ay, int az);
 void inputdevice_tablet_info (int maxx, int maxy, int maxz, int maxax, int maxay, int maxaz, int xres, int yres);
 void inputdevice_tablet_strobe (void);


#define JSEM_MODE_DEFAULT 0
#define JSEM_MODE_MOUSE 1
#define JSEM_MODE_JOYSTICK 2
#define JSEM_MODE_JOYSTICK_ANALOG 3
#define JSEM_MODE_MOUSE_CDTV 4
#define JSEM_MODE_JOYSTICK_CD32 5
#define JSEM_MODE_LIGHTPEN 6

#define JSEM_KBDLAYOUT 0
#define JSEM_JOYS      100
#define JSEM_MICE      200
#define JSEM_END       300
#define JSEM_NONE      300
#define JSEM_XARCADE1LAYOUT (JSEM_KBDLAYOUT + 3)
#define JSEM_XARCADE2LAYOUT (JSEM_KBDLAYOUT + 4)
#define JSEM_DECODEVAL(port,p) ((p)->jports[port].id)
#define JSEM_ISNUMPAD(port,p)        (jsem_iskbdjoy (port,p) == JSEM_KBDLAYOUT)
#define JSEM_ISCURSOR(port,p)        (jsem_iskbdjoy (port,p) == JSEM_KBDLAYOUT + 1)
#define JSEM_ISSOMEWHEREELSE(port,p) (jsem_iskbdjoy (port,p) == JSEM_KBDLAYOUT + 2)
#define JSEM_ISXARCADE1(port,p) (jsem_iskbdjoy(port,p) == JSEM_XARCADE1LAYOUT)
#define JSEM_ISXARCADE2(port,p) (jsem_iskbdjoy(port,p) == JSEM_XARCADE2LAYOUT)
#define JSEM_LASTKBD 5
#define JSEM_ISANYKBD(port,p)        (jsem_iskbdjoy (port,p) >= JSEM_KBDLAYOUT && jsem_iskbdjoy(port,p) < JSEM_KBDLAYOUT + JSEM_LASTKBD)

 int jsem_isjoy    (int port, const struct uae_prefs *p);
 int jsem_ismouse  (int port, const struct uae_prefs *p);
 int jsem_iskbdjoy (int port, const struct uae_prefs *p);
 void do_fake_joystick (int nr, int *fake);

 int inputdevice_uaelib (const TCHAR *, const TCHAR *);

#define INPREC_JOYPORT 1
#define INPREC_JOYBUTTON 2
#define INPREC_KEY 3
#define INPREC_DISKINSERT 4
#define INPREC_DISKREMOVE 5
#define INPREC_VSYNC 6
#define INPREC_CIAVSYNC 7
#define INPREC_END 0xff
#define INPREC_QUIT 0xfe

extern int input_recording;
 void inprec_close (void);
 int inprec_open (TCHAR*, int);
 void inprec_rend (void);
 void inprec_rstart (uae_u8);
 void inprec_ru8 (uae_u8);
 void inprec_ru16 (uae_u16);
 void inprec_ru32 (uae_u32);
 void inprec_rstr (const TCHAR*);
 int inprec_pstart (uae_u8);
 void inprec_pend (void);
 uae_u8 inprec_pu8 (void);
 uae_u16 inprec_pu16 (void);
 uae_u32 inprec_pu32 (void);
 int inprec_pstr (TCHAR*);

 int inputdevice_testread (TCHAR *name);
 int inputdevice_istest (void);
