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
extern int DialogDimColor;
extern unsigned char DialogPalette[];
extern clientinfo clnt;
extern void Quit (void);
extern void Hint (const char *str);

extern int NewJournalEntry (accountinfo *a);
extern int EditJournalEntry (entryinfo *e);
extern int CopyJournalEntry (entryinfo *e);
extern int DeleteJournalEntry (entryinfo *e);

static accountinfo *acntab;
static balance **total;
static char parentmask [MAXACCOUNTS/8];
static int acntabsz = 0;
static int naccounts;
static int amode = 0;           // признак отображения режима доступа

static int ViewAccount (accountinfo *a);

char *acnum (long acn)
{
	static char buf [40];

	snprintf (buf, sizeof(buf), "%ld", acn);
	return buf+1;
}

static void FreeBalances ()
{
	// Удаляем старые балансы.
	for (int i=0; i<naccounts; ++i)
		if (total[i]) {
			free (total[i]);
			total[i] = 0;
		}
}

//
// Зачитываем план счетов.
//
void LoadAccounts ()
{
	FreeBalances ();
	naccounts = 0;
	for (accountinfo *a=chart_first(&clnt); a; a=chart_next(&clnt)) {
		if (naccounts >= acntabsz) {
			if (! acntabsz) {
				acntabsz += 16;
				acntab = (accountinfo*) malloc (acntabsz *
					sizeof (*acntab));
				total = (balance**) malloc (acntabsz *
					sizeof (*total));
			} else {
				acntabsz += 16;
				acntab = (accountinfo*) realloc (acntab,
					acntabsz * sizeof (*acntab));
				total = (balance**) realloc (total,
					acntabsz * sizeof (*total));
			}
			if (! acntab || ! total) {
				V.Error (ErrorColor, TextColor, " Ошибка ",
					" Готово ", "Мало памяти");
				Quit ();
			}
		}
		acntab[naccounts] = *a;
		total[naccounts] = 0;
		++naccounts;
		if (naccounts % 5 == 0) {
			char buf[80];
			snprintf (buf, sizeof(buf), "%d", naccounts);
			Hint (buf);
			V.Sync ();
		}
	}
	Hint ("");

	// Вычисляем наличие подсчетов.
	memset (parentmask, 0, sizeof (parentmask));
	accountinfo *b;
	for (auto *a=acntab; a<acntab+naccounts; ++a)
		for (b=acntab; b<acntab+naccounts; ++b)
			if (a->acn == b->pacn) {
				int i = a - acntab;
				parentmask [i/8] |= 1 << (i & 7);
				break;
			}
}

accountinfo *FindAccount (long acn)
{
	for (int i=0; i<naccounts; ++i)
		if (acntab[i].acn == acn)
			return acntab+i;
	return 0;
}

static accountinfo **AccountList (long pacn, int *nacc)
{
	accountinfo **list = 0;
	int listsz = 0;
	*nacc = 0;

	for (accountinfo *a=acntab; a<acntab+naccounts; ++a) {
		if (a->pacn != pacn)
			continue;
		if (*nacc >= listsz) {
			if (! listsz) {
				listsz += 16;
				list = (accountinfo**) malloc (listsz *
					sizeof (*list));
			} else {
				listsz += 16;
				list = (accountinfo**) realloc (list,
					listsz * sizeof (*list));
			}
			if (! list) {
				V.Error (ErrorColor, TextColor, " Ошибка ",
					" Готово ", "Мало памяти");
				Quit ();
			}
		}
		list[(*nacc)++] = a;
	}
	return list;
}

int IsParent (accountinfo *a)
{
	if (! a)
		return 0;
	int i = a - acntab;
	return (parentmask [i/8] >> (i & 7)) & 1;
}

int ChooseAccountList (Screen *scr, int y, int x, int w, long pacn,
	accountinfo **a)
{
	int nacc;
	accountinfo **list;
again:
	list = AccountList (pacn, &nacc);
	if (! nacc)
		return 0;
	if (nacc == 1) {
		*a = list[0];
		pacn = list[0]->acn;
		free (list);
		goto again;
	}

	int color = DialogColor;
	int curscolor = DialogOptionColor;

	int h = scr->Lines / 2;
	if (h > nacc+2)
		h = nacc+2;
	int plen = h-2;

	int off;
	if (y >= scr->Lines * 3 / 4)
		off = h-2;
	else if (y >= scr->Lines / 2)
		off = h/2;
	else
		off = 1;
	y -= off;

	// Сохраняем экран.
	Box box (*scr, y, x, h+1, w+2);

	// Рисуем.
	scr->Clear (y, x, h, w, color);
	scr->DrawFrame (y, x, h, w, color);
	for (int n=0; n<w; ++n)
		scr->AttrLow (y+h, x+n+1);
	for (int n=1; n<h; ++n)
		scr->AttrLow (y+n, x+w);

	int cline = 0;          // текущая строка (курсор)
	int topline = 0;        // верхняя строка на экране
	if (*a) {
	        accountinfo **p;
		for (p=list+nacc-1; p>list; --p) {
			if (*p <= *a)
				break;
                }
		cline = p - list;
		if (cline >= plen) {
			topline = cline - plen/2;
			if (topline + plen > nacc)
				topline = nacc - plen;
			if (topline < 0)
				topline = 0;
		}
	}
	int ret;
	for (;;) {
		for (int i=0; i<plen; ++i) {
			scr->Clear (y+i+1, x+1, 1, w-2, color);
			if (topline + i >= nacc)
				continue;
			accountinfo *p = list [topline+i];
			char buf[80];
			snprintf (buf, sizeof(buf), "%s%c %s", acnum (p->acn),
				p->deleted ? '#' : IsParent (p) ?
				'/' : ' ', p->descr);
			scr->PutLimited (y+i+1, x+1, buf, w-2, color);
		}
		for (;;) {
			Box cursor (*scr, y+1+cline-topline, x+1, 1, w-2);
			scr->Put (cursor, curscolor);
			scr->HideCursor ();
			scr->Sync ();
			int k = scr->GetKey ();
			scr->Put (cursor);
			switch (k) {
			default:
				scr->Beep ();
				continue;
			case ' ':               // Space - choose
				ret = 1;
				*a = list[cline];
				goto ret;
			case cntrl ('['):       // Escape - exit
			case cntrl ('C'):       // ^C - exit
				ret = 0;
				goto ret;
			case meta ('J'):        // F10
				ret = -1;
ret:                            scr->Put (box);
				if (list)
					free (list);
				return ret;
			case cntrl ('J'):       // Enter
			case cntrl ('M'):       // Return
				if (! IsParent (list[cline])) {
					*a = list[cline];
					ret = 1;
					goto ret;
				}
				ret = ChooseAccountList (scr, y+2+cline-topline,
				    x+1, w, list[cline]->acn, a);
				if (ret != 0)
					goto ret;
				continue;
			case meta ('d'):        // down
				if (cline >= nacc-1)
					continue;
				++cline;
				if (topline+plen > cline)
					continue;
				++topline;
				break;
			case meta ('u'):        // up
				if (cline <= 0)
					continue;
				--cline;
				if (topline <= cline)
					continue;
				--topline;
				break;
			case meta ('n'):        // next page
				if (topline+plen >= nacc)
					continue;
				topline += plen-1;
				cline += plen-1;
				if (topline+plen > nacc) {
					cline -= topline - (nacc - plen);
					topline = nacc - plen;
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
				if (cline >= nacc-1)
					continue;
				topline = nacc - plen;
				if (topline < 0)
					topline = 0;
				cline = nacc - 1;
				if (cline < 0)
					cline = 0;
				break;
			}
			break;
		}
	}
}

int ChooseAccount (Screen *scr, int y, int x, int w, long *v, char *str)
{
	accountinfo *a = 0;
	*str = 0;

	if (v)
		a = FindAccount (*v);
	int ret = 0;
	if (scr)
		ret = ChooseAccountList (scr, y, x, w, 0, &a);
	if (a) {
		if (v)
			*v = a->acn;
		snprintf (str, ENAMSZ, "%s%c %s", acnum (a->acn),
			a->deleted ? '#' : IsParent (a) ?
			'/' : ' ', a->descr);
	}
	return ret;
}

int ChooseSubAccount (Screen *scr, int y, int x, int w, long *v, char *str)
{
	accountinfo *a = 0;
	*str = 0;

	if (v)
		a = FindAccount (*v);
	int ret = 0;
	if (scr) {
		long pacn = 0;
		if (a)
			pacn = IsParent (a) ? a->acn : a->pacn;
		ret = ChooseAccountList (scr, y, x, w, pacn, &a);
	}
	if (a) {
		if (v)
			*v = a->acn;
		snprintf (str, ENAMSZ, "%s%c %s", acnum (a->acn),
			a->deleted ? '#' : IsParent (a) ?
			'/' : ' ', a->descr);
	}
	return ret;
}

static int NewAccount (long pacn)
{
	accountinfo info, *a = pacn ? FindAccount (pacn) : 0;
	Dialog *dlg;
	char abuf [40], pbuf[80];

	memset (&info, 0, sizeof (info));
	if (a) {
		snprintf (abuf, sizeof(abuf), "%ld", pacn);
		snprintf (pbuf, sizeof(pbuf), "%ld/ %.20s", a->acn, a->descr);
		info.passive = a->passive;
		info.anal = a->anal;
		dlg = new Dialog (" Новый субсчет ", DialogPalette,
			"(++++++++++ {Номер счета} S:10:10 +++++++++"
			"{Синт.счет} T:20 ) {}"
			"{Описание} S:50:50 {}",
			abuf+1, pbuf+1, &info.descr);
	} else {
		strcpy (abuf, "1");
		dlg = new Dialog (" Новый счет ", DialogPalette,
			"(R :0{Актив} :1{Пассив} +++++++++"
			"{Номер счета} S:10:10 +++++++++"
			"{Синт.счет} L:20:40 ) {}"
			"{Описание} S:50:50 {}"
			"B {Аналитический учет}",
			&info.passive, abuf+1, &pacn, ChooseAccount,
			&info.descr, &info.anal);
	}
again:
	if (! dlg)
		return 0;
	if (dlg->Run (&V) <= 0) {
		delete dlg;
		return 0;
	}
	if (! abuf[1] || ! info.descr[0]) {
		V.Beep ();
		goto again;
	}
	delete dlg;
	info.acn = strtol (abuf, 0, 10);
	info.pacn = pacn;
	chart_add (&clnt, &info);
	return 1;
}

static int EditAccount (accountinfo *a)
{
	accountinfo info;
	int i, rgroup[GRMAX], wgroup[GRMAX];

	memset (&info, 0, sizeof (info));
	info.acn = a->acn;
	strncpy (info.descr, a->descr, ANAMSZ);
	for (i=0; i<GRMAX; ++i) {
		rgroup[i] = (a->rmask >> i) & 1;
		wgroup[i] = (a->wmask >> i) & 1;
	}

	Dialog dlg (" Изменение счета ", DialogPalette,
		"{Описание} S :50:50 {}"
		"({Чтение} + (B{0 } B{1 } B{2 } + B{3 } B{4 } B{5 } + B{6 } B{7 } B{8 } +"
		"B{9 } B{10} B{11} + B{12} B{13} B{14} + B{15})) {}"
		"({Запись} + (B{0 } B{1 } B{2 } + B{3 } B{4 } B{5 } + B{6 } B{7 } B{8 } +"
		"B{9 } B{10} B{11} + B{12} B{13} B{14} + B{15}))",
		info.descr,
		rgroup+0,  rgroup+1,  rgroup+2,  rgroup+3,
		rgroup+4,  rgroup+5,  rgroup+6,  rgroup+7,
		rgroup+8,  rgroup+9,  rgroup+10, rgroup+11,
		rgroup+12, rgroup+13, rgroup+14, rgroup+15,
		wgroup+0,  wgroup+1,  wgroup+2,  wgroup+3,
		wgroup+4,  wgroup+5,  wgroup+6,  wgroup+7,
		wgroup+8,  wgroup+9,  wgroup+10, wgroup+11,
		wgroup+12, wgroup+13, wgroup+14, wgroup+15);
	if (dlg.Run (&V) <= 0)
		return 0;

	for (i=0; i<GRMAX; ++i) {
		info.rmask |= (rgroup[i] & 1L) << i;
		info.wmask |= (wgroup[i] & 1L) << i;
	}

	chart_edit (&clnt, &info);
	return 1;
}

static int DeleteAccount (accountinfo *a)
{
	if (a->deleted)
		return 0;

	char buf [80], name [80];
	snprintf (buf, sizeof(buf), "Удалить %s счет %s:", a->passive ? "пассивный" :
		"активный", acnum (a->acn));
	snprintf (name, sizeof(name), "\"%.50s\"?", a->descr);

	if (V.Popup (" Удаление счета ", buf, name, " Нет ", " Да ",
	    0, DialogColor, DialogOptionColor) != 1)
		return 0;

	chart_delete (&clnt, a->acn);
	return 1;
}

static void PrintTotal (int y, accountinfo **list, int nacc)
{
	long a_rub = 0, p_rub = 0, a_doll = 0, p_doll = 0;
	char buf [64];

	V.Clear (y, 1, 1, V.Columns-2, BorderColor);

	for (int i=0; i<nacc; ++i) {
		// Нет смысла печатать итог, если известны не все счета.
		if (! list[i]->rmask)
			return;

		int anum = list[i] - acntab;
		balance *b = total[anum];
		if (! b) {
			b = account_balance (&clnt, list[i]->acn);
			if (! b)
				continue;
			total[anum] = (balance*) malloc (sizeof (*b));
			if (total[anum])
				*total[anum] = *b;
		}
		if (list[i]->passive) {
			p_rub  += b->credit.rub  - b->debit.rub;
			p_doll += b->credit.doll - b->debit.doll;
		} else {
			a_rub  += b->debit.rub  - b->credit.rub;
			a_doll += b->debit.doll - b->credit.doll;
		}
	}
	V.Put (y, 29, "Итого:", BorderColor);

	int dr_color, cr_color, dd_color, cd_color;
	dr_color = cr_color = dd_color = cd_color = TextColor;

	if (a_rub < 0) a_rub = -a_rub, dr_color = TextNegColor;
	snprintf (buf, sizeof(buf), a_rub ? "%ld" : "-", a_rub);
	V.Put (y, 46 - strlen (buf), buf, dr_color);

	if (p_rub < 0) p_rub = -p_rub, cr_color = TextNegColor;
	snprintf (buf, sizeof(buf), p_rub ? "%ld" : "-", p_rub);
	V.Put (y, 67 - strlen (buf), buf, cr_color);

	if (a_doll < 0) a_doll = -a_doll, dd_color = TextNegColor;
	snprintf (buf, sizeof(buf), a_doll ? "$%ld" : "-", a_doll);
	V.Put (y, 56 - strlen (buf), buf, dd_color);

	if (p_doll < 0) p_doll = -p_doll, cd_color = TextNegColor;
	snprintf (buf, sizeof(buf), p_doll ? "$%ld" : "-", p_doll);
	V.Put (y, 77 - strlen (buf), buf, cd_color);
}

static void ViewLine (accountinfo **list, int lnum, int y)
{
	int color = (lnum & 1) ? TextColor : TextAltColor;

	V.Clear (y, 1, 1, V.Columns-2, color);
	V.VertLine (0, y, 1, BorderColor);
	V.VertLine (V.Columns-1, y, 1, BorderColor);
	if (lnum < 0)
		return;

	accountinfo *a = list[lnum];

	V.Print (y, 4, color, "%s", acnum (a->acn));
	if (a->deleted)
		V.Put ('#', color);
	else if (IsParent (a))
		V.Put ('/', color);
	else
		V.Put (' ', color);
	V.Print (color, " %.50s", a->descr);

	if (! a->rmask)
		return;

	int anum = a - acntab;
	balance *b = total[anum];
	if (! b) {
		b = account_balance (&clnt, a->acn);
		if (! b)
			return;
		total[anum] = (balance*) malloc (sizeof (*b));
		if (! total[anum])
			return;
		*total[anum] = *b;
	}

	char buf [64];
	if (amode) {
		V.Move (y, 32);
		for (int i=0; i<GRMAX; ++i)
			V.Put ((a->rmask >> i) & 1 ?
				"0123456789012345"[i] : '-', color);
		V.Put ("  ", color);
		for (int i=0; i<GRMAX; ++i)
			V.Put ((a->wmask >> i) & 1 ?
				"0123456789012345"[i] : '-', color);
		long rec = b->debit.rec + b->credit.rec;
		V.Print (y, 67, color, "%c", a->anal ? '+' : ' ');
		if (rec)
			V.Print (y, 71, color, "%6d", rec);
	} else if (! a->deleted) {
		val v;
		if (a->passive) {
			v.rub  = b->credit.rub  - b->debit.rub;
			v.doll = b->credit.doll - b->debit.doll;
			v.pcs  = b->credit.pcs  - b->debit.pcs;
		} else {
			v.rub  = b->debit.rub  - b->credit.rub;
			v.doll = b->debit.doll - b->credit.doll;
			v.pcs  = b->debit.pcs  - b->credit.pcs;
		}
		int r_color, d_color, p_color;
		r_color = d_color = p_color = color;

		if (v.rub < 0) v.rub = -v.rub, r_color ^= TextNegColor ^ TextColor;
		snprintf (buf, sizeof(buf), v.rub ? "%ld" : "-", v.rub);
		V.Put (y, (a->passive ? 67 : 46) - strlen (buf), buf, r_color);

		if (v.doll < 0) v.doll = -v.doll, d_color ^= TextNegColor ^ TextColor;
		snprintf (buf, sizeof(buf), v.doll ? "$%ld" : "-", v.doll);
		V.Put (y, (a->passive ? 77 : 56) - strlen (buf), buf, d_color);

		if (a->anal) {
			if (v.pcs < 0) v.pcs = -v.pcs, p_color ^= TextNegColor ^ TextColor;
			snprintf (buf, sizeof(buf), v.pcs ? "%ld" : "-", v.pcs);
			V.Put (y, 35 - strlen (buf), buf, p_color);
		}
	}
}

static int ViewAccountList (long pacn)
{
	int nacc;
	const char *hint = "\1F4\2-Редактирование  \1F7\2-Новый счет  \1F8\2-Удаление";
	accountinfo **list = AccountList (pacn, &nacc);

	// Сохраняем экран.
	Box box (V, 1, 0, V.Lines-1, V.Columns);
	Hint (hint);

	// Рисуем.
	int y = 6;
	int plen = V.Lines - y - 4;
	V.DrawFrame (1, 0, V.Lines-2, V.Columns, BorderColor);
	if (pacn) {
		char buf [40];
		snprintf (buf, sizeof(buf), " Субсчета %s ", acnum (pacn));
		V.Put (1, (V.Columns - strlen (buf)) / 2, buf, BorderColor);
	}
	V.HorLine (y-1, 1, V.Columns-2, BorderColor);
	V.HorLine (V.Lines-4, 1, V.Columns-2, BorderColor);
	PrintTotal (V.Lines-3, list, nacc);

	int topline = 0;        // верхняя строка на экране
	int cline = 0;          // текущая строка (курсор)
	int reload_flag = 0;
again:
	V.Clear (y-4, 1, 3, V.Columns-2, TextColor);
	V.Put (y-2, 4, "Счет", BorderColor);
	if (amode)
		V.Put (y-2, 32, "Чтение            Запись           Ан     Зап",
			BorderColor);
	else {
		V.Put (y-4, 44, "Актив                Пассив", BorderColor);
		V.HorLine (y-3, 37, 19, BorderColor);
		V.HorLine (y-3, 58, 19, BorderColor);
		V.Put (y-2, 30, "Штуки      Рубли   Доллары      Рубли   Доллары",
			BorderColor);
	}
	for (;;) {
		Hint (hint);
		for (int i=0; i<plen; ++i)
			ViewLine (list, topline+i<nacc ? topline+i : -1, y+i);
		for (;;) {
			Box cursor (V, y+cline-topline, 2, 1, 27);
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
				if (list)
					free (list);
				return reload_flag;
			case cntrl ('J'):       // Enter
			case cntrl ('M'):       // Return
			case meta ('r'):        // right
				if (cline >= nacc || list[cline]->deleted)
					continue;
				if (IsParent (list[cline])) {
					if (ViewAccountList (list[cline]->acn))
						goto reload;
				} else if (list[cline]->rmask)
					if (ViewAccount (list[cline]))
						goto reload;
				break;
			case meta ('d'):        // down
				if (cline >= nacc-1)
					continue;
				++cline;
				if (topline+plen > cline)
					continue;
				++topline;
				V.DelLine (y, TextColor);
				V.InsLine (y + plen-1, TextColor);
				ViewLine (list, topline+plen-1, y+plen-1);
				continue;
			case meta ('u'):        // up
				if (cline <= 0)
					continue;
				--cline;
				if (topline <= cline)
					continue;
				--topline;
				V.DelLine (y + plen-1, TextColor);
				V.InsLine (y, TextColor);
				ViewLine (list, topline, y);
				continue;
			case ' ':               // Space - view mode
				amode = !amode;
				goto again;
			case meta ('n'):        // next page
				if (topline+plen >= nacc)
					continue;
				topline += plen-1;
				cline += plen-1;
				if (topline+plen > nacc) {
					cline -= topline - (nacc - plen);
					topline = nacc - plen;
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
				if (cline >= nacc-1)
					continue;
				topline = nacc - plen;
				if (topline < 0)
					topline = 0;
				cline = nacc - 1;
				if (cline < 0)
					cline = 0;
				break;
			case meta ('G'):        // F7 - insert
				if (! NewAccount (pacn))
					continue;
reload:                         LoadAccounts ();
				if (list)
					free (list);
				list = AccountList (pacn, &nacc);
				PrintTotal (V.Lines-3, list, nacc);
				if (cline >= nacc)
					cline = nacc - 1;
				if (cline < 0)
					cline = 0;
				if (topline > cline)
					topline = cline;
				reload_flag = 1;
				break;
			case meta ('H'):        // F8 - delete
				if (!nacc || !DeleteAccount (list[cline]))
					continue;
				goto reload;
			case meta ('D'):        // F4 - edit
				if (!nacc || !EditAccount (list[cline]))
					continue;
				goto reload;
			}
			break;
		}
	}
}

void Accounts ()
{
	FreeBalances ();
	ViewAccountList (0);
}

static entryinfo *enttab;
static int enttabsz;

static entryinfo *ScanPoint;
static int ScanCount;

static int ScanInit (long acn)
{
	ScanPoint = account_first (&clnt, acn);
	ScanCount = 0;
	if (! ScanPoint)
		return 0;
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
	while (ScanCount < n && (ScanPoint = account_next (&clnt)) != 0) {
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

static void ViewEntryLine (int lnum, int y, accountinfo *a)
{
	int color = (lnum & 1) ? TextColor : TextAltColor;

	V.Clear (y, 1, 1, V.Columns-2, color);
	V.VertLine (0, y, 1, BorderColor);
	V.VertLine (V.Columns-1, y, 1, BorderColor);
	if (lnum < 0)
		return;

	entryinfo *e = enttab + lnum;
	int day, mon, year;
	date2ymd (e->date, &year, &mon, &day);

	V.Print (y, 2, color, "%02d.%02d.%02d  %d",
		day, mon, year % 100, e->eid);
	if (e->deleted)
		V.Put ('#', color);
	V.Print (y, 19, color, "%.34s", e->descr);
	V.Put (y, 77, '-', color);
	V.Put (y, 59, '-', color);

	int c;
	if (a->acn == e->debit)
		c = 60;
	else if (a->acn == e->credit)
		c = 78;
	else
		return;

	char buf[80];
	if (e->amount.rub)
		snprintf (buf, sizeof(buf), "  %ld", e->amount.rub);
	else
		snprintf (buf, sizeof(buf), "  $%ld", e->amount.doll);
	if (a->anal)
		snprintf (buf + strlen(buf), sizeof(buf) - strlen(buf), " (%ld)", e->amount.pcs);
	V.Put (y, c - strlen (buf), buf, color);
}

static void PrintTotalAccount (int y, accountinfo *a)
{
	char buf [64];

	V.Clear (y, 1, 2, V.Columns-2, BorderColor);

	balance *b = account_balance (&clnt, a->acn);
	if (! b)
		return;

	V.Put (y, 40, "Итого:", BorderColor);

	int dr_color, cr_color, dd_color, cd_color;
	dr_color = cr_color = dd_color = cd_color = TextColor;

	if (b->debit.rub < 0) b->debit.rub = -b->debit.rub, dr_color = TextNegColor;
	snprintf (buf, strlen(buf), b->debit.rub ? "%ld" : "-", b->debit.rub);
	V.Put (y, 60 - strlen (buf), buf, dr_color);

	if (b->credit.rub < 0) b->credit.rub = -b->credit.rub, cr_color = TextNegColor;
	snprintf (buf, strlen(buf), b->credit.rub ? "%ld" : "-", b->credit.rub);
	V.Put (y, 78 - strlen (buf), buf, cr_color);

	if (b->debit.doll < 0) b->debit.doll = -b->debit.doll, dd_color = TextNegColor;
	snprintf (buf, strlen(buf), b->debit.doll ? "$%ld" : "-", b->debit.doll);
	V.Put (y+1, 60 - strlen (buf), buf, dd_color);

	if (b->credit.doll < 0) b->credit.doll = -b->credit.doll, cd_color = TextNegColor;
	snprintf (buf, strlen(buf), b->credit.doll ? "$%ld" : "-", b->credit.doll);
	V.Put (y+1, 78 - strlen (buf), buf, cd_color);
}

static int ViewAccount (accountinfo *a)
{
	int nent = ScanInit (a->acn);

	// Сохраняем экран.
	Box box (V, 1, 0, V.Lines-1, V.Columns);
	Hint ("\1F4\2-Редактирование  \1F5\2-Повторить проводку  \1F7\2-Новая проводка  \1F8\2-Удаление");

	// Рисуем.
	int y = 4;
	int h = V.Lines - y - 5;
	int plen = h;
	V.DrawFrame (1, 0, V.Lines-2, V.Columns, BorderColor);

	char buf [80];
	snprintf (buf, strlen(buf), " %s %.50s ", acnum (a->acn), a->descr);
	V.Put (y-3, (V.Columns - strlen (buf)) / 2, buf, BorderColor);

	V.HorLine (y-1, 1, V.Columns-2, BorderColor);
	V.HorLine (V.Lines-5, 1, V.Columns-2, BorderColor);
	PrintTotalAccount (V.Lines-4, a);

	V.Clear (y-2, 1, 1, V.Columns-2, BorderColor);
	V.Put (y-2, 4, "Дата   Номер   Описание                            Дебет            Кредит",
		BorderColor);

	int topline = 0;        // верхняя строка на экране
	int cline = 0;          // текущая строка (курсор)
	int reload_flag = 0;
	for (;;) {
		if (ScanPoint && nent < topline + plen)
			nent = ScanTo (topline + plen);
		for (int i=0; i<plen; ++i)
			ViewEntryLine (topline+i<nent ? topline+i : -1, y+i, a);
		for (;;) {
			Box cursor (V, y+cline-topline, 1, 1, 50);
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
				return reload_flag;
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
				V.InsLine (y + plen-1, TextColor);
				ViewEntryLine (topline+plen-1, y+plen-1, a);
				continue;
			case meta ('u'):        // up
				if (cline <= 0)
					continue;
				--cline;
				if (topline <= cline)
					continue;
				--topline;
				V.DelLine (y + plen-1, TextColor);
				V.InsLine (y, TextColor);
				ViewEntryLine (topline, y, a);
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
			case meta ('E'):        // F5 - copy
				if (!nent || !CopyJournalEntry (enttab+cline))
					continue;
				goto reload;
			case meta ('G'):        // F7 - insert
				if (! NewJournalEntry (a))
					continue;
reload:                         nent = ScanInit (a->acn);
				PrintTotalAccount (V.Lines-4, a);
				cline = 0;
				topline = 0;
				reload_flag = 1;
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
