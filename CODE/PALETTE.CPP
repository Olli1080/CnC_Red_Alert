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

#include "palette.h"

// Angle-bracket on purpose: reaches WIN32LIB/INCLUDE/PALETTE.H (the C-style Set_Palette/
// Fade_Palette_To primitives), not this directory's own "palette.h" - see that file's guard
// comment for why the two can't share a guard name.
#include <palette.h>

RGBClass PaletteClass::CurrentPalette[PaletteClass::COLOR_COUNT];

void PaletteClass::Set(void) const
{
	unsigned char buffer[COLOR_COUNT * 3];
	for (int i = 0; i < COLOR_COUNT; i++) {
		buffer[i * 3 + 0] = (unsigned char)Colors[i].Red_Component();
		buffer[i * 3 + 1] = (unsigned char)Colors[i].Green_Component();
		buffer[i * 3 + 2] = (unsigned char)Colors[i].Blue_Component();
		CurrentPalette[i] = Colors[i];
	}
	::Set_Palette(buffer);
}

void PaletteClass::Set(int fade_delay, void (*callback)(void)) const
{
	if (fade_delay <= 0) {
		Set();
		return;
	}

	unsigned char buffer[COLOR_COUNT * 3];
	for (int i = 0; i < COLOR_COUNT; i++) {
		buffer[i * 3 + 0] = (unsigned char)Colors[i].Red_Component();
		buffer[i * 3 + 1] = (unsigned char)Colors[i].Green_Component();
		buffer[i * 3 + 2] = (unsigned char)Colors[i].Blue_Component();
		CurrentPalette[i] = Colors[i];
	}
	::Fade_Palette_To(buffer, (unsigned int)fade_delay, callback);
}

void PaletteClass::Adjust(int ratio, PaletteClass const & target)
{
	for (int i = 0; i < COLOR_COUNT; i++) {
		Colors[i].Adjust(ratio, target.Colors[i]);
	}
}

void PaletteClass::Adjust(int ratio)
{
	for (int i = 0; i < COLOR_COUNT; i++) {
		Colors[i].Adjust(ratio, BlackColor);
	}
}

void PaletteClass::Partial_Adjust(int ratio, PaletteClass const & target, char const * lut)
{
	for (int i = 0; i < COLOR_COUNT; i++) {
		if (lut[i]) {
			Colors[i].Adjust(ratio, target.Colors[i]);
		}
	}
}

void PaletteClass::Partial_Adjust(int ratio, char const * lut)
{
	for (int i = 0; i < COLOR_COUNT; i++) {
		if (lut[i]) {
			Colors[i].Adjust(ratio, BlackColor);
		}
	}
}

int PaletteClass::Closest_Color(RGBClass const & rgb) const
{
	int best_index = 0;
	int best_difference = Colors[0].Difference(rgb);

	for (int i = 1; i < COLOR_COUNT; i++) {
		int difference = Colors[i].Difference(rgb);
		if (difference < best_difference) {
			best_difference = difference;
			best_index = i;
		}
	}

	return best_index;
}
