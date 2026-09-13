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

// C++ replacement for FILLRECT.ASM (sub-milestone 1c - Agent/plans_done/code_1c_port_log.md):
// normalizes the two corner points, clips the resulting rectangle to the viewport, and fills each
// row with memset. See GETPIX.CPP's comment for the Width+XAdd+Pitch stride convention.

#include <gbuffer.h>
#include <string.h>
#include <algorithm>

extern "C" void __cdecl Buffer_Fill_Rect(void * thisptr, int x1, int y1, int x2, int y2, unsigned char color)
{
	GraphicViewPortClass * viewport = (GraphicViewPortClass *)thisptr;

	if (x1 > x2) std::swap(x1, x2);
	if (y1 > y2) std::swap(y1, y2);

	int width = viewport->Get_Width();
	int height = viewport->Get_Height();

	if (x1 < 0) x1 = 0;
	if (y1 < 0) y1 = 0;
	if (x2 >= width) x2 = width - 1;
	if (y2 >= height) y2 = height - 1;
	if (x1 > x2 || y1 > y2) return;

	unsigned char * base = (unsigned char *)viewport->Get_Graphic_Buffer()->Get_Buffer() + viewport->Get_Offset();
	int stride = width + viewport->Get_XAdd() + viewport->Get_Pitch();
	int fill_width = x2 - x1 + 1;

	for (int y = y1; y <= y2; y++) {
		memset(base + (long)y * stride + x1, color, fill_width);
	}
}
