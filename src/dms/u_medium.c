
/*
 *     xDMS  v1.3  -  Portable DMS archive unpacker  -  Public Domain
 *     Written by     Andre Rodrigues de la Rocha  <adlroc@usa.net>
 *
 *     Main decompression functions used in MEDIUM mode
 *
 */


#include <string.h>

#include "cdata.h"
#include "u_medium.h"
#include "getbits.h"
#include "tables.h"


#define MBITMASK 0x3fff


<<<<<<< HEAD
USHORT medium_text_loc;
=======
USHORT dms_medium_text_loc;
>>>>>>> p-uae/v2.1.0



USHORT Unpack_MEDIUM(UCHAR *in, UCHAR *out, USHORT origsize){
	USHORT i, j, c;
	UCHAR u, *outend;


	initbitbuf(in);

	outend = out+origsize;
	while (out < outend) {
		if (GETBITS(1)!=0) {
			DROPBITS(1);
<<<<<<< HEAD
			*out++ = text[medium_text_loc++ & MBITMASK] = (UCHAR)GETBITS(8);
=======
			*out++ = dms_text[dms_medium_text_loc++ & MBITMASK] = (UCHAR)GETBITS(8);
>>>>>>> p-uae/v2.1.0
			DROPBITS(8);
		} else {
			DROPBITS(1);
			c = GETBITS(8);  DROPBITS(8);
			j = (USHORT) (d_code[c]+3);
			u = d_len[c];
			c = (USHORT) (((c << u) | GETBITS(u)) & 0xff);  DROPBITS(u);
			u = d_len[c];
			c = (USHORT) ((d_code[c] << 8) | (((c << u) | GETBITS(u)) & 0xff));  DROPBITS(u);
<<<<<<< HEAD
			i = (USHORT) (medium_text_loc - c - 1);

			while(j--) *out++ = text[medium_text_loc++ & MBITMASK] = text[i++ & MBITMASK];
			
		}
	}
	medium_text_loc = (USHORT)((medium_text_loc+66) & MBITMASK);
=======
			i = (USHORT) (dms_medium_text_loc - c - 1);

			while(j--) *out++ = dms_text[dms_medium_text_loc++ & MBITMASK] = dms_text[i++ & MBITMASK];

		}
	}
	dms_medium_text_loc = (USHORT)((dms_medium_text_loc+66) & MBITMASK);
>>>>>>> p-uae/v2.1.0

	return 0;
}


