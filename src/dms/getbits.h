
<<<<<<< HEAD
extern ULONG mask_bits[], bitbuf;
extern UCHAR *indata, bitcount;

#define GETBITS(n) ((USHORT)(bitbuf >> (bitcount-(n))))
#define DROPBITS(n) {bitbuf &= mask_bits[bitcount-=(n)]; while (bitcount<16) {bitbuf = (bitbuf << 8) | *indata++;  bitcount += 8;}}
=======
extern ULONG dms_mask_bits[], dms_bitbuf;
extern UCHAR *dms_indata, dms_bitcount;

#define GETBITS(n) ((USHORT)(dms_bitbuf >> (dms_bitcount-(n))))
#define DROPBITS(n) {dms_bitbuf &= dms_mask_bits[dms_bitcount-=(n)]; while (dms_bitcount<16) {dms_bitbuf = (dms_bitbuf << 8) | *dms_indata++;  dms_bitcount += 8;}}
>>>>>>> p-uae/v2.1.0


void initbitbuf(UCHAR *);

