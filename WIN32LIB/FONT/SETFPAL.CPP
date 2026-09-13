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

// C++ replacement for SETFPAL.ASM's Set_Font_Palette_Range (sub-milestone 1c -
// Agent/plans_done/code_1c_port_log.md). Writes into WIN32LIB/DRAWBUFF/TXTPRNT.CPP's ColorXlat
// table (extern-shared exactly as the two original ASM modules shared it), mirroring each supplied
// color into both the low-nibble slot (index N) and the high-nibble slot (index N*16) - see
// TXTPRNT.CPP's header comment for why Buffer_Print reads the table at both of those index schemes.

extern "C" unsigned char ColorXlat[256];

extern "C" void __cdecl Set_Font_Palette_Range(void const * palette, int start, int end)
{
	start &= 0x0F;
	end &= 0x0F;
	if (end < start) return;

	unsigned char const * colors = (unsigned char const *)palette;
	for (int i = start; i <= end; i++) {
		unsigned char color = colors[i - start];
		ColorXlat[i] = color;
		ColorXlat[i * 16] = color;
	}
}
