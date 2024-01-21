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
// #define DEBUG
// #define INITFILE             // send 'if' on display open
// #define STRINIT              // send 'is' on display open
// #define NOKEYPAD             // don't use 'ks', 'ke'

#include <signal.h>
#include <fcntl.h>
#include <stdio.h>

#include "Screen.h"
#include "Termcap.h"
#include "TtyPrivate.h"
#include "KeyPrivate.h"
#include "extern.h"

const int NOCHANGE = -1;

static char MS, C2;
static int NF, NB, LINES, COLS;
static const char *AS, *AE, *AC, *GS, *GE, *G1, *G2;
static char *GT;
static const char *CS, *SF, *SR;
static const char *CL, *CE, *CM, *SE, *SO, *TE, *TI, *VI, *VE, *VS;
static const char *AL, *DL, *FS, *MD, *MH, *ME, *MR;
static const char *CF, *CB, *MF, *MB;

#ifndef NOKEYPAD
static const char *KS, *KE;
#endif

#ifdef CYRILLIC
static char Cy;
static const char *Cs, *Ce, *Ct;
#endif

static struct Captab outtab [] = {
	{ "ms", CAPFLG, 0, &MS, 0, 0, },
	{ "C2", CAPFLG, 0, &C2, 0, 0, },
#ifdef CYRILLIC
	{ "CY", CAPFLG, 0, &Cy, 0, 0, },
#endif
	{ "li", CAPNUM, 0, 0, &LINES, 0, },
	{ "co", CAPNUM, 0, 0, &COLS, 0, },
	{ "Nf", CAPNUM, 0, 0, &NF, 0, },
	{ "Nb", CAPNUM, 0, 0, &NB, 0, },
	{ "cl", CAPSTR, 0, 0, 0, &CL, },
	{ "ce", CAPSTR, 0, 0, 0, &CE, },
	{ "cm", CAPSTR, 0, 0, 0, &CM, },
	{ "se", CAPSTR, 0, 0, 0, &SE, },
	{ "so", CAPSTR, 0, 0, 0, &SO, },
	{ "Cf", CAPSTR, 0, 0, 0, &CF, },
	{ "Cb", CAPSTR, 0, 0, 0, &CB, },
	{ "Mf", CAPSTR, 0, 0, 0, &MF, },
	{ "Mb", CAPSTR, 0, 0, 0, &MB, },
	{ "md", CAPSTR, 0, 0, 0, &MD, },
	{ "mh", CAPSTR, 0, 0, 0, &MH, },
	{ "mr", CAPSTR, 0, 0, 0, &MR, },
	{ "me", CAPSTR, 0, 0, 0, &ME, },
	{ "te", CAPSTR, 0, 0, 0, &TE, },
	{ "ti", CAPSTR, 0, 0, 0, &TI, },
	{ "vi", CAPSTR, 0, 0, 0, &VI, },
	{ "vs", CAPSTR, 0, 0, 0, &VS, },
	{ "ve", CAPSTR, 0, 0, 0, &VE, },
#ifndef NOKEYPAD
	{ "ks", CAPSTR, 0, 0, 0, &KS, },
	{ "ke", CAPSTR, 0, 0, 0, &KE, },
#endif
	{ "al", CAPSTR, 0, 0, 0, &AL, },
	{ "dl", CAPSTR, 0, 0, 0, &DL, },
	{ "fs", CAPSTR, 0, 0, 0, &FS, },
	{ "as", CAPSTR, 0, 0, 0, &AS, },
	{ "ae", CAPSTR, 0, 0, 0, &AE, },
	{ "ac", CAPSTR, 0, 0, 0, &AC, },
	{ "gs", CAPSTR, 0, 0, 0, &GS, },
	{ "ge", CAPSTR, 0, 0, 0, &GE, },
	{ "g1", CAPSTR, 0, 0, 0, &G1, },
	{ "g2", CAPSTR, 0, 0, 0, &G2, },
	{ "gt", CAPSTR, 0, 0, 0, (const char**)&GT, },
	{ "cs", CAPSTR, 0, 0, 0, &CS, },
	{ "sf", CAPSTR, 0, 0, 0, &SF, },
	{ "sr", CAPSTR, 0, 0, 0, &SR, },
#ifdef CYRILLIC
	{ "Cs", CAPSTR, 0, 0, 0, &Cs, },
	{ "Ce", CAPSTR, 0, 0, 0, &Ce, },
	{ "Ct", CAPSTR, 0, 0, 0, &Ct, },
#endif
	{ { 0, 0, }, 0, 0, 0, 0, 0, },
};

static unsigned char textlinedraw [11] = {
	'-',    // 0    horisontal line
	'|',    // 1    vertical line
	'+',    // 2    lower left corner
	'-',    // 3    lower center
	'+',    // 4    lower right corner
	'|',    // 5    left center
	'+',    // 6    center cross
	'|',    // 7    right center
	'+',    // 8    upper left corner
	'-',    // 9    upper center
	'+',    // 10   upper right corner
};

static struct Keytab keytab [] = {
	{ "kl",         0,              meta ('l'),   },
	{ "kr",         0,              meta ('r'),   },
	{ "ku",         0,              meta ('u'),   },
	{ "kd",         0,              meta ('d'),   },
	{ "kN",         0,              meta ('n'),   },
	{ "kP",         0,              meta ('p'),   },
	{ "kh",         0,              meta ('h'),   },
	{ "kH",         0,              meta ('e'),   },
	{ "@7",         0,              meta ('e'),   },
	{ "kI",         0,              cntrl ('T'),  },
	{ "kb",         0,              cntrl ('H'),  },
	{ "k.",         0,              meta ('x'),   },
	{ "kD",         0,              meta ('x'),   },
	{ "kE",         0,              meta ('h'),   },
	{ "kS",         0,              meta ('e'),   },
	{ "k1",         0,              meta ('A'),   },
	{ "k2",         0,              meta ('B'),   },
	{ "k3",         0,              meta ('C'),   },
	{ "k4",         0,              meta ('D'),   },
	{ "k5",         0,              meta ('E'),   },
	{ "k6",         0,              meta ('F'),   },
	{ "k7",         0,              meta ('G'),   },
	{ "k8",         0,              meta ('H'),   },
	{ "k9",         0,              meta ('I'),   },
	{ "k0",         0,              meta ('J'),   },
	{ "k;",         0,              meta ('J'),   },
	{ "f1",         0,              meta ('A'),   },
	{ "f2",         0,              meta ('B'),   },
	{ "f3",         0,              meta ('C'),   },
	{ "f4",         0,              meta ('D'),   },
	{ "f5",         0,              meta ('E'),   },
	{ "f6",         0,              meta ('F'),   },
	{ "f7",         0,              meta ('G'),   },
	{ "f8",         0,              meta ('H'),   },
	{ "f9",         0,              meta ('I'),   },
	{ "f0",         0,              meta ('J'),   },
	{ 0,            "\0331",        meta ('A'),   },
	{ 0,            "\0332",        meta ('B'),   },
	{ 0,            "\0333",        meta ('C'),   },
	{ 0,            "\0334",        meta ('D'),   },
	{ 0,            "\0335",        meta ('E'),   },
	{ 0,            "\0336",        meta ('F'),   },
	{ 0,            "\0337",        meta ('G'),   },
	{ 0,            "\0338",        meta ('H'),   },
	{ 0,            "\0339",        meta ('I'),   },
	{ 0,            "\0330",        meta ('J'),   },
	{ 0,            "\033l",        meta ('l'),   },
	{ 0,            "\033r",        meta ('r'),   },
	{ 0,            "\033u",        meta ('u'),   },
	{ 0,            "\033d",        meta ('d'),   },
	{ 0,            "\033n",        meta ('n'),   },
	{ 0,            "\033p",        meta ('p'),   },
	{ 0,            "\033h",        meta ('h'),   },
	{ 0,            "\033e",        meta ('e'),   },
	{ 0,            "\033\033",     cntrl ('C'),  },
	{ 0,            0,              0,            },
};

#ifdef SIGTSTP
static Screen *screenptr;

void _Screen_tstp (int)
{
	if (screenptr) {
		screenptr->putStr (screenptr->Goto (CM, 0, screenptr->Lines-1));
		screenptr->Restore ();
	}
	kill (0, SIGSTOP);
	if (screenptr) {
		screenptr->Reopen ();
		screenptr->Redraw ();
	}
}
#endif // SIGTSTP

#ifdef CYRILLIC
void Screen::cyr (int on)
{
	if (on) {
		if (! cyroutput) {
			if (Cs)
				putStr (Cs);
			cyroutput = 1;
		}
	} else {
		if (cyroutput) {
			if (Ce)
				putStr (Ce);
			cyroutput = 0;
		}
	}
}
#endif

inline int Screen::putRawChar (unsigned char c)
{
	if (outptr >= outbuf + sizeof (outbuf))
		Flush ();
	return (*outptr++ = c);
}

inline void Screen::putChar (unsigned char c)
{
	if (UpperCaseMode) {
		if (c>='a' && c<='z')
			c += 'A' - 'a';
		else if (c>=0300 && c<=0336)
			c += 040;
		else if (c == 0337)
			c = '\'';
	}
#ifdef CYRILLIC
	if (c>=0300 && c<=0377) {
		cyr (1);
		c = CyrOutputTable [c - 0300];
	} else if (c>' ' && c<='~')
		cyr (0);
#endif
	putRawChar (c);
}

void Screen::putStr (const char *cp)
{
	int c;

	if (! cp)
		return;
	cp = skipDelay (cp);
	while ((c = *cp++))
		putRawChar (c);
}

const char *Screen::skipDelay (const char *cp)
{
	while (*cp>='0' && *cp<='9')
		++cp;
	if (*cp == '.') {
		++cp;
		while (*cp>='0' && *cp<='9')
			++cp;
	}
	if (*cp == '*')
		++cp;
	return (cp);
}

#ifdef INITFILE
static typefile (s)
char *s;
{
	int d, n;
	char buf [64];

	if ((d = open (s, 0)) < 0)
		return;
	Flush ();
	while ((n = read (d, buf, sizeof (buf))) > 0)
		write (1, buf, (unsigned) n);
	close (d);
}
#endif

Screen::Screen (int cmode, int gmode)
{
	VisualMask = VisualInverse;
	if (cmode > 1)
		VisualMask |= VisualColors | VisualBold | VisualDim;
	else if (cmode == 1)
		VisualMask |= VisualBold | VisualDim;
	if (gmode)
		VisualMask |= VisualGraph;
	UpperCaseMode = 0;
	outptr = outbuf;
	hidden = 0;
	beepflag = 0;
	flgs = oldflgs = NormalAttr = ClearAttr = 0x700;
	cury = curx = oldy = oldx = 0;
	clear = 1;
#ifdef CYRILLIC
	cyrinput = cyroutput = 0;
#endif

	char buf[2048];
	if (! InitCap (buf)) {
		outerr ("cannot read termcap\n");
		exit (1);
	}
	if (! Init ()) {
		outerr ("cannot initialize terminal\n");
		exit (1);
	}
	InitKey (keytab);
	Open ();
}

Screen::~Screen ()
{
	Close ();
	for (int i=0; i<Lines; ++i) {
		if (scr && scr[i])
			delete scr[i];
		if (oldscr && oldscr[i])
			delete oldscr[i];
	}
	if (scr)
		delete scr;
	if (oldscr)
		delete oldscr;
	if (firstch)
		delete firstch;
	if (lastch)
		delete lastch;
	if (lnum)
		delete lnum;
}

int Screen::Init ()
{
	GetCap (outtab);
	Lines = LINES;
	Columns = COLS;

	char *p = getenv ("LINES");
	if (p && *p)
		Lines = atoi (p);
	p = getenv ("COLUMNS");
	if (p && *p)
		Columns = atoi (p);

	if (Lines <= 6 || Columns <= 30)
		return (0);
	if (! CM)
		return (0);
	scrool = AL && DL;
	if (! (rscrool = SF && SR))
		SF = SR = 0;

	if (ME) {
		if (SO)
			SO = skipDelay (SO);
		else if (MR)
			SO = skipDelay (MR);
	}

	Visuals = 0;
	if (NF>0 && NB>0 && CF && (CB || C2))
		Visuals |= VisualColors;
	if (MH)
		Visuals |= VisualDim;
	if (MD)
		Visuals |= VisualBold;
	if (SO)
		Visuals |= VisualInverse;
	if (G1 || G2 || GT || AC)
		Visuals |= VisualGraph;

	if (HasColors ()) {
		if (NF > 16)
			NF = 16;
		if (NB > 16)
			NB = 16;
		if (! MF)
			MF = "0123456789ABCDEF";
		if (! MB)
			MB = "0123456789ABCDEF";
		for (int i=0; i<16; ++i) {
			ctab [i] = btab [i] = -1;
                }
		for (int i=0; i<16 && i<NF; ++i) {
			if (! MF [i])
				break;
			else if (MF[i]>='0' && MF[i]<='9')
				ctab [MF [i] - '0'] = i;
			else if (MF[i]>='A' && MF[i]<='F')
				ctab [MF [i] - 'A' + 10] = i;
                }
		for (int i=0; i<16 && i<NB; ++i) {
			if (! MB [i])
				break;
			else if (MB[i]>='0' && MB[i]<='9')
				btab [MB [i] - '0'] = i;
			else if (MF[i]>='A' && MF[i]<='F')
				btab [MB [i] - 'A' + 10] = i;
                }
		for (int i=1; i<8; ++i) {
			if (ctab[i] >= 0 && ctab[i+8] < 0)
				ctab [i+8] = ctab [i];
			if (ctab[i+8] >= 0 && ctab[i] < 0)
				ctab [i] = ctab [i+8];
			if (btab[i] >= 0 && btab[i+8] < 0)
				btab [i+8] = btab [i];
			if (btab[i+8] >= 0 && btab[i] < 0)
				btab [i] = btab [i+8];
		}
	} else {
		CE = 0;         // Don't use clear to end of line.
        }
	if (HasGraph ()) {
		const char *g = 0;
		if (G1)
			g = G1;
		else if (G2)
			g = G2;
		else if (GT) {
			GT [1] = GT [0];
			g = GT+1;
		}
		if (g) {
			for (int i=0; i<11 && *g; ++i, ++g)
				linedraw [i] = *g;
		} else if (AC) {
			GS = AS;
			GE = AE;
			for (; AC[0] && AC[1]; AC+=2)
				switch (AC[0]) {
				case 'l': linedraw [8] = AC[1]; break;
				case 'q': linedraw [0] = AC[1]; break;
				case 'k': linedraw [10] = AC[1]; break;
				case 'x': linedraw [1] = AC[1]; break;
				case 'j': linedraw [4] = AC[1]; break;
				case 'm': linedraw [2] = AC[1]; break;
				case 'w': linedraw [9] = AC[1]; break;
				case 'u': linedraw [7] = AC[1]; break;
				case 'v': linedraw [3] = AC[1]; break;
				case 't': linedraw [5] = AC[1]; break;
				case 'n': linedraw [6] = AC[1]; break;
				}
		}
	}
#ifdef CYRILLIC
	if (Cy) {
		int fd;

		if (! Cs)
			Cs = "\16";
		if (! Ce)
			Ce = "\17";
		if (! Ct)
			goto simpletab;
		fd = open (Ct, 0);
		if (fd < 0)
			goto simpletab;
		read (fd, (char*) CyrOutputTable, sizeof (CyrOutputTable));
		read (fd, (char*) CyrInputTable, sizeof (CyrInputTable));
		close (fd);
	} else {
		int i;
		Cs = Ce = 0;
simpletab:
		for (i=' '; i<='~'; ++i)
			CyrInputTable [i-' '] = i;
		for (i=0300; i<=0377; ++i)
			CyrOutputTable [i-0300] = i;
	}
#endif // CYRILLIC

	oldscr = new short* [Lines];
	scr = new short* [Lines];
	firstch = new short [Lines];
	lastch = new short [Lines];
	lnum = new short [Lines];

	if (!oldscr || !scr || !firstch || !lastch || !lnum) {
nomem:          outerr ("out of memory in Screen Init\n");
		Close ();
		return (0);
	}

	for (int i=0; i<Lines; ++i) {
		firstch [i] = lastch [i] = NOCHANGE;
		lnum [i] = i;
	}
	for (int i=0; i<Lines; ++i) {
		scr[i] = new short [Columns];
		oldscr[i] = new short [Columns];
	}
	for (int i=0; i<Lines; ++i) {
		if (! scr[i] || ! oldscr[i])
			goto nomem;
		for (short *sp=scr[i]; sp < scr[i]+Columns; ++sp)
			*sp = ' ' | NormalAttr;
		for (short *p=oldscr[i]; p < oldscr[i]+Columns; ++p)
			*p = ' ' | NormalAttr;
	}

#ifdef CYRILLIC
	cyr (0);
#endif
	return (1);
}

void Screen::Open ()
{
	SetTty ();
#ifdef SIGTSTP
	screenptr = this;
	signal (SIGTSTP, _Screen_tstp);
#endif
	if (TI)
		putStr (TI);
#ifndef NOKEYPAD
	if (KS)
		putStr (KS);
#endif
}

void Screen::Reopen ()
{
	SetTty ();
#ifdef SIGTSTP
	signal (SIGTSTP, _Screen_tstp);
#endif
	if (TI)
		putStr (TI);
#ifndef NOKEYPAD
	if (KS)
		putStr (KS);
#endif
}

void Screen::Close ()
{
	if (oldscr)
		setAttr (NormalAttr);
#ifdef CYRILLIC
	cyr (0);
#endif
	if (FS)
		putStr (FS);
	if (TE)
		putStr (TE);
#ifndef NOKEYPAD
	if (KE)
		putStr (KE);
#endif
	Flush ();
	ResetTty ();
#ifdef SIGTSTP
	signal (SIGTSTP, SIG_DFL);
	screenptr = 0;
#endif
}

void Screen::Restore ()
{
	if (oldscr)
		setAttr (NormalAttr);
#ifdef CYRILLIC
	cyr (0);
#endif
#ifdef STRINIT
	if (FS)
		putStr (FS);
#endif
	if (TE)
		putStr (TE);
#ifndef NOKEYPAD
	if (KE)
		putStr (KE);
#endif
	Flush ();
	ResetTty ();
#ifdef SIGTSTP
	signal (SIGTSTP, SIG_DFL);
#endif
}

void Screen::Flush ()
{
	if (outptr > outbuf)
		write (1, outbuf, (unsigned) (outptr-outbuf));
	outptr = outbuf;
}

void Screen::setColor (int fg, int bg)
{
	// This should be optimized later.
	// For example, we could have an array of 128 color
	// switching escape strings, precompiled during Init phase.
	fg = ctab [fg];
	if (fg < 0)
		fg = 7;
	bg = ctab [bg];
	if (bg < 0)
		bg = 0;
	if (C2) {
		putStr (Goto (CF, bg, fg));
	} else {
		putStr (Goto (CF, 0, fg));
		putStr (Goto (CB, 0, bg));
	}
}

void Screen::setAttr (int c)
{
	c &= 0xff00;
	if (oldflgs == c)
		return;

	// Set line drawing mode.
	if ((c ^ oldflgs) & GraphAttr) {
		if (HasGraph ())
			putStr ((c & GraphAttr) ? GS : GE);
		oldflgs ^= GraphAttr;
		if (oldflgs == c)
			return;
	}

	// Set direct color.
	if (HasColors ()) {
		setColor (c>>8 & 15, c>>12 & 7);
		oldflgs = c;
		return;
	}

	// Set dim/bold/inverse.
	int fg = c>>8 & 15;
	switch (Visuals & VisualMask & (VisualBold|VisualDim|VisualInverse)) {
	case VisualBold | VisualDim | VisualInverse:
		if (c & 0x7000) {
			if (fg > 7) goto bold_inverse;
			else if (fg < 7) goto dim_inverse;
			else goto inverse;
                }
		if (fg > 7) goto bold;
		if (fg < 7) goto dim;
		goto normal;
	case VisualDim | VisualInverse:
		if (! (c & 0x7000)) {
			if (fg >= 7) goto normal;
			else goto dim;
                }
		if (fg >= 7) goto inverse;
	dim_inverse:
		putStr (MH);
		putStr (SO);
		break;
	case VisualBold | VisualInverse:
		if (! (c & 0x7000)) {
			if (fg <= 7) goto normal;
			else goto bold;
                }
		if (fg <= 7) goto inverse;
	bold_inverse:
		putStr (MD);
		putStr (SO);
		break;
	case VisualBold | VisualDim:
		if (fg > 7) goto bold;
		if (fg < 7) goto dim;
		goto normal;
	case VisualInverse:
		if (! (c & 0x7000)) goto normal;
	inverse:
		putStr (ME);
		putStr (SO);
		break;
	case VisualBold:
		if (fg <= 7) goto normal;
	bold:
		putStr (ME ? ME : SE);
		putStr (MD);
		break;
	case VisualDim:
		if (fg >= 7) goto normal;
	dim:
		putStr (ME ? ME : SE);
		putStr (MH);
		break;
	case 0:
	normal:
		putStr (ME ? ME : SE);
		break;
	}
	oldflgs = c;
}

void Screen::clearScreen ()
{
	// Reset screen attributes to "normal" state.
	if (HasGraph ())
		putStr (GE);
	if (HasColors ())
		setColor (ClearAttr>>8 & 15, ClearAttr>>12 & 7);
	else {
		putStr (ME ? ME : SE);
		oldflgs = ClearAttr = NormalAttr;
	}

	// Clear screen.
	putStr (CL);

	oldy = 0;
	oldx = 0;
	for (int y=0; y<Lines; ++y) {
		firstch[y] = 0;
		lastch[y] = Columns-1;
		int n = Columns;
		for (short *p=oldscr[y]; --n>=0; ++p)
			*p = ' ' | ClearAttr;
	}
}

void Screen::Sync (int y)
{
	// Update the line on the screen.
	if (firstch [y] == NOCHANGE)
		return;

	short x = firstch [y];
	short *prev = &oldscr[y][x];
	short *next = &scr[y][x];
	short *end = &scr[y][lastch[y]];

	// Search the first nonblank character from the end of line.
	short *e = end;
	if (CE) {
		short *p = &scr[y][Columns-1];
		if ((*p & 0xff) == ' ')
			while (p>next && p[0] == p[-1])
				--p;
		// If there are more than 4 characters to clear, use CE.
		if (e>p+4 || (e>p && y >= Lines-1))
			e = p-1;
	}
	for (; next <= e; ++prev, ++next, ++x) {
		if (*next == *prev)
			continue;
		if (x >= Columns-1 && y >= Lines-1)
			return;
		moveTo (y, x);
		setAttr (*next);
		putChar (*prev = *next);
		++oldx;
	}

	// Clear the end of line.
	// First check if it is needed.
	for (int i=0; next+i<= end; ++i)
		if (next[i] != prev[i]) {
			moveTo (y, x);
			setAttr (*end);
			putStr (CE);
			while (next <= end)
				*prev++ = *next++;
			break;
		}
	firstch [y] = NOCHANGE;
}

void Screen::scroolScreen ()
{
	int line, n, topline, botline;
	int mustreset = 0;

	for (line=0; line<Lines; ++line) {
		// find next range to scrool

		// skip fresh lines
		while (line < Lines && lnum [line] < 0)
			++line;

		// last line reached - no range to scrool
		if (line >= Lines)
			break;

		// top line found
		topline = line;

		// skip range of old lines
		while (line < Lines-1 && lnum [line] + 1 == lnum [line+1])
			++line;

		// bottom line found
		botline = line;

		// compute number of scrools, >0 - forward
		n = topline - lnum [topline];

		if (n == 0)
			continue;
		else if (n > 0)
			topline = lnum [topline];
		else if (n < 0)
			botline = lnum [botline];

		// do scrool
		if (rscrool && !scrool && !CS) {
			// cannot scrool small regions if no scrool region
			if (2 * (botline - topline) < Lines-2) {
				for (line=topline; line<=botline; ++line) {
					firstch [line] = 0;
					lastch [line] = Columns-1;
				}
				return;
			}
			if (topline > 0) {
				for (line=0; line<topline; ++line) {
					firstch [line] = 0;
					lastch [line] = Columns-1;
				}
				topline = 0;
			}
			if (botline < Lines-1) {
				for (line=botline+1; line<Lines; ++line) {
					firstch [line] = 0;
					lastch [line] = Columns-1;
				}
				botline = Lines-1;
			}
		}

		// update in-core screen image
		if (n > 0) {
			for (int y=botline-n+1; y<=botline; ++y) {
				short *temp = oldscr[y];
				for (short *end = &temp[Columns-1]; end>=temp; --end)
					*end = ' ' | ClearAttr;
			}
			for (int i=n; i>0; --i) {
				short *temp = oldscr[botline];
				for (int y=botline; y>topline; --y)
					oldscr[y] = oldscr[y-1];
				oldscr[topline] = temp;
			}
		} else {
			for (int y=topline; y<topline-n; ++y) {
				short *temp = oldscr[y];
				for (short *end = &temp[Columns-1]; end>=temp; --end)
					*end = ' ' | ClearAttr;
			}
			for (int i=n; i<0; ++i) {
				short *temp = oldscr[topline];
				for (int y=topline; y<botline; ++y)
					oldscr[y] = oldscr[y+1];
				oldscr[botline] = temp;
			}
		}

		// set scrool region
		if (CS) {
			putStr (Goto (CS, botline, topline));
			mustreset = 1;
		}

		// do scrool n lines forward or backward
		if (n > 0) {
			if (CS || !scrool) {
				moveTo (CS ? topline : 0, 0);
				setAttr (ClearAttr);
				while (--n >= 0)
					putStr (SR);
			} else {
				while (--n >= 0) {
					moveTo (botline, 0);
					setAttr (ClearAttr);
					putStr (DL);
					moveTo (topline, 0);
					setAttr (ClearAttr);
					putStr (AL);
				}
			}
		} else {
			if (CS || !scrool) {
				moveTo (CS ? botline : Lines-1, 0);
				setAttr (ClearAttr);
				while (++n <= 0)
					putStr (SF);
			} else {
				while (++n <= 0) {
					moveTo (topline, 0);
					setAttr (ClearAttr);
					putStr (DL);
					moveTo (botline, 0);
					setAttr (ClearAttr);
					putStr (AL);
				}
			}
		}
	}
	if (mustreset)
		// unset scrool region
		putStr (Goto (CS, Lines-1, 0));
	// After scrolling the cursor position is lost.
	oldx = oldy = -1;
}

void Screen::moveTo (int y, int x)
{
	if (oldx==x && oldy==y)
		return;
	if (oldy==y && x>oldx && x<oldx+7) {
		short i;

		while (oldx < x) {
			i = oldscr[oldy][oldx];
			if ((i & ~0377) == oldflgs)
				putChar (i);
			else break;
			++oldx;
		}
		if (oldx == x)
			return;
	}
	if (!MS && oldflgs != NormalAttr) {
		putStr (ME);
		oldflgs = NormalAttr;
	}
	putStr (Goto (CM, x, y));
	oldx = x;
	oldy = y;
}

void Screen::pokeChar (int y, int x, int c)
{
	if (x<0 || x>=Columns)
		return;
	if (scr[y][x] != c) {
		if (firstch[y] == NOCHANGE)
			firstch[y] = lastch[y] = x;
		else if (x < firstch[y])
			firstch[y] = x;
		else if (x > lastch[y])
			lastch[y] = x;
		scr[y][x] = c;
	}
}

void Screen::Put (int c, int attr)
{
	if (attr)
		attr = attr<<8 & 0x7f00;
	else
		attr = flgs;

	switch (c = (unsigned char) c) {
	case ULC: case UT: case URC: case CLT: case CX: case CRT:
	case LLC: case LT: case LRC: case VERT: case HOR:
		c = "\10\11\12\5\6\7\2\3\4\1\0" [c - ULC];
		c = GraphAttr | (HasGraph () ? linedraw [c] : textlinedraw [c]);
	default:
		pokeChar (cury, curx++, c | attr);
		return;
	case '\t':
		for (int n = 8 - (curx & 7); n >= 0; n--) {
			pokeChar (cury, curx++, ' ' | attr);
                }
		return;
	case '\b':
		--curx;
		return;
	case '\n':
		while (curx < Columns)
			pokeChar (cury, curx++, ' ' | attr);
		if (++cury >= Lines)
			cury = 0;
	case '\r':
		curx = 0;
		return;
	}
}

void Screen::Sync ()
{
	if (clear) {
		clearScreen ();
		clear = 0;
		for (int y=0; y<Lines; y++)
			lnum[y] = y;
	} else if (rscrool || scrool)
		scroolScreen ();
	for (int y=0; y<Lines; y++) {
		Sync (y);
		lnum[y] = y;
	}
	if (! hidden && curx >= 0 && curx < Columns)
		moveTo (cury, curx);
	else if (VI)
		putStr (VI);
	else
		moveTo (Lines-1, Columns-1);
	if (beepflag) {
		putRawChar ('\007');
		beepflag = 0;
	}
	Flush ();
}

void Screen::Clear (int attr)
{
	if (! HasColors ())
		ClearAttr = NormalAttr;
	else if (attr)
		ClearAttr = attr<<8 & 0x7f00;
	else
		ClearAttr = flgs;
	for (int y=0; y<Lines; y++) {
		int minx = NOCHANGE;
		int maxx = NOCHANGE;
		short *end = &scr[y][Columns];
		for (short *sp=scr[y]; sp<end; sp++)
			if (*sp != (' ' | ClearAttr)) {
				maxx = sp - scr[y];
				if (minx == NOCHANGE)
					minx = sp - scr[y];
				*sp = ' ' | ClearAttr;
			}
		if (minx != NOCHANGE) {
			if (firstch[y] > minx
					|| firstch[y] == NOCHANGE)
				firstch[y] = minx;
			if (lastch[y] < maxx)
				lastch[y] = maxx;
		}
		lnum[y] = y;
	}
	curx = cury = 0;
	clear = 1;
	hidden = 0;
}

void Screen::DelLine (int n, int attr)
{
	if (n<0 || n>=Lines)
		return;
	if (attr)
		attr = attr<<8 & 0x7f00;
	else
		attr = flgs;
	short *temp = scr[n];
	for (int y=n; y < Lines-1; y++) {
		scr[y] = scr[y+1];
		lnum [y] = lnum [y+1];
		if (scrool || rscrool) {
			firstch [y] = firstch [y+1];
			lastch [y] = lastch [y+1];
		} else {
			firstch [y] = 0;
			lastch [y] = Columns-1;
		}
	}
	scr[Lines-1] = temp;
	lnum [Lines-1] = -1;
	firstch [Lines-1] = 0;
	lastch [Lines-1] = Columns-1;
	for (short *end = &temp[Columns]; temp<end; *temp++ = ' ' | attr);
}

void Screen::InsLine (int n, int attr)
{
	if (n<0 || n>=Lines)
		return;
	if (attr)
		attr = attr<<8 & 0x7f00;
	else
		attr = flgs;
	short *temp = scr[Lines-1];
	for (int y=Lines-1; y>n; --y) {
		scr[y] = scr[y-1];
		lnum [y] = lnum [y-1];
		if (scrool || rscrool) {
			firstch [y] = firstch [y-1];
			lastch [y] = lastch [y-1];
		} else {
			firstch [y] = 0;
			lastch [y] = Columns-1;
		}
	}
	lnum [n] = -1;
	scr[n] = temp;
	firstch [n] = 0;
	lastch [n] = Columns-1;
	for (short *end = &temp[Columns]; temp<end; *temp++ = ' ' | attr);
}

void Screen::Clear (int y, int x, int ny, int nx, int attr, int sym)
{
	if (attr)
		attr = attr<<8 & 0x7f00;
	else
		attr = flgs;
	sym = (sym & 0xff) | attr;
	for (; --ny>=0; ++y)
		for (int i=nx; --i>=0;)
			pokeChar (y, x+i, sym);
}

void Screen::PrintVect (int attr, const char *fmt, va_list vect)
{
	char buf [512];

	vsnprintf(buf, sizeof(buf), fmt, vect);
	Put (buf, attr);
}

void Screen::Print (int attr, const char *fmt, ...)
{
        va_list ap;
        va_start(ap, fmt);
	PrintVect (attr, fmt, ap);
        va_end(ap);
}

void Screen::Print (int y, int x, int attr, const char *fmt, ...)
{
        va_list ap;
        va_start(ap, fmt);
	Move (y, x);
	PrintVect (attr, fmt, ap);
        va_end(ap);
}

void Screen::HorLine (int y, int x, int nx, int attr)
{
	int sym = HasGraph () ? linedraw [0] : textlinedraw [0];
	if (attr)
		attr = attr<<8 & 0x7f00;
	else
		attr = flgs;
	while (--nx >= 0)
		pokeChar (y, x++, sym | GraphAttr | attr);
}

void Screen::VertLine (int x, int y, int ny, int attr)
{
	int sym = HasGraph () ? linedraw [1] : textlinedraw [1];
	if (attr)
		attr = attr<<8 & 0x7f00;
	else
		attr = flgs;
	while (--ny >= 0)
		pokeChar (y++, x, sym | GraphAttr | attr);
}

void Screen::DrawFrame (int y, int x, int ny, int nx, int attr)
{
	HorLine (y, x+1, nx-2, attr);
	HorLine (y+ny-1, x+1, nx-2, attr);
	VertLine (x, y+1, ny-2, attr);
	VertLine (x+nx-1, y+1, ny-2, attr);

	if (attr)
		attr = (attr<<8 & 0x7f00) | GraphAttr;
	else
		attr = flgs | GraphAttr;
	unsigned char *sym = HasGraph () ? linedraw : textlinedraw;
	pokeChar (y,      x,      sym[8]  | attr);      /* ULC */
	pokeChar (y,      x+nx-1, sym[10] | attr);      /* URC */
	pokeChar (y+ny-1, x,      sym[2]  | attr);      /* LLC */
	pokeChar (y+ny-1, x+nx-1, sym[4]  | attr);      /* LRC */
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
		short *p = &scr[y][x];
		for (int xx=0; xx<nx; ++xx, ++p) {
			*p = (*p & 0x80ff) | attr;
                }
		if (firstch[y] == NOCHANGE) {
			firstch[y] = x;
			lastch[y] = x+nx-1;
		} else if (x < firstch[y])
			firstch[y] = x;
		else if (x+nx-1 > lastch[y])
			lastch[y] = x+nx-1;
	}
}

void Screen::AttrLow (int y, int x)
{
	if (x>=0 && x<Columns) {
		int c = scr[y][x];
		if (c & 0x7000) {
			c &= ~0x7800;
			if (! (c & 0x700))
				c |= 0x700;
		} else
			c = ' ';
		pokeChar (y, x, c);
	}
}

void Screen::ClearLine (int y, int x, int attr)
{
	if (y < 0) {
		y = cury;
		x = curx;
	}
	if (x >= Columns)
		return;
	if (x < 0)
		x = 0;
	if (attr)
		attr = attr<<8 & 0x7f00;
	else
		attr = flgs;
	int minx = NOCHANGE;
	short *end = &scr[y][Columns];
	short *maxx = &scr[y][x];
	for (short *sp=maxx; sp<end; sp++)
		if (*sp != (' ' | attr)) {
			maxx = sp;
			if (minx == NOCHANGE)
				minx = sp - scr[y];
			*sp = ' ' | attr;
		}
	if (minx != NOCHANGE) {
		if (firstch[y] > minx || firstch[y] == NOCHANGE)
			firstch[y] = minx;
		if (lastch[y] < maxx - scr[y])
			lastch[y] = maxx - scr[y];
	}
}
