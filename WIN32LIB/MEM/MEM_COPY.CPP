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

// C++ replacement for MEM_COPY.ASM (sub-milestone 1c - Agent/plans_done/code_1c_port_log.md). The
// original hand-optimized its own forward/backward overlap-safe byte+dword copy - exactly what the
// standard library's memmove already does. Largest_Mem_Block (the file's other export) is wrapped
// in "IF 0" in the original source (DOS DPMI real-mode memory query, dead code) and was never
// ported.

#include <string.h>

extern "C" void __cdecl Mem_Copy(void const * source, void * dest, unsigned long bytes_to_copy)
{
	if (!source || !dest || source == dest) return;
	memmove(dest, source, bytes_to_copy);
}
