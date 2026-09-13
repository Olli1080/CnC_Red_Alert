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

// C++ replacement for REVERSE.ASM (sub-milestone 1c - Agent/plans_done/code_1c_port_log.md).

extern "C" {

long __cdecl Reverse_Long(long number)
{
	unsigned long n = (unsigned long)number;
	return (long)(((n & 0x000000FFu) << 24) | ((n & 0x0000FF00u) << 8) |
				  ((n & 0x00FF0000u) >> 8)  | ((n & 0xFF000000u) >> 24));
}

short __cdecl Reverse_Short(short number)
{
	unsigned short n = (unsigned short)number;
	return (short)(((n & 0x00FFu) << 8) | ((n & 0xFF00u) >> 8));
}

// Swaps the high and low 16-bit halves of the value (NOT a full byte reversal - each half keeps
// its own byte order, matching the original's `ror eax,16`).
long __cdecl Swap_Long(long number)
{
	unsigned long n = (unsigned long)number;
	return (long)((n << 16) | (n >> 16));
}

}
