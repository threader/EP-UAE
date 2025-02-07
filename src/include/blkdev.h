
#define MAX_TOTAL_DEVICES 8
#define DEVICE_SCSI_BUFSIZE (65536 - 1024)

//#define device_debug write_log
#define device_debug

#define	INQ_DASD	0x00		/* Direct-access device (disk) */
#define	INQ_SEQD	0x01		/* Sequential-access device (tape) */
#define	INQ_PRTD	0x02 		/* Printer device */
#define	INQ_PROCD	0x03 		/* Processor device */
#define	INQ_OPTD	0x04		/* Write once device (optical disk) */
#define	INQ_WORM	0x04		/* Write once device (optical disk) */
#define	INQ_ROMD	0x05		/* CD-ROM device */
#define	INQ_SCAN	0x06		/* Scanner device */
#define	INQ_OMEM	0x07		/* Optical Memory device */
#define	INQ_JUKE	0x08		/* Medium Changer device (jukebox) */
#define	INQ_COMM	0x09		/* Communications device */
#define	INQ_IT8_1	0x0A		/* IT8 */
#define	INQ_IT8_2	0x0B		/* IT8 */
#define	INQ_STARR	0x0C		/* Storage array device */
#define	INQ_ENCL	0x0D		/* Enclosure services device */
#define	INQ_NODEV	0x1F		/* Unknown or no device */
#define	INQ_NOTPR	0x1F		/* Logical unit not present (SCSI-1) */

#define DEVICE_TYPE_ANY 1
#define DEVICE_TYPE_SCSI 2

#define DF_SCSI 0
#define DF_IOCTL 1

struct device_info {
    int type;
    int media_inserted;
    int removable;
    int write_protected;
    int cylinders;
    int trackspercylinder;
    int sectorspertrack;
    int bytespersector;
    int bus, target, lun;
    int id;
    char label[MAX_DPATH];
	char mediapath[MAX_DPATH];
};

struct device_scsi_info {
    uae_u8 *buffer;
    int bufsize;
};

struct amigascsi
{
    uae_u8 *data;
    uae_s32 len;
    uae_u8 cmd[16];
    uae_s32 cmd_len;
    uae_u8 flags;
    uae_u8 sensedata[256];
    uae_u16 sense_len;
    uae_u16 cmdactual;
    uae_u8 status;
    uae_u16 actual;
    uae_u16 sactual;
};

typedef int                  (*open_bus_func)           (int flags);
typedef void                 (*close_bus_func)          (void);
typedef int                  (*open_device_func)        (int);
typedef void                 (*close_device_func)       (int);
typedef struct device_info * (*info_device_func)        (int, struct device_info *);
typedef struct device_scsi_info* (*scsiinfo_func)(int, struct device_scsi_info*);
typedef const uae_u8 *       (*execscsicmd_out_func)    (int, const uae_u8 *, int);
typedef const uae_u8 *       (*execscsicmd_in_func)     (int, const uae_u8 *, int, int *);
typedef int                  (*execscsicmd_direct_func) (int, uaecptr);

typedef int                  (*pause_func)              (int, int);
typedef int                  (*stop_func)               (int);
typedef int                  (*play_func)               (int, uae_u32, uae_u32, int);
typedef void				 (*volume_func)				(int, uae_u16);
typedef const uae_u8 *       (*qcode_func)              (int);
typedef const uae_u8 *       (*toc_func)                (int);
typedef const uae_u8 *       (*read_func)               (int, int);
typedef uae_u8 *			 (*rawread_func)			(int, int, int);
typedef int                  (*write_func)              (int, int);
typedef int                  (*isatapi_func)            (int);
typedef int					 (*ismedia_func)			(int, int);

struct device_functions {
    open_bus_func     openbus;
    close_bus_func    closebus;
    open_device_func  opendev;
    close_device_func closedev;
    info_device_func  info;

    execscsicmd_out_func    exec_out;
    execscsicmd_in_func     exec_in;
    execscsicmd_direct_func exec_direct;

    pause_func pause;
    stop_func  stop;
    play_func  play;
	volume_func volume;
    qcode_func qcode;
    toc_func   toc;
    read_func  read;
    rawread_func rawread;
    write_func write;

    isatapi_func isatapi;

    open_device_func  opendevthread;
    close_device_func closedevthread;

    scsiinfo_func scsiinfo;

    ismedia_func ismedia;

};

extern struct device_functions *device_func[2];

extern int                  device_func_init         (int flags);
extern void device_func_reset(void);
extern int                  sys_command_open         (int mode, int unitnum);
extern void                 sys_command_close        (int mode, int unitnum);
extern struct device_info * sys_command_info         (int mode, int unitnum, struct device_info *di);
extern struct device_scsi_info *sys_command_scsi_info (int mode, int unitnum, struct device_scsi_info *di);
extern void                 sys_command_cd_pause        (int mode, int unitnum, int paused);
extern void                 sys_command_cd_stop         (int mode, int unitnum);
extern int                  sys_command_cd_play         (int mode, int unitnum, uae_u32 startmsf, uae_u32 endmsf, int);
extern void					sys_command_cd_volume (int mode, int unitnum, uae_u16 volume);
extern const uae_u8 *       sys_command_cd_qcode        (int mode, int unitnum);
extern const uae_u8 *		sys_command_cd_toc          (int mode, int unitnum);
extern const uae_u8 *		sys_command_cd_read         (int mode, int unitnum, int offset);
extern uae_u8 *				sys_command_cd_rawread (int mode, int unitnum, int offset, int size);
extern uae_u8 *				sys_command_read (int mode, int unitnum, int offset);
extern int                  sys_command_write        (int mode, int unitnum, int offset);
extern int sys_command_scsi_direct_native(int unitnum, struct amigascsi *as);
extern int                  sys_command_scsi_direct  (int unitnum, uaecptr request);
extern int sys_command_ismedia (int mode, int unitnum, int quick);
extern int                  sys_command_open_thread  (int mode, int unitnum);
extern void                 sys_command_close_thread (int mode, int unitnum);


void scsi_atapi_fixup_pre  (uae_u8 *scsi_cmd, int *len, uae_u8 **data, unsigned int *datalen, int *parm);
void scsi_atapi_fixup_post (uae_u8 *scsi_cmd, int len, uae_u8 *olddata, uae_u8 *data, unsigned int *datalen, int parm);

void scsi_log_before (const uae_u8 *cdb,  int cdblen,  const uae_u8 *data,  int datalen);
void scsi_log_after  (const uae_u8 *data, int datalen, const uae_u8 *sense, int senselen);

extern void cdimage_vsync (void);
