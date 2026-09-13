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

// C++ replacement for PAL.ASM (sub-milestone 1c - Agent/plans_done/code_1c_port_log.md): the
// original file's live code path (guarded "IF 1", the alternate hardware-VGA-DAC path below it is
// dead under "ELSE") is just a 768-byte copy into CurrentPalette followed by a call into
// Set_DD_Palette - the further "ifdef NOT_FOR_WIN95" block underneath that (raw VGA port I/O,
// vertical-blank polling) never compiles for a WIN32 build and isn't ported. PaletteTable (a
// second global PAL.ASM declared) is dead for WIN32 too - nothing outside PAL.ASM itself and the
// already-superseded WWFLAT32 predecessor ever references it.

#include <windows.h>
#include <string.h>

extern "C" void Set_DD_Palette(void * palette);

extern "C" unsigned char CurrentPalette[768] = { 0 };

extern "C" void __cdecl Set_Palette_Range(void * palette)
{
	memcpy(CurrentPalette, palette, 768);
	Set_DD_Palette(palette);
}

extern "C" BOOL __cdecl Bump_Color(void * palette, int changable, int target)
{
	unsigned char * edit = (unsigned char *)palette + changable * 3;
	unsigned char const * desired = (unsigned char *)palette + target * 3;
	BOOL changed = FALSE;

	for (int i = 0; i < 3; i++) {
		if (desired[i] != edit[i]) {
			changed = TRUE;
			if (desired[i] > edit[i]) {
				edit[i]++;
			} else {
				edit[i]--;
			}
		}
	}

	return changed;
}
