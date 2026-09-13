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

// C++ replacement for SCALE.ASM's Linear_Scale_To_Linear (sub-milestone 1c -
// Agent/plans_done/code_1c_port_log.md).
//
// Two Cohen-Sutherland-style clip passes (source rect against the source viewport, carrying the
// adjustment proportionally into the destination rect; then destination rect against the
// destination viewport, carrying the adjustment proportionally back into the source rect - both
// proportional adjustments use the *original*, pre-clip src/dst position and size, matching the
// original code's reliance on its never-mutated ARG values), then a nearest-neighbor pixel sampler.
//
// The original's per-pixel/per-row stepping used a Bresenham-style fixed-point accumulator (a
// carry-into-the-source-pointer trick) to walk the source image at a constant src/dst ratio without
// per-pixel division. That accumulator is mathematically defined to land on exactly
// floor(k * src_size / dst_size) at step k (that's the whole point of the technique), so computing
// each row/column's source index directly with the same integer division reproduces identical pixel
// sampling with no risk of an off-by-one from mistranslating the carry logic.

#include <gbuffer.h>

namespace {

// Clips [lo0,lo1) against [0,bound); on either edge clipping, proportionally adjusts the paired
// [other0,other1) range using the *original* (pre-clip) lo_origin/lo_size -> other_origin/other_size
// ratio, matching both of the original's two clip passes (only the argument order differs between
// clipping source-against-src-viewport vs dest-against-dst-viewport). Returns false if the trivial
// reject/accept test already resolves the axis (accept leaves both ranges untouched).
bool Clip_Axis(int & lo0, int & lo1, int bound, int & other0, int & other1,
	int lo_origin, int lo_size, int other_origin, int other_size, int & code_lo, int & code_hi)
{
	code_lo = (lo0 < 0) ? 2 : 0;
	code_hi = (lo1 > bound) ? 1 : 0;
	if (!code_lo && !code_hi) return true;	// trivially inside, nothing to do

	if (code_lo) {
		other0 = other_origin + (int)(((long long)(0 - lo_origin) * other_size) / lo_size);
		lo0 = 0;
	}
	if (code_hi) {
		other1 = other_origin + (int)(((long long)(bound - lo_origin) * other_size) / lo_size);
		lo1 = bound;
	}
	return true;
}

}	// namespace

extern "C" BOOL __cdecl Linear_Scale_To_Linear(void * thisptr, void * dest_ptr,
	int src_x, int src_y, int dst_x, int dst_y,
	int src_width, int src_height, int dst_width, int dst_height,
	BOOL trans, char * remap)
{
	if (dst_width == 0 || dst_height == 0 || src_width == 0 || src_height == 0) return FALSE;

	GraphicViewPortClass * src_vp = (GraphicViewPortClass *)thisptr;
	GraphicViewPortClass * dst_vp = (GraphicViewPortClass *)dest_ptr;

	int src_x0 = src_x, src_y0 = src_y, src_x1 = src_x + src_width, src_y1 = src_y + src_height;
	int dst_x0 = dst_x, dst_y0 = dst_y, dst_x1 = dst_x + dst_width, dst_y1 = dst_y + dst_height;

	// Pass 1: clip source rect against the source viewport, adjusting the destination rect.
	{
		int xlo, xhi, ylo, yhi;
		Clip_Axis(src_x0, src_x1, src_vp->Get_Width(), dst_x0, dst_x1, src_x, src_width, dst_x, dst_width, xlo, xhi);
		Clip_Axis(src_y0, src_y1, src_vp->Get_Height(), dst_y0, dst_y1, src_y, src_height, dst_y, dst_height, ylo, yhi);
		int code0 = (xlo ? 8 : 0) | (ylo ? 2 : 0);
		int code1 = (xhi ? 4 : 0) | (yhi ? 1 : 0);
		if (code0 & code1) return FALSE;
	}

	// Pass 2: clip destination rect against the destination viewport, adjusting the source rect.
	{
		int xlo, xhi, ylo, yhi;
		Clip_Axis(dst_x0, dst_x1, dst_vp->Get_Width(), src_x0, src_x1, dst_x, dst_width, src_x, src_width, xlo, xhi);
		Clip_Axis(dst_y0, dst_y1, dst_vp->Get_Height(), src_y0, src_y1, dst_y, dst_height, src_y, src_height, ylo, yhi);
		int code0 = (xlo ? 8 : 0) | (ylo ? 2 : 0);
		int code1 = (xhi ? 4 : 0) | (yhi ? 1 : 0);
		if (code0 & code1) return FALSE;
	}

	int count_x = dst_x1 - dst_x0;
	int count_y = dst_y1 - dst_y0;
	if (count_x <= 0 || count_y <= 0) return FALSE;

	int src_stride = src_vp->Get_Width() + src_vp->Get_XAdd() + src_vp->Get_Pitch();
	int dst_stride = dst_vp->Get_Width() + dst_vp->Get_XAdd() + dst_vp->Get_Pitch();
	unsigned char const * src_base = (unsigned char const *)src_vp->Get_Graphic_Buffer()->Get_Buffer() + src_vp->Get_Offset();
	unsigned char * dst_base = (unsigned char *)dst_vp->Get_Graphic_Buffer()->Get_Buffer() + dst_vp->Get_Offset();
	unsigned char const * remap_table = (unsigned char const *)remap;

	for (int row = 0; row < count_y; row++) {
		int src_row = src_y0 + (int)(((long long)row * src_height) / dst_height);
		unsigned char const * src_line = src_base + (long)src_row * src_stride;
		unsigned char * dst_line = dst_base + (long)(dst_y0 + row) * dst_stride + dst_x0;

		for (int col = 0; col < count_x; col++) {
			int src_col = src_x0 + (int)(((long long)col * src_width) / dst_width);
			unsigned char pixel = src_line[src_col];

			if (trans && pixel == 0) continue;
			dst_line[col] = remap_table ? remap_table[pixel] : pixel;
		}
	}

	return TRUE;
}
