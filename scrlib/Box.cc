//
// Subwindow (box) implementation.
//
// Copyright (C) 1994 Cronyx Ltd.
// Author: Serge Vakulenko, <vak@zebub.msk.su>
//
// This software is distributed with NO WARRANTIES, not even the implied
// warranties for MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
//
// Authors grant any other persons or organisations permission to use
// or modify this software as long as this message is kept with the software,
// all derivative works or modified versions.
//
#include "Screen.h"
#include "extern.h"

Box::Box (Screen &scr, int by, int bx, int bny, int bnx)
{
	y = by;
	x = bx;
	ny = bny;
	nx = bnx;
	mem = new short [ny * nx];
	if (! mem)
		return;
	for (int yy=0; yy<ny; ++yy) {
		short *p = &scr.scr[yy+y][x];
		short *q = &mem[yy*nx];
		for (int xx=0; xx<nx; ++xx)
			*q++ = *p++;
	}
}

void Screen::Put (Box &box, int attr)
{
	if (! box.mem)
		return;
	if (attr)
		attr = attr<<8 & 0x7f00;
	for (int yy=0; yy<box.ny; ++yy) {
		short *q = & box.mem [yy * box.nx];
		for (int xx=0; xx<box.nx; ++xx) {
		        auto ch = attr ? ((*q++ & 0377) | attr) : *q++;
			pokeChar (box.y + yy, box.x + xx, ch);
                }
	}
}
