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

// C++ replacement for PUTPIX.ASM (sub-milestone 1c - Agent/plans_done/code_1c_port_log.md). See
// GETPIX.CPP's comment for the Width+XAdd+Pitch stride convention.

#include <gbuffer.h>

extern "C" void __cdecl Buffer_Put_Pixel(void * thisptr, int x_pixel, int y_pixel, unsigned char color)
{
	GraphicViewPortClass * viewport = (GraphicViewPortClass *)thisptr;

	if ((unsigned)x_pixel >= (unsigned)viewport->Get_Width()) return;
	if ((unsigned)y_pixel >= (unsigned)viewport->Get_Height()) return;

	unsigned char * base = (unsigned char *)viewport->Get_Graphic_Buffer()->Get_Buffer() + viewport->Get_Offset();
	int stride = viewport->Get_Width() + viewport->Get_XAdd() + viewport->Get_Pitch();

	base[(long)y_pixel * stride + x_pixel] = color;
}
