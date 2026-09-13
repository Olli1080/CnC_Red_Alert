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

// C++ replacement for FADING.ASM's Build_Fading_Table (sub-milestone 1c -
// Agent/plans_done/code_1c_port_log.md).
//
// For every palette index 1-255, computes an "ideal" faded color (each RGB gun moved a `frac/256`
// fraction of the way from that index's own color toward `color`'s), then scans the rest of the
// palette (skipping the index being remapped) for the closest real color by summed squared RGB
// distance, and records that match's index into dest[]. Index 0 (transparent) is never remapped.
// On an exact-distance tie, the ASM's own comparison (<=, not <) keeps overwriting through every
// tying candidate, so the *last* (highest-index) tie wins, not the first - reproduced as-is here.
// The ideal-color math truncates each intermediate to a byte exactly as the original's 8-bit
// register arithmetic did (see Byte_Sub), which matters because the fade math relies on that
// wraparound rather than full-range signed subtraction.

namespace {

inline signed char Byte_Sub(int a, int b)
{
	return (signed char)(unsigned char)(a - b);
}

}

extern "C" void * __cdecl Build_Fading_Table(void const * palette, void const * dest, long color, long frac)
{
	if (!palette || !dest) return (void *)dest;

	if (frac >= 0x100) frac = 0xFF;
	int half_frac = (int)frac >> 1;

	unsigned char const * pal = (unsigned char const *)palette;
	unsigned char * out = (unsigned char *)dest;

	unsigned char target[3] = { pal[color * 3 + 0], pal[color * 3 + 1], pal[color * 3 + 2] };

	out[0] = 0;

	for (int i = 1; i <= 255; i++) {
		unsigned char ideal[3];
		for (int c = 0; c < 3; c++) {
			int orig = pal[i * 3 + c];
			int diff = Byte_Sub(orig, target[c]);
			int doubled = (diff * half_frac) << 1;
			unsigned int high_byte = ((unsigned int)(unsigned short)doubled) >> 8;
			ideal[c] = (unsigned char)(orig - high_byte);
		}

		int match_color = (int)color;		// default: the fade target color itself
		unsigned int match_value = 0xFFFFFFFFu;

		for (int j = 1; j <= 255; j++) {
			if (j == i) continue;

			int dr = Byte_Sub(pal[j * 3 + 0], ideal[0]);
			int dg = Byte_Sub(pal[j * 3 + 1], ideal[1]);
			int db = Byte_Sub(pal[j * 3 + 2], ideal[2]);
			unsigned int distance = (unsigned int)(dr * dr + dg * dg + db * db);

			if (distance == 0) { match_color = j; break; }
			if (distance <= match_value) { match_value = distance; match_color = j; }
		}

		out[i] = (unsigned char)match_color;
	}

	return (void *)dest;
}
