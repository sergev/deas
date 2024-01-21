//
// 1) Выделение записей на счете/в журнале (Ins/"-"),
//    подсчет и отображение суммы выделенных проводок.
//
// 2) То же самое в режиме списка счетов.
//
// 3) Просмотр счетов по направлениям.
//
// 4) Оборотная ведомость в режиме просмотра списка счетов.
//
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/time.h>
#include <sys/wait.h>
#include <signal.h>
#include <sys/types.h>

#include "Screen.h"
#include "Popup.h"
#include "Dialog.h"
#include "deas.h"

#define GMAS_VERSION "1.1"

clientinfo clnt;

void Entries ();
void Accounts ();
void Journal ();
void Users ();
void Options ();
void LoadAccounts ();
void LoadEntries ();

struct {
	char *button;
	void (*func)();
	int big;
	char *help;
	int r, w, h;
} menu[] = {
	{ "Операции", Entries, 1,
		"Проводка операций по счетам\n"
		},
	{ "Счета", Accounts, 1,
		"Просмотр счетов, балансов и\nпроводок по счетам"
		},
	{ "Журнал", Journal, 1,
		"Просмотр журнала проводок"
		},
	{ "Пользователи", Users, 0,
		"Просмотр списка пользователей\n"
		"системы и прав доступа"
		},
#if 0
	{ "Установки", Options, 0,
		"Установка даты и режима\n"
		"просмотра удаленных записей"
		},
#endif
	{ 0 }
};

Screen V;

int LightTextColor;
int InverseTextColor;
int InverseLightTextColor;
int DimTextColor;
int InverseDimTextColor;
int ErrorColor;

int TextColor;
int TextAltColor;
int TextNegColor;
int CursorColor;
int BorderColor;
int DialogColor;
int DialogOptionColor;
int BackgroundColor;
int HelpColor;
int HintColor;
int LightHintColor;
int MenuColor;
int MenuBorderColor;
int ActiveMenuColor;

unsigned char DialogPalette[9] =
{
	0x1f, 0x1f, 0x1e, 0x30, 0x3b, 0x70, 0x74, 0x7f, 0x7e
};

int fillchar;		// символ для заполнения фона
int fastmode = 0;	// признак "быстрого" режима, без заполнения фона
char username [LOGNSZ]; // регистрационное имя пользователя
int delflg;             // флаг отображения удаленных записей
int admin;              // признак администратора

extern char **environ;

void Help (char *item);

void Quit ()
{
	V.Clear ();
	V.Move (V.Lines - 1, 0);
	V.Sync ();
	exit (0);
}

void Killed (int)
{
	V.Move (V.Lines - 1, 0);
	V.ClearLine (0x07);
	V.Sync ();
	fprintf (stderr, "Interrupted\n");
	exit (0);
}

void PromptQuit ()
{
	V.DelHotKey (meta ('J'));
	V.DelHotKey (alt ('x'));
	V.DelHotKey (alt ('q'));
	V.DelHotKey (cntrl ('Q'));
	int choice = V.Popup (" Выход ", "Вы действительно хотите выйти?", 0,
		" Да ", " Нет ", 0, ErrorColor, TextColor);
	if (choice == 0)
		Quit ();
	V.AddHotKey (meta ('J'), (void(*)(int)) PromptQuit);
	V.AddHotKey (alt ('x'), (void(*)(int)) PromptQuit);
	V.AddHotKey (alt ('q'), (void(*)(int)) PromptQuit);
	V.AddHotKey (cntrl ('Q'), (void(*)(int)) PromptQuit);
	V.Sync ();
}

void RunShell ()
{
	Box box (V, 0, 0, V.Lines, V.Columns);
	V.Clear (0x07);
	V.Move (0, 0);
	V.Sync ();
	V.Restore ();

	char *shell = getenv ("SHELL");
	if (! shell || *shell != '/')
		shell = "/bin/sh";
	int t = fork ();

	// Потомок.
	if (t == 0) {
		for (int i=3; i<20; ++i)
			close (i);
		execle (shell, 1 + strchr (shell, '/'), "-i", (char*) 0, environ);
		_exit (-1);                     // file not found
	}

	// Предок.
	if (t > 0) {
		#ifdef SIGTSTP
		// Игнорируем suspend, пока ждем завершения шелла.
		void (*oldtstp)(int) = signal (SIGTSTP, SIG_IGN);
		#endif

		// Ждем, пока он не отпадет.
		int status;
		while (t != wait (&status))
			continue;

		#ifdef SIGTSTP
		// Восстановим реакцию на suspend.
		signal (SIGTSTP, (void(*)(int))oldtstp);
		// Вернем терминал себе, если shell его отобрал.
		// Иначе получим SIGTTOU.
		tcsetpgrp (2, getpid ());
		#endif
	}
	V.Reopen ();
	V.Clear (TextColor);
	V.Put (box);
	V.HideCursor ();
}

void Fatal (char *msg)
{
	V.Error (ErrorColor, TextColor, " Ошибка ", " Готово ",
		"Ошибка базы: %s", msg);
	Quit ();
}

void Message (char *str, ...)
{
	V.Put (0, 0, ' ', InverseTextColor);
	if (str)
		V.PrintVect (InverseTextColor, str, &str + 1);
	V.ClearLine (InverseTextColor);
}

void Hint (char *str)
{
	V.Put (V.Lines-1, 0, ' ', HintColor);
	for (int attr=HintColor; *str; ++str)
		if (*str == '\1')
			attr = LightHintColor;
		else if (*str == '\2')
			attr = HintColor;
		else
			V.Put (*str, attr);
	V.ClearLine (HintColor);
}

void Redraw (int key)
{
	V.Redraw ();
	V.Sync ();
}

void PutMultistring (int r, int c0, int maxc, char *str, int color)
{
	int c;

	V.Move (r, c0);
	for (c=c0; *str; ++str) {
		if (*str == '\n') {
			V.Move (++r, c = c0);
			continue;
		}
		if (c-c0 < maxc)
			V.Put (*str, color);
		++c;
	}
}

void Title ()
{
	char buf[32];
	int year, mon, day;

	V.ClearLine (0, 0, InverseTextColor);
	if (*username)
		V.Print (0, 1, InverseTextColor, "Пользователь: %.16s", username);
	date2ymd (option_date (&clnt, 0), &year, &mon, &day);
	sprintf (buf, "%d %.3s %d", day,
		"ДекЯнвФевМарАпрМайИюнИюлАвгСенОктНояДек" + mon*3, year);
	V.Put (0, V.Columns-12, buf, InverseTextColor);
}

#if 0
void Options ()
{
	static Dialog *dlg;

	int d = delflg;
	int id, im, iy;
	date2ymd (deas_now, &iy, &im, &id);
	long day = id;
	long mon = im;
	long year = iy;
again:
	if (! dlg)
		dlg = new Dialog (" Установки ", DialogPalette,
			"R :1{Отображать удаленные объекты} :0{Не отображать} {}"
			"{ День} | N:1:99999:1"
			"{Месяц} | N:1:99999:1"
			"{  Год} | N:1:99999:1",
			&d, &day, &mon, &year);
	if (dlg->Run (&V) <= 0)
		return;

	long n = ymd2date (year, mon, day);
	if (n < 0) {
		V.Beep ();
		goto again;
	}
	option_deleted (&clnt, delflg = d);
	option_date (&clnt, n);
	Title ();
}
#endif

static long msec (int interval)
{
	static struct timeval tv0;
	struct timeval tv;

	if (! interval) {
		gettimeofday (&tv0, 0);
		return 0;
	}
	gettimeofday (&tv, 0);
	return (tv.tv_sec  - tv0.tv_sec)  * 1000L +
	       (tv.tv_usec - tv0.tv_usec) / 1000L;
}

int main (int argc, char **argv)
{
	signal (SIGINT, Killed);
	signal (SIGTERM, Killed);
#ifdef SIGQUIT
	signal (SIGQUIT, Killed);
#endif
	// Установки для msdos.  Перенос теста на прочие
	// платформы пока не планируется.
	fillchar = ' ';
	fastmode = 0;

	LightTextColor        = 0x71;
	DimTextColor          = 0x74;
	InverseTextColor      = 0x1b;
	InverseLightTextColor = 0x1f;
	InverseDimTextColor   = 0x17;
	ErrorColor            = 0x4f;

	TextColor             = 0x70;
	TextAltColor          = 0x30;
	TextNegColor          = 0x7c;
	CursorColor           = 0x1f;
	BorderColor           = 0x78;
	BackgroundColor       = 0x70;
	HelpColor             = 0x7b;
	HintColor             = 0x47;
	LightHintColor        = 0x4e;
	MenuColor             = 0x1b;
	ActiveMenuColor       = 0x3f;
	MenuBorderColor       = 0x3c;
	DialogColor           = 0x1f;
	DialogOptionColor     = 0x3f;

	// Установка клавиш перерисовки экрана и вызова help.
	// Они работают всегда.
	V.AddHotKey (cntrl ('R'), Redraw);
	V.AddHotKey (cntrl ('L'), Redraw);
	// V.AddHotKey (meta ('A'), Help);
	V.AddHotKey (meta ('J'), (void(*)(int)) PromptQuit);
	V.AddHotKey (alt ('x'), (void(*)(int)) PromptQuit);
	V.AddHotKey (alt ('q'), (void(*)(int)) PromptQuit);
	V.AddHotKey (cntrl ('Q'), (void(*)(int)) PromptQuit);

	// Если терминал монохромный, принудительно устанавливаем
	// черно-белую палитру.
	if (! V.HasColors ()) {
		TextColor             = 0x07;
		LightTextColor        = 0x0f;
		DimTextColor          = 0x07;
		InverseTextColor      = 0x70;
		InverseLightTextColor = 0x70;
		InverseDimTextColor   = 0x70;
		HelpColor             = 0x70;
		ErrorColor            = 0x70;
	}

	// Стирание экрана нужным цветом, который стал известен
	// после зачитывания setup.
	if (fastmode)
		V.Clear (BackgroundColor);
	else
		V.Clear (0, 0, V.Lines, V.Columns, BackgroundColor, fillchar);

	Message ("Учетная система Кроникс, Версия %s, Copyright (C) 1996 Кроникс",
		GMAS_VERSION);

	V.HideCursor ();
	Hint ("");

	/*
	 * Открываем базу.
	 */
	char *host;
	if (argc > 1)
		host = argv[1];
	else
		host = getenv ("DEASHOST");
	Flash *alert = new Flash (&V, "", "Подсоединение к серверу...",
		InverseLightTextColor);
	if (! deas_init (host, 0, Fatal)) {
		delete alert;
		V.Error (ErrorColor, TextColor, " Ошибка ", " Готово ",
			"Ошибка при инициализации базы");
		Quit ();
	}
	delete alert;

	/*
	 * Запрашиваем имя пользователя.
	 */
	msec (0);
	for (;;) {
		char password[40];
		static Dialog *dlg;
		username[0] = 0;
		password[0] = 0;
again:
		if (! dlg)
			dlg = new Dialog (" Регистрация ", DialogPalette,
				"++{} {   Имя:} | S:16:16 | { }"
				"{} {Пароль:} | p:16:16 { }",
				username, password);
		if (dlg->Run (&V) <= 0 || ! *username || ! *password)
			goto again;
		Flash *alert = new Flash (&V, "", "Проверка полномочий...",
			InverseLightTextColor);
		clnt.u = user_register (username, password, msec (1));
		delete alert;
		if (clnt.u)
			break;
		V.Error (ErrorColor, TextColor, " Ошибка ", " Готово ",
			"Неверное имя пользователя");
	}
	option_deleted (&clnt, delflg = 0);
	Title ();

	/*
	 * Проверяем, не администратор ли.
	 */
	if (user_first (&clnt))
		admin = 1;

	/*
	 * Загружаем список счетов.
	 */
	alert = new Flash (&V, "", "Загрузка счетов...",
		InverseLightTextColor);
	LoadAccounts ();
	delete alert;

	/*
	 * Загружаем меню проводок.
	 */
	alert = new Flash (&V, "", "Загрузка проводок...",
		InverseLightTextColor);
	LoadEntries ();
	delete alert;

	/*
	 * Рисуем и запускаем основное меню.
	 */
	int h = 0;
	int w = 0;
	int i;
	for (i = 0; menu[i].button; ++i) {
		menu[i].r = h;
		menu[i].h = menu[i].big ? 3 : 1;
		h += 1 + menu[i].h;

		menu[i].w = strlen (menu[i].button);
		if (w < menu[i].w)
			w = menu[i].w;
	}

	int nent = i;
	if (! admin) {
		/* Выключаем подменю "пользователи". */
		--nent;
		h -= 1 + menu[3].h;
	}

	w += 6;
	if (w >= V.Columns/2)
		w = V.Columns/2 - 1;

	int roff = (V.Lines + 2 - h) / 2;
	for (i=0; i<nent; ++i)
		menu[i].r += roff;

	int c = (V.Columns/2 - w) / 2;

	V.DrawFrame (roff-2, c-3, h+3, w+7, BorderColor);
	for (i=0; i<nent; ++i)
		V.Clear (menu[i].r, c, menu[i].h, w, MenuColor, ' ');

	int ch = 0;
	PutMultistring (V.Lines/2-2, V.Columns/2+4, V.Columns/2-4,
		menu[ch].help, HelpColor);
	for (;;) {
		for (int i=0; i<nent; ++i) {
			int color = (i == ch) ? ActiveMenuColor : MenuColor;
			V.Clear (menu[i].r, c, menu[i].h, w, color, ' ');
			if (menu[i].big) {
				V.Put (menu[i].r+1, c + (w - menu[i].w) / 2,
					menu[i].button, color);
				if (i == ch)
					V.DrawFrame (menu[i].r, c, 3, w, MenuBorderColor);
			} else {
				V.Put (menu[i].r, c + (w - menu[i].w) / 2,
					menu[i].button, color);
				if (i == ch) {
					V.Put (menu[i].r, c, '[', MenuBorderColor);
					V.Put (menu[i].r, c+w-1, ']', MenuBorderColor);
				}
			}
		}
		V.HideCursor ();
		V.Sync ();
		switch (V.GetKey ()) {
		default:
			V.Beep ();
			continue;
		case cntrl ('M'):
		case cntrl ('J'):
		case meta ('r'):        // right
			if (menu[ch].func) {
				(*menu[ch].func) ();
				Hint ("");
			}
			continue;
		case meta ('d'):        // down
			if (++ch >= nent)
				ch = 0;
			break;
		case meta ('u'):        // up
			if (--ch < 0)
				ch = nent-1;
			break;
		}
		V.Clear (2, V.Columns/2+2, V.Lines-4, V.Columns/2-4,
			TextColor, ' ');
		PutMultistring (V.Lines/2-2, V.Columns/2+4, V.Columns/2-4,
			menu[ch].help, HelpColor);
	}
}
