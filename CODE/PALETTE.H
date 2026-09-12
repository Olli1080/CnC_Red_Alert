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

// Deliberately NOT guarded as PALETTE_H: WIN32LIB/INCLUDE/PALETTE.H (the low-level C-style
// Set_Palette/Fade_Palette_To API this class wraps) already claims that guard name. Since this
// file and that one are reached via quote vs. angle-bracket includes of the identically-named
// "palette.h" respectively (see PALETTE.CPP), a shared guard would make whichever is included
// second a silent no-op.
#ifndef CODE_PALETTECLASS_H
#define CODE_PALETTECLASS_H

#include "rgb.h"
#include "noinit.h"

/*
**	PaletteClass never had a surviving implementation in this GPL source drop - RGB.H only ever
**	carried a forward declaration ("class PaletteClass;") and a "friend class PaletteClass;"
**	grant inside RGBClass. This reconstruction is written purely from how every call site across
**	CODE/ (~50 files: GamePalette, BlackPalette, CCPalette, WhitePalette, ScorePalette,
**	InGamePalette, etc.) actually uses it - a fixed-size array of 256 RGBClass entries, indexable,
**	settable (instantly or faded over time via WIN32LIB's Set_Palette/Fade_Palette_To), and
**	blendable toward another palette. It compiles and links against the existing WIN32LIB palette
**	primitives; whether the underlying VGA-era Set_Palette_Range call actually paints anything
**	under modern Win32/DX12 is a separate, already-known gap left to the Phase 4 rendering
**	rewrite (see Agent/knowledge_base/modernization_options.md) - out of scope for getting CODE/
**	to compile and link.
*/
class PaletteClass
{
	public:
		enum {
			COLOR_COUNT = 256
		};

		PaletteClass(void) {};
		PaletteClass(NoInitClass const &) {};

		/*
		**	Builds a palette with every entry set to the same color (e.g. BlackPalette,
		**	WhitePalette in GLOBALS.CPP).
		*/
		PaletteClass(RGBClass const & color) {
			for (int i = 0; i < COLOR_COUNT; i++) {
				Colors[i] = color;
			}
		};

		RGBClass & operator[] (int index) {return Colors[index];};
		RGBClass const & operator[] (int index) const {return Colors[index];};

		/*
		**	Implicit conversion to a raw color buffer, for the low-level WIN32LIB routines
		**	(e.g. Build_Fading_Table in MISC.H, Load_Title_Screen) that take palettes as
		**	"void const *"/"unsigned char *", and for direct memcpy of the whole palette.
		**	unsigned char (not RGBClass) so it also satisfies "unsigned char *" parameters
		**	directly, without a second (disallowed) user-defined conversion.
		*/
		operator unsigned char const * (void) const {return (unsigned char const *)Colors;};
		operator unsigned char * (void) {return (unsigned char *)Colors;};

		/*
		**	Makes this palette the active display palette immediately.
		*/
		void Set(void) const;

		/*
		**	Fades the display palette to this palette over "fade_delay" timer ticks, invoking
		**	the optional callback once per fade step (matches WIN32LIB's Fade_Palette_To).
		*/
		void Set(int fade_delay, void (*callback)(void) = 0) const;

		/*
		**	Blends every color in this palette toward the corresponding color in "target" by
		**	"ratio" (0=no change, 0xFF=fully become target). Defaults to blending toward black.
		*/
		void Adjust(int ratio, PaletteClass const & target);
		void Adjust(int ratio);

		/*
		**	Like Adjust, but only touches entries where "lut[index]" is non-zero (used to fade
		**	the picture area of a screen while leaving text colors untouched).
		*/
		void Partial_Adjust(int ratio, PaletteClass const & target, char const * lut);
		void Partial_Adjust(int ratio, char const * lut);

		/*
		**	Finds the index of the palette entry closest in color to "rgb".
		*/
		int Closest_Color(RGBClass const & rgb) const;

		/*
		**	Snapshot of whatever palette was last pushed to the display via Set(), regardless of
		**	which PaletteClass instance it came from - mirrors how the codebase uses
		**	"PaletteClass::CurrentPalette[i]" as if it were tracking hardware state.
		*/
		static RGBClass CurrentPalette[COLOR_COUNT];

	private:
		RGBClass Colors[COLOR_COUNT];
};

#endif // CODE_PALETTECLASS_H
