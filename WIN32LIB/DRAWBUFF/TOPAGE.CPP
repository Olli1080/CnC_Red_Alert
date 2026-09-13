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

// C++ replacement for TOPAGE.ASM (sub-milestone 1c - Agent/plans_done/code_1c_port_log.md) - the
// mirror image of TOBUFF.CPP (linear buffer -> viewport instead of viewport -> linear buffer).
// Same TRANSP-compiled-out note applies. See GETPIX.CPP's comment for the stride convention.

#include <gbuffer.h>
#include <string.h>

extern "C" long __cdecl Buffer_To_Page(int x_pixel, int y_pixel, int pixel_width, int pixel_height, void * src, void * dest)
{
	if (!src) return 0;

	GraphicViewPortClass * viewport = (GraphicViewPortClass *)dest;
	int vp_width = viewport->Get_Width();
	int vp_height = viewport->Get_Height();

	int src_x0 = 0, src_y0 = 0;
	int dst_x0 = x_pixel, dst_y0 = y_pixel;
	int dst_x1 = x_pixel + pixel_width;
	int dst_y1 = y_pixel + pixel_height;

	if (dst_x0 < 0) { src_x0 = -dst_x0; dst_x0 = 0; }
	if (dst_y0 < 0) { src_y0 = -dst_y0; dst_y0 = 0; }
	if (dst_x1 > vp_width) dst_x1 = vp_width;
	if (dst_y1 > vp_height) dst_y1 = vp_height;

	int copy_width = dst_x1 - dst_x0;
	int copy_height = dst_y1 - dst_y0;
	if (copy_width <= 0 || copy_height <= 0) return 0;

	// Offset alone is the complete pixel-data pointer - see CLEAR.CPP's comment. Adding
	// Get_Buffer() on top (the original mistake here) double-counts the pointer.
	unsigned char * dst_base = (unsigned char *)viewport->Get_Offset();
	int dst_stride = vp_width + viewport->Get_XAdd() + viewport->Get_Pitch();
	unsigned char const * src_ptr = (unsigned char const *)src + (long)src_y0 * pixel_width + src_x0;

	for (int row = 0; row < copy_height; row++) {
		memcpy(dst_base + (long)(dst_y0 + row) * dst_stride + dst_x0, src_ptr, copy_width);
		src_ptr += pixel_width;
	}

	return 1;
}
