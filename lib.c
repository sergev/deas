#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include "deas.h"
#include "pgplib.h"

#define WORKDIR         "/var/deas"             /* рабочий каталог сервера */
#define USERFILE        "config"                /* список пользователей */
#define CHARTFILE       "chart"                 /* план счетов */
#define JOURNFILE       "journal"               /* журнал проводок */

#define ISSPACE(c)      strchr (" \t\r\n\f", c)

#define GROUP(n)        (1L << (n))             /* маска группы */
#define MANAGER(u)      ((u)->gmask & GROUP(0)) /* суперправа доступа */
#define SORT(a,n,f)     qsort (a, n, sizeof(a[0]), (int(*)())f)
#define DATE()          ((time (0) - 631152000L) / 60 / 60 / 24)
#define MASK_UNION(a,b) ((a) | (b))

/*
 * Открытый счет.
 */
typedef struct _account_t {
	accountinfo     info;           /* информация о счете */
	long            *entry;         /* таблица номеров проводок */
	long            nent;           /* количество проводок */
	long            entsz;          /* размер таблицы entry */
	balance         total;          /* баланс */
	balance         monthly [12];   /* помесячный баланс */
	int             tvalid;         /* признак корректного баланса */
} account_t;

char deas_E [KEYSZ];
char deas_D [KEYSZ];
char deas_P [KEYSZ];
char deas_Q [KEYSZ];
char deas_U [KEYSZ];
char deas_N [KEYSZ];

static userdata utab[MAXUSERS];
static int nusers;

static account_t acntab[MAXACCOUNTS];
static int naccounts;

static menuinfo menutab[MAXMENU];
static int nmenu;

static entryinfo *journal;
static long *jindex;
static int jlen;
static int jsz;

static int chartfd = -1;        /* файл плана счетов */
static int journfd = -1;        /* файл журнала */

static void (*deas_fatal) (char*);

static char enc[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz@%0123456789";

static int dec (char c)
{
	char *p = strchr (enc, c);
	return p ? p-enc : 0;
}

#if 0
static void printkey (char *name, char *val)
{
	int len, i;
	int reg, n;

	for (len=KEYSZ; len>0; --len)
		if (val[len-1])
			break;
	printf ("%s ", name);

	reg = len;
	n = 8;
	i = 0;
	for (;;) {
		putchar (enc[reg & 077]);
		reg >>= 6;
		n -= 6;
		if (n < 6) {
			if (i >= len)
				break;
			reg |= (unsigned char) val[i++] << n;
			n += 8;
		}
	}
	if (n > 0)
		putchar (enc[reg & 077]);
	printf ("\n");
}
#endif

static void scankey (char *str, char *val)
{
	int len, reg, n;

	memset (val, 0, KEYSZ);
	while (strchr (" \t\r\n", *str))
		++str;

	reg = dec (*str++);
	reg |= dec (*str++) << 6;
	len = (unsigned char) reg;
	reg >>= 8;
	n = 6+6 - 8;
	while (len > 0) {
		reg |= dec (*str++) << n;
		n += 6;
		if (n >= 8) {
			*val++ = reg;
			reg >>= 8;
			n -= 8;
			--len;
		}
	}
}

static char *getstring (char *str, int len, char *input)
{
	int lim = 0;
	char *sav;

	*str = 0;
	while (*input && ISSPACE (*input))
		++input;
	sav = input;
	if (*input == '"')
		lim = 1, ++input;
	while (*input && (! lim || *input != '"') &&
	    (lim || ! ISSPACE (*input)) && len-- > 0)
		*str++ = *input++;
	if (lim && *input == '"')
		++input;
	if (input <= sav)
		return 0;
	if (len)
		*str = 0;
	return input;
}

static int month (long date)
{
	int y, m, d;

	date2ymd (date, &y, &m, &d);
	return m-1;
}

static void vadd (val *v, long rub, long doll, long pcs)
{
	v->rub += rub;
	v->doll += doll;
	v->pcs += pcs;
	++v->rec;
}

static void badd (balance *a, balance *b)
{
	a->debit.rub   += b->debit.rub;
	a->debit.doll  += b->debit.doll;
	a->debit.pcs   += b->debit.pcs;
	a->debit.rec   += b->debit.rec;
	a->credit.rub  += b->credit.rub;
	a->credit.doll += b->credit.doll;
	a->credit.pcs  += b->credit.pcs;
	a->credit.rec  += b->credit.rec;
}

/*
 * Неоптимизированный поиск счета в плане счетов.
 */
static accountinfo *cfind (long acn)
{
	account_t *a;

	for (a=acntab; a<acntab+naccounts; ++a)
		if (a->info.acn == acn)
			return &a->info;
	return 0;
}

/*
 * Чтение файла плана счетов.
 */
static int cinit ()
{
	chartfd = open (CHARTFILE, 2);
	if (chartfd < 0) {
		close (creat (CHARTFILE, 0600));
		chartfd = open (CHARTFILE, 2);
		if (chartfd < 0)
			return 0;
	}
	for (naccounts=0; naccounts<MAXACCOUNTS; ++naccounts) {
		if (crypt_read (chartfd, &acntab[naccounts].info,
		    sizeof(accountinfo)) != sizeof(accountinfo))
			break;
		acntab[naccounts].nent = 0;
	}
	return 1;
}

/*
 * Запись файла плана счетов.
 */
static void csave (void)
{
	int i;

	lseek (chartfd, 0L, 0);
	for (i=0; i<naccounts; ++i)
		crypt_write (chartfd, &acntab[i].info, sizeof(accountinfo));
	fsync (chartfd);
}

/*
 * Сравнение счетов для упорядочения.
 */
static int ccompare (account_t *a, account_t *b)
{
	char acn[64], bcn[64];

	sprintf (acn, "%ld", a->info.acn);
	sprintf (bcn, "%ld", b->info.acn);
	return strcmp (acn, bcn);
}

/*
 * Чтение номера счета. Согласно отечественному плану
 * бухгалтерских счетов, номер счета может начинаться с нуля.
 * Чтобы учесть ведущие нули, приписываем к номеру счета спереди единицу.
 */
static long getacn (char *str)
{
	long acn = 1;
	while (*str >= '0' && *str <= '9')
		acn = acn * 10 + *str++ - '0';
	return acn;
}

/*
 * Чтение файла пользователей.
 */
static int uinit ()
{
	FILE *fd;
	userdata *c = utab;
	char line [1024], val [256], *p, *np;

	fd = fopen (USERFILE, "r");
	if (! fd)
		return 0;
	nusers = 0;
	while (fgets (line, sizeof(line), fd)) {
		if (! *line || *line == '\r' || *line == '\n' || *line == '#')
			continue;
		p = line + strlen(line) - 1;
		while (p >= line && (*p == '\n' || *p == '\r'))
			*p-- = 0;
		if (strncmp (line, "user ", 5) == 0) {
			/* учетное имя */
			c = utab + nusers++;
			memset (c, 0, sizeof(*c));
			c->info.gmask = 0;
			if (! getstring (c->info.user, UNAMSZ, line+5))
				goto failed;
		} else if (strncmp (line, "logname ", 8) == 0) {
			/* регистрационное имя */
			if (! getstring (c->info.logname, LOGNSZ, line+8))
				goto failed;
		} else if (strncmp (line, "fullname ", 9) == 0) {
			/* подробная информация */
			if (! getstring (c->info.descr, UDESCRSZ, line+9))
				goto failed;
		} else if (strncmp (line, "syse ", 5) == 0) {
			/* системный ключ E */
			scankey (line+5, deas_E);
		} else if (strncmp (line, "sysd ", 5) == 0) {
			/* системный ключ D */
			scankey (line+5, deas_D);
		} else if (strncmp (line, "sysp ", 5) == 0) {
			/* системный ключ P */
			scankey (line+5, deas_P);
		} else if (strncmp (line, "sysq ", 5) == 0) {
			/* системный ключ Q */
			scankey (line+5, deas_Q);
		} else if (strncmp (line, "sysu ", 5) == 0) {
			/* системный ключ U */
			scankey (line+5, deas_U);
		} else if (strncmp (line, "sysn ", 5) == 0) {
			/* системный ключ N */
			scankey (line+5, deas_N);
		} else if (strncmp (line, "keye ", 5) == 0) {
			/* ключ пользователя E */
			scankey (line+5, c->E);
		} else if (strncmp (line, "keyn ", 5) == 0) {
			/* ключ пользователя N */
			scankey (line+5, c->N);
		} else if (strncmp (line, "group ", 6) == 0) {
			/* список групп доступа */
			if (! getstring (val, sizeof(val), line+6))
				goto failed;
			for (p=val; *p; ) {
				c->info.gmask |= GROUP (strtol (p, &np, 0));
				if (np == p)
					break;
				p = np;
				if (*p == ',')
					++p;
			}
		} else if (strncmp (line, "menu ", 5) == 0) {
			/* меню проводок */
			menuinfo *m = menutab + nmenu++;
			memset (m, 0, sizeof(*m));
			if (! getstring (m->line, sizeof (m->line), line+5))
				goto failed;
		} else if (*line == '+') {
			/* строка меню проводок */
			menuinfo *m = menutab + nmenu++;
			char *ptr, buf[8];
			memset (m, 0, sizeof(*m));
			ptr = getstring (val, sizeof (val), line+1);
			if (! ptr)
				goto failed;
			m->credit = getacn (val);
			ptr = getstring (val, sizeof (val), ptr);
			if (! ptr)
				goto failed;
			m->debit = getacn (val);
			ptr = getstring (m->line, sizeof (m->line), ptr);
			if (! ptr)
				goto failed;
			ptr = getstring (m->descr, sizeof (m->descr), ptr);
			if (ptr && getstring (buf, sizeof (buf), ptr))
				if (*buf == '$')
					m->currency = 2;
				else if (*buf == 'р' || *buf == 'Р' ||
				    *buf == 'r' || *buf == 'R')
					m->currency = 1;
		} else {
failed:                 fclose (fd);
			return 0;
		}
	}
	fclose (fd);
	if (! *(long*)deas_E || ! *(long*)deas_D || ! *(long*)deas_P ||
	    ! *(long*)deas_Q || ! *(long*)deas_U || ! *(long*)deas_N)
		return 0;
	return 1;
}

static int acompare (long *na, long *nb)
{
	entryinfo *a = journal + *na - 1;
	entryinfo *b = journal + *nb - 1;

	if (a->date < b->date)
		return -1;
	if (a->date > b->date)
		return 1;
	if (a->eid < b->eid)
		return -1;
	if (a->eid > b->eid)
		return 1;
	return 0;
}

/*
 * Инициализация файла журнала.
 */
static int jinit ()
{
	struct stat st;
	int i;

	journfd = open (JOURNFILE, 2);
	if (journfd < 0) {
		close (creat (JOURNFILE, 0600));
		journfd = open (JOURNFILE, 2);
		if (journfd < 0)
			return 0;
	}

	/* Блокируем файл. */
	if (flock (journfd, LOCK_EX) < 0 || stat (JOURNFILE, &st) < 0) {
failed:         close (journfd);
		return 0;
	}

	/* Выделяем память под журнал. */
	jlen = st.st_size / sizeof (entryinfo);
	jsz = (jlen + 64) / 64 * 64;
	journal = (entryinfo*) malloc (jsz * sizeof (entryinfo));
	jindex = (long*) malloc (jsz * sizeof (long));
	if (! journal || ! jindex)
		goto failed;

	/* Зачитываем и декодируем журнал. */
	for (i=0; i<jlen; ++i) {
		if (crypt_read (journfd, journal+i, sizeof(entryinfo)) !=
		    sizeof(entryinfo))
			goto failed;
		jindex[i] = i+1;
	}
	if (jlen)
		SORT (jindex, jlen, acompare);
	return 1;
}

/*
 * Чтение проводки из журнала.
 */
static entryinfo *jread (long eid)
{
	if (eid < 1 || eid > jlen)
		return 0;
	return journal + eid - 1;
}

/*
 * Запись проводки в журнал.
 */
static void jsave (entryinfo *e)
{
	if (e->eid < 1 || e->eid > jlen)
		return;
	if (e != journal + e->eid - 1)
		journal [e->eid - 1] = *e;
	lseek (journfd, (e->eid - 1) * sizeof(*e), 0);
	crypt_write (journfd, e, sizeof(entryinfo));
	fsync (journfd);
}

/*
 * Открываем счет.
 */
static account_t *afind (long acn)
{
	account_t *t;

	for (t=acntab; t<acntab+naccounts; ++t)
		if (t->info.acn == acn)
			return t;
	return 0;
}

static void ainvalidate (account_t *t)
{
	while (t) {
		t->tvalid = 0;
		if (! t->info.pacn)
			return;
		t = afind (t->info.pacn);
	}
}

/*
 * Запись проводки на счет.
 */
static void aappend (account_t *t, entryinfo *e)
{
	if (t->nent >= t->entsz) {
		if (! t->entsz) {
			t->entsz = 16;
			t->entry = (long*) malloc (t->entsz * sizeof (long));
		} else {
			t->entsz += 16;
			t->entry = (long*) realloc (t->entry, t->entsz * sizeof (long));
		}
		if (! t->entry) {
			t->entsz = 0;
			t->nent = 0;
			return;
		}
	}
	t->entry[t->nent++] = e->eid;
	ainvalidate (t);
}

/*
 * Запись проводки на счет.
 */
static void asave (account_t *t, entryinfo *e)
{
	aappend (t, e);
	if (t->nent)
		SORT (t->entry, t->nent, acompare);
}

/*
 * Удаление проводки со счета.
 */
static void adelete (account_t *t, entryinfo *e)
{
	int i;

	for (i=0; i<t->nent; ++i)
		if (t->entry [i] == e->eid)
			break;
	if (i >= t->nent)
		return;
	if (i < t->nent - 1)
		memmove (t->entry + i, t->entry + i + 1,
			(t->nent - i - 1) * sizeof (long));
	--t->nent;
	ainvalidate (t);
}

/*
 * Обновление проводки на счете.
 */
static void aupdate (account_t *t, entryinfo *e)
{
	int i;

	ainvalidate (t);
	for (i=0; i<t->nent; ++i)
		if (t->entry [i] == e->eid)
			break;
	if (i >= t->nent)
		aappend (t, e);
	if (t->nent)
		SORT (t->entry, t->nent, acompare);
}

/*
 * Вычисление баланса на текущую дату.
 */
static void acompute (account_t *t)
{
	entryinfo *e;
	account_t *s;
	int i, m;

	memset (&t->total, 0, sizeof (t->total));
	memset (t->monthly, 0, sizeof (t->monthly));

	/* Суммируем записи счета. */
	for (i=0; i<t->nent; ++i) {
		e = jread (t->entry [i]);
		if (! e || ! e->eid || e->deleted)
			continue;
		m = month (e->date);
		if (t->info.acn == e->debit) {
			vadd (&t->total.debit, e->amount.rub,
				e->amount.doll, e->amount.pcs);
			if (m >= 0 && m < 12)
				vadd (&t->monthly[m].debit, e->amount.rub,
					e->amount.doll, e->amount.pcs);
		} else if (t->info.acn == e->credit) {
			vadd (&t->total.credit, e->amount.rub,
				e->amount.doll, e->amount.pcs);
			if (m >= 0 && m < 12)
				vadd (&t->monthly[m].credit, e->amount.rub,
					e->amount.doll, e->amount.pcs);
		}
	}

	/* Добавляем балансы подсчетов. */
	for (s=acntab; s<acntab+naccounts; ++s) {
		if (s->info.deleted || ! s->info.acn ||
		    s->info.pacn != t->info.acn)
			continue;
		if (! s->tvalid)
			acompute (s);
		badd (&t->total, &s->total);
		for (m=0; m<12; ++m)
			badd (&t->monthly[m], &s->monthly[m]);
	}
	t->tvalid = 1;
}

/*--------------------------------------------------*/
void deas_close ()
{
	account_t *t;

	/* Закрываем все счета. */
	for (t=acntab; t<acntab+naccounts; ++t)
		if (t->entsz) {
			free (t->entry);
			t->entsz = 0;
		}
	naccounts = 0;

	if (chartfd >= 0)
		close (chartfd);
	if (journfd >= 0)
		close (journfd);
	chartfd = -1;
	journfd = -1;
}

int deas_init (char *hostdir, int port, void (*fatal)(char*))
{
	entryinfo *e;
	account_t *db, *cr, *c;
	long eid;

	if (chartfd >= 0)
		deas_close ();
	chartfd = -1;
	journfd = -1;
	deas_fatal = fatal;

	if (! hostdir)
		hostdir = WORKDIR;
	if (chdir (hostdir) < 0)
		goto failed;

	/* Читаем план счетов, таблицу юзеров, журнал. */
	crypt_init ("D0blen1r9cun19s1em");
	if (! cinit () || ! uinit () || ! jinit ())
		goto failed;

	/* Готовим счета. */
	for (eid=1; eid <= jlen; ++eid) {
		e = jread (eid);
		if (! e || ! e->eid || e->deleted)
			continue;
		db = afind (e->debit);
		cr = afind (e->credit);
		if (db)
			aappend (db, e);
		if (cr)
			aappend (cr, e);
	}
	for (c=acntab; c<acntab+naccounts; ++c)
		if (c->nent)
			SORT (c->entry, c->nent, acompare);

	return 1;
failed:
	if (chartfd >= 0)
		close (chartfd);
	if (journfd >= 0)
		close (journfd);
	chartfd = -1;
	journfd = -1;
	deas_fatal ("deas_init");
	return 0;
}

userdata *user_register (char *logname, char *password, long seed)
{
	userdata *c;

	/* Пароль не проверяем. */
	for (c=utab; c<utab+nusers; ++c)
		if (strncmp (c->info.logname, logname, LOGNSZ) == 0)
			return c;
	return 0;
}

char *user_N (clientinfo *c)
{
	return c->u->N;
}

char *user_E (clientinfo *c)
{
	return c->u->E;
}

long option_deleted (clientinfo *c, long on)
{
	long old = c->delflg;
	if (on >= 0)
		c->delflg = (on != 0);
	return old;
}

long option_date (clientinfo *c, long date)
{
	return DATE ();
}

/*--------------------------------------------------*/
menuinfo *menu_first (clientinfo *c)
{
	c->midx = 0;
	return menu_next (c);
}

static int awritable (long pacn, long gmask)
{
	account_t *t;

	for (t=acntab; t<acntab+naccounts; ++t)
		if (t->info.pacn == pacn && (t->info.wmask & gmask))
			return 1;
	return 0;
}

menuinfo *menu_next (clientinfo *c)
{
	menuinfo *m;
	accountinfo *cr, *db;

	for (; c->midx < nmenu; ++c->midx) {
		m = menutab + c->midx;
		if (! m->credit)
			break;

		/* Проверяем наличие корреспондирующих счетов. */
		if (! (cr = cfind (m->credit)) || ! (db = cfind (m->debit)))
			continue;

		/* Проверяем доступ. */
		if (MANAGER (&c->u->info))
			break;
		if (! (cr->wmask & c->u->info.gmask) &&
		    ! awritable (cr->pacn, c->u->info.gmask))
			continue;
		if (! (db->wmask & c->u->info.gmask) &&
		    ! awritable (db->pacn, c->u->info.gmask))
			continue;
		break;
	}
	if (c->midx >= nmenu)
		return 0;
	++c->midx;
	return m;
}

/*--------------------------------------------------*/
userinfo *user_first (clientinfo *c)
{
	c->uidx = 0;
	return user_next (c);
}

userinfo *user_next (clientinfo *c)
{
	if (! MANAGER (&c->u->info) || c->uidx >= nusers)
		return 0;
	return &utab[c->uidx++].info;
}

/*--------------------------------------------------*/
accountinfo *chart_first (clientinfo *c)
{
	c->cidx = 0;
	return chart_next (c);
}

accountinfo *chart_next (clientinfo *c)
{
	static accountinfo info;

	for (; c->cidx < naccounts; ++c->cidx) {
		/* Пропускаем удаленные счета. */
		if (! c->delflg && acntab[c->cidx].info.deleted)
			continue;

		info = acntab[c->cidx++].info;
		if (! MANAGER (&c->u->info)) {
			info.rmask = (c->u->info.gmask & info.rmask) != 0;
			info.wmask = (c->u->info.gmask & info.wmask) != 0;
		}
		return &info;
	}
	return 0;
}

accountinfo *chart_info (clientinfo *c, long acn)
{
	accountinfo *a = cfind (acn);
	static accountinfo info;

	/* Проверяем доступ. */
	if (! a || ! (c->u->info.gmask & a->rmask))
		return 0;
	if (! MANAGER (&c->u->info)) {
		info.rmask = (c->u->info.gmask & info.rmask) != 0;
		info.wmask = (c->u->info.gmask & info.wmask) != 0;
		a = &info;
	}
	return a;
}

void chart_edit (clientinfo *c, accountinfo *i)
{
	int changed = 0;
	accountinfo *a = cfind (i->acn);

	/* Пропускаем удаленные счета. */
	if (! a || (! c->delflg && a->deleted))
		return;

	/* Проверяем доступ. */
	if (! (c->u->info.gmask & a->wmask))
		return;

	/* Менеджер может править маски доступа. */
	if (! i->rmask)
		i->rmask = GROUP(0);
	if (! i->wmask)
		i->wmask = GROUP(0);
	if (MANAGER (&c->u->info)) {
		if (a->rmask != i->rmask) {
			a->rmask = i->rmask;
			changed = 1;
		}
		if (a->wmask != i->wmask) {
			a->wmask = i->wmask;
			changed = 1;
		}
	}

	if (strncmp (a->descr, i->descr, ANAMSZ) != 0) {
		strncpy (a->descr, i->descr, ANAMSZ-1);
		a->descr[sizeof(a->descr)-1] = 0;
		changed = 1;
	}

	if (changed)
		csave ();
}

void chart_add (clientinfo *c, accountinfo *i)
{
	account_t *a;
	accountinfo *p = 0;

	if (naccounts >= MAXACCOUNTS)
		return;

	if (i->pacn) {
		/* Проверяем доступ на запись к родительскому счету. */
		p = cfind (i->pacn);
		if (! p || p->deleted || ! (c->u->info.gmask & p->wmask))
			return;
	} else if (! MANAGER (&c->u->info))
		/* Только менеджер может создавать корневые счета. */
		return;

	/* Может, такой счет уже есть? */
	p = cfind (i->acn);
	if (p) {
		if (! p->deleted)
			return;
		/* Используем старый счет. */
		a = afind (i->acn);
		if (! a)
			return;
	} else
		/* Заводим новый счет. */
		a = acntab + naccounts++;

	memset (a, 0, sizeof (*a));
	a->info = *i;
	a->info.descr[sizeof(a->info.descr)-1] = 0;

	/* Корневые счета по умолчанию открыты только для менеджеров. */
	a->info.rmask = i->rmask | c->u->info.gmask | GROUP(0);
	a->info.wmask = i->wmask | c->u->info.gmask | GROUP(0);

	SORT (acntab, naccounts, ccompare);
	csave ();
}

void chart_delete (clientinfo *c, long acn)
{
	accountinfo *p;
	account_t *t = afind (acn);

	/* Удалять можно только пустые счета, не содержащие проводок. */
	if (! t || t->nent)
		return;

	if (! MANAGER (&c->u->info)) {
		/* Только менеджер может удалять корневые счета. */
		if (! t->info.pacn)
			return;

		/* Проверяем доступ на запись к родительскому счету. */
		p = cfind (t->info.pacn);
		if (! p || p->deleted || ! (c->u->info.gmask & p->wmask))
			return;
	}
	t->info.deleted = 1;
	csave ();
}

/*--------------------------------------------------*/
entryinfo *account_first (clientinfo *c, long acn)
{
	c->ap = afind (acn);
	c->aidx = 0;

	/* Проверяем доступ. */
	if (! c->ap || ! (c->u->info.gmask & c->ap->info.rmask)) {
		c->ap = 0;
		return 0;
	}
	return account_next (c);
}

entryinfo *account_next (clientinfo *c)
{
	entryinfo *e;

	if (! c->ap)
		return 0;
	while (c->aidx < c->ap->nent && (e = jread (c->ap->entry [c->aidx++]))) {
		/* Пропускаем удаленные записи. */
		if (! e->eid || (! c->delflg && e->deleted))
			continue;

		/* Проверяем доступ. */
		if (c->u->info.gmask & e->rmask)
			return e;
	}
	return 0;
}

balance *account_balance (clientinfo *c, long acn)
{
	account_t *t = afind (acn);

	/* Пропускаем удаленные счета. */
	if (! t || (! c->delflg && t->info.deleted))
		return 0;

	/* Проверяем доступ. */
	if (! (c->u->info.gmask & t->info.rmask))
		return 0;

	/* Пересчитываем баланс. */
	if (! t->tvalid)
		acompute (t);
	return &t->total;
}

balance *account_report (clientinfo *c, long acn)
{
	account_t *t = afind (acn);

	/* Пропускаем удаленные счета. */
	if (! t || (! c->delflg && t->info.deleted))
		return 0;

	/* Проверяем доступ. */
	if (! (c->u->info.gmask & t->info.rmask))
		return 0;

	/* Пересчитываем баланс. */
	if (! t->tvalid)
		acompute (t);
	return t->monthly;
}

/*--------------------------------------------------*/
entryinfo *journal_first (clientinfo *c)
{
	c->jidx = 0;
	return journal_next (c);
}

entryinfo *journal_next (clientinfo *c)
{
	entryinfo *e;

	while (c->jidx < jlen && (e = jread (jindex [c->jidx++]))) {
		/* Пропускаем удаленные записи. */
		if (! e->eid || (! c->delflg && e->deleted))
			continue;

		/* Проверяем доступ. */
		if (c->u->info.gmask & e->rmask)
			return e;
	}
	return 0;
}

entryinfo *journal_info (clientinfo *c, long eid)
{
	entryinfo *e = jread (eid);

	/* Проверяем доступ. */
	if (! e || ! e->eid || ! (c->u->info.gmask & e->rmask))
		return 0;
	return e;
}

void journal_add (clientinfo *c, entryinfo *i)
{
	account_t *db = afind (i->debit);
	account_t *cr = afind (i->credit);

	if (! db || ! (c->u->info.gmask & db->info.wmask) ||
	    ! cr || ! (c->u->info.gmask & cr->info.wmask))
		return;

	if (i->eid && i->eid != jlen+1)
		return;

	/* Заводим новую проводку. */
	if (jlen >= jsz) {
		jsz += 64;
		journal = (entryinfo*) realloc (journal,
			jsz * sizeof (entryinfo));
		jindex = (long*) realloc (jindex, jsz * sizeof (long));
		if (! journal || ! jindex)
			deas_fatal ("no memory for journal");
	}
	jindex[jlen] = jlen+1;
	i->eid = ++jlen;

	if (! i->date)
		i->date = DATE ();
	i->amount.rec = 1;
	i->rmask = MASK_UNION (db->info.rmask, cr->info.rmask);

	/* Только менеджер может устанавливать владельца проводки. */
	if (! MANAGER (&c->u->info) || ! i->user[0])
		strncpy (i->user, c->u->info.user, UNAMSZ-1);

	i->user[sizeof(i->user)-1] = 0;
	i->descr[sizeof(i->descr)-1] = 0;

	jsave (i);
	SORT (jindex, jlen, acompare);
	asave (db, i);
	asave (cr, i);
}

void journal_edit (clientinfo *c, entryinfo *i)
{
	int changed = 0;
	entryinfo *e = jread (i->eid);
	account_t *db, *cr, *olddb, *oldcr;

	if (! e || ! e->eid)
		return;
	i->deleted = 0;

	/* Открываем новые счета. */
	db = afind (i->debit);
	cr = afind (i->credit);
	if (! db || ! (c->u->info.gmask & db->info.wmask) ||
	    ! cr || ! (c->u->info.gmask & cr->info.wmask))
		return;

	if (e->debit != i->debit) {
		if (! e->deleted) {
			/* Вычеркиваем из старого счета. */
			olddb = afind (e->debit);
			if (olddb)
				adelete (olddb, e);
		}
		e->debit = i->debit;
		changed = 1;
	}

	if (e->credit != i->credit) {
		if (! e->deleted) {
			/* Вычеркиваем из старого счета. */
			oldcr = afind (e->credit);
			if (oldcr)
				adelete (oldcr, e);
		}
		e->credit = i->credit;
		changed = 1;
	}

	/* Только менеджер может изменять владельца проводки. */
	if (MANAGER (&c->u->info) && strncmp (e->user, i->user, UNAMSZ) != 0) {
		strncpy (e->user, i->user, UNAMSZ-1);
		changed = 1;
	}

	if (strncmp (e->descr, i->descr, ANAMSZ) != 0) {
		strncpy (e->descr, i->descr, ANAMSZ-1);
		changed = 1;
	}
	if (e->date != i->date) {
		e->date = i->date;
		changed = 1;
	}
	if (e->amount.rub != i->amount.rub ||
	    e->amount.doll != i->amount.doll ||
	    e->amount.pcs != i->amount.pcs) {
		e->amount.rub = i->amount.rub;
		e->amount.doll = i->amount.doll;
		e->amount.pcs = i->amount.pcs;
		changed = 1;
	}
	if (e->deleted) {
		e->deleted = 0;
		changed = 1;
	}

	if (! changed)
		return;
	e->rmask = MASK_UNION (cr->info.rmask, db->info.rmask);
	e->user[sizeof(e->user)-1] = 0;
	e->descr[sizeof(e->descr)-1] = 0;
	aupdate (db, e);
	aupdate (cr, e);
	jsave (e);
	SORT (jindex, jlen, acompare);
}

void journal_delete (clientinfo *c, long eid)
{
	entryinfo *e = jread (eid);
	account_t *db, *cr;

	if (! e || ! e->eid || e->deleted)
		return;
	db = afind (e->debit);
	cr = afind (e->credit);

	/* Проверяем доступ. */
	if ((db && ! (c->u->info.gmask & db->info.wmask)) ||
	    (cr && ! (c->u->info.gmask & cr->info.wmask)))
		return;

	/* Записываем удаленную проводку. */
	e->deleted = 1;
	if (db)
		adelete (db, e);
	if (cr)
		adelete (cr, e);
	jsave (e);
}
