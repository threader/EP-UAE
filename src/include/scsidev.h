 /*
  * UAE - The Un*x Amiga Emulator
  *
  * a SCSI device
  *
  * (c) 1995 Bernd Schmidt (hardfile.c)
  * (c) 1999 Patrick Ohly
  * (c) 2001-2005 Toni Wilen
  */

uaecptr scsidev_startup (uaecptr resaddr);
void scsidev_install (void);
void scsidev_exit (void);
void scsidev_reset (void);
void scsidev_start_threads (void);
void scsi_do_disk_change (int device_id, int insert);

extern int log_scsi;
void scsidev_reset (void);
void scsidev_start_threads (void);
int scsi_do_disk_change (int device_id, int insert);

extern int log_scsi;

#ifdef _WIN32
#define UAESCSI_SPTI 1
#define UAESCSI_SPTISCAN 2
#define UAESCSI_ASPI_FIRST 3
#define UAESCSI_ADAPTECASPI 3
#define UAESCSI_NEROASPI 4
#define UAESCSI_FROGASPI 5
#endif