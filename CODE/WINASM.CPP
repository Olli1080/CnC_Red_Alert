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

// C++ replacement for the still-live symbols in WINASM.ASM (sub-milestone 1c -
// Agent/plans_done/code_1c_port_log.md). Everything else in that file (FastGetPortHardware_,
// HMWaitForOK_, PortOpenGreenleafFast_, etc.) is dead Greenleaf Communications Library glue for the
// modem/null-modem feature already excluded elsewhere in this port (see CODE/CMakeLists.txt's
// NULLMGR.CPP note) - those symbols are declared with a raw trailing-underscore label (no "C"
// qualifier), which is a different, never-actually-linked name from what any live caller uses.
//
// Stop_Execution's original body was `nop; ret` - already a no-op.
// Set_Palette_Register only has one call site (WINSTUB.CPP's Colour_Debug), which nothing in this
// codebase calls (dead debug helper) - implemented for real anyway via the existing DirectDraw
// palette path (PAL.CPP's CurrentPalette/Set_DD_Palette) rather than the original's raw VGA DAC
// port I/O, which is impossible in Win32 usermode regardless of toolchain (same reasoning as the
// outportb/outport stubs in JSHELL.H/RGB.H).

#include <string.h>

extern "C" unsigned char CurrentPalette[768];
extern "C" void Set_DD_Palette(void * palette);

extern "C" void __cdecl Stop_Execution(void)
{
}

extern "C" void __cdecl Set_Palette_Register(int number, int red, int green, int blue)
{
	if ((unsigned)number >= 256) return;

	CurrentPalette[number * 3 + 0] = (unsigned char)((red & 0x3F) * 4);
	CurrentPalette[number * 3 + 1] = (unsigned char)((green & 0x3F) * 4);
	CurrentPalette[number * 3 + 2] = (unsigned char)((blue & 0x3F) * 4);
	Set_DD_Palette(CurrentPalette);
}

// ModeX_Blit copied a 320x200 buffer to a DOS/Win95 "Mode X" planar VGA screen - a hardware video
// mode that has no equivalent under DirectDraw (this build's only rendering path) and is only ever
// called from GSCREEN.CPP's Blit_Display() in the branch where the display width is exactly 320,
// which this modernized build's DirectDraw-managed viewport never produces. No-op rather than
// reconstructed.
extern "C" void __cdecl ModeX_Blit(void * source)
{
	(void)source;
}

// Asm_Interpolate/Asm_Interpolate_Line_Double/Asm_Interpolate_Line_Interpolate (originally
// WINASM.ASM) implement INTERPAL.CPP's Interpolate_2X_Scale (320x200-to-640x400-style UI background
// scaling for MAPSEL.CPP/SCORE.CPP, unrelated to movie playback). All three share the same
// horizontal-interpolation core: each destination row is twice the source row's width, built by
// looking up PaletteInterpolationTable[a][b] (already populated by INTERPAL.CPP's
// Create_Palette_Interpolation_Table) for the color "halfway between" every adjacent source pixel
// pair, so that a source pixel and a blended pixel alternate in the output; the very last output
// pixel in a row is forced to 0 since there is no further pixel to blend toward.
//
// The three variants differ only in how they fill the SECOND destination row that corresponds to
// each source row: Asm_Interpolate doesn't fill it at all (dest_width, precomputed by the caller as
// 2x the real per-row stride, skips straight over it - only used where CopyType is never actually
// set to the case that calls this, so the gap is harmless in practice); Line_Double duplicates the
// same interpolated row into it (vertical nearest-neighbor); Line_Interpolate (never actually
// reached - CopyType is only ever 0 or 1 anywhere in this codebase - ported anyway for completeness)
// blends vertically between consecutive interpolated rows to synthesize a genuinely new one.

extern "C" unsigned char PaletteInterpolationTable[256][256];

namespace {

void Interpolate_Row(unsigned char const * src, unsigned char * dst, int width)
{
	unsigned char const * s = src;
	int pairs = (width - 2) / 2;

	for (int i = 0; i < pairs; i++) {
		unsigned char p0 = s[0], p1 = s[1], p2 = s[2];
		*dst++ = p0;
		*dst++ = PaletteInterpolationTable[p0][p1];
		*dst++ = p1;
		*dst++ = PaletteInterpolationTable[p1][p2];
		s += 2;
	}

	unsigned char pl0 = s[0], pl1 = s[1];
	*dst++ = pl0;
	*dst++ = PaletteInterpolationTable[pl0][pl1];
	*dst++ = pl1;
	*dst++ = 0;
}

void Interpolate_Between_Lines(unsigned char const * line1, unsigned char const * line2, unsigned char * dest, int width_bytes)
{
	for (int i = 0; i < width_bytes; i++) {
		dest[i] = PaletteInterpolationTable[line1[i]][line2[i]];
	}
}

}	// namespace

extern "C" void __cdecl Asm_Interpolate(unsigned char * src_ptr, unsigned char * dest_ptr, int source_height, int source_width, int dest_width)
{
	unsigned char * old_dest = dest_ptr;
	unsigned char const * src = src_ptr;

	for (int line = 0; line < source_height; line++) {
		Interpolate_Row(src, old_dest, source_width);
		src += source_width;
		old_dest += dest_width;
	}
}

extern "C" void __cdecl Asm_Interpolate_Line_Double(unsigned char * src_ptr, unsigned char * dest_ptr, int source_height, int source_width, int dest_width)
{
	unsigned char * old_dest = dest_ptr;
	unsigned char const * src = src_ptr;
	int row_stride = dest_width / 2;
	int out_width = 2 * source_width;

	for (int line = 0; line < source_height; line++) {
		Interpolate_Row(src, old_dest, source_width);
		memcpy(old_dest + row_stride, old_dest, out_width);
		src += source_width;
		old_dest += dest_width;
	}
}

extern "C" void __cdecl Asm_Interpolate_Line_Interpolate(unsigned char * src_ptr, unsigned char * dest_ptr, int source_lines, int source_width, int dest_width)
{
	int row_stride = dest_width / 2;
	int out_width = 2 * source_width;

	unsigned char * top_line = new unsigned char[out_width];
	unsigned char * bottom_line = new unsigned char[out_width];
	unsigned char * line_buffer = new unsigned char[out_width];
	unsigned char * next_line = top_line;
	unsigned char * last_line = bottom_line;

	unsigned char const * src = src_ptr;
	unsigned char * old_dest = dest_ptr;

	Interpolate_Row(src, next_line, source_width);
	src += source_width;
	{ unsigned char * t = next_line; next_line = last_line; last_line = t; }
	source_lines--;

	while (source_lines > 0) {
		Interpolate_Row(src, next_line, source_width);
		Interpolate_Between_Lines(last_line, next_line, line_buffer, out_width);

		memcpy(old_dest, last_line, out_width);
		memcpy(old_dest + row_stride, line_buffer, out_width);
		old_dest += dest_width;

		src += source_width;
		{ unsigned char * t = next_line; next_line = last_line; last_line = t; }
		source_lines--;
	}

	Interpolate_Row(src, next_line, source_width);
	memcpy(old_dest, next_line, out_width);

	delete[] top_line;
	delete[] bottom_line;
	delete[] line_buffer;
}
