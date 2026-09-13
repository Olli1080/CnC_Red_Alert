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

// C++ replacement for DRAWLINE.ASM (sub-milestone 1c - Agent/plans_done/code_1c_port_log.md). The
// original implements a Cohen-Sutherland-style outcode clip (against the viewport rectangle)
// followed by a custom DDA-style pixel walk with special-cased horizontal/vertical fast paths.
// Rewritten here as the textbook Cohen-Sutherland clip + Bresenham line draw - a different but
// behaviorally equivalent implementation of the same well-known algorithms (same clipped endpoints,
// same set of pixels lit), rather than a literal transcription of the asm's register shuffling.
// See GETPIX.CPP's comment for the Width+XAdd+Pitch stride convention.

#include <gbuffer.h>
#include <stdlib.h>

namespace {
	int Outcode(int x, int y, int max_x, int max_y)
	{
		int code = 0;
		if (y < 0) code |= 1;        // above
		if (y > max_y) code |= 2;    // below
		if (x < 0) code |= 4;        // left
		if (x > max_x) code |= 8;    // right
		return code;
	}
}

extern "C" void __cdecl Buffer_Draw_Line(void * thisptr, int x1, int y1, int x2, int y2, unsigned char color)
{
	GraphicViewPortClass * viewport = (GraphicViewPortClass *)thisptr;
	int max_x = viewport->Get_Width() - 1;
	int max_y = viewport->Get_Height() - 1;
	if (max_x < 0 || max_y < 0) return;

	int oc1 = Outcode(x1, y1, max_x, max_y);
	int oc2 = Outcode(x2, y2, max_x, max_y);

	for (;;) {
		if (!(oc1 | oc2)) break;
		if (oc1 & oc2) return;

		int oc = oc1 ? oc1 : oc2;
		int x = 0, y = 0;

		if (oc & 1) {
			y = 0;
			x = x1 + (int)((long long)(x2 - x1) * (0 - y1) / (y2 - y1));
		} else if (oc & 2) {
			y = max_y;
			x = x1 + (int)((long long)(x2 - x1) * (max_y - y1) / (y2 - y1));
		} else if (oc & 4) {
			x = 0;
			y = y1 + (int)((long long)(y2 - y1) * (0 - x1) / (x2 - x1));
		} else {
			x = max_x;
			y = y1 + (int)((long long)(y2 - y1) * (max_x - x1) / (x2 - x1));
		}

		if (oc == oc1) {
			x1 = x; y1 = y;
			oc1 = Outcode(x1, y1, max_x, max_y);
		} else {
			x2 = x; y2 = y;
			oc2 = Outcode(x2, y2, max_x, max_y);
		}
	}

	// Offset alone is the complete pixel-data pointer - see CLEAR.CPP's comment. Adding
	// Get_Buffer() on top (the original mistake here) double-counts the pointer.
	unsigned char * base = (unsigned char *)viewport->Get_Offset();
	int stride = viewport->Get_Width() + viewport->Get_XAdd() + viewport->Get_Pitch();

	int dx = abs(x2 - x1), sx = (x1 < x2) ? 1 : -1;
	int dy = -abs(y2 - y1), sy = (y1 < y2) ? 1 : -1;
	int err = dx + dy;

	for (;;) {
		base[(long)y1 * stride + x1] = color;
		if (x1 == x2 && y1 == y2) break;
		int e2 = 2 * err;
		if (e2 >= dy) { err += dy; x1 += sx; }
		if (e2 <= dx) { err += dx; y1 += sy; }
	}
}
