#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "Screen.h"
#include "Dialog.h"
#include "deas.h"

extern Screen V;
extern int CursorColor;
extern int TextColor;
extern int TextAltColor;
extern int TextNegColor;
extern int BorderColor;
extern int ErrorColor;
extern int DialogColor;
extern int DialogOptionColor;
extern int MenuColor;
extern int MenuBorderColor;
extern int ActiveMenuColor;
extern unsigned char DialogPalette[];
extern clientinfo clnt;
extern void Quit (void);
extern accountinfo *FindAccount (long acn);
extern int IsParent (accountinfo *a);
extern int ChooseSubAccount (Screen *scr, int y, int x, int w, long *v, char *str);
extern void Hint (char *str);

static menuinfo *menutab;
static int menutabsz = 0;
static int nmenu;

//
// Зачитываем меню проводок.
//
void LoadEntries ()
{
	nmenu = 0;
	for (menuinfo *a=menu_first(&clnt); a; a=menu_next(&clnt)) {
		if (nmenu >= menutabsz) {
			if (! menutabsz) {
				menutabsz += 16;
				menutab = (menuinfo*) malloc (menutabsz *
					sizeof (*menutab));
			} else {
				menutabsz += 16;
				menutab = (menuinfo*) realloc (menutab,
					menutabsz * sizeof (*menutab));
			}
			if (! menutab) {
				V.Error (ErrorColor, TextColor, " Ошибка ",
					" Готово ", "Мало памяти");
				Quit ();
			}
		}
		menutab[nmenu] = *a;
		++nmenu;
		if (nmenu % 5 == 0) {
			char buf[80];
			sprintf (buf, "%d", nmenu);
			Hint (buf);
			V.Sync ();
		}
	}
	Hint ("");
}

static void NewEntry (menuinfo *m)
{
	accountinfo *pc, *pd;
	entryinfo info;

	memset (&info, 0, sizeof (info));
	long sum = 0;
	int doll = 0;
	long db = m->debit;
	long cr = m->credit;
	Dialog *dlg;
	char *title = " Новая проводка ";
	char crbuf [80], dbbuf [80];
	strncpy (info.descr, m->descr, sizeof (info.descr));
again:
	pc = FindAccount (m->credit);
	pd = FindAccount (m->debit);
	if (! pc || ! pd)
		return;
	if (IsParent (pd)) {
		if (IsParent (pc)) {
			if (pd->anal || pc->anal) {
				if (m->currency != 1 && m->currency != 2)
					dlg = new Dialog (title, DialogPalette,
					    "(++{Куда                     Откуда}"
					    "L:20:40 | {   } | L:20:40 ) {}"
					    "(++{Сумма} N:1:1999999999:1 R :0{Рубли} :1{Доллары}"
					    "++++++++++++++{Штуки} N:1:99999999:1 ) {}"
					    "{Описание} S :50:80",
					    &db, ChooseSubAccount, &cr, ChooseSubAccount,
					    &sum, &doll, &info.amount.pcs, &info.descr);
				else if (m->currency == 1)
					dlg = new Dialog (title, DialogPalette,
					    "(++{Куда                     Откуда}"
					    "L:20:40 | {   } | L:20:40 ) {}"
					    "(++{Сумма - рубли} N:1:1999999999:1"
					    "++++++++++++++{Штуки} N:1:99999999:1 ) {}"
					    "{Описание} S :50:80",
					    &db, ChooseSubAccount, &cr, ChooseSubAccount,
					    &sum, &info.amount.pcs, &info.descr);
				else
					dlg = new Dialog (title, DialogPalette,
					    "(++{Куда                     Откуда}"
					    "L:20:40 | {   } | L:20:40 ) {}"
					    "(++{Сумма - доллары} N:1:1999999999:1"
					    "++++++++++++++{Штуки} N:1:99999999:1 ) {}"
					    "{Описание} S :50:80",
					    &db, ChooseSubAccount, &cr, ChooseSubAccount,
					    &sum, &info.amount.pcs, &info.descr);
			} else {
				if (m->currency != 1 && m->currency != 2)
					dlg = new Dialog (title, DialogPalette,
					    "(++{Куда                     Откуда}"
					    "L:20:40 | {   } | L:20:40 ) {}"
					    "(++{Сумма} N:1:1999999999:1 R :0{Рубли} :1{Доллары}) {}"
					    "{Описание} S :50:80",
					    &db, ChooseSubAccount, &cr, ChooseSubAccount,
					    &sum, &doll, &info.descr);
				else if (m->currency == 1)
					dlg = new Dialog (title, DialogPalette,
					    "(++{Куда                     Откуда}"
					    "L:20:40 | {   } | L:20:40 ) {}"
					    "(++{Сумма - рубли} N:1:1999999999:1) {}"
					    "{Описание} S :50:80",
					    &db, ChooseSubAccount, &cr, ChooseSubAccount,
					    &sum, &info.descr);
				else
					dlg = new Dialog (title, DialogPalette,
					    "(++{Куда                     Откуда}"
					    "L:20:40 | {   } | L:20:40 ) {}"
					    "(++{Сумма - доллары} N:1:1999999999:1) {}"
					    "{Описание} S :50:80",
					    &db, ChooseSubAccount, &cr, ChooseSubAccount,
					    &sum, &info.descr);
			}
		} else {
			sprintf (crbuf, "%ld  %.20s", pc->acn, pc->descr);
			if (pd->anal || pc->anal) {
				if (m->currency != 1 && m->currency != 2)
					dlg = new Dialog (title, DialogPalette,
					    "(++{Куда                     Откуда}"
					    "L:20:40 | {   } | T:20 ) {}"
					    "(++{Сумма} N:1:1999999999:1 R :0{Рубли} :1{Доллары}"
					    "++++++++++++++{Штуки} N:1:99999999:1 ) {}"
					    "{Описание} S :50:80",
					    &db, ChooseSubAccount, crbuf+1,
					    &sum, &doll, &info.amount.pcs, &info.descr);
				else if (m->currency == 1)
					dlg = new Dialog (title, DialogPalette,
					    "(++{Куда                     Откуда}"
					    "L:20:40 | {   } | T:20 ) {}"
					    "(++{Сумма - рубли} N:1:1999999999:1"
					    "++++++++++++++{Штуки} N:1:99999999:1 ) {}"
					    "{Описание} S :50:80",
					    &db, ChooseSubAccount, crbuf+1,
					    &sum, &info.amount.pcs, &info.descr);
				else
					dlg = new Dialog (title, DialogPalette,
					    "(++{Куда                     Откуда}"
					    "L:20:40 | {   } | T:20 ) {}"
					    "(++{Сумма - доллары} N:1:1999999999:1"
					    "++++++++++++++{Штуки} N:1:99999999:1 ) {}"
					    "{Описание} S :50:80",
					    &db, ChooseSubAccount, crbuf+1,
					    &sum, &info.amount.pcs, &info.descr);
			} else {
				if (m->currency != 1 && m->currency != 2)
					dlg = new Dialog (title, DialogPalette,
					    "(++{Куда                     Откуда}"
					    "L:20:40 | {   } | T:20 ) {}"
					    "(++{Сумма} N:1:1999999999:1 R :0{Рубли} :1{Доллары}) {}"
					    "{Описание} S :50:80",
					    &db, ChooseSubAccount, crbuf+1,
					    &sum, &doll, &info.descr);
				else if (m->currency == 1)
					dlg = new Dialog (title, DialogPalette,
					    "(++{Куда                     Откуда}"
					    "L:20:40 | {   } | T:20 ) {}"
					    "(++{Сумма - рубли} N:1:1999999999:1) {}"
					    "{Описание} S :50:80",
					    &db, ChooseSubAccount, crbuf+1,
					    &sum, &info.descr);
				else
					dlg = new Dialog (title, DialogPalette,
					    "(++{Куда                     Откуда}"
					    "L:20:40 | {   } | T:20 ) {}"
					    "(++{Сумма - доллары} N:1:1999999999:1) {}"
					    "{Описание} S :50:80",
					    &db, ChooseSubAccount, crbuf+1,
					    &sum, &info.descr);
			}
		}
	} else {
		sprintf (dbbuf, "%ld  %.20s", pd->acn, pd->descr);
		if (IsParent (pc)) {
			if (pd->anal || pc->anal) {
				if (m->currency != 1 && m->currency != 2)
					dlg = new Dialog (title, DialogPalette,
					    "(++{Куда                     Откуда}"
					    "T:20 | {   } | L:20:40 ) {}"
					    "(++{Сумма} N:1:1999999999:1 R :0{Рубли} :1{Доллары}"
					    "++++++++++++++{Штуки} N:1:99999999:1 ) {}"
					    "{Описание} S :50:80",
					    dbbuf+1, &cr, ChooseSubAccount,
					    &sum, &doll, &info.amount.pcs, &info.descr);
				else if (m->currency == 1)
					dlg = new Dialog (title, DialogPalette,
					    "(++{Куда                     Откуда}"
					    "T:20 | {   } | L:20:40 ) {}"
					    "(++{Сумма - рубли} N:1:1999999999:1"
					    "++++++++++++++{Штуки} N:1:99999999:1 ) {}"
					    "{Описание} S :50:80",
					    dbbuf+1, &cr, ChooseSubAccount,
					    &sum, &info.amount.pcs, &info.descr);
				else
					dlg = new Dialog (title, DialogPalette,
					    "(++{Куда                     Откуда}"
					    "T:20 | {   } | L:20:40 ) {}"
					    "(++{Сумма - доллары} N:1:1999999999:1"
					    "++++++++++++++{Штуки} N:1:99999999:1 ) {}"
					    "{Описание} S :50:80",
					    dbbuf+1, &cr, ChooseSubAccount,
					    &sum, &info.amount.pcs, &info.descr);
			} else {
				if (m->currency != 1 && m->currency != 2)
					dlg = new Dialog (title, DialogPalette,
					    "(++{Куда                     Откуда}"
					    "T:20 | {   } | L:20:40 ) {}"
					    "(++{Сумма} N:1:1999999999:1 R :0{Рубли} :1{Доллары}) {}"
					    "{Описание} S :50:80",
					    dbbuf+1, &cr, ChooseSubAccount,
					    &sum, &doll, &info.descr);
				else if (m->currency == 1)
					dlg = new Dialog (title, DialogPalette,
					    "(++{Куда                     Откуда}"
					    "T:20 | {   } | L:20:40 ) {}"
					    "(++{Сумма - рубли} N:1:1999999999:1) {}"
					    "{Описание} S :50:80",
					    dbbuf+1, &cr, ChooseSubAccount,
					    &sum, &info.descr);
				else
					dlg = new Dialog (title, DialogPalette,
					    "(++{Куда                     Откуда}"
					    "T:20 | {   } | L:20:40 ) {}"
					    "(++{Сумма - доллары} N:1:1999999999:1) {}"
					    "{Описание} S :50:80",
					    dbbuf+1, &cr, ChooseSubAccount,
					    &sum, &info.descr);
			}
		} else {
			sprintf (crbuf, "%ld  %.20s", pc->acn, pc->descr);
			if (pd->anal || pc->anal) {
				if (m->currency != 1 && m->currency != 2)
					dlg = new Dialog (title, DialogPalette,
					    "(++{Куда                     Откуда}"
					    "T:20 | {   } | T:20 ) {}"
					    "(++{Сумма} N:1:1999999999:1 R :0{Рубли} :1{Доллары}"
					    "++++++++++++++{Штуки} N:1:99999999:1 ) {}"
					    "{Описание} S :50:80",
					    dbbuf+1, crbuf+1,
					    &sum, &doll, &info.amount.pcs, &info.descr);
				else if (m->currency == 1)
					dlg = new Dialog (title, DialogPalette,
					    "(++{Куда                     Откуда}"
					    "T:20 | {   } | T:20 ) {}"
					    "(++{Сумма - рубли} N:1:1999999999:1"
					    "++++++++++++++{Штуки} N:1:99999999:1 ) {}"
					    "{Описание} S :50:80",
					    dbbuf+1, crbuf+1,
					    &sum, &info.amount.pcs, &info.descr);
				else
					dlg = new Dialog (title, DialogPalette,
					    "(++{Куда                     Откуда}"
					    "T:20 | {   } | T:20 ) {}"
					    "(++{Сумма - доллары} N:1:1999999999:1"
					    "++++++++++++++{Штуки} N:1:99999999:1 ) {}"
					    "{Описание} S :50:80",
					    dbbuf+1, crbuf+1,
					    &sum, &info.amount.pcs, &info.descr);
			} else {
				if (m->currency != 1 && m->currency != 2)
					dlg = new Dialog (title, DialogPalette,
					    "(++{Куда                     Откуда}"
					    "T:20 | {   } | T:20 ) {}"
					    "(++{Сумма} N:1:1999999999:1 R :0{Рубли} :1{Доллары}) {}"
					    "{Описание} S :50:80",
					    dbbuf+1, crbuf+1,
					    &sum, &doll, &info.descr);
				else if (m->currency == 1)
					dlg = new Dialog (title, DialogPalette,
					    "(++{Куда                     Откуда}"
					    "T:20 | {   } | T:20 ) {}"
					    "(++{Сумма - рубли} N:1:1999999999:1) {}"
					    "{Описание} S :50:80",
					    dbbuf+1, crbuf+1,
					    &sum, &info.descr);
				else
					dlg = new Dialog (title, DialogPalette,
					    "(++{Куда                     Откуда}"
					    "T:20 | {   } | T:20 ) {}"
					    "(++{Сумма - доллары} N:1:1999999999:1) {}"
					    "{Описание} S :50:80",
					    dbbuf+1, crbuf+1,
					    &sum, &info.descr);
			}
		}
	}
	if (dlg->Run (&V) <= 0) {
		delete dlg;
		return;
	}
	delete dlg;

	pc = FindAccount (cr);
	pd = FindAccount (db);
	if (!sum || !pc || !pd || IsParent (pc) || IsParent (pd) ||
	    ((pd->anal || pc->anal) && ! info.amount.pcs)) {
		V.Beep ();
		goto again;
	}
	info.debit = db;
	info.credit = cr;
	if (m->currency == 2 || doll)
		info.amount.doll = sum;
	else
		info.amount.rub = sum;

	journal_add (&clnt, &info);
}

static void ViewMenuLine (menuinfo **list, int lnum, int y, int w, int cursor)
{
	int color = cursor ? ActiveMenuColor : MenuColor;

	int c = 4;
	if (w < V.Columns/2)
		c += (V.Columns/2 - w) / 2;

	V.Clear (y, c, 1, w, TextColor);
	if (lnum < 0)
		return;

	menuinfo *m = list[lnum];
	V.Clear (y, c, 1, w, color);
	V.Put (y, c + (w - strlen (m->line)) / 2, m->line, color);
	if (cursor) {
		V.Put (y, c, '[', MenuBorderColor);
		V.Put (y, c+w-1, ']', MenuBorderColor);
	}
}

static void ViewEntries (menuinfo **list, int nm)
{
	// Сохраняем экран.
	Box box (V, 1, 0, V.Lines-2, V.Columns);
	V.Clear (1, 0, V.Lines-2, V.Columns, TextColor);

	int y = 2;
	int plen = (V.Lines - y - 1) / 2;
	if (nm < plen) {
		y += plen - nm;
		plen = nm;
	}

	int w = 0;
	for (int i=0; i<nm; ++i) {
		int iw = strlen (list[i]->line);
		if (w < iw)
			w = iw;
	}
	w += 2;

	int c = 4;
	if (w < V.Columns/2)
		c += (V.Columns/2 - w) / 2;
	V.DrawFrame (y-1, c-2, plen*2+1, w+4, BorderColor);

	int topline = 0;                        // верхняя строка на экране
	int cline = 0;                          // текущая строка (курсор)
	for (;;) {
		for (int i=0; i<plen; ++i)
			ViewMenuLine (list, topline+i<nm ? topline+i : -1, y+i*2, w, 0);
		for (;;) {
			ViewMenuLine (list, cline, y+(cline-topline)*2, w, 1);
			V.HideCursor ();
			V.Sync ();
			int k = V.GetKey ();
			ViewMenuLine (list, cline, y+(cline-topline)*2, w, 0);
			switch (k) {
			default:
				V.Beep ();
				continue;
			case cntrl ('['):       // Escape - exit
			case cntrl ('C'):       // ^C - exit
			case meta ('l'):        // left
				V.Put (box);
				return;
			case cntrl ('J'):       // Enter
			case cntrl ('M'):       // Return
			case meta ('r'):        // right
				if (cline >= nm)
					continue;
				if (! list[cline]->credit) {
					menuinfo *m = list[cline];
					menuinfo *clist [32];
					int cnm = 0;

					while (++m < menutab+nmenu && m->credit)
						clist[cnm++] = m;
					if (cnm)
						ViewEntries (clist, cnm);
				} else
					NewEntry (list[cline]);
				break;
			case meta ('d'):        // down
				if (cline >= nm-1)
					continue;
				++cline;
				if (topline+plen > cline)
					continue;
				++topline;
				V.DelLine (y, TextColor);
				V.DelLine (y, TextColor);
				V.InsLine (y + (plen-1)*2, TextColor);
				V.InsLine (y + (plen-1)*2, TextColor);
				ViewMenuLine (list, topline+plen-1, y+(plen-1)*2, w, 0);
				continue;
			case meta ('u'):        // up
				if (cline <= 0)
					continue;
				--cline;
				if (topline <= cline)
					continue;
				--topline;
				V.DelLine (y + (plen-1)*2, TextColor);
				V.DelLine (y + (plen-1)*2, TextColor);
				V.InsLine (y, TextColor);
				V.InsLine (y, TextColor);
				ViewMenuLine (list, topline, y, w, 0);
				continue;
			case meta ('n'):        // next page
				if (topline+plen >= nm)
					continue;
				topline += plen-1;
				cline += plen-1;
				if (topline+plen > nm) {
					cline -= topline - (nm - plen);
					topline = nm - plen;
				}
				break;
			case meta ('p'):        // prev page
				if (topline <= 0)
					continue;
				topline -= plen-1;
				cline -= plen-1;
				if (topline < 0) {
					cline -= topline;
					topline = 0;
				}
				break;
			case meta ('h'):        // home
				if (cline == 0)
					continue;
				topline = cline = 0;
				break;
			case meta ('e'):        // end
				if (cline >= nm-1)
					continue;
				topline = nm - plen;
				if (topline < 0)
					topline = 0;
				cline = nm - 1;
				if (cline < 0)
					cline = 0;
				break;
			}
			break;
		}
	}
}

void Entries ()
{
	menuinfo *list [32];
	int nm = 0;

	for (menuinfo *m=menutab; m<menutab+nmenu; ++m)
		if (! m->credit)
			list[nm++] = m;
	if (! nm)
		return;
	ViewEntries (list, nm);
}
