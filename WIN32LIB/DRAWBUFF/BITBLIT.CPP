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

// C++ replacement for BITBLIT.ASM (sub-milestone 1c - Agent/plans_done/code_1c_port_log.md): a
// Cohen-Sutherland-style clip of the source rect against the source viewport (carrying the
// adjustment into the destination rect), then a second clip of the destination rect against the
// destination viewport (carrying the adjustment back into the source rect), then the actual copy.
//
// The original hand-rolled a full forward/backward byte-direction choice (comparing source and
// destination linear offsets) to stay correct when blitting a viewport onto an overlapping region
// of itself. Replicated here as a single up-front direction choice (row order, not per-instruction
// direction) followed by per-row memmove, which is the standard-library-safe equivalent - memmove
// already handles any remaining intra-row overlap, and processing whole rows in the correct
// top-to-bottom or bottom-to-top order handles the cross-row case the same way the original's
// esi-vs-edi comparison did.
//
// See GETPIX.CPP's comment for the Width+XAdd+Pitch stride convention.

#include <gbuffer.h>
#include <string.h>

extern "C" BOOL __cdecl Linear_Blit_To_Linear(void * thisptr, void * dest_ptr, int x_pixel, int y_pixel,
	int dest_x0, int dest_y0, int pixel_width, int pixel_height, BOOL trans)
{
	GraphicViewPortClass * src_vp = (GraphicViewPortClass *)thisptr;
	GraphicViewPortClass * dst_vp = (GraphicViewPortClass *)dest_ptr;

	int src_width = src_vp->Get_Width();
	int src_height = src_vp->Get_Height();

	int src_x0 = x_pixel, src_y0 = y_pixel;
	int src_x1 = x_pixel + pixel_width;
	int src_y1 = y_pixel + pixel_height;
	int dst_x0 = dest_x0, dst_y0 = dest_y0;

	// Clip source rect to source viewport; carry the adjustment into the destination rect.
	if (src_x0 < 0) { dst_x0 -= src_x0; src_x0 = 0; }
	if (src_y0 < 0) { dst_y0 -= src_y0; src_y0 = 0; }
	if (src_x1 > src_width) src_x1 = src_width;
	if (src_y1 > src_height) src_y1 = src_height;
	if (src_x0 >= src_x1 || src_y0 >= src_y1) return FALSE;

	int dst_x1 = dst_x0 + (src_x1 - src_x0);
	int dst_y1 = dst_y0 + (src_y1 - src_y0);

	// Clip destination rect to destination viewport; carry the adjustment back into the source rect.
	int dst_width = dst_vp->Get_Width();
	int dst_height = dst_vp->Get_Height();
	if (dst_x0 < 0) { src_x0 -= dst_x0; dst_x0 = 0; }
	if (dst_y0 < 0) { src_y0 -= dst_y0; dst_y0 = 0; }
	if (dst_x1 > dst_width) dst_x1 = dst_width;
	if (dst_y1 > dst_height) dst_y1 = dst_height;

	int copy_width = dst_x1 - dst_x0;
	int copy_height = dst_y1 - dst_y0;
	if (copy_width <= 0 || copy_height <= 0) return FALSE;

	// Offset alone is the complete pixel-data pointer - see CLEAR.CPP's comment. Adding
	// Get_Buffer() on top (the original mistake here) double-counts the pointer.
	unsigned char * src_base = (unsigned char *)src_vp->Get_Offset();
	int src_stride = src_width + src_vp->Get_XAdd() + src_vp->Get_Pitch();
	unsigned char * dst_base = (unsigned char *)dst_vp->Get_Offset();
	int dst_stride = dst_width + dst_vp->Get_XAdd() + dst_vp->Get_Pitch();

	unsigned char * src_first_row = src_base + (long)src_y0 * src_stride + src_x0;
	unsigned char * dst_first_row = dst_base + (long)dst_y0 * dst_stride + dst_x0;

	if (trans) {
		// Color 0 is the transparent key - skip it, copy everything else.
		unsigned char * src_row = src_first_row;
		unsigned char * dst_row = dst_first_row;
		for (int y = 0; y < copy_height; y++) {
			for (int x = 0; x < copy_width; x++) {
				unsigned char pixel = src_row[x];
				if (pixel) dst_row[x] = pixel;
			}
			src_row += src_stride;
			dst_row += dst_stride;
		}
	} else if (dst_first_row > src_first_row) {
		// Destination comes after source in memory (e.g. scrolling a viewport down within the
		// same underlying surface) - copy bottom row first so a row's read always happens before
		// any later row's write could have clobbered it.
		for (int y = copy_height - 1; y >= 0; y--) {
			memmove(dst_first_row + (long)y * dst_stride, src_first_row + (long)y * src_stride, copy_width);
		}
	} else {
		for (int y = 0; y < copy_height; y++) {
			memmove(dst_first_row + (long)y * dst_stride, src_first_row + (long)y * src_stride, copy_width);
		}
	}

	return TRUE;
}
