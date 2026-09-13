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

// C++ replacement for 2KEYFBUF.ASM's Buffer_Frame_To_Page (sub-milestone 1c -
// Agent/plans_done/code_1c_port_log.md).
//
// The original is a ~3900-line hand-optimized blitter with a huge jump-table of per-flag-
// combination inner loops (normal/transparent/ghost/fading/predator, each with its own MMX and
// clip-vs-noclip variants) plus a "new" (ShapeHeaderType-cache) and "old" (raw-buffer) source
// format. This port is scoped to exactly what this codebase's only call sites (all four inside
// CONQUER.CPP's CC_Draw_Shape "special shadow drawing code used for aircraft and bullets") actually
// exercise: SHAPE_TRANS is always requested, combined with one of GHOST+FADING, FADING alone,
// PREDATOR alone, or GHOST alone/neither - never PREDATOR combined with GHOST or FADING. Every
// other flag combination the original supported (plain opaque draws, PARTIAL, non-transparent
// ghost/fading, etc.) has no caller in this GPL release and is not implemented.
//
// Source format: `Buffer` is whatever CC_Draw_Shape's Build_Frame call returned. When
// UseOldShapeDraw is set (rotated/scaled shapes - CONQUER.CPP sets this itself before calling in
// that case) it's already a flat, decoded pixel buffer. Otherwise it's a ShapeHeaderType* into the
// big pre-decompressed shape cache, exactly like the "new system" branch of the original -
// resolved here by calling the already-correct, already-compiled Get_Shape_Header_Data (2KEYFRAM.CPP)
// rather than re-deriving that pointer arithmetic (which involves a cache-base pointer truncated to
// 32 bits elsewhere in this codebase - not something to silently "fix" as a side effect of this port).
//
// Effects (per confirmed call site data):
//   TRANS    - source pixel value 0 means transparent: leave the destination pixel untouched.
//   GHOST    - `ghostdata` is a 512-byte table (IsTranslucent[256] + Translucent[256], see
//              DisplayClass::SpecialGhost): if IsTranslucent[src_pixel] == 0xFF, draw the shape's
//              own pixel; otherwise replace the *destination* pixel with
//              Translucent[current_dest_pixel] - every real caller uses SpecialGhost, whose
//              IsTranslucent half is all zero, so this always takes the "replace with
//              Translucent[dest]" branch (a same-color shadow silhouette effect), but the general
//              form is implemented rather than hard-coding that.
//   FADING   - `fadingtable` (256 bytes) applied to the source pixel `fadingnum` times in a row
//              (every call site passes fadingnum=1, i.e. one lookup).
//   PREDATOR - a Bresenham-style dither: an accumulator advances by a fixed step each opaque pixel,
//              and whenever it overflows (255 times out of 256, given this codebase's calls never
//              set SHAPE_PARTIAL to change that step) the pixel is replaced with a *destination*
//              pixel read from a small, cyclically-advancing forward offset - the "cloak shimmer"
//              look. The original also supports a negative-offset table variant reached by an
//              obscure address-arithmetic trick keyed off the sign of the caller's offset argument;
//              this port always cycles through the same small positive offset table regardless of
//              sign, which still produces a correct, non-crashing shimmer pattern - the effect is
//              purely cosmetic (a cloaking flicker), not gameplay-affecting, so an approximate but
//              safe pattern was chosen over risking a wild pointer read from mis-replicating that
//              trick.

#include "gbuffer.h"
#include <stdarg.h>
#include <string.h>

extern "C" bool UseOldShapeDraw;
extern void * Get_Shape_Header_Data(void * ptr);

namespace {

enum {
	SHAPE_CENTER = 0x0020,
	SHAPE_TRANS = 0x0040,
	SHAPE_FADING = 0x0100,
	SHAPE_PREDATOR = 0x0200,
	SHAPE_GHOST = 0x1000,
};

short const BFPredTable[8] = { 1, 3, 2, 5, 2, 3, 4, 1 };
int BFPredOffset = 0;
unsigned int BFPartialCount = 0;

void Draw_Row_Trans(unsigned char const * src, unsigned char * dst, int count)
{
	for (int i = 0; i < count; i++) {
		if (src[i] != 0) dst[i] = src[i];
	}
}

void Draw_Row_Ghost_Trans(unsigned char const * src, unsigned char * dst, int count,
	unsigned char const * is_translucent, unsigned char const * translucent)
{
	for (int i = 0; i < count; i++) {
		unsigned char pixel = src[i];
		if (pixel == 0) continue;

		unsigned char cat = is_translucent[pixel];
		dst[i] = (cat == 0xFF) ? pixel : translucent[(size_t)cat * 256 + dst[i]];
	}
}

void Draw_Row_Fading_Trans(unsigned char const * src, unsigned char * dst, int count,
	unsigned char const * fading_table, int fading_num)
{
	for (int i = 0; i < count; i++) {
		unsigned char pixel = src[i];
		if (pixel == 0) continue;

		for (int f = 0; f < fading_num; f++) pixel = fading_table[pixel];
		dst[i] = pixel;
	}
}

void Draw_Row_Ghost_Fading_Trans(unsigned char const * src, unsigned char * dst, int count,
	unsigned char const * is_translucent, unsigned char const * translucent,
	unsigned char const * fading_table, int fading_num)
{
	for (int i = 0; i < count; i++) {
		unsigned char pixel = src[i];
		if (pixel == 0) continue;

		unsigned char cat = is_translucent[pixel];
		unsigned char value = (cat == 0xFF) ? pixel : translucent[(size_t)cat * 256 + dst[i]];
		for (int f = 0; f < fading_num; f++) value = fading_table[value];
		dst[i] = value;
	}
}

void Draw_Row_Predator_Trans(unsigned char const * src, unsigned char * dst, int count)
{
	for (int i = 0; i < count; i++) {
		if (src[i] == 0) continue;

		BFPartialCount += 0x100;
		if ((BFPartialCount & 0xFF00) == 0) continue;	// rare: dither says "leave transparent"

		int word_offset = BFPredTable[(BFPredOffset >> 1) & 7];
		BFPredOffset += 2;
		dst[i] = dst[i + word_offset];
	}
}

}	// namespace

extern "C" long __cdecl Buffer_Frame_To_Page(int x_pixel, int y_pixel, int pixel_width, int pixel_height,
	void * Buffer, GraphicViewPortClass & view, int flags, ...)
{
	if (!Buffer) return 0;

	if (flags & SHAPE_CENTER) {
		x_pixel -= pixel_width >> 1;
		y_pixel -= pixel_height >> 1;
	}

	va_list args;
	va_start(args, flags);

	unsigned char const * ghost_is_translucent = NULL;
	unsigned char const * ghost_translucent = NULL;
	if (flags & SHAPE_GHOST) {
		unsigned char const * table = (unsigned char const *)va_arg(args, void *);
		ghost_is_translucent = table;
		ghost_translucent = table + 256;
	}

	unsigned char const * fading_table = NULL;
	int fading_num = 0;
	if (flags & SHAPE_FADING) {
		fading_table = (unsigned char const *)va_arg(args, void *);
		fading_num = va_arg(args, int) & 0x3F;
		if (fading_num == 0) flags &= ~SHAPE_FADING;
	}

	if (flags & SHAPE_PREDATOR) {
		int predoffset = va_arg(args, int);
		BFPredOffset = (predoffset < 0 ? -predoffset : predoffset) * 2 & 0x0E;
		BFPartialCount = 0;
	}

	va_end(args);

	unsigned char const * pixel_data = UseOldShapeDraw
		? (unsigned char const *)Buffer
		: (unsigned char const *)Get_Shape_Header_Data(Buffer);

	// Clip the destination rect against the viewport (same Cohen-Sutherland-derived pattern used
	// throughout this port's other blitters); only the near edge needs a source-pointer
	// adjustment when clipped, since leaving the far edge untouched already yields the correct
	// reduced width/height once subtracted from the (now-zeroed) near edge.
	int x0 = x_pixel, y0 = y_pixel;
	int x1 = x_pixel + pixel_width, y1 = y_pixel + pixel_height;
	int vp_width = view.Get_Width();
	int vp_height = view.Get_Height();

	bool reject = (x0 < 0 && x1 < 0) || (x0 > vp_width && x1 > vp_width) ||
		(y0 < 0 && y1 < 0) || (y0 > vp_height && y1 > vp_height);
	if (reject) return 0;

	int src_x = 0, src_y = 0;
	if (x0 < 0) { src_x = -x0; x0 = 0; }
	if (y0 < 0) { src_y = -y0; y0 = 0; }
	if (x1 > vp_width) x1 = vp_width;
	if (y1 > vp_height) y1 = vp_height;

	int copy_width = x1 - x0;
	int copy_height = y1 - y0;
	if (copy_width <= 0 || copy_height <= 0) return 0;

	int dst_stride = view.Get_Width() + view.Get_XAdd() + view.Get_Pitch();
	// Offset alone is the complete pixel-data pointer - see WIN32LIB/DRAWBUFF/CLEAR.CPP's comment.
	// Adding Get_Buffer() on top (the original mistake here) double-counts the pointer.
	unsigned char * dst_row = (unsigned char *)view.Get_Offset() + (long)y0 * dst_stride + x0;
	unsigned char const * src_row = pixel_data + (long)src_y * pixel_width + src_x;

	bool trans = (flags & SHAPE_TRANS) != 0;
	bool ghost = (flags & SHAPE_GHOST) != 0;
	bool fading = (flags & SHAPE_FADING) != 0;
	bool predator = (flags & SHAPE_PREDATOR) != 0;

	for (int row = 0; row < copy_height; row++) {
		if (!trans) {
			memcpy(dst_row, src_row, copy_width);
		} else if (ghost && fading) {
			Draw_Row_Ghost_Fading_Trans(src_row, dst_row, copy_width, ghost_is_translucent, ghost_translucent, fading_table, fading_num);
		} else if (ghost) {
			Draw_Row_Ghost_Trans(src_row, dst_row, copy_width, ghost_is_translucent, ghost_translucent);
		} else if (fading) {
			Draw_Row_Fading_Trans(src_row, dst_row, copy_width, fading_table, fading_num);
		} else if (predator) {
			Draw_Row_Predator_Trans(src_row, dst_row, copy_width);
		} else {
			Draw_Row_Trans(src_row, dst_row, copy_width);
		}

		src_row += pixel_width;
		dst_row += dst_stride;
	}

	return 1;
}
