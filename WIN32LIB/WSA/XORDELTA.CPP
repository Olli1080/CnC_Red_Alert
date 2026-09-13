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

// C++ replacement for XORDELTA.ASM (sub-milestone 1c - Agent/plans_done/code_1c_port_log.md) - the
// original file's own commented-out C reference implementation at the bottom (written for the old
// 16-bit real-mode MK_FP/Normalize_Pointer world) confirms the opcode format below; this is that
// same format ported to flat 32-bit pointers.
//
// Opcode stream format (each opcode byte, read from `delta`, XORs or skips bytes in `target`):
//   0x01-0x7F        SHORTDUMP: xor the next N bytes from delta onto target (N = opcode).
//   0x00             SHORTRUN: next byte = count, next byte = value; xor `value` onto target, count times.
//   0x80             read a signed 16-bit word W next:
//                        W > 0   -> LONGSKIP: target += W.
//                        W == 0  -> stop.
//                        W < 0   -> let M = W's low 15 bits (i.e. W & 0x7FFF):
//                                     bit14 of M set   -> LONGRUN: next byte = value, xor it (M & 0x3FFF) times.
//                                     bit14 of M clear -> LONGDUMP: xor the next M bytes from delta onto target.
//   0x81-0xFF        SHORTSKIP: target += (opcode - 0x80).

extern "C" unsigned int __cdecl Apply_XOR_Delta(char * target, char * delta)
{
	unsigned char * to = (unsigned char *)target;
	unsigned char const * from = (unsigned char const *)delta;

	for (;;) {
		unsigned int opcode = *from++;

		if (opcode == 0) {
			unsigned int count = *from++;
			unsigned char value = *from++;
			for (unsigned int i = 0; i < count; i++) *to++ ^= value;
			continue;
		}
		if (opcode < 0x80) {
			for (unsigned int i = 0; i < opcode; i++) *to++ ^= *from++;
			continue;
		}

		unsigned int rest = opcode - 0x80;
		if (rest != 0) {
			to += rest;
			continue;
		}

		short word_code = (short)((unsigned short)from[0] | ((unsigned short)from[1] << 8));
		from += 2;

		if (word_code > 0) {
			to += word_code;
			continue;
		}
		if (word_code == 0) break;

		unsigned int m = (unsigned short)word_code & 0x7FFF;
		if (m & 0x4000) {
			unsigned int count = m & 0x3FFF;
			unsigned char value = *from++;
			for (unsigned int i = 0; i < count; i++) *to++ ^= value;
		} else {
			for (unsigned int i = 0; i < m; i++) *to++ ^= *from++;
		}
	}

	return 64000U;
}

// Apply_XOR_Delta_To_Page_Or_Viewport (WSA.CPP's real call site - a rectangular target with a row
// width and a per-row skip, such as a viewport row, rather than one flat buffer) uses the exact
// same opcode stream format as Apply_XOR_Delta above, but positions writes by (row, column) instead
// of a flat pointer: `width` bytes per row, then `nextrow` bytes to skip to reach the next row.
// Every op - literal run, single delta, RLE run, or skip - advances by one or more columns and
// wraps to a new row (looping for skips large enough to cross more than one row boundary, matching
// the original's own multi-row skip loop) rather than a flat pointer add.
namespace {

void Apply_Delta_Rows(unsigned char * target, unsigned char const * delta, int width, int nextrow, bool do_copy)
{
	int const stride = width + nextrow;
	unsigned char * row_start = target;
	int col = 0;
	unsigned char * out = target;

	auto advance = [&](int n) {
		col += n;
		while (col >= width) {
			col -= width;
			row_start += stride;
		}
		out = row_start + col;
	};
	auto put = [&](unsigned char value) {
		if (do_copy) *out = value; else *out ^= value;
		advance(1);
	};

	unsigned char const * src = delta;

	for (;;) {
		unsigned int opcode = *src++;

		if (opcode == 0) {
			unsigned int count = *src++;
			unsigned char value = *src++;
			for (unsigned int i = 0; i < count; i++) put(value);
			continue;
		}
		if (opcode < 0x80) {
			for (unsigned int i = 0; i < opcode; i++) put(*src++);
			continue;
		}

		unsigned int rest = opcode - 0x80;
		if (rest != 0) {
			advance((int)rest);
			continue;
		}

		short word_code = (short)((unsigned short)src[0] | ((unsigned short)src[1] << 8));
		src += 2;

		if (word_code > 0) {
			advance(word_code);
			continue;
		}
		if (word_code == 0) break;

		unsigned int m = (unsigned short)word_code & 0x7FFF;
		if (m & 0x4000) {
			unsigned int count = m & 0x3FFF;
			unsigned char value = *src++;
			for (unsigned int i = 0; i < count; i++) put(value);
		} else {
			for (unsigned int i = 0; i < m; i++) put(*src++);
		}
	}
}

}	// namespace

extern "C" void __cdecl Apply_XOR_Delta_To_Page_Or_Viewport(void * target, void * delta, int width, int nextrow, int copy)
{
	Apply_Delta_Rows((unsigned char *)target, (unsigned char const *)delta, width, nextrow, copy != 0);
}
