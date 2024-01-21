#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "Screen.h"
#include "Dialog.h"
#include "deas.h"

extern Screen V;
extern int TextColor;
extern int TextAltColor;
extern int BorderColor;
extern int ErrorColor;
extern int CursorColor;
extern int DialogColor;
extern int DialogOptionColor;
extern unsigned char DialogPalette[];
extern clientinfo clnt;
extern void Quit (void);
extern void Hint (char *str);
extern char *acnum (long acn);
extern int ChooseAccount (Screen *scr, int y, int x, int w, long *v, char *str);
extern int IsParent (accountinfo *a);
extern accountinfo *FindAccount (long acn);

static entryinfo *enttab;
static int enttabsz;

static entryinfo *ScanPoint;
static int ScanCount;

static int ScanInit ()
{
	ScanPoint = journal_first (&clnt);
	ScanCount = 0;
	if (! ScanPoint)
		return ScanCount;
	if (! enttabsz) {
		enttabsz += 16;
		enttab = (entryinfo*) malloc (enttabsz * sizeof (*enttab));
		if (! enttab) {
			V.Error (ErrorColor, TextColor, " Ошибка ",
				" Готово ", "Мало памяти");
			Quit ();
		}
	}
	enttab[ScanCount++] = *ScanPoint;
	return ScanCount;
}

static long ScanTo (long n)
{
	if (! ScanPoint)
		return ScanCount;
	while (ScanCount < n && (ScanPoint = journal_next (&clnt)) != 0) {
		if (ScanCount >= enttabsz) {
			enttabsz += 16;
			enttab = (entryinfo*) realloc (enttab,
				enttabsz * sizeof (*enttab));
			if (! enttab) {
				V.Error (ErrorColor, TextColor, " Ошибка ",
					" Готово ", "Мало памяти");
				Quit ();
			}
		}
		enttab[ScanCount++] = *ScanPoint;
	}
	return ScanCount;
}

int NewJournalEntry (accountinfo *a)
{
	entryinfo info;

	memset (&info, 0, sizeof (info));
	long sum = 0;
	int doll = 0;
	long db = a ? a->acn : 0;
	long cr = a ? a->acn : 0;

	Dialog *dlg = (!a || a->anal) ?
	    new Dialog (" Новая проводка ", DialogPalette,
		"(++{Куда                     Откуда}"
		"L:20:40 | { } | L:20:40 ) {}"
		"(++{Сумма} N:1:1999999999:1 R :0{Рубли} :1{Доллары}"
		"++++++++++++++{Штуки} N:1:99999999:1 ) {}"
		"{Описание} S :50:80",
		&db, ChooseAccount, &cr, ChooseAccount,
		&sum, &doll, &info.amount.pcs, &info.descr) :
	    new Dialog (" Новая проводка ", DialogPalette,
		"(++{Куда                     Откуда}"
		"L:20:40 | { } | L:20:40 ) {}"
		"(++{Сумма} N:1:1999999999:1 R :0{Рубли} :1{Доллары} ) {}"
		"{Описание} S :50:80",
		&db, ChooseAccount, &cr, ChooseAccount,
		&sum, &doll, &info.descr);
again:
	if (dlg->Run (&V) <= 0) {
		delete dlg;
		return 0;
	}
	accountinfo *c = 0, *d = 0;
	if (! db || ! cr || ! (c = FindAccount (cr)) ||
	    ! (d = FindAccount (db)) || ! d->wmask || ! c->wmask || ! sum ||
	    (a && a->anal && ! info.amount.pcs)) {
		V.Beep ();
		goto again;
	}
	delete dlg;
	info.debit = db;
	info.credit = cr;
	if (doll)
		info.amount.doll = sum;
	else
		info.amount.rub = sum;

	journal_add (&clnt, &info);
	return 1;
}

int EditJournalEntry (entryinfo *e)
{
	entryinfo info = *e;

	int id, im, iy;
	date2ymd (info.date, &iy, &im, &id);
	long day = id;
	long mon = im;
	long year = iy;
	long db = e->debit;
	long cr = e->credit;
	int doll = (info.amount.doll != 0);
	long sum = doll ? info.amount.doll : info.amount.rub;

	Dialog dlg (" Изменение проводки ", DialogPalette,
		"({Куда                        Откуда}"
		"L:20:40 | {    } | L:20:40 ) {}"
		"({Сумма} N:1:1999999999:1 R :0{Рубли} :1{Доллары}"
		"+++++++++{Штуки} N:1:99999999:1"
		"++++++++{Дата} N:1:99:1 | N:1:99:1 | N:1:9999:1 ) {}"
		"{Описание} S :50:50 {}"
		"{Владелец} S :16:16 {}",
		&db, ChooseAccount, &cr, ChooseAccount,
		&sum, &doll, &info.amount.pcs, &day, &mon, &year,
		info.descr, info.user);
again:
	if (dlg.Run (&V) <= 0)
		return 0;

	info.date = ymd2date (year, mon, day);
	if (info.date < 0 || ! db || ! cr || !sum) {
		V.Beep ();
		goto again;
	}
	if (doll) {
		info.amount.doll = sum;
		info.amount.rub = 0;
	} else {
		info.amount.rub = sum;
		info.amount.doll = 0;
	}
	info.debit = db;
	info.credit = cr;

	journal_edit (&clnt, &info);
	return 1;
}

int CopyJournalEntry (entryinfo *e)
{
	entryinfo info = *e;

	int id, im, iy;
	date2ymd (info.date, &iy, &im, &id);
	long day = id;
	long mon = im;
	long year = iy;
	long db = e->debit;
	long cr = e->credit;
	int doll = (info.amount.doll != 0);
	long sum = doll ? info.amount.doll : info.amount.rub;

	Dialog dlg (" Дубликат проводки ", DialogPalette,
		"({Куда                        Откуда}"
		"L:20:40 | {    } | L:20:40 ) {}"
		"({Сумма} N:1:1999999999:1 R :0{Рубли} :1{Доллары}"
		"+++++++++{Штуки} N:1:99999999:1"
		"++++++++{Дата} N:1:99:1 | N:1:99:1 | N:1:9999:1 ) {}"
		"{Описание} S :50:50 {}"
		"{Владелец} S :16:16 {}",
		&db, ChooseAccount, &cr, ChooseAccount,
		&sum, &doll, &info.amount.pcs, &day, &mon, &year,
		info.descr, info.user);
again:
	if (dlg.Run (&V) <= 0)
		return 0;

	info.date = ymd2date (year, mon, day);
	if (info.date < 0 || ! db || ! cr || !sum) {
		V.Beep ();
		goto again;
	}
	if (doll) {
		info.amount.doll = sum;
		info.amount.rub = 0;
	} else {
		info.amount.rub = sum;
		info.amount.doll = 0;
	}
	info.debit = db;
	info.credit = cr;
	info.eid = 0;

	journal_add (&clnt, &info);
	return 1;
}

int DeleteJournalEntry (entryinfo *e)
{
	if (e->deleted)
		return 0;

	char buf [80], name [80];
	sprintf (buf, "Удалить проводку #%ld:", e->eid);
	sprintf (name, "\"%.50s\"?", e->descr);

	if (V.Popup (" Удаление проводки ", buf, name, " Нет ", " Да ",
	    0, DialogColor, DialogOptionColor) != 1)
		return 0;

	journal_delete (&clnt, e->eid);
	return 1;
}

static void ViewJournalLine (int lnum, int y)
{
	V.Clear (y, 1, 1, V.Columns-2, TextAltColor);
	V.Clear (y+1, 1, 2, V.Columns-2, TextColor);
	V.VertLine (0, y, 3, BorderColor);
	V.VertLine (V.Columns-1, y, 3, BorderColor);
	if (lnum < 0)
		return;

	entryinfo *e = enttab + lnum;
	int day, mon, year;
	date2ymd (e->date, &year, &mon, &day);

	V.Print (y, 2, TextAltColor, "%02d.%02d.%02d  %d",
		day, mon, year % 100, e->eid);
	if (e->deleted)
		V.Put ('#', TextAltColor);
	V.Print (y, 19, TextAltColor, "%.50s", e->descr);

	V.Put (y+1, 27, acnum (e->debit), TextColor);
	accountinfo *a = FindAccount (e->debit);
	if (a)
		V.Print (TextColor, "%c %s", a->deleted ? '#' : IsParent (a) ?
			'/' : ' ', a->descr);

	V.Put (y+2, 27, acnum (e->credit), TextColor);
	a = FindAccount (e->credit);
	if (a)
		V.Print (TextColor, "%c %s", a->deleted ? '#' : IsParent (a) ?
			'/' : ' ', a->descr);

	char buf[80];
	if (e->amount.rub && e->amount.doll)
		sprintf (buf, " %ld+$%ld", e->amount.rub, e->amount.doll);
	else if (e->amount.rub)
		sprintf (buf, " %ld", e->amount.rub);
	else if (e->amount.doll)
		sprintf (buf, " $%ld", e->amount.doll);
	V.Put (y, 78 - strlen (buf), buf, TextAltColor);
	if (e->amount.pcs) {
		sprintf (buf, "(%ld)", e->amount.pcs);
		V.Put (y+1, 78 - strlen(buf), buf, TextColor);
	}
}

void Journal ()
{
	Hint ("\1F4\2 - Редактирование  \1F7\2 - Новая проводка  \1F8\2 - Удаление");

	int nent = ScanInit ();

	// Сохраняем экран.
	Box box (V, 1, 0, V.Lines-1, V.Columns);

	// Рисуем.
	int y = 4;
	int h = (V.Lines - y - 2) / 3 * 3;
	int plen = h/3;
	V.DrawFrame (y-3, 0, plen*3+4, V.Columns, BorderColor);

	V.HorLine (y-1, 1, V.Columns-2, BorderColor);
	V.Clear (y-2, 1, 1, V.Columns-2, BorderColor);
	V.Put (y-2, 4, "Дата   Номер   Описание  Дебет/Кредит                         Сумма (Штук)",
		BorderColor);

	int topline = 0;        // верхняя строка на экране
	int cline = 0;          // текущая строка (курсор)
	for (;;) {
		if (ScanPoint && nent < topline + plen)
			nent = ScanTo (topline + plen);
		for (int i=0; i<plen; ++i)
			ViewJournalLine (topline+i<nent ? topline+i : -1, y+i*3);
		for (;;) {
			Box cursor (V, y+3*(cline-topline), 1, 1, V.Columns-2);
			V.Put (cursor, CursorColor);
			V.HideCursor ();
			V.Sync ();
			int k = V.GetKey ();
			V.Put (cursor);
			switch (k) {
			default:
				V.Beep ();
				continue;
			case cntrl ('['):       // Escape - exit
			case cntrl ('C'):       // ^C - exit
			case meta ('l'):        // left
				V.Put (box);
				return;
			case meta ('d'):        // down
				if (ScanPoint && nent < topline + plen + 1)
					nent = ScanTo (topline + plen + 1);
				if (cline >= nent-1)
					continue;
				++cline;
				if (topline+plen > cline)
					continue;
				++topline;
				V.DelLine (y, TextColor);
				V.DelLine (y, TextColor);
				V.DelLine (y, TextColor);
				V.InsLine (y + (plen-1)*3, TextColor);
				V.InsLine (y + (plen-1)*3, TextColor);
				V.InsLine (y + (plen-1)*3, TextColor);
				ViewJournalLine (topline+plen-1, y+(plen-1)*3);
				continue;
			case meta ('u'):        // up
				if (cline <= 0)
					continue;
				--cline;
				if (topline <= cline)
					continue;
				--topline;
				V.DelLine (y + (plen-1)*3, TextColor);
				V.DelLine (y + (plen-1)*3, TextColor);
				V.DelLine (y + (plen-1)*3, TextColor);
				V.InsLine (y, TextColor);
				V.InsLine (y, TextColor);
				V.InsLine (y, TextColor);
				ViewJournalLine (topline, y);
				continue;
			case meta ('n'):        // next page
				if (ScanPoint && nent < topline + plen + plen)
					nent = ScanTo (topline + plen + plen);
				if (topline+plen >= nent)
					continue;
				topline += plen-1;
				cline += plen-1;
				if (topline+plen > nent) {
					cline -= topline - (nent - plen);
					topline = nent - plen;
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
				if (ScanPoint)
					nent = ScanTo (999999999);
				if (cline >= nent-1)
					continue;
				topline = nent - plen;
				if (topline < 0)
					topline = 0;
				cline = nent - 1;
				if (cline < 0)
					cline = 0;
				break;
			case meta ('G'):        // F7 - insert
				if (! NewJournalEntry (0))
					continue;
reload:                         nent = ScanInit ();
				cline = 0;
				topline = 0;
				break;
			case meta ('H'):        // F8 - delete
				if (!nent || !DeleteJournalEntry (enttab+cline))
					continue;
				goto reload;
			case meta ('D'):        // F4 - edit
				if (!nent || !EditJournalEntry (enttab+cline))
					continue;
				goto reload;
			}
			break;
		}
	}
}
