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

// C++ replacement for WWMOUSE.ASM (sub-milestone 1c - Agent/plans_done/code_1c_port_log.md). These
// three free functions need access to WWMouseClass's private cursor/background-buffer fields (the
// original read them via raw struct-offset access into the same layout) - granted via the friend
// declarations added to WIN32LIB/INCLUDE/MOUSE.H and WIN32LIB/KEYBOARD/MOUSE.H (both copies; see
// their own header comments).
//
// Mouse_Shadow_Buffer and Draw_Mouse both clip a CursorWidth x CursorHeight rect (positioned at
// (x,y) minus a hot-spot offset) against the destination viewport, the same Cohen-Sutherland style
// used throughout this port's other blitters - only the near edge (x0/y0) needs an explicit source-
// pointer adjustment when clipped, since leaving the far edge (x1/y1) untouched already yields the
// correct reduced copy width/height once it's subtracted from the (now-zeroed) near edge.

#include <mouse.h>
#include <shape.h>
#include <iff.h>
#include <string.h>

namespace {

bool Clip_Cursor_Rect(int & x0, int & y0, int & x1, int & y1, int vp_width, int vp_height,
	int cursor_width, long & src_offset, int & buffx0)
{
	bool reject = (x0 < 0 && x1 < 0) || (x0 > vp_width && x1 > vp_width) ||
		(y0 < 0 && y1 < 0) || (y0 > vp_height && y1 > vp_height);
	if (reject) return false;

	if (x0 < 0) { buffx0 -= x0; x0 = 0; }
	if (y0 < 0) { src_offset += (long)(-y0) * cursor_width; y0 = 0; }
	if (x1 > vp_width) x1 = vp_width;
	if (y1 > vp_height) y1 = vp_height;

	return (x1 - x0 > 0) && (y1 - y0 > 0);
}

}	// namespace

extern "C" {

void __cdecl Mouse_Shadow_Buffer(void * thisptr, GraphicViewPortClass * scr, void * buffer, int x, int y, int hotx, int hoty, int store)
{
	WWMouseClass * mouse = (WWMouseClass *)thisptr;

	int x0 = x - hotx, y0 = y - hoty;
	int x1 = x0 + mouse->CursorWidth, y1 = y0 + mouse->CursorHeight;
	long src_offset = 0;
	int buffx0 = 0;

	if (!Clip_Cursor_Rect(x0, y0, x1, y1, scr->Get_Width(), scr->Get_Height(), mouse->CursorWidth, src_offset, buffx0)) {
		return;
	}

	int copy_width = x1 - x0;
	int copy_height = y1 - y0;
	int dst_stride = scr->Get_Width() + scr->Get_XAdd() + scr->Get_Pitch();

	// Offset alone is the complete pixel-data pointer - see CLEAR.CPP's comment. Adding
	// Get_Buffer() on top (the original mistake here) double-counts the pointer.
	unsigned char * screen = (unsigned char *)scr->Get_Offset() + (long)y0 * dst_stride + x0;
	unsigned char * buf = (unsigned char *)buffer + src_offset + buffx0;

	if (!store) {
		for (int row = 0; row < copy_height; row++) {
			memcpy(screen, buf, copy_width);
			buf += mouse->CursorWidth;
			screen += dst_stride;
		}
	} else {
		for (int row = 0; row < copy_height; row++) {
			memcpy(buf, screen, copy_width);
			screen += dst_stride;
			buf += mouse->CursorWidth;
		}
	}
}

void __cdecl Draw_Mouse(void * thisptr, GraphicViewPortClass * scr, int mousex, int mousey)
{
	WWMouseClass * mouse = (WWMouseClass *)thisptr;

	int x0 = mousex - mouse->MouseXHot, y0 = mousey - mouse->MouseYHot;
	int x1 = x0 + mouse->CursorWidth, y1 = y0 + mouse->CursorHeight;
	long src_offset = 0;
	int buffx0 = 0;

	if (!Clip_Cursor_Rect(x0, y0, x1, y1, scr->Get_Width(), scr->Get_Height(), mouse->CursorWidth, src_offset, buffx0)) {
		return;
	}

	int copy_width = x1 - x0;
	int copy_height = y1 - y0;
	int dst_stride = scr->Get_Width() + scr->Get_XAdd() + scr->Get_Pitch();

	// Offset alone is the complete pixel-data pointer - see CLEAR.CPP's comment. Adding
	// Get_Buffer() on top (the original mistake here) double-counts the pointer.
	unsigned char * dest = (unsigned char *)scr->Get_Offset() + (long)y0 * dst_stride + x0;
	unsigned char const * src = (unsigned char const *)mouse->MouseCursor + src_offset + buffx0;

	for (int row = 0; row < copy_height; row++) {
		for (int col = 0; col < copy_width; col++) {
			unsigned char pixel = src[col];
			if (pixel) dest[col] = pixel;
		}
		src += mouse->CursorWidth;
		dest += dst_stride;
	}
}

}	// extern "C"

namespace {

// Unpacks the RLE format used by mouse cursor shape data: a nonzero byte is one literal pixel
// (remapped through `remap` for 16-color shapes), a zero byte introduces a run of `next byte`
// transparent (value 0) pixels.
void Decode_Cursor_RLE(unsigned char const * src, unsigned char * dest, int pixel_count, unsigned char const * remap)
{
	while (pixel_count > 0) {
		unsigned char v = *src++;
		if (v != 0) {
			*dest++ = remap ? remap[v] : v;
			pixel_count--;
		} else {
			unsigned char run = *src++;
			memset(dest, 0, run);
			dest += run;
			pixel_count -= run;
		}
	}
}

}	// namespace

extern "C" void * __cdecl ASM_Set_Mouse_Cursor(void * thisptr, int xhotspot, int yhotspot, void * cursor)
{
	WWMouseClass * mouse = (WWMouseClass *)thisptr;

	int datasize = Get_Shape_Uncomp_Size(cursor);
	int swidth = Get_Shape_Width(cursor);
	int sheight = Get_Shape_Original_Height(cursor);

	if (swidth <= mouse->MaxWidth && sheight <= mouse->MaxHeight) {
		unsigned char const * src = (unsigned char const *)cursor;
		unsigned short stype = *(unsigned short const *)src;
		bool is_16color = (stype & MAKESHAPE_COMPACT) != 0;
		unsigned char const * remap = NULL;

		if (!(stype & MAKESHAPE_NOCOMP)) {
			// Compressed shape: decompress header + pixel data into the shared staging buffer
			// (_ShapeBuffer), then fall through to the same RLE unpacking used for an
			// already-uncompressed shape.
			unsigned char * stage = (unsigned char *)_ShapeBuffer;
			unsigned char const * comp_src = src + 2;	// past the type word

			*(unsigned short *)stage = (unsigned short)(stype | MAKESHAPE_NOCOMP);
			unsigned char * stage_dest = stage + 2;

			int header_words = is_16color ? 12 : 4;
			memcpy(stage_dest, comp_src, header_words * 2);
			comp_src += header_words * 2;
			stage_dest += header_words * 2;

			LCW_Uncompress((void *)comp_src, stage_dest, datasize);

			src = stage;
		}

		src += 10;	// past the 2-byte type word + 4-word (8-byte) base header
		if (is_16color) {
			remap = src;
			src += 16;	// past the 16-byte (8-word) 16-color remap table
		}

		Decode_Cursor_RLE(src, (unsigned char *)mouse->MouseCursor, swidth * sheight, remap);

		mouse->MouseXHot = xhotspot;
		mouse->MouseYHot = yhotspot;
		mouse->CursorHeight = sheight;
		mouse->CursorWidth = swidth;
	}

	void * old_cursor = mouse->PrevCursor;
	mouse->PrevCursor = (char *)cursor;
	return old_cursor;
}
