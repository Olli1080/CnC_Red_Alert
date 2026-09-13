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

// C++ replacement for LCWCOMP.ASM (sub-milestone 1c - Agent/plans_done/code_1c_port_log.md).
//
// This is a fresh LZ77-style encoder for the LCW format that LCW_Uncompress (LCWUNCMP.CPP) already
// decodes, rather than a literal translation of the original hand-optimized assembly search/match
// routine. The two only need to agree with each other on the bitstream format - LCW_Comp's only
// callers (LCWPIPE.CPP/LCWSTRAW.CPP) compress data at runtime (save games, network blocks) that
// this same codebase decompresses again later, so matching the original encoder's exact output
// byte-for-byte isn't required, only a correct, self-consistent round trip. The four op-code
// families below are exactly the ones LCW_Uncompress understands:
//   0x00-0x7F        short copy from dest:  count=(op>>4)+3 (3-10), back-distance=((op&0xF)<<8)|b1 (0-4095)
//   0x81-0xBF        literal run:           count=op&0x3F (1-63) raw bytes follow
//   0xC0-0xFD        medium copy from dest: count=(op&0x3F)+3 (3-64), abs offset = word (0-65535)
//   0xFF             long copy from dest:   word count, word abs offset
//   0x80             end of data
// (0xFE, "long run of one repeated byte", is a valid encode this decoder accepts but is never
// emitted here - a repeated run is already representable, just as correctly, via a copy-from-dest
// referencing back over itself, so the extra op code isn't needed for a correct encoder.)
//
// Match finding uses a single-candidate hash table (most recent position for each 3-byte prefix,
// no chaining) - cheap and O(n) on average, at the cost of some compression ratio versus the
// original's exhaustive backward search. Since "long"/"medium" copies encode an *absolute* offset
// into the destination buffer in a 16-bit word, any match candidate beyond that range is simply
// skipped (falls back to a literal byte) rather than emitting a broken reference; real callers keep
// each LCW_Comp() call under 64KB of input, so this path is not expected to trigger in practice.

#include <string.h>

extern "C" {

int __cdecl LCW_Comp(void const * source, void * dest, int length)
{
	unsigned char const * src = (unsigned char const *)source;
	unsigned char * const dst_start = (unsigned char *)dest;
	unsigned char * dst = dst_start;

	if (length <= 0) {
		*dst++ = 0x80;
		return (int)(dst - dst_start);
	}

	enum { HASH_BITS = 13, HASH_SIZE = 1 << HASH_BITS };
	int hash_table[HASH_SIZE];
	memset(hash_table, 0xFF, sizeof(hash_table));	// -1 = "no candidate yet"

	unsigned char * literal_op = NULL;	// opcode byte of the currently-open literal run, if any
	int literal_len = 0;					// bytes already stored in that run (0 if none open)

	int pos = 0;
	while (pos < length) {
		int best_len = 0;
		int best_pos = 0;

		if (pos + 3 <= length) {
			unsigned prefix = (unsigned)src[pos] | ((unsigned)src[pos + 1] << 8) | ((unsigned)src[pos + 2] << 16);
			unsigned h = (prefix * 2654435761u) >> (32 - HASH_BITS);
			int candidate = hash_table[h];
			hash_table[h] = pos;

			if (candidate >= 0) {
				int max_len = length - pos;
				int len = 0;
				while (len < max_len && src[candidate + len] == src[pos + len]) len++;
				if (len >= 3) {
					best_len = len;
					best_pos = candidate;
				}
			}
		}

		if (best_len >= 3) {
			int back_distance = pos - best_pos;
			int remaining = best_len;
			int match_src = best_pos;
			bool emitted = true;

			while (remaining >= 3 && emitted) {
				if (back_distance <= 0xFFF && remaining <= 10) {
					unsigned code = ((unsigned)(remaining - 3) << 4) | ((back_distance >> 8) & 0x0F);
					*dst++ = (unsigned char)code;
					*dst++ = (unsigned char)(back_distance & 0xFF);
					remaining = 0;
				} else if (match_src <= 0xFFFF && remaining <= 64) {
					*dst++ = (unsigned char)(0xC0 | (remaining - 3));
					*dst++ = (unsigned char)(match_src & 0xFF);
					*dst++ = (unsigned char)((match_src >> 8) & 0xFF);
					remaining = 0;
				} else if (match_src <= 0xFFFF) {
					int count = (remaining > 0xFFFF) ? 0xFFFF : remaining;
					*dst++ = 0xFF;
					*dst++ = (unsigned char)(count & 0xFF);
					*dst++ = (unsigned char)((count >> 8) & 0xFF);
					*dst++ = (unsigned char)(match_src & 0xFF);
					*dst++ = (unsigned char)((match_src >> 8) & 0xFF);
					remaining -= count;
					match_src += count;
					back_distance = pos + (best_len - remaining) - match_src;
				} else {
					// Match reference is outside the format's addressable 16-bit range - bail out
					// to the literal path below instead of emitting a broken copy.
					emitted = false;
				}
			}

			if (emitted) {
				pos += best_len;
				literal_op = NULL;
				literal_len = 0;
				continue;
			}
		}

		if (literal_op == NULL || literal_len >= 63) {
			literal_op = dst;
			*dst++ = 0x80;
			literal_len = 0;
		}
		*dst++ = src[pos];
		literal_len++;
		*literal_op = (unsigned char)(0x80 | literal_len);
		pos++;
	}

	*dst++ = 0x80;
	return (int)(dst - dst_start);
}

}
