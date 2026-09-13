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

// C++ replacement for AUDUNCMP.ASM's Decompress_Frame (sub-milestone 1c -
// Agent/plans_done/code_1c_port_log.md) - the "Westwood ADPCM" (SCOMP_WESTWOOD) codec used for
// in-game sound effect (.AUD) playback, distinct from the separate SOS/"General MIDI" codec
// (SOSCODEC.ASM/OLSOSDEC.ASM) used for SCOMP_SOS streams.
//
// Each source byte is a code (top 2 bits) plus a 6-bit data/count field, decoded against a running
// "previous sample" value:
//   code 2 (RAW), data bit5 set   -> one sample: previous + sign-extend(data's low 5 bits), saturated.
//   code 2 (RAW), data bit5 clear -> (data+1) literal bytes copied straight from source to dest;
//                                    `previous` becomes the last byte copied.
//   code 1 (4BIT)                 -> (data+1) source bytes, each two nibble-packed deltas (low
//                                    nibble first) looked up in _4bitdecode, applied to `previous`.
//   code 0 (2BIT)                 -> (data+1) source bytes, each four 2-bit-packed deltas (low bits
//                                    first) looked up in _2bitdecode, applied to `previous`.
//   code 3 (SILENCE)              -> (data+1) copies of the unchanged `previous` sample (a run of
//                                    zero deltas).
// All delta application saturates to the 0-255 byte range rather than wrapping, matching the
// original's carry-flag-based clamp (Sat_Add below is the direct, obviously-correct equivalent).

#include <string.h>

namespace {

enum { CODE_2BIT = 0, CODE_4BIT = 1, CODE_RAW = 2, CODE_SILENCE = 3 };

signed char const _2bitdecode[4] = { -2, -1, 0, 1 };
signed char const _4bitdecode[16] = { -9, -8, -6, -5, -4, -3, -2, -1, 0, 1, 2, 3, 4, 5, 6, 8 };

inline unsigned char Sat_Add(unsigned char base, int delta)
{
	int v = (int)base + delta;
	if (v < 0) return 0;
	if (v > 255) return 255;
	return (unsigned char)v;
}

}	// namespace

extern "C" long __cdecl Decompress_Frame(void * source, void * dest, long count)
{
	if (!source || !dest || count == 0) return 0;

	unsigned char const * src = (unsigned char const *)source;
	unsigned char * out = (unsigned char *)dest;
	long remaining = count;
	unsigned char previous = 0x80;

	while (remaining > 0) {
		unsigned char byte = *src++;
		int code = byte >> 6;
		int data = byte & 0x3F;

		if (code == CODE_RAW) {
			if (data & 0x20) {
				signed char delta = (signed char)(data << 3) >> 3;
				previous = Sat_Add(previous, delta);
				*out++ = previous;
				remaining--;
			} else {
				int rawcount = data + 1;
				memcpy(out, src, rawcount);
				src += rawcount;
				out += rawcount;
				remaining -= rawcount;
				previous = out[-1];
			}
		} else if (code == CODE_4BIT) {
			int nibble_bytes = data + 1;
			for (int i = 0; i < nibble_bytes; i++) {
				unsigned char packed = *src++;
				previous = Sat_Add(previous, _4bitdecode[packed & 0x0F]);
				*out++ = previous;
				remaining--;
				previous = Sat_Add(previous, _4bitdecode[(packed >> 4) & 0x0F]);
				*out++ = previous;
				remaining--;
			}
		} else if (code == CODE_2BIT) {
			int packed_bytes = data + 1;
			for (int i = 0; i < packed_bytes; i++) {
				unsigned char packed = *src++;
				for (int shift = 0; shift < 8; shift += 2) {
					previous = Sat_Add(previous, _2bitdecode[(packed >> shift) & 0x03]);
					*out++ = previous;
					remaining--;
				}
			}
		} else {	// CODE_SILENCE
			int run = data + 1;
			memset(out, previous, run);
			out += run;
			remaining -= run;
		}
	}

	return (long)(src - (unsigned char const *)source);
}
