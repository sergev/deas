//
// Popup windows declarations.
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
class Screen;
class Box;

class Flash {
private:
	Box *box;
	Screen *scr;
public:
	Flash (Screen *s, const char *head, const char *msg, int color);
	~Flash ();
};
