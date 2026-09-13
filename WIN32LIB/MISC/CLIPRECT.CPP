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

// C++ replacement for CLIPRECT.ASM (sub-milestone 1c - Agent/plans_done/code_1c_port_log.md).
//
// Clip_Rect is a textbook Cohen-Sutherland rectangle clip: each corner point (x0,y0) and (x1,y1)
// (x1=x0+dw, y1=y0+dh) gets a 4-bit outcode against the [0,width]x[0,height] window (bit3=x<0,
// bit2=x>width, bit1=y<0, bit0=y>height); AND-of-codes-nonzero means trivially outside, OR-of-codes
// zero means trivially inside and unmodified, otherwise each set bit clips that one edge in turn.
// Confine_Rect instead only ever shifts the rect's position (never resizes it) so it fits within
// the window, or clamps to the near edge if it's larger than the window on that axis.

extern "C" {

int __cdecl Clip_Rect(int * x, int * y, int * w, int * h, int width, int height)
{
	int x0 = *x, y0 = *y, dw = *w, dh = *h;
	int x1 = x0 + dw, y1 = y0 + dh;

	int code0 = ((x0 < 0) << 3) | ((x0 > width) << 2) | ((y0 < 0) << 1) | (y0 > height);
	int code1 = ((x1 < 0) << 3) | ((x1 > width) << 2) | ((y1 < 0) << 1) | (y1 > height);

	if (code0 & code1) return -1;		// entirely outside the window
	if ((code0 | code1) == 0) return 0;	// entirely inside, unmodified

	if (code0 & 0x8) {			// x0 < 0: clip left edge
		dw += x0;
		x0 = 0;
	}
	if (code0 & 0x2) {			// y0 < 0: clip top edge
		dh += y0;
		y0 = 0;
	}
	if (code1 & 0x4) {			// x1 > width: clip right edge
		dw = width - x0;
		if (dw <= 0) { *x = x0; *y = y0; *w = dw; *h = dh; return -1; }
	}
	if (code1 & 0x1) {			// y1 > height: clip bottom edge
		dh = height - y0;
		if (dh <= 0) { *x = x0; *y = y0; *w = dw; *h = dh; return -1; }
	}

	*x = x0; *y = y0; *w = dw; *h = dh;
	return 1;
}

int __cdecl Confine_Rect(int * x, int * y, int dw, int dh, int width, int height)
{
	int changed = 0;
	int x0 = *x, y0 = *y;
	int x1 = x0 + dw, y1 = y0 + dh;

	// Faithfully reproduces the original's exact edge-case behavior: a rectangle already flush
	// against 0 on this axis (coordinate exactly 0) still reports "changed" even though nothing
	// actually moves - matches the ASM's own TEST-based trivial-fit check.
	if (((-x0) & (x1 - width - 1)) >= 0) {
		changed = 1;
		if (-x0 < 0) {
			x0 -= (x1 - width);
		} else {
			x0 = 0;
		}
	}
	if (((-y0) & (y1 - height - 1)) >= 0) {
		changed = 1;
		if (-y0 < 0) {
			y0 -= (y1 - height);
		} else {
			y0 = 0;
		}
	}

	*x = x0;
	*y = y0;
	return changed;
}

}
