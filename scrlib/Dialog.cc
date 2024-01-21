//
// Dialog windows implementation.
//
// Copyright (C) 1996 Cronyx Ltd.
// Author: Serge Vakulenko, <vak@cronyx.ru>
//
// This software is distributed with NO WARRANTIES, not even the implied
// warranties for MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
//
// Authors grant any other persons or organisations permission to use
// or modify this software as long as this message is kept with the software,
// all derivative works or modified versions.
//

//
// horizontal glue		|
// horizontal space		-
// vertical space		{}
// next row			space
// next column			+
// Group			(...)
// Group with centering		<...>
// Group with frame		[...]
// Label			{label}
// Button int			B {label}
// String char* (maxlen)	S :width :maxlen
// Password char* (maxlen)      P :width :maxlen
// Title char*                  T :width
// RadioButton int		R :val {label} :val {label}...
// Number long (min max step)	N :min :max :step
// Command			* :val {label}
// List                         L :width :maxw
//
#include <stdarg.h>
#include "extern.h"
#include "Screen.h"
#include "Dialog.h"

enum {
	ColorText		    = 0,
	ColorFrame		    = 1,
	ColorTextSelected	    = 2,
	ColorField		    = 3,
	ColorFieldSelected	    = 4,
	ColorCommand		    = 5,
	ColorCommandDefault	    = 6,
	ColorCommandSelected	    = 7,
	ColorCommandDefaultSelected = 8,
};

static const int GRP_FRAMED = 1;
static const int GRP_CENTER = 2;

#if 1
extern "C" {
	void Message (char *fmt, ...);
	getch (void);
};
#endif

int DialogElement::baserow;
int DialogElement::basecol;
DialogElement *DialogElement::list;
unsigned char *DialogElement::palette;
Screen *DialogElement::scr;

DialogElement::DialogElement (int row, int col, int reg)
{
	r = row;
	c = col;
	w = h = 0;
	oblig = 0;
	if (reg)
		if (list) {
			next = list;
			prev = list->prev;
			list->prev = this;
			prev->next = this;
		} else
			list = next = prev = this;
}

DialogLabel::DialogLabel (int row, int col, char **p)
	: DialogElement (row, col)
{
	label = GetString (p);
	w = label ? strlen (label) : 0;
	h = 1;
}

DialogButton::DialogButton (int row, int col, char **p, int *v)
	: DialogElement (row, col, 1)
{
	label = GetString (p);
	w = 4 + (label ? strlen (label) : 0);
	h = 1;
	val = v;
}

DialogRadioButton::DialogRadioButton (int row, int col, char **p, int *v)
	: DialogElement (row, col, 1)
{
	setval = GetNumber (p);
	label = GetString (p);
	int len = label ? strlen (label) : 0;
	w = len ? len + 4 : 3;
	h = 1;
	val = v;
}

DialogList::DialogList (int row, int col, char **p, long *v, int (*f) (...))
	: DialogElement (row, col, 1)
{
	w = 2 + GetNumber (p);
	maxw = GetNumber (p);
	if (! maxw)
		maxw = 32;
	h = 1;
	val = v;
	func = (int(*)(Screen*,int,int,int,long*,char*)) f;
	(*func) (0, 0, 0, 0, v, str);
}

DialogTitle::DialogTitle (int row, int col, char **p, char *v)
	: DialogElement (row, col, 0)
{
	w = GetNumber (p);
	h = 1;
	val = v;
}

DialogString::DialogString (int row, int col, char **p, char *v)
	: DialogElement (row, col, 1)
{
	w = GetNumber (p);
	maxlen = GetNumber (p);
	if (! maxlen)
		maxlen = 32;
	h = 1;
	val = v;
}

DialogPassword::DialogPassword (int row, int col, char **p, char *v)
	: DialogElement (row, col, 1)
{
	w = GetNumber (p);
	maxlen = GetNumber (p);
	if (! maxlen)
		maxlen = 32;
	h = 1;
	val = v;
}

DialogNumber::DialogNumber (int row, int col, char **p, long *v)
	: DialogElement (row, col, 1)
{
	min = GetNumber (p);
	max = GetNumber (p);
	step = GetNumber (p);
	if (! max)
		max = 999999999;
	if (! step)
		step = 1;
	w = 1;
	for (long i=max; i; i/=10)
		++w;
	h = 1;
	val = v;
}

DialogMask::DialogMask (int row, int col, char **p, long *v)
	: DialogElement (row, col, 1)
{
	w = GetNumber (p);
	h = 1;
	val = v;
}

DialogCommand::DialogCommand (int row, int col, char **p)
	: DialogElement (row, col, 1)
{
	val = GetNumber (p);
	label = GetString (p);
	w = label ? strlen (label) : 0;
	h = 1;
}

int DialogButton::Run ()
{
	scr->Put (r+baserow, c+basecol, *val?"[*] ":"[ ] ",
		palette[ColorTextSelected]);
	if (label)
		scr->Put (label, palette[ColorTextSelected]);
	for (;;) {
		scr->Move (r+baserow, c+basecol+1);
		scr->Sync ();
		switch (scr->GetKey ()) {
		default: scr->Beep (); continue;
		case cntrl ('['): return DialogCANCEL;	// Esc
		case cntrl ('C'): return DialogCANCEL;	// ^C
		case meta ('J'):  return DialogCANCEL;	// F10
		case cntrl ('M'): return DialogOK;	// Return
		case cntrl ('J'): return DialogOK;	// Newline
		case cntrl ('I'): return DialogNEXT;	// Tab
		case cntrl ('H'): return DialogPREV;	// Backspace
		case meta ('d'):  return DialogDOWN;
		case meta ('r'):  return DialogRIGHT;
		case meta ('u'):  return DialogUP;
		case meta ('l'):  return DialogLEFT;
		case meta ('h'):  return DialogHOME;
		case meta ('e'):  return DialogEND;
		case ' ':				// Space -- switch
			*val = ! *val;
			scr->Put (*val?'*':' ', palette[ColorTextSelected]);
			continue;
		}
	}
}

int DialogRadioButton::Run ()
{
	scr->Move (r+baserow, c+basecol);
	scr->Put (*val == setval ? "(*) ":"( ) ",
		palette[ColorTextSelected]);
	if (label)
		scr->Put (label, palette[ColorTextSelected]);
	for (;;) {
		scr->Move (r+baserow, c+basecol+1);
		scr->Sync ();
		switch (scr->GetKey ()) {
		default: scr->Beep (); continue;
		case cntrl ('['): return DialogCANCEL;	// Esc
		case cntrl ('C'): return DialogCANCEL;	// ^C
		case meta ('J'):  return DialogCANCEL;	// F10
		case cntrl ('M'): return DialogOK;	// Return
		case cntrl ('J'): return DialogOK;	// Newline
		case cntrl ('I'): return DialogNEXT;	// Tab
		case cntrl ('H'): return DialogPREV;	// Backspace
		case meta ('d'):  return DialogDOWN;
		case meta ('r'):  return DialogRIGHT;
		case meta ('u'):  return DialogUP;
		case meta ('l'):  return DialogLEFT;
		case meta ('h'):  return DialogHOME;
		case meta ('e'):  return DialogEND;
		case ' ':				// Space -- switch
			*val = setval;
			scr->Put ('*', palette[ColorTextSelected]);
			for (DialogElement *e=list; e; e=e->Next())
				if (e != this)
					e->Redraw ();
			continue;
		}
	}
}

int DialogCommand::Run ()
{
	if (label)
		scr->Put (r+baserow, c+basecol, label, val==1 ?
			palette[ColorCommandDefaultSelected] :
			palette[ColorCommandSelected]);
	for (;;) {
		scr->HideCursor ();
		scr->Sync ();
		switch (scr->GetKey ()) {
		default: scr->Beep (); continue;
		case cntrl ('['): return DialogCANCEL;	// Esc
		case cntrl ('C'): return DialogCANCEL;	// ^C
		case meta ('J'):  return DialogCANCEL;	// F10
		case cntrl ('I'): return DialogNEXT;	// Tab
		case cntrl ('H'): return DialogPREV;	// Backspace
		case meta ('d'):  return DialogDOWN;
		case meta ('r'):  return DialogRIGHT;
		case meta ('u'):  return DialogUP;
		case meta ('l'):  return DialogLEFT;
		case meta ('h'):  return DialogHOME;
		case meta ('e'):  return DialogEND;
		case cntrl ('M'): return val;		// Return
		case cntrl ('J'): return val;		// Newline
		}
	}
}

int DialogList::Run ()
{
again:
	int len = strlen (str);
	scr->Move (r+baserow, c+basecol);
	scr->Put ('[', palette[ColorText]);
	for (int i=0; i<w-2; ++i)
		scr->Put (i<len ? str[i] : ' ', palette[ColorFieldSelected]);
	scr->Put (']', palette[ColorText]);
	for (;;) {
		scr->Move (r+baserow, c+basecol+1);
		scr->Sync ();
		int k = scr->GetKey ();
		switch (k) {
		default: scr->Beep (); continue;
		case cntrl ('['): return DialogCANCEL;	// Esc
		case cntrl ('C'): return DialogCANCEL;	// ^C
		case meta ('J'):  return DialogCANCEL;	// F10
		case cntrl ('M'): return DialogOK;	// Return
		case cntrl ('J'): return DialogOK;	// Newline
		case cntrl ('I'): return DialogNEXT;	// Tab
		case meta ('d'):  return DialogDOWN;
		case meta ('u'):  return DialogUP;
		case meta ('r'):  return DialogRIGHT;
		case meta ('l'):  return DialogLEFT;
		case meta ('h'):  return DialogHOME;
		case meta ('e'):  return DialogEND;
		case ' ':         break;                // Choose

		}
		int col = c + basecol;
		if (col + maxw >= scr->Columns)
			col -= maxw - w;
		if ((*func) (scr, r+baserow, col, maxw, val, str) < 0)
			return DialogCANCEL;
		goto again;
	}
}

int DialogString::Run ()
{
	scr->PutLimited (r+baserow, c+basecol, val, w,
		palette[ColorFieldSelected]);
	int x = 0;
	int len = strlen (val);
	for (;;) {
		scr->Move (r+baserow, c+basecol+x);
		scr->Sync ();
		int k = scr->GetKey ();
		switch (k) {
		case cntrl ('['): return DialogCANCEL;	// Esc
		case cntrl ('C'): return DialogCANCEL;	// ^C
		case meta ('J'):  return DialogCANCEL;	// F10
		case cntrl ('M'): return DialogOK;	// Return
		case cntrl ('J'): return DialogOK;	// Newline
		case cntrl ('I'): return DialogNEXT;	// Tab
		case meta ('d'):  return DialogDOWN;
		case meta ('u'):  return DialogUP;
		case meta ('r'):			// Right
			if (x < len)
				++x;
			continue;
		case meta ('l'):			// Left
			if (x > 0)
				--x;
			continue;
		case meta ('h'):			// Home
			x = 0;
			continue;
		case meta ('e'):			// End
			x = len;
			continue;
		default:
			if (k < ' ' || k == 0177 || k > 0377 || len >= maxlen) {
				scr->Beep ();
				continue;
			}
			for (int i=len; i>x; --i)
				val[i] = val[i-1];
			val[x] = k;
			if (len < maxlen)
				++len;
			val[len] = 0;
			scr->PutLimited (val+x, w-x,
				palette[ColorFieldSelected]);
			++x;
			break;
		case cntrl ('H'):			// Backspace
			if (x == 0)
				continue;
			--x;
			scr->Move (r+baserow, c+basecol+x);
			// Fall through...
		case meta ('x'):			// Delete
			if (x == len)
				continue;
			for (i=x; i<len; ++i)
				val[i] = val[i+1];
			--len;
			scr->PutLimited (val+x, w-x,
				palette[ColorFieldSelected]);
			scr->Put (' ', palette[ColorFieldSelected]);
			break;
		}
	}
}

int DialogPassword::Run ()
{
	int len = strlen (val);
	int x = 0;
	scr->Move (r+baserow, c+basecol);
	for (int i=0; i<w; ++i)
		scr->Put (i<len ? '*' : ' ', palette[ColorFieldSelected]);
	for (;;) {
		scr->Move (r+baserow, c+basecol+x);
		scr->Sync ();
		int k = scr->GetKey ();
		switch (k) {
		case cntrl ('['): return DialogCANCEL;	// Esc
		case cntrl ('C'): return DialogCANCEL;	// ^C
		case meta ('J'):  return DialogCANCEL;	// F10
		case cntrl ('M'): return DialogOK;	// Return
		case cntrl ('J'): return DialogOK;	// Newline
		case cntrl ('I'): return DialogNEXT;	// Tab
		case meta ('d'):  return DialogDOWN;
		case meta ('u'):  return DialogUP;
		case meta ('r'):			// Right
			if (x < len)
				++x;
			continue;
		case meta ('l'):			// Left
			if (x > 0)
				--x;
			continue;
		case meta ('h'):			// Home
			x = 0;
			continue;
		case meta ('e'):			// End
			x = len;
			continue;
		default:
			if (k < ' ' || k == 0177 || k > 0377 || len >= maxlen) {
				scr->Beep ();
				continue;
			}
			for (int i=len; i>x; --i)
				val[i] = val[i-1];
			val[x++] = k;
			if (len < maxlen)
				++len;
			val[len] = 0;
			scr->Put (r+baserow, c+basecol+len-1, '*',
				palette[ColorFieldSelected]);
			break;
		case cntrl ('H'):			// Backspace
			if (x == 0)
				continue;
			--x;
			// Fall through...
		case meta ('x'):			// Delete
			if (x == len)
				continue;
			for (i=x; i<len; ++i)
				val[i] = val[i+1];
			--len;
			scr->Put (r+baserow, c+basecol+len, ' ',
				palette[ColorFieldSelected]);
			break;
		}
	}
}

int DialogNumber::Run ()
{
	char buf[80];
	int ret = 0;

	*buf = 0;
	if (*val)
		sprintf (buf, "%ld", *val);
	scr->Put (r+baserow, c+basecol, buf, palette[ColorFieldSelected]);
	int x = 0;
	int len = strlen (buf);
	while (! ret) {
		scr->Move (r+baserow, c+basecol+x);
		scr->Sync ();
		int k = scr->GetKey ();
		switch (k) {
		case cntrl ('['): return DialogCANCEL;		// Esc
		case cntrl ('C'): return DialogCANCEL;		// ^C
		case meta ('J'):  return DialogCANCEL;		// F10
		case cntrl ('M'): ret = DialogOK;   break;	// Return
		case cntrl ('J'): ret = DialogOK;   break;	// Newline
		case cntrl ('I'): ret = DialogNEXT; break;	// Tab
		case meta ('d'):  ret = DialogDOWN; break;
		case meta ('u'):  ret = DialogUP;   break;
		case meta ('r'):				// Right
			if (x < len)
				++x;
			continue;
		case meta ('l'):				// Left
			if (x > 0)
				--x;
			continue;
		case meta ('h'):				// Home
			x = 0;
			continue;
		case meta ('e'):				// End
			x = len;
			continue;
		default:
			if (k < '0' || k > '9' || len >= w) {
				scr->Beep ();
				continue;
			}
			for (int i=len; i>x; --i)
				buf[i] = buf[i-1];
			buf[x] = k;
			buf[++len] = 0;
			scr->PutLimited (buf+x, w-x,
				palette[ColorFieldSelected]);
			++x;
			break;
		case cntrl ('H'):				// Backspace
			if (x <= 0)
				continue;
			--x;
			scr->Move (r+baserow, c+basecol+x);
			// Fall through...
		case meta ('x'):				// Delete
			if (x == len)
				continue;
			for (i=x; i<len; ++i)
				buf[i] = buf[i+1];
			--len;
			scr->PutLimited (buf+x, w-x,
				palette[ColorFieldSelected]);
			scr->Put (' ', palette[ColorFieldSelected]);
			break;
		}
	}
	*val = strtol (buf, 0, 0);
	if (*val > max)
		*val = max;
	*val = (*val - min) / step * step + min;
	return ret;
}

int DialogMask::Run ()
{
	int ret = 0;
	int x = 0;
	while (! ret) {
		scr->Move (r+baserow, c+basecol);
		for (int i=0; i<w; ++i)
			scr->Put (*val >> i & 1 ? '#' : '.',
				palette[ColorFieldSelected]);
		scr->Move (r+baserow, c+basecol+x);
		scr->Sync ();
		int k = scr->GetKey ();
		switch (k) {
		case cntrl ('['): return DialogCANCEL;		// Esc
		case cntrl ('C'): return DialogCANCEL;		// ^C
		case meta ('J'):  return DialogCANCEL;		// F10
		case cntrl ('M'): ret = DialogOK;   break;	// Return
		case cntrl ('J'): ret = DialogOK;   break;	// Newline
		case cntrl ('I'): ret = DialogNEXT; break;	// Tab
		case meta ('d'):  ret = DialogDOWN; break;
		case meta ('u'):  ret = DialogUP;   break;
		case ' ':					// Space
			*val ^= 1L << x;
			// fall through...
		case meta ('r'):				// Right
			if (x < w-1)
				++x;
			continue;
		case cntrl ('H'):				// Backspace
		case meta ('l'):				// Left
			if (x > 0)
				--x;
			continue;
		case meta ('h'):				// Home
			x = 0;
			continue;
		case meta ('e'):				// End
			x = w-1;
			continue;
		default:
			scr->Beep ();
			continue;
		}
	}
	return ret;
}

DialogGroup::DialogGroup (int row0, int col0, char **fmt, void ***arg,
	int flags, DialogGroup *parent) : DialogElement (row0, col0)
{
	DialogElement *v = 0;
	void *arg1;

	tab = 0;
	maxnv = 0;
	nv = 0;
	frameflag = (flags & GRP_FRAMED) != 0;
	if (frameflag)
		++row0, col0 += 2;
	int currow = row0;
	int curcol = col0;
	while (**fmt) {
		int oblig = (**fmt >= 'a' && **fmt <= 'z');
		switch (*(*fmt)++) {
		default:
			continue;
		case 0:
			--(*fmt);
			// fall through...
		case ']':
		case ')':
		case '>':
			if (frameflag)
				++h, w += 2;
			if ((flags & GRP_CENTER) && ! col0)
				Center (parent->w);
			return;
		case '-':
			++curcol;
			continue;
		case '|':
			if (! v)
				continue;
			currow = v->r;
			curcol = v->c + v->w + 1;
			continue;
		case '+':
			currow = row0;
			col0 = curcol = c + (++w);
			continue;
		case '{':
			if (**fmt == '}') {
				++currow;
				++(*fmt);
				continue;
			}
			--(*fmt);
			v = new DialogLabel (currow, curcol, fmt);
			break;
		case '_':
			v = new DialogLine (currow, curcol, this);
			break;
		case '*':
			v = new DialogCommand (currow, curcol, fmt);
			break;
		case '(':
			v = new DialogGroup (currow, curcol, fmt, arg);
			break;
		case '[':
			v = new DialogGroup (currow, curcol, fmt,
				arg, GRP_FRAMED);
			break;
		case '<':
			v = new DialogGroup (currow, curcol, fmt,
				arg, GRP_CENTER, this);
			break;
		case 'b': case 'B':
			v = new DialogButton (currow, curcol, fmt,
				(int*) *(*arg)++);
			v->oblig = oblig;
			break;
		case 't': case 'T':
			v = new DialogTitle (currow, curcol, fmt,
				(char*) *(*arg)++);
			v->oblig = oblig;
			break;
		case 's': case 'S':
			v = new DialogString (currow, curcol, fmt,
				(char*) *(*arg)++);
			v->oblig = oblig;
			break;
		case 'p': case 'P':
			v = new DialogPassword (currow, curcol, fmt,
				(char*) *(*arg)++);
			v->oblig = oblig;
			break;
		case 'l': case 'L':
			arg1 = *(*arg)++;
			v = new DialogList (currow, curcol, fmt, (long*) arg1,
				(int(*)(...)) *(*arg)++);
			v->oblig = oblig;
			break;
		case 'r': case 'R':
			v = new DialogRadioButton (currow, curcol, fmt,
				(int*) *(*arg)++);
			break;
		case ':':
			--(*fmt);
			v = new DialogRadioButton (currow, curcol, fmt,
				(int*) (*arg)[-1]);
			break;
		case 'n': case 'N':
			v = new DialogNumber (currow, curcol, fmt,
				(long*) *(*arg)++);
			v->oblig = oblig;
			break;
		case 'm': case 'M':
			v = new DialogMask (currow, curcol, fmt,
				(long*) *(*arg)++);
			v->oblig = oblig;
			break;
		}
		if (v) {
			Add (v);
			curcol = col0;
			currow = v->r + v->h;
		}
	}
}

void DialogGroup::Add (DialogElement *v)
{
	if (nv >= maxnv) {
		maxnv += 10;
		DialogElement **newtab = new DialogElement *[maxnv];
		if (! newtab)
			return;
		if (nv) {
			memcpy (newtab, tab, nv * sizeof (*tab));
			delete tab;
		}
		tab = newtab;
	}
	tab[nv++] = v;
	if (w < v->c + v->w - c)
		w = v->c + v->w - c;
	if (h < v->r + v->h - r)
		h = v->r + v->h - r;
}

DialogGroup::~DialogGroup ()
{
	if (! tab)
		return;
	for (int i=0; i<nv; ++i)
		if (tab[i])
			delete tab[i];
	delete tab;
}

void DialogGroup::Center (int wid)
{
	if (w < wid)
		w = wid;
	int i;
	if (nv > 1 && tab[0]->r == tab[1]->r) {		// horizontal group
		int n, space = w;
		for (i=0; i<nv; ++i)
			space -= tab[i]->w;
		int off = 0;
		for (i=0, n=nv+1; i<nv; --n, ++i) {
			off += space/n;
			tab[i]->c += off;
			space -= space/n;
		}
	} else 						// vertical group
		for (i=0; i<nv; ++i)
			tab[i]->c = (w - tab[i]->w) / 2;
}

DialogElement *DialogElement::Find (int dir)
{
	DialogElement *e, *x = this;
	int d = 0, dr, dc, xd;

	switch (dir) {
	case DialogRIGHT:
		for (xd=9999, e=list; e; e=e->Next())
			if (e->r == r && e->c > c &&
			    (d = e->c - c - w) < xd)
				x = e, xd = d;
		break;
	case DialogLEFT:
		for (xd=9999, e=list; e; e=e->Next())
			if (e->r == r && e->c < c &&
			    (d = c - e->c - e->w) < xd)
				x = e, xd = d;
		break;
	case DialogUP:
		for (xd=9999, e=list; e; e=e->Next())
			if (e->c == c && e->r < r &&
			    (d = r - e->r - e->h) < xd)
				x = e, xd = d;
		break;
	case DialogDOWN:
		for (xd=9999, e=list; e; e=e->Next())
			if (e->c == c && e->r > r &&
			    (d = e->r - r - h) < xd)
				x = e, xd = d;
		break;
	}
	if (x != this)
		return x;
	for (xd=9999, e=list; e; e=e->Next()) {
		switch (dir) {
		case DialogDOWN:  if (e->r > r)         break; continue;
		case DialogUP:    if (e->r < r)         break; continue;
		case DialogRIGHT: if (e->c >= c + w)    break; continue;
		case DialogLEFT:  if (e->c + e->w <= c) break; continue;
		}
		dr = e->r - r;
		if (e->c + e->w < c)
			dc = c - e->c - e->w;
		else if (e->c > c + w)
			dc = e->c - c - w;
		else
			dc = 0;
		d = 4 * dr * dr + dc * dc;
		if (d < xd) {
			x = e;
			xd = d;
		}
	}
	return x;
}

DialogElement *DialogElement::Home ()
{
	DialogElement *e, *x = 0;
	int xd = 9999;

	for (e=list; e; e=e->Next()) {
		int d = 4 * e->r * e->r + e->c * e->c;
		if (d < xd) {
			x = e;
			xd = d;
		}
	}
	return x;
}

DialogElement *DialogElement::End ()
{
	DialogElement *e, *x = 0;
	int xd = 0;

	for (e=list; e; e=e->Next()) {
		int d = 4 * e->r * e->r + e->c * e->c;
		if (d > xd) {
			x = e;
			xd = d;
		}
	}
	return x;
}

int Dialog::Run (Screen *scr)
{
	int w = grp->w + 4;
	int h = grp->h + 2;
	r = 1 + (scr->Lines - h) / 2;
	c = 2 + (scr->Columns - w) / 2;

	// Save box.
	Box box (*scr, r-1, c-2, h+1, w+1);
	scr->Clear (r, c-1, h-2, 1, palette[ColorText], ' ');
	scr->Clear (r, c+w-4, h-2, 1, palette[ColorText], ' ');

	// Draw frame.
	scr->DrawFrame (r-1, c-2, h, w, palette[ColorFrame]);
	scr->Put (r-1, c-2 + (w-strlen(title)) / 2, title, palette[ColorFrame]);

	// Draw shadow.
	for (int i=0; i<w; ++i)
		scr->AttrLow (r-1+h, c+i-1);
	for (i=0; i<h-1; ++i)
		scr->AttrLow (r+i, c+w-2);

	DialogElement::scr = scr;
	DialogElement::baserow = r;
	DialogElement::basecol = c;
	DialogElement::palette = palette;
	DialogElement::list = list;

	grp->Draw ();					// draw dialog

	DialogElement *e = list;
	int n = DialogCANCEL;
	while (e) {
		n = e->Run ();				// run dialog
		if (n == DialogOK)
			for (; e->next != e->list; e = e->next)
				if (e->next->oblig) {
					n = DialogNEXT;
					break;
				}
		if (n >= 0)
			break;
		e->Draw ();
		switch (n) {
		default:	 scr->Beep ();		     continue;
		case DialogHOME: e = DialogElement::Home (); continue;
		case DialogEND:  e = DialogElement::End ();  continue;
		case DialogNEXT: e = e->next;		     continue;
		case DialogPREV: e = e->prev;		     continue;
		case DialogUP:
		case DialogDOWN:
		case DialogRIGHT:
		case DialogLEFT: e = e->Find (n);	     continue;
		}
	}
	scr->Put (box);					// restore screen
	return (n);
}

char *DialogElement::GetString (char **p)
{
	char buf[256], *q = buf;

	while (**p && (**p==' ' || **p=='\t' || **p=='\r' || **p=='\n'))
		++(*p);
	if (**p == '{') {
		while (*++(*p) && **p!='}')
			*q++ = **p;
		if (**p)
			(*p)++;
	}
	*q = 0;
	q = new char [strlen(buf) + 1];
	if (! q)
		return (0);
	strcpy (q, buf);
	return (q);
}

long DialogElement::GetNumber (char **p)
{
	long v = 0;

	while (**p && (**p==' ' || **p=='\t' || **p=='\r' || **p=='\n'))
		++(*p);
	if (**p != ':')
		return (0);
	while (*++(*p) && **p>='0' && **p<='9')
		v = v * 10 + **p - '0';
	return (v);
}

Dialog::Dialog (char *t, unsigned char *pal, char *fmt, ...)
{
	void **arg = (void**) &fmt + 1;

	title = t;
	palette = pal;
	DialogElement::list = 0;
	grp = new DialogGroup (0, 0, &fmt, &arg);
	list = DialogElement::list;
	if (grp->w < strlen (title) - 2)
		grp->w = strlen (title) - 2;
}

void DialogLabel::Draw ()
{
	if (label)
		scr->Put (r+baserow, c+basecol, label, palette[ColorText]);
}

void DialogLine::Draw ()
{
	if (! w) {
		w = parent->w;
		if (parent->frameflag)
			w -= 4;
	}
	scr->HorLine (r+baserow, c+basecol, w, palette[ColorText]);
}

void DialogButton::Draw ()
{
	scr->Put (r+baserow, c+basecol, *val?"[*] ":"[ ] ", palette[ColorText]);
	if (label)
		scr->Put (label, palette[ColorText]);
}

void DialogNumber::Draw ()
{
	scr->Print (r+baserow, c+basecol, palette[ColorField], "%*s", w, "");
	scr->Print (r+baserow, c+basecol, palette[ColorField], "%ld", *val);
}

void DialogMask::Draw ()
{
	scr->Move (r+baserow, c+basecol);
	for (int i=0; i<w; ++i)
		scr->Put (*val >> i & 1 ? '#' : '.', palette[ColorField]);
}

void DialogCommand::Draw ()
{
	if (label)
		scr->Put (r+baserow, c+basecol, label, val==1 ?
			palette[ColorCommandDefault] : palette[ColorCommand]);
}

void DialogRadioButton::Draw ()
{
	scr->Move (r+baserow, c+basecol);
	scr->Put (*val == setval ? "(*) ":"( ) ", palette[ColorText]);
	if (label)
		scr->Put (label, palette[ColorText]);
}

void DialogRadioButton::Redraw ()
{
	scr->Move (r+baserow, c+basecol+1);
	scr->Put (*val == setval ? '*' : ' ', palette[ColorText]);
}

void DialogList::Draw ()
{
	int len = strlen (str);
	scr->Move (r+baserow, c+basecol);
	scr->Put ('[', palette[ColorText]);
	for (int i=0; i<w-2; ++i)
		scr->Put (i<len ? str[i] : ' ', palette[ColorField]);
	scr->Put (']', palette[ColorText]);
}

void DialogTitle::Draw ()
{
	int len = strlen (val);
	scr->Move (r+baserow, c+basecol);
	for (int i=0; i<w; ++i)
		scr->Put (i<len ? val[i] : ' ', palette[ColorField]);
}

void DialogString::Draw ()
{
	int len = strlen (val);
	scr->Move (r+baserow, c+basecol);
	for (int i=0; i<w; ++i)
		scr->Put (i<len ? val[i] : ' ', palette[ColorField]);
}

void DialogPassword::Draw ()
{
	int len = strlen (val);
	scr->Move (r+baserow, c+basecol);
	for (int i=0; i<w; ++i)
		scr->Put (i<len ? '*' : ' ', palette[ColorField]);
}

void DialogGroup::Draw ()
{
	scr->Clear (r+baserow, c+basecol, h, w, palette[ColorText], ' ');
	if (frameflag)
		scr->DrawFrame (r+baserow, c+basecol, h, w,
			palette[ColorFrame]);
	for (int i=0; i<nv; ++i)
		tab[i]->Draw ();
}
