//
// Alternative termcap parsing routine,
// based on standard libtermcap library.
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

#include "Screen.h"
#include "Captab.h"
#include "extern.h"
#include <term.h>

const int BUFSIZE = 2048; // max length of termcap entry

static const char *tname; // terminal name
static char *tbuf;        // terminal entry buffer

int Screen::InitCap(char *bp)
{
    if (tbuf)
        return (1);
    if (!(tname = getenv("TERM")))
        tname = "unknown";
    if (tgetent(bp, tname) <= 0)
        return (0);
    tbuf = bp;
    return (1);
}

void Screen::GetCap(struct Captab *t)
{
    char begarea[BUFSIZE];
    char *area = begarea;

    if (!area)
        return;
    for (struct Captab *p = t; p->tname[0]; ++p)
        switch (p->ttype) {
        case CAPNUM:
            *(p->ti) = tgetnum(p->tname);
            break;
        case CAPFLG:
            *(p->tc) = tgetflag(p->tname);
            break;
        case CAPSTR:
            *(p->ts) = tgetstr(p->tname, &area);
            break;
        }

    char *bp = new char[area - begarea];
    if (!bp) {
        return;
    }
    memcpy(bp, begarea, area - begarea);
    for (auto *p = t; p->tname[0]; ++p)
        if (p->ttype == CAPSTR && *(p->ts))
            *(p->ts) += bp - begarea;
#ifdef DEBUG
    for (p = t; p->tname[0]; ++p) {
        printf("%c%c", p->tname[0], p->tname[1]);
        switch (p->ttype) {
        case CAPNUM:
            printf("#%d\n", *(p->ti));
            break;
        case CAPFLG:
            printf(" %s\n", *(p->tc) ? "on" : "off");
            break;
        case CAPSTR:
            if (*(p->ts))
                printf("='%s'\n", *(p->ts));
            else
                printf("=NULL\n");
            break;
        }
    }
#endif
}
