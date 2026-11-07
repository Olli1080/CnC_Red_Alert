#include <stdint.h>		// Port: uintptr_t

/*
**	Command & Conquer Red Alert(tm)
**	Copyright 2025 Electronic Arts Inc.
**
**	This program is free software: you can redistribute it and/or modify
**	it under the terms of the GNU General Public License as published by
**	the Free Software Foundation, either version 3 of the License, or
**	(at your option) any later version.
**
**	This program is distributed in the hope that it will be useful,
**	but WITHOUT ANY WARRANTY; without even the implied warranty of
**	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
**	GNU General Public License for more details.
**
**	You should have received a copy of the GNU General Public License
**	along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

/* $Header: /CounterStrike/LCWUNCMP.CPP 1     3/03/97 10:25a Joe_bostic $ */
/***************************************************************************
 **    C O N F I D E N T I A L --- W E S T W O O D   S T U D I O S        **
 ***************************************************************************
 *                                                                         *
 *               Project Name : WESTWOOD LIBRARY (PSX)                     *
 *                                                                         *
 *                 File Name : LCWUNCMP.CPP                                *
 *                                                                         *
 *                Programmer : Ian M. Leslie                               *
 *                                                                         *
 *                Start Date : May 17, 1995                                *
 *                                                                         *
 *               Last Update : May 17, 1995    [IML]                       *
 *                                                                         *
 *-------------------------------------------------------------------------*
 * Functions:                                                              *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

extern "C" {

/***************************************************************************
 * LCW_UNCOMPRESS -- Decompress an LCW encoded data block.                 *
 *                                                                         *
 * Uncompress data to the following codes in the format b = byte, w = word *
 * n = byte code pulled from compressed data.                              *
 *                                                                         *
 * Command code, n        |Description                                     *
 * ------------------------------------------------------------------------*
 * n=0xxxyyyy,yyyyyyyy    |short copy back y bytes and run x+3   from dest *
 * n=10xxxxxx,n1,n2,...,nx+1|med length copy the next x+1 bytes from source*
 * n=11xxxxxx,w1          |med copy from dest x+3 bytes from offset w1     *
 * n=11111111,w1,w2       |long copy from dest w1 bytes from offset w2     *
 * n=11111110,w1,b1       |long run of byte b1 for w1 bytes                *
 * n=10000000             |end of data reached                             *
 *                                                                         *
 *                                                                         *
 * INPUT:                                                                  *
 *      void * source ptr                                                  *
 *      void * destination ptr                                             *
 *      unsigned long length of uncompressed data                          *
 *                                                                         *
 *                                                                         *
 * OUTPUT:                                                                 *
 *     unsigned long # of destination bytes written                        *
 *                                                                         *
 * WARNINGS:                                                               *
 *     3rd argument is dummy. It exists to provide cross-platform          *
 *      compatibility. Note therefore that this implementation does not    *
 *      check for corrupt source data by testing the uncompressed length.  *
 *                                                                         *
 * HISTORY:                                                                *
 *    03/20/1995 IML : Created.                                            *
 *=========================================================================*/
/* Port: LCW_Decode implements both entry points below. `limit` is the first byte that must not be
   written (dest + length). The original assembler (WIN32LIB/IFF/LCWUNCMP.ASM) honored the `length`
   argument - it stopped as soon as `length` bytes were produced and clamped every run to the remaining
   space - and WSA.CPP depends on that: it decompresses a frame in place, with the compressed data at
   the END of the destination, and for the largest frames the output overwrites the last bytes of the
   source. The decoder then reads garbage past the real end marker; the clamp is what ends it. */
static unsigned long LCW_Decode (void * source, void * dest, unsigned char * limit)
{
	unsigned char * source_ptr, * dest_ptr, * copy_ptr, op_code, data;
	unsigned	  count, * word_dest_ptr, word_data;

	/* Copy the source and destination ptrs. */
	source_ptr = (unsigned char*) source;
	dest_ptr   = (unsigned char*) dest;

	while (1 /*TRUE*/) {

		if (dest_ptr >= limit) return ((unsigned long) (dest_ptr - (unsigned char*) dest));
		size_t remain = (size_t) (limit - dest_ptr);

		/* Read in the operation code. */
		op_code = *source_ptr++;

		if (!(op_code & 0x80)) {

			/* Do a short copy from destination. */
			count	 = (op_code >> 4) + 3;
			if (count > remain) count = (unsigned) remain;
			copy_ptr = dest_ptr - ((unsigned) *source_ptr++ + (((unsigned) op_code & 0x0f) << 8));

			while (count--) *dest_ptr++ = *copy_ptr++;

		} else {

			if (!(op_code & 0x40)) {

				if (op_code == 0x80) {

					/* Return # of destination bytes written. */
					return ((unsigned long) (dest_ptr - (unsigned char*) dest));

				} else {

					/* Do a medium copy from source. */
					count = op_code & 0x3f;
					if (count > remain) count = (unsigned) remain;

					while (count--) *dest_ptr++ = *source_ptr++;
				}

			} else {

				if (op_code == 0xfe) {

					/* Do a long run. */
					/* Port: the original C reference wrote the run in aligned 8-byte chunks, which can store up to 4
					   bytes beyond the run and underflows `count` for short, misaligned runs. The WSA decoder
					   decompresses in place (source at the end of the destination), where such stray writes
					   destroy not-yet-read source bytes. Fill exactly `count` bytes instead. */
					count = *source_ptr + ((unsigned) *(source_ptr + 1) << 8);
					data = *(source_ptr + 2);
					source_ptr += 3;
					if (count > remain) count = (unsigned) remain;

					while (count--) *dest_ptr++ = data;

				} else {

					if (op_code == 0xff) {

						/* Do a long copy from destination. */
						count	 = *source_ptr + ((unsigned) *(source_ptr + 1) << 8);
						if (count > remain) count = (unsigned) remain;
						copy_ptr = (unsigned char*) dest + *(source_ptr + 2) + ((unsigned) *(source_ptr + 3) << 8);
						source_ptr += 4;

						while (count--) *dest_ptr++ = *copy_ptr++;

					} else {

						/* Do a medium copy from destination. */
						count = (op_code & 0x3f) + 3;
						if (count > remain) count = (unsigned) remain;
						copy_ptr = (unsigned char*) dest + *source_ptr + ((unsigned) *(source_ptr + 1) << 8);
						source_ptr += 2;

						while (count--) *dest_ptr++ = *copy_ptr++;
					}
				}
			}
		}
	}
}

/* The historic C reference ignored `length` (callers pass unrelated sizes), keep that. */
unsigned long __cdecl LCW_Uncompress (void * source, void * dest, unsigned long)
{
	return LCW_Decode(source, dest, (unsigned char *) UINTPTR_MAX);
}

/* Same as the assembler: never writes at or beyond dest + length. Used by WSA.CPP. */
unsigned long __cdecl LCW_Uncompress_Bounded (void * source, void * dest, unsigned long length)
{
	return LCW_Decode(source, dest, (unsigned char *) dest + length);
}

}
