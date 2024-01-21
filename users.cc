#include <string.h>
#include "Screen.h"
#include "deas.h"

extern Screen V;
extern int TextColor;
extern int BorderColor;
extern clientinfo clnt;

static userinfo utab[MAXUSERS];
static int nusers;

static void ViewLine (userinfo *u, int y)
{
	V.Clear (y, 1, 2, V.Columns-2, TextColor);
	V.VertLine (0, y, 2, BorderColor);
	V.VertLine (V.Columns-1, y, 2, BorderColor);
	if (u) {
		V.Print (y, 4, TextColor, "%-16.16s  %-8.8s ",
			u->user, u->logname);
		for (int i=0; i<GRMAX; ++i)
			V.Put ((u->gmask >> i) & 1 ?
				"0123456789012345"[i] : '-', TextColor);
		V.Put (' ', TextColor);
		if (strlen (u->descr) <= 30)
			V.Put (u->descr, TextColor);
		else {
			int c = V.Col();
			int ch = u->descr [30];
			u->descr [30] = 0;
			char *p = strrchr (u->descr, ' ');
			u->descr [30] = ch;
			p = p ? p+1 : u->descr + 30;
			V.PutLimited (u->descr, p - u->descr, TextColor);
			V.PutLimited (y+1, c, p, UDESCRSZ-1-30, TextColor);
		}

	}
}

void Users ()
{
	userinfo *u;

	// Зачитываем список пользователей.
	nusers = 0;
	u = user_first (&clnt);
	while (u) {
		utab[nusers++] = *u;
		u = user_next (&clnt);
	}

	// Сохраняем экран.
	Box box (V, 1, 0, V.Lines-1, V.Columns);

	// Рисуем.
	int y = 4;
	int h = (V.Lines - y - 2) / 2 * 2;
	int plen = h / 2;
	V.DrawFrame (y-3, 0, plen*2+4, V.Columns, BorderColor);
	V.HorLine (y-1, 1, V.Columns-2, BorderColor);
	V.Put (y-2, 4, "Пользователь      Вход     Группы           Фамилия, Имя, Отчество", BorderColor);

	// Номер строки, находящейся в первой строке экрана.
	int cline = 0;
	for (;;) {
		for (int i=0; i<plen; ++i)
			ViewLine (cline+i<nusers ? utab+cline+i : 0, y+i*2);
		for (;;) {
			V.HideCursor ();
			V.Sync ();
			switch (V.GetKey ()) {
			default:
				V.Beep ();
				continue;
			case cntrl ('['):       // Escape - exit
			case cntrl ('C'):       // ^C - exit
			case meta ('l'):        // left
				V.Put (box);
				return;
			case cntrl ('M'):       // ^M - down
			case cntrl ('J'):       // ^J - down
			case meta ('d'):        // down
				if (cline+plen >= nusers)
					continue;
				++cline;
				V.DelLine (y, TextColor);
				V.DelLine (y, TextColor);
				V.InsLine (y + (plen-1)*2, TextColor);
				V.InsLine (y + (plen-1)*2, TextColor);
				ViewLine (utab+cline+plen-1, y+(plen-1)*2);
				continue;
			case meta ('u'):        // up
				if (cline <= 0)
					continue;
				--cline;
				V.DelLine (y + (plen-1)*2, TextColor);
				V.DelLine (y + (plen-1)*2, TextColor);
				V.InsLine (y, TextColor);
				V.InsLine (y, TextColor);
				ViewLine (utab+cline, y);
				continue;
			case meta ('n'):        // next page
				if (cline+plen >= nusers)
					continue;
				cline += plen - 1;
				break;
			case meta ('p'):        // prev page
				if (cline <= 0)
					continue;
				if (cline < plen-1)
					cline = 0;
				else
					cline -= plen - 1;
				break;
			case meta ('h'):        // home
				if (cline == 0)
					continue;
				cline = 0;
				break;
			case meta ('e'):        // end
				if (cline+plen >= nusers)
					// Уже на последней странице.
					continue;
				cline = nusers - plen;
				break;
			}
			break;
		}
	}
}

