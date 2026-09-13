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

// C++ replacement for both STAMP.ASM and STMPCACH.ASM (sub-milestone 1c -
// Agent/plans_done/code_1c_port_log.md) - combined into one file because the two share file-scope
// state (LastIconset/MapPtr/IconCount/IconSize/StampPtr/IsTrans, all "global" - i.e.
// cross-module-visible - in the original assembly) and Is_Icon_Cached calls Init_Stamps directly.
// Of STMPCACH.ASM's exports, only Cache_Copy_Icon and Is_Icon_Cached are ever actually called (from
// WIN32LIB/DRAWBUFF/ICONCACH.CPP and GBUFFER.H) - Clear_Icon_Pointers/Get_Icon_Index/Get_Free_Index
// and the IconPointers global have no call sites anywhere in this GPL release and are not ported.
// Of STAMP.ASM's two drawing routines, only the clipped variant (Buffer_Draw_Stamp_Clip, called
// from GBUFFER.H) is actually used; Buffer_Draw_Stamp is ported too since it costs little extra and
// keeps the declared DRAWBUFF.H prototype backed by a real definition.
//
// Init_Stamps' MapPtr calculation intentionally matches the original ASM's actual behavior, which
// sets MapPtr = icondata + control->Map unconditionally, without checking whether the set even has
// a map table (a stray, unreferenced "??nomap:" label in the original suggests a conditional branch
// used to exist and was later deleted, leaving the unconditional assignment behind) - ported as-is
// rather than "fixed", since every caller's own MapPtr-non-null check is written against this same
// behavior (MapPtr is non-null once Init_Stamps has run at all, effectively always, since icondata
// is never null when Init_Stamps actually applies it).

#include <gbuffer.h>
#include <tile.h>
#include <iconcach.h>
#include <string.h>

namespace {

void const * LastIconset = NULL;		// Pointer to last iconset initialized.
unsigned char const * StampPtr = NULL;	// Pointer to icon pixel data.
unsigned char const * IsTrans = NULL;	// Pointer to per-icon transparency flag table.
unsigned char const * MapPtr = NULL;	// Pointer to logical->physical icon map (see note above).
int IconWidth = 0;
int IconHeight = 0;
int IconSize = 0;		// Bytes per icon (IconWidth * IconHeight).
int IconCount = 0;

void Init_Stamps(void const * icondata)
{
	if (!icondata) return;
	if (LastIconset == icondata) return;
	LastIconset = icondata;

	IControl_Type const * control = (IControl_Type const *)icondata;
	unsigned char const * base = (unsigned char const *)icondata;

	IconCount = control->Count;
	IconWidth = control->Width;
	IconHeight = control->Height;
	IconSize = IconWidth * IconHeight;
	MapPtr = base + control->Map;
	StampPtr = base + control->Icons;
	IsTrans = base + control->TransFlag;
}

// Common tail end of Is_Icon_Cached: icon_data/icon have already been validated and remapped by
// the caller. Finds icon_data's registration entry, and either returns its already-cached slot or
// tries to cache it now.
int Is_Stamp_Registered(void const * icon_data)
{
	for (int i = 0; i < MAX_ICON_SETS; i++) {
		if (IconSetList[i].IconSetPtr == icon_data) return i;
	}
	return -1;
}

}	// namespace

extern "C" {

void __cdecl Cache_Copy_Icon(void const * icon_ptr, void * surface_ptr, int pitch)
{
	unsigned char const * src = (unsigned char const *)icon_ptr;
	unsigned char * dst = (unsigned char *)surface_ptr;

	for (int y = 0; y < ICON_HEIGHT; y++) {
		memcpy(dst, src, ICON_WIDTH);
		src += ICON_WIDTH;
		dst += pitch;
	}
}

int __cdecl Is_Icon_Cached(void const * icon_data, int icon)
{
	if (!icon_data) return -1;

	Init_Stamps(icon_data);

	if (MapPtr) icon = MapPtr[icon];
	if ((unsigned)icon >= (unsigned)IconCount) return -1;

	int set_index = Is_Stamp_Registered(icon_data);
	if (set_index == -1) return -1;

	IconSetType & entry = IconSetList[set_index];
	int lookup_index = entry.IconListOffset / 2 + icon;	// IconListOffset is stored as a byte
															// offset (see Register_Icon_Set in
															// ICONCACH.CPP), IconCacheLookup is a
															// short array - divide by 2 to convert.
	if (IconCacheLookup[lookup_index] != -1) {
		return (unsigned short)IconCacheLookup[lookup_index];
	}

	int free_slot = Get_Free_Cache_Slot();
	if (free_slot < 0) return -1;

	void const * icon_ptr = StampPtr + (long)icon * IconSize;
	if (!Cache_New_Icon(free_slot, (void *)icon_ptr)) return -1;

	IconCacheLookup[lookup_index] = (short)free_slot;
	return free_slot;
}

void __cdecl Buffer_Draw_Stamp(void const * thisptr, void const * icondata, int icon, int x_pixel, int y_pixel, void const * remap)
{
	if (!icondata) return;

	Init_Stamps(icondata);

	if (MapPtr) icon = MapPtr[icon];
	if ((unsigned)icon >= (unsigned)IconCount) return;

	GraphicViewPortClass * viewport = (GraphicViewPortClass *)thisptr;
	int stride = viewport->Get_Width() + viewport->Get_XAdd() + viewport->Get_Pitch();

	// Offset alone is the complete pixel-data pointer - see CLEAR.CPP's comment. Adding
	// Get_Buffer() on top (the original mistake here) double-counts the pointer.
	unsigned char * dest = (unsigned char *)viewport->Get_Offset();
	dest += (long)y_pixel * stride + x_pixel;

	unsigned char const * src = StampPtr + (long)icon * IconSize;

	if (remap) {
		unsigned char const * xlat = (unsigned char const *)remap;
		for (int y = 0; y < IconHeight; y++) {
			for (int x = 0; x < IconWidth; x++) {
				unsigned char pixel = xlat[*src++];
				if (pixel) dest[x] = pixel;
			}
			dest += stride;
		}
	} else if (!IsTrans[icon]) {
		for (int y = 0; y < IconHeight; y++) {
			memcpy(dest, src, IconWidth);
			src += IconWidth;
			dest += stride;
		}
	} else {
		for (int y = 0; y < IconHeight; y++) {
			for (int x = 0; x < IconWidth; x++) {
				unsigned char pixel = src[x];
				if (pixel) dest[x] = pixel;
			}
			src += IconWidth;
			dest += stride;
		}
	}
}

void __cdecl Buffer_Draw_Stamp_Clip(void const * thisptr, void const * icondata, int icon, int x_pixel, int y_pixel,
	void const * remap, int min_x, int min_y, int max_x, int max_y)
{
	if (!icondata) return;

	Init_Stamps(icondata);

	if (MapPtr) icon = MapPtr[icon];
	if ((unsigned)icon >= (unsigned)IconCount) return;

	int iwidth = IconWidth;
	int height = IconHeight;
	unsigned char const * src = StampPtr + (long)icon * IconSize;

	// Convert the window rect (min_x/min_y origin, max_x/max_y initially a width/height per
	// GBUFFER.H's caller) into absolute viewport-space boundaries, and make x_pixel/y_pixel
	// absolute too (they arrive window-relative).
	max_x += min_x;
	x_pixel += min_x;
	max_y += min_y;
	y_pixel += min_y;

	if (x_pixel >= max_x) return;
	if (y_pixel >= max_y) return;
	if (y_pixel + IconHeight <= min_y) return;
	if (x_pixel + IconWidth <= min_x) return;

	// Horizontal clip: `skip` accumulates the total left+right pixels clipped off each row, added
	// to the source pointer after every row so it lands back at the next row's left-clip offset.
	if (x_pixel < min_x) {
		int left_clip = min_x - x_pixel;
		src += left_clip;
		iwidth -= left_clip;
		x_pixel = min_x;
	}
	int skip = IconWidth - iwidth;

	if (x_pixel + iwidth > max_x) {
		int old_width = iwidth;
		iwidth = max_x - x_pixel;
		skip += old_width - iwidth;
	}

	// Vertical clip.
	if (min_y > y_pixel) {
		int top_clip = min_y - y_pixel;
		height -= top_clip;
		src += (long)top_clip * IconWidth;
		y_pixel = min_y;
	}
	if (y_pixel + height > max_y) {
		height = max_y - y_pixel;
	}

	GraphicViewPortClass * viewport = (GraphicViewPortClass *)thisptr;
	int stride = viewport->Get_Width() + viewport->Get_XAdd() + viewport->Get_Pitch();

	// Offset alone is the complete pixel-data pointer - see CLEAR.CPP's comment. Adding
	// Get_Buffer() on top (the original mistake here) double-counts the pointer.
	unsigned char * dest = (unsigned char *)viewport->Get_Offset();
	dest += (long)y_pixel * stride + x_pixel;

	if (remap) {
		unsigned char const * xlat = (unsigned char const *)remap;
		for (int y = 0; y < height; y++) {
			for (int x = 0; x < iwidth; x++) {
				unsigned char pixel = xlat[src[x]];
				if (pixel) dest[x] = pixel;
			}
			src += iwidth + skip;
			dest += stride;
		}
	} else if (!IsTrans[icon]) {
		for (int y = 0; y < height; y++) {
			memcpy(dest, src, iwidth);
			src += iwidth + skip;
			dest += stride;
		}
	} else {
		for (int y = 0; y < height; y++) {
			for (int x = 0; x < iwidth; x++) {
				unsigned char pixel = src[x];
				if (pixel) dest[x] = pixel;
			}
			src += iwidth + skip;
			dest += stride;
		}
	}
}

}	// extern "C"
