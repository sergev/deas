//
// Screen management package implementation for MSDOS.
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
#include <dos.h>
#include <io.h>
#include <stdlib.h>
#include <string.h>
#include <conio.h>
#include <stdio.h>

static unsigned char linedraw [11] = {
	0xda, 0xc2, 0xbf, 0xc3, 0xc5, 0xb4, 0xc0, 0xc1, 0xd9, 0xb3, 0xc4,
};

inline void outerr (char *str)
{
	write (2, str, strlen (str));
}

inline void set (short far *p, int val, unsigned len)
{
	while (len--)
		*p++ = val;
}

inline void move (short far *to, short far *from, unsigned len)
{
	while (len--)
		*to++ = *from++;
}

// Some ideas of dos bios interface are taken from
// sources of elvis editor, by Guntram Blohm.

Screen::Screen (int colormode, int graphmode)
{
	// Determine number of screen columns. Also set attrset according
	// to monochrome/color screen.
	//
	// Getting the number of rows is hard. Most screens support 25 only,
	// EGA/VGA also support 43/50 lines, and some OEM's even more.
	// Unfortunately, there is no standard bios variable for the number
	// of lines, and the bios screen memory size is always rounded up
 	// to 0x1000. So, we'll really have to cheat.
 	// When using the screen memory size, keep in mind that each character
 	// byte has an associated attribute byte.
 	//
 	// Uses: word at 40:4c contains memory size
 	//       byte at 40:84 # of rows-1 (sometimes)
	//       byte at 40:4a # of columns

	Lines = *(unsigned char far*) 0x400084L + 1;
	Columns = *(unsigned char far*) 0x40004aL;
	int memsz = *(int far*) 0x40004cL;

	// Screen size less then 4K? then we have 25 lines only.
	if (memsz <= 4096)
		Lines = 25;

	// Does it all make sense together with memory size?
	else if ((Columns * Lines * 2 + 4095) / 4096 * 4096 != memsz) {
		// Oh oh. Emit '\n's until screen starts scrolling.
		union REGS regs;

		// Move to top of screen.
		regs.x.ax = 0x200;
		regs.x.bx = 0;
		regs.x.dx = 0;
		int86 (0x10, &regs, &regs);

		int oldline = 0;
		for (;;) {
			// Write newline to screen.
			regs.x.ax = 0xe0a;
			regs.x.bx = 0;
			int86 (0x10, &regs, &regs);

			// Read cursor position.
			regs.x.ax = 0x300;
			regs.x.bx = 0;
			int86 (0x10, &regs, &regs);
			int line = regs.h.dh;

			if (oldline == line) {
				Lines = line + 1;
				break;
			}
			oldline = line;
		}
	}

	scr = new short far* [Lines];
	if (! scr) {
		outerr ("Out of memory in Screen constructor\n");
		exit (1);
	}

	// Check for monochrome mode.
	ColorMode = (*(unsigned char far *) 0x400049L != 7);
	if (ColorMode)
		scr[0] = (short far *) 0xb8000000L;
	else
		scr[0] = (short far *) 0xb0000000L;

	for (int y=1; y<Lines; ++y)
		scr[y] = scr[y-1] + Columns;

	flgs = NormalAttr = 0x0700;
	hidden = 0;
	ungetflag = 0;
	ncallb = 0;
#if 0
	ULC           = '\20';
	UT            = '\21';
	URC           = '\22';
	CLT           = '\23';
	CX            = '\24';
	CRT           = '\25';
	LLC           = '\26';
	LT            = '\27';
	LRC           = '\30';
	VERT          = '\31';
	HOR           = '\32';
#else
	ULC           = 0xda;
	UT            = 0xc2;
	URC           = 0xbf;
	CLT           = 0xc3;
	CX            = 0xc5;
	CRT           = 0xb4;
	LLC           = 0xc0;
	LT            = 0xc1;
	LRC           = 0xd9;
	VERT          = 0xb3;
	HOR           = 0xc4;
#endif
	Clear ();
}

Screen::~Screen ()
{
	// Set cursor position to 0,0.
	if (curx<0 || curx>=Columns)
		curx = 0;
	if (cury<0 || cury>=Lines)
		cury = 0;
	union REGS regs;
	regs.x.ax = 0x200;
	regs.x.bx = 0;
	regs.x.dx = cury << 8 | (unsigned char) curx;
	int86 (0x10, &regs, &regs);
	delete scr;
}

void Screen::Clear (int attr)
{
	if (attr)
		attr = attr<<8 & 0x7f00;
	else
		attr = flgs;
	for (int y=0; y<Lines; ++y)
		set (scr[y], ' ' | attr, Columns);
	curx = cury = 0;
}

void Screen::Put (int c, int attr)
{
	if (attr)
		attr = attr<<8 & 0x7f00;
	else
		attr = flgs;
	switch (c = (unsigned char) c) {
#if 0
	case '\20': case '\21': case '\22': case '\23': case '\24': case '\25':
	case '\26': case '\27': case '\30': case '\31': case '\32':
		c = linedraw [c - ULC];
#endif
	default:
		if (curx>=0 && curx<Columns)
			scr[cury][curx] = c | attr;
		++curx;
		return;
	case '\t':
		int n;
		for (n=8-(curx&7); --n>=0; ++curx)
			if (curx>=0 && curx<Columns)
				scr[cury][curx] = ' ' | attr;
		return;
	case '\b':
		--curx;
		return;
	case '\n':
		while (curx < Columns)
			if (curx >= 0)
				scr[cury][curx] = ' ' | attr;
		if (++cury >= Lines)
			cury = 0;
	case '\r':
		curx = 0;
		return;
	}
}

void Screen::Sync ()
{
	// Set cursor position.
	// If it is out of screen, cursor will disappear.
	if (curx<0 || curx>=Columns || cury<0 || cury>=Lines)
		hidden = 1;
	union REGS regs;
	regs.x.ax = 0x200;
	regs.x.bx = 0;
	regs.x.dx = hidden ? -1 : cury << 8 | (unsigned char) curx;
	int86 (0x10, &regs, &regs);
}

void Screen::Beep ()
{
	sound (600);
	delay (40);
	sound (400);
	delay (60);
	sound (600);
	delay (40);
	nosound ();
}

void Screen::ClearLine (int y, int x, int attr)
{
	if (attr)
		attr = attr<<8 & 0x7f00;
	else
		attr = flgs;
	if (y < 0) {
		y = cury;
		x = curx;
	}
	if (x < 0)
		x = 0;
	if (x < Columns)
		set (scr[y] + x, ' ' | attr, Columns - x);
}

void Screen::UngetKey (int k)
{
	ungetflag = 1;
	ungetkey = k;
}

int Screen::GetKey ()
{
	if (ungetflag) {
		ungetflag = 0;
		return (ungetkey);
	}
nextkey:
	int c = getch ();
	if (! c) {
		c = getch ();
		switch (c) {
		default:	c = 0;		break;
		case ';':       c = meta ('A');	break;	// F1
		case '<':       c = meta ('B');	break;	// F2
		case '=':       c = meta ('C');	break;	// F3
		case '>':       c = meta ('D');	break;	// F4
		case '?':       c = meta ('E');	break;	// F5
		case '@':       c = meta ('F');	break;	// F6
		case 'A':       c = meta ('G');	break;	// F7
		case 'B':       c = meta ('H');	break;	// F8
		case 'C':       c = meta ('I');	break;	// F9
		case 'D':       c = meta ('J');	break;	// F10
		case 'E':       c = meta ('K');	break;	// F11
		case 'F':       c = meta ('L');	break;	// F12
		case 'K':       c = meta ('l');	break;	// left
		case 'M':       c = meta ('r');	break;	// right
		case 'H':       c = meta ('u');	break;	// up
		case 'P':       c = meta ('d');	break;	// down
		case 'G':       c = meta ('h');	break;	// home
		case 'O':       c = meta ('e');	break;	// end
		case 'Q':       c = meta ('n');	break;	// nexy page
		case 'I':       c = meta ('p');	break;	// prev page
		case 'R':       c = meta ('i');	break;	// insert
		case 'S':       c = meta ('x');	break;	// delete
		case 0x10:      c = alt ('q');	break;	// Alt-Q
		case 0x11:      c = alt ('w');	break;	// Alt-W
		case 0x12:      c = alt ('e');	break;	// Alt-E
		case 0x13:      c = alt ('r');	break;	// Alt-R
		case 0x14:      c = alt ('t');	break;	// Alt-T
		case 0x15:      c = alt ('y');	break;	// Alt-Y
		case 0x16:      c = alt ('u');	break;	// Alt-U
		case 0x17:      c = alt ('i');	break;	// Alt-I
		case 0x18:      c = alt ('o');	break;	// Alt-O
		case 0x19:      c = alt ('p');	break;	// Alt-P
		case 0x1e:      c = alt ('a');	break;	// Alt-A
		case 0x1f:      c = alt ('s');	break;	// Alt-S
		case 0x20:      c = alt ('d');	break;	// Alt-D
		case 0x21:      c = alt ('f');	break;	// Alt-F
		case 0x22:      c = alt ('g');	break;	// Alt-G
		case 0x23:      c = alt ('h');	break;	// Alt-H
		case 0x24:      c = alt ('j');	break;	// Alt-J
		case 0x25:      c = alt ('k');	break;	// Alt-K
		case 0x26:      c = alt ('l');	break;	// Alt-L
		case 0x2c:      c = alt ('z');	break;	// Alt-Z
		case 0x2d:      c = alt ('x');	break;	// Alt-X
		case 0x2e:      c = alt ('c');	break;	// Alt-C
		case 0x2f:      c = alt ('v');	break;	// Alt-V
		case 0x30:      c = alt ('b');	break;	// Alt-B
		case 0x31:      c = alt ('n');	break;	// Alt-N
		case 0x32:      c = alt ('m');	break;	// Alt-M
		}
	}
gotkey:
	int found = 0;
	struct callBack *cb = callBackTable;
	for (int j=ncallb; --j>=0; ++cb)
		if (cb->key == c) {
			found = 1;
			(*cb->func) (c);
		}
	if (found)
		goto nextkey;
	return (c);
}

void Screen::AddHotKey (int key, void (*f) (int))
{
	if (ncallb >= sizeof (callBackTable) / sizeof (callBackTable[0]))
		return;
	callBackTable[ncallb].key = key;
	callBackTable[ncallb].func = f;
	++ncallb;
}

void Screen::DelHotKey (int key)
{
	if (ncallb <= 0)
		return;
	struct callBack *a = callBackTable;
	struct callBack *b = a;
	for (int j=ncallb; --j>=0; ++a)
		if (a->key != key)
			*b++ = *a;
	ncallb = b - callBackTable;
}

void Screen::PrintVect (int attr, char *fmt, void *vect)
{
	char buf [512];

	vsprintf (buf, fmt, vect);
	Put (buf, attr);
}

void Screen::Print (int attr, char *fmt, ...)
{
	PrintVect (attr, fmt, &fmt + 1);
}

void Screen::Print (int y, int x, int attr, char *fmt, ...)
{
	Move (y, x);
	PrintVect (attr, fmt, &fmt + 1);
}

void Screen::Clear (int y, int x, int ny, int nx, int attr, int sym)
{
	if (attr)
		attr = attr<<8 & 0x7f00;
	else
		attr = flgs;
	sym |= attr;
	for (; --ny>=0; ++y) {
		for (int i=nx; --i>=0;)
			scr[y][x+i] = sym;
	}
}

void Screen::DelLine (int y, int attr)
{
	if (y<0 || y>=Lines)
		return;
	if (attr)
		attr = attr<<8 & 0x7f00;
	else
		attr = flgs;
	for (; y<Lines-1; ++y)
		move (scr[y], scr[y+1], Columns);
	set (scr[Lines-1], ' ' | attr, Columns);
}

void Screen::InsLine (int y, int attr)
{
	if (y<0 || y>=Lines)
		return;
	if (attr)
		attr = attr<<8 & 0x7f00;
	else
		attr = flgs;
	for (int i=Lines-1; i>y; --i)
		move (scr[i], scr[i-1], Columns);
	set (scr[y], ' ' | attr, Columns);
}

void Screen::AttrLow (int y, int x)
{
	if (x<0 || x>=Columns)
		return;
	int c = scr[y][x] & ~0xf800;
#if defined (MSDOS) || defined (__MSDOS__)
	switch (c & 0xff) {
	case 0xb0: case 0xb1: case 0xb2:
		c = (c & ~0xff) | ' ';
	}
#endif
	if (c & 0x700)
		scr[y][x] = c;
	else
		scr[y][x] = c | 0x700;
}

void Screen::HorLine (int y, int x, int nx, int attr)
{
	if (attr)
		attr = attr<<8 & 0x7f00;
	else
		attr = flgs;
	while (--nx >= 0)
		scr[y][x++] = linedraw[10] | attr;
}

void Screen::VertLine (int x, int y, int ny, int attr)
{
	if (attr)
		attr = attr<<8 & 0x7f00;
	else
		attr = flgs;
	while (--ny >= 0)
		scr[y++][x] = linedraw[9] | attr;
}

void Screen::DrawFrame (int y, int x, int ny, int nx, int attr)
{
	HorLine (y, x+1, nx-2, attr);
	HorLine (y+ny-1, x+1, nx-2, attr);
	VertLine (x, y+1, ny-2, attr);
	VertLine (x+nx-1, y+1, ny-2, attr);
	Put (y, x, ULC, attr);
	Put (y, x+nx-1, URC, attr);
	Put (y+ny-1, x, LLC, attr);
	Put (y+ny-1, x+nx-1, LRC, attr);
}

void Screen::AttrSet (int y, int x, int ny, int nx, int attr)
{
	if (x < 0) {
		nx += x;
		x = 0;
	}
	if (x>=Columns || nx<0)
		return;
	if (attr)
		attr = attr<<8 & 0x7f00;
	else
		attr = flgs;
	for (; --ny>=0; ++y) {
		short far *p = & scr [y] [x];
		for (int xx=0; xx<nx; ++xx, ++p)
			*p = *p & 0xff | attr;
	}
}
