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

// C++ replacement for TXTPRNT.ASM's Buffer_Print/ColorXlat/Get_Font_Palette_Ptr (sub-milestone 1c -
// Agent/plans_done/code_1c_port_log.md). This is the "live" of two duplicate text-print
// implementations in this GPL release - FONT/TEXTPRNT.ASM defines an unrelated older Text_Print
// entry point over its own separate ColorXlat table (same duplicate-implementation pattern already
// seen with WWSTD.H and PROFILE.CPP/WPROFILE.CPP elsewhere in this codebase) and is not ported;
// every live caller (GBUFFER.H's Buffer_Print method, FONT/SETFPAL.CPP's Set_Font_Palette_Range)
// uses this DRAWBUFF module's Buffer_* naming and this file's ColorXlat.
//
// Font file layout (all offsets relative to FontPtr, all block-offset fields are WORDs):
//   +4  infoblock offset   - infoblock[4] = MaxHeight, infoblock[5] = MaxWidth (unused here)
//   +6  offsetblock offset - WORD per character code: that character's pixel-data offset from FontPtr
//   +8  widthblock offset  - BYTE per character code: that character's pixel width
//   +12 heightblock offset - BYTE pair per character code: [0]=blank rows above the glyph,
//                            [1]=number of real data rows; MaxHeight-(these two) = blank rows below
//
// Glyph pixel data is nibble-packed, two pixels per byte (low nibble drawn first, then high). Each
// nibble is translated through the 256-byte ColorXlat table before being written: entries 0-15 are
// used for the low nibble (a direct 0-15 index), while the high nibble is looked up at
// (nibble << 4) - i.e. only the exact multiples of 16 in the table are ever read for that case,
// which is why the table's data below fills each 16-byte block with a single leading value and
// zeroes - both indexing schemes resolve to the SAME set of 16 logical entries. ColorXlat[0] is the
// background color and ColorXlat[1] (mirrored at ColorXlat[16]) is the foreground color, both
// overwritten per Buffer_Print call from its fcolor/bcolor arguments; entries 2-15 (and their
// mirrors at 32,48,...,240) are literal palette color indices, used for anti-aliased/shaded glyph
// edges. A translated color of exactly 0 means "transparent" (skip the pixel) - this is why
// Buffer_Print's caller passing bcolor=0 gives a see-through background, matching the color-0-is-
// transparent convention used throughout this codebase's blitters.

#include <gbuffer.h>
#include <font.h>
#include <stdint.h>

extern "C" unsigned char ColorXlat[256] = {
	0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F,
	0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x03, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x04, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x05, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x06, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x07, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x08, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x09, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x0A, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x0B, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x0C, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x0D, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x0E, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x0F, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
};

namespace {

enum {
	FONT_INFO_BLOCK = 4,
	FONT_OFFSET_BLOCK = 6,
	FONT_WIDTH_BLOCK = 8,
	FONT_HEIGHT_BLOCK = 12,
	FONT_INFO_MAX_HEIGHT = 4,
};

inline unsigned Read_Word(unsigned char const * p) { return p[0] | (p[1] << 8); }

}	// namespace

extern "C" long __cdecl Buffer_Print(void * thisptr, char const * string, int x_pixel, int y_pixel, int fcolor, int bcolor)
{
	if (!string) return 0;

	GraphicViewPortClass * viewport = (GraphicViewPortClass *)thisptr;
	int vpwidth = viewport->Get_Width();
	int vpheight = viewport->Get_Height();
	int bufferwidth = vpwidth + viewport->Get_XAdd() + viewport->Get_Pitch();

	// Offset alone is the complete pixel-data pointer - see CLEAR.CPP's comment. Adding
	// Get_Buffer() on top (the original mistake here) double-counts the pointer.
	unsigned char * curline = (unsigned char *)viewport->Get_Offset() + (long)y_pixel * bufferwidth;
	unsigned char * startdraw = curline + x_pixel;
	int original_x = x_pixel;

	unsigned char const * font = (unsigned char const *)FontPtr;
	if (!font) goto overflow;

	{
		unsigned char const * infoblock = font + Read_Word(font + FONT_INFO_BLOCK);
		unsigned char const * offsetblock = font + Read_Word(font + FONT_OFFSET_BLOCK);
		unsigned char const * widthblock = font + Read_Word(font + FONT_WIDTH_BLOCK);
		unsigned char const * heightblock = font + Read_Word(font + FONT_HEIGHT_BLOCK);

		unsigned char maxheight = infoblock[FONT_INFO_MAX_HEIGHT];
		if ((int)maxheight + y_pixel > vpheight) goto overflow;
		y_pixel += maxheight;

		ColorXlat[1] = (unsigned char)fcolor;
		ColorXlat[16] = (unsigned char)fcolor;
		ColorXlat[0] = (unsigned char)bcolor;

		for (;;) {
			unsigned char code = (unsigned char)*string++;
			if (code == 0) break;

			unsigned char * edi = startdraw;

			if (code == 10 || code == 13) goto line_feed;

			{
				unsigned char char_width = widthblock[code];
				int new_x = x_pixel + char_width + FontXSpacing;
				startdraw += char_width;
				startdraw += FontXSpacing;

				if (new_x > vpwidth) {
					string--;
					code = 0;
					goto line_feed;
				}

				x_pixel = new_x;
				int nextdraw = bufferwidth - char_width;

				unsigned data_offset = Read_Word(offsetblock + (unsigned)code * 2);
				unsigned char const * src = font + data_offset;

				unsigned char const * height_entry = heightblock + (unsigned)code * 2;
				unsigned char top_blank = height_entry[0];
				unsigned char char_height = height_entry[1];
				unsigned char bottom_blank = (unsigned char)(maxheight - (top_blank + char_height));

				if (top_blank != 0) {
					unsigned char bg = ColorXlat[0];
					if (bg != 0) {
						for (int row = 0; row < top_blank; row++) {
							for (int col = 0; col < char_width; col++) *edi++ = bg;
							edi += nextdraw;
						}
					} else {
						edi += (long)top_blank * bufferwidth;
					}
				}

				for (int row = 0; row < char_height; row++) {
					for (int col = 0; col < char_width; ) {
						unsigned char byte_val = *src++;
						unsigned char lo = byte_val & 0x0F;
						unsigned char hi = byte_val & 0xF0;

						unsigned char color = ColorXlat[lo];
						if (color != 0) *edi = color;
						edi++;
						col++;
						if (col >= char_width) break;

						color = ColorXlat[hi];
						if (color != 0) *edi = color;
						edi++;
						col++;
					}
					edi += nextdraw;
				}

				if (bottom_blank != 0) {
					unsigned char bg = ColorXlat[0];
					if (bg != 0) {
						for (int row = 0; row < bottom_blank; row++) {
							for (int col = 0; col < char_width; col++) *edi++ = bg;
							edi += nextdraw;
						}
					}
				}
			}
			continue;

line_feed:
			{
				int step = (int)maxheight + FontYSpacing;
				int new_y = y_pixel + step;
				if (new_y > vpheight) goto overflow;

				curline += (long)step * bufferwidth;
				y_pixel = new_y;

				int reset_x = (code == 10) ? 0 : original_x;
				x_pixel = reset_x;
				startdraw = curline + reset_x;
			}
			continue;
		}
	}

	// Callers only ever use this return value as a boolean/diagnostic "did it draw" signal (every
	// live call site discards it) - the original 32-bit ASM returned the raw destination pointer
	// as an integer, which cannot be preserved as a LONG on a 64-bit build; truncate rather than
	// widen the return type, since GBUFFER.H's own callers already declare it `unsigned long`.
	return (long)(intptr_t)startdraw;

overflow:
	return 0;
}

extern "C" void * __cdecl Get_Font_Palette_Ptr(void)
{
	return ColorXlat;
}
