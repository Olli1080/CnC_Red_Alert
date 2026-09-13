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

// C++ replacement for REMAP.ASM (sub-milestone 1c - Agent/plans_done/code_1c_port_log.md): clips
// the region to the viewport, then replaces each pixel in the region with remap_table[pixel] (a
// 256-entry palette-index lookup table, e.g. for player-color remapping). See GETPIX.CPP's comment
// for the Width+XAdd+Pitch stride convention.

#include <gbuffer.h>

extern "C" void __cdecl Buffer_Remap(void * thisptr, int x0, int y0, int width, int height, void * remap_table)
{
	if (!remap_table) return;

	GraphicViewPortClass * viewport = (GraphicViewPortClass *)thisptr;
	int vp_width = viewport->Get_Width();
	int vp_height = viewport->Get_Height();

	int x1 = x0 + width;
	int y1 = y0 + height;

	if (x0 < 0) x0 = 0;
	if (y0 < 0) y0 = 0;
	if (x1 > vp_width) x1 = vp_width;
	if (y1 > vp_height) y1 = vp_height;
	if (x0 >= x1 || y0 >= y1) return;

	unsigned char * base = (unsigned char *)viewport->Get_Graphic_Buffer()->Get_Buffer() + viewport->Get_Offset();
	int stride = vp_width + viewport->Get_XAdd() + viewport->Get_Pitch();
	unsigned char const * table = (unsigned char const *)remap_table;

	for (int y = y0; y < y1; y++) {
		unsigned char * row = base + (long)y * stride;
		for (int x = x0; x < x1; x++) {
			row[x] = table[row[x]];
		}
	}
}
