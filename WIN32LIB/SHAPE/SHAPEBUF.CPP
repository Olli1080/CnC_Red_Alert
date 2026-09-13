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

// C++ replacement for SHAPE.ASM/SETSHAPE.ASM's _ShapeBuffer/_ShapeBufferSize/Set_Shape_Buffer
// (sub-milestone 1c - Agent/plans_done/code_1c_port_log.md). Both ASM files define this same pair
// of globals identically (a duplicate-implementation situation like several others in this port);
// this is the one definition needed regardless of which SHAPE-module ASM variant is "the real one" -
// a question this file's existence makes moot for 1c's purposes.

extern "C" {

char * _ShapeBuffer = 0;
long _ShapeBufferSize = 0;

void __cdecl Set_Shape_Buffer(void const * buffer, int size)
{
	_ShapeBuffer = (char *)buffer;
	_ShapeBufferSize = size;
}

}
