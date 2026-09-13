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

// C++ replacement for TOBUFF.ASM (sub-milestone 1c - Agent/plans_done/code_1c_port_log.md). The
// original's TRANSP-guarded color-key path is compiled out (TRANSP equ 0 in the source), so only
// the plain forward-copy path is ported. See GETPIX.CPP's comment for the Width+XAdd+Pitch stride
// convention. dest's own row stride is the caller's requested pixel_width, not the clipped width -
// clipping only shrinks which pixels get copied and where they land within that fixed-stride buffer.

#include <gbuffer.h>
#include <string.h>

extern "C" long __cdecl Buffer_To_Buffer(void * thisptr, int x_pixel, int y_pixel, int pixel_width, int pixel_height, void * dest, long buffer_size)
{
	GraphicViewPortClass * viewport = (GraphicViewPortClass *)thisptr;
	int vp_width = viewport->Get_Width();
	int vp_height = viewport->Get_Height();

	int dest_x0 = 0, dest_y0 = 0;
	int src_x0 = x_pixel, src_y0 = y_pixel;
	int src_x1 = x_pixel + pixel_width;
	int src_y1 = y_pixel + pixel_height;

	if (src_x0 < 0) { dest_x0 = -src_x0; src_x0 = 0; }
	if (src_y0 < 0) { dest_y0 = -src_y0; src_y0 = 0; }
	if (src_x1 > vp_width) src_x1 = vp_width;
	if (src_y1 > vp_height) src_y1 = vp_height;

	int copy_width = src_x1 - src_x0;
	int copy_height = src_y1 - src_y0;
	if (copy_width <= 0 || copy_height <= 0) return 0;

	if ((long)pixel_width * copy_height > buffer_size) return 0;

	// Offset alone is the complete pixel-data pointer - see CLEAR.CPP's comment. Adding
	// Get_Buffer() on top (the original mistake here) double-counts the pointer.
	unsigned char const * src_base = (unsigned char const *)viewport->Get_Offset();
	int src_stride = vp_width + viewport->Get_XAdd() + viewport->Get_Pitch();

	unsigned char * dst = (unsigned char *)dest + (long)dest_y0 * pixel_width + dest_x0;

	for (int row = 0; row < copy_height; row++) {
		memcpy(dst, src_base + (long)(src_y0 + row) * src_stride + src_x0, copy_width);
		dst += pixel_width;
	}

	return 1;
}
