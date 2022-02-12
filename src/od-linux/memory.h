<<<<<<< HEAD
 /* 
  * UAE - The Un*x Amiga Emulator
  * 
=======
 /*
  * UAE - The Un*x Amiga Emulator
  *
>>>>>>> p-uae/v2.1.0
  * See if this OS has mmap.
  *
  * Copyright 1996 Bernd Schmidt
  */

#undef USE_MAPPED_MEMORY

#if USER_PROGRAMS_BEHAVE > 0
#define USE_MAPPED_MEMORY
#endif

#define CAN_MAP_MEMORY
