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

// C++ replacement for CLEAR.ASM (sub-milestone 1c - Agent/plans_done/code_1c_port_log.md). The
// original hand-rolled DWORD alignment for the memset loop is left to the C runtime's own memset,
// which already does this. See GETPIX.CPP's comment for the Width+XAdd+Pitch stride convention.

#include <gbuffer.h>
#include <string.h>

extern "C" void __cdecl Buffer_Clear(void * thisptr, unsigned char color)
{
	GraphicViewPortClass * viewport = (GraphicViewPortClass *)thisptr;

	unsigned char * dest = (unsigned char *)viewport->Get_Graphic_Buffer()->Get_Buffer() + viewport->Get_Offset();
	int width = viewport->Get_Width();
	int stride = width + viewport->Get_XAdd() + viewport->Get_Pitch();
	int height = viewport->Get_Height();

	for (int row = 0; row < height; row++) {
		memset(dest, color, width);
		dest += stride;
	}
}
