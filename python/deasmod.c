#include "Python.h"
#include "deas.h"
#include <stdlib.h>
#include <unistd.h>

static clientinfo clnt;

static void fatal (char *msg)
{
	fprintf (stderr, "Ошибка базы: %s\n", msg);
	exit (1);
}

static PyObject *deas_reguser (PyObject *self, PyObject *args)
{
	char *host = 0, *user = 0, *password = 0;
	long port = 0;

	if (! PyArg_ParseTuple (args, "|sssl:deas.register",
	    &user, &password, &host, &port))
		return 0;
	if (! host || ! *host) {
		host = getenv ("DEASHOST");
		if (! host)
			host = "127.0.0.1";
	}
	if (! deas_init (host, port, fatal))
		fatal ("нет связи с сервером");
	if (! user || ! *user) {
		user = getenv ("USER");
		if (! user)
			user = getenv ("LOGNAME");
	}
	if (! password || ! *password) {
		password = getenv ("DEASPASS");
		if (! password)
			password = getpass ("Password: ");
	}
	clnt.u = user_register (user, password, (long) self);
	if (! clnt.u)
		fatal ("неверный пароль");
	Py_INCREF (Py_None);
	return Py_None;
}

static PyObject *deas_delflag (PyObject *self, PyObject *args)
{
	long dflg;

	if (! clnt.u || ! PyArg_ParseTuple (args, "l:deas.delflag", &dflg))
		return 0;
	dflg = option_deleted (&clnt, dflg);
	return Py_BuildValue ("l", dflg);
}

static PyObject *deas_date (PyObject *self, PyObject *args)
{
	long d;

	if (! clnt.u || ! PyArg_ParseTuple (args, "l:deas.date", &d))
		return 0;
	d = option_date (&clnt, d);
	return Py_BuildValue ("l", d);
}

static PyObject *deas_chart_first (PyObject *self, PyObject *args)
{
	accountinfo *a;

	if (! clnt.u || ! PyArg_ParseTuple (args, ":deas.chart_first"))
		return 0;
	a = chart_first (&clnt);
	if (a)
		return Py_BuildValue ("(llllslll)", a->acn, a->pacn, a->passive, a->anal,
			a->descr, a->deleted, a->rmask, a->wmask);
	Py_INCREF (Py_None);
	return Py_None;
}

static PyObject *deas_chart_next (PyObject *self, PyObject *args)
{
	accountinfo *a;

	if (! clnt.u || ! PyArg_ParseTuple (args, ":deas.chart_next"))
		return 0;
	a = chart_next (&clnt);
	if (a)
		return Py_BuildValue ("(llllslll)", a->acn, a->pacn, a->passive, a->anal,
			a->descr, a->deleted, a->rmask, a->wmask);
	Py_INCREF (Py_None);
	return Py_None;
}

static PyObject *deas_chart_info (PyObject *self, PyObject *args)
{
	long acn;
	accountinfo *a;

	if (! clnt.u || ! PyArg_ParseTuple (args, "l:deas.chart_info", &acn))
		return 0;
	a = chart_info (&clnt, acn);
	if (a)
		return Py_BuildValue ("(llllslll)", a->acn, a->pacn, a->passive, a->anal,
			a->descr, a->deleted, a->rmask, a->wmask);
	Py_INCREF (Py_None);
	return Py_None;
}

static PyObject *deas_chart_add (PyObject *self, PyObject *args)
{
	accountinfo info, *a = &info;
	char *dp;

	if (! clnt.u || ! PyArg_ParseTuple (args, "llllslll:deas.chart_add",
	    &a->acn, &a->pacn, &a->passive, &a->anal, &dp,
	    &a->deleted, &a->rmask, &a->wmask))
		return 0;
	strncpy (a->descr, dp, sizeof (a->descr) - 1);
	chart_add (&clnt, a);
	Py_INCREF (Py_None);
	return Py_None;
}

static PyObject *deas_chart_edit (PyObject *self, PyObject *args)
{
	accountinfo info, *a = &info;
	char *dp;

	if (! clnt.u || ! PyArg_ParseTuple (args, "llllslll:deas.chart_edit",
	    &a->acn, &a->pacn, &a->passive, &a->anal, &dp,
	    a->descr, &a->deleted, &a->rmask, &a->wmask))
		return 0;
	strncpy (a->descr, dp, sizeof (a->descr) - 1);
	chart_edit (&clnt, a);
	Py_INCREF (Py_None);
	return Py_None;
}

static PyObject *deas_chart_delete (PyObject *self, PyObject *args)
{
	long acn;

	if (! clnt.u || ! PyArg_ParseTuple (args, "l:deas.chart_delete", &acn))
		return 0;
	chart_delete (&clnt, acn);
	Py_INCREF (Py_None);
	return Py_None;
}

static PyObject *deas_account_first (PyObject *self, PyObject *args)
{
	long acn;
	entryinfo *e;

	if (! clnt.u || ! PyArg_ParseTuple (args, "l:deas.account_first", &acn))
		return 0;
	e = account_first (&clnt, acn);
	if (e)
		return Py_BuildValue ("(llll(llll)slls)", e->eid, e->date,
			e->debit, e->credit, e->amount.rub, e->amount.doll,
			e->amount.pcs, e->amount.rec, e->descr, e->deleted,
			e->rmask, e->user);
	Py_INCREF (Py_None);
	return Py_None;
}

static PyObject *deas_account_next (PyObject *self, PyObject *args)
{
	entryinfo *e;

	if (! clnt.u || ! PyArg_ParseTuple (args, ":deas.account_next"))
		return 0;
	e = account_next (&clnt);
	if (e)
		return Py_BuildValue ("(llll(llll)slls)", e->eid, e->date,
			e->debit, e->credit, e->amount.rub, e->amount.doll,
			e->amount.pcs, e->amount.rec, e->descr, e->deleted,
			e->rmask, e->user);
	Py_INCREF (Py_None);
	return Py_None;
}

static PyObject *deas_account_balance (PyObject *self, PyObject *args)
{
	long acn;
	balance *b;

	if (! clnt.u || ! PyArg_ParseTuple (args, "l:deas.account_balance", &acn))
		return 0;
	b = account_balance (&clnt, acn);
	if (! b)
		return 0;
	return Py_BuildValue ("((llll)(llll))",
		b->debit.rub, b->debit.doll, b->debit.pcs, b->debit.rec,
		b->credit.rub, b->credit.doll, b->credit.pcs, b->credit.rec);
}

static PyObject *deas_account_report (PyObject *self, PyObject *args)
{
	long acn;
	balance *b;

	if (! clnt.u || ! PyArg_ParseTuple (args, "l:deas.account_balance", &acn))
		return 0;
	b = account_report (&clnt, acn);
	if (! b)
		return 0;
	return Py_BuildValue ("[(llll)(llll),(llll)(llll),(llll)(llll),"
		"(llll)(llll),(llll)(llll),(llll)(llll),(llll)(llll),"
		"(llll)(llll),(llll)(llll),(llll)(llll),(llll)(llll),"
		"(llll)(llll)]",
		b[0].debit.rub, b[0].debit.doll, b[0].debit.pcs, b[0].debit.rec,
		b[0].credit.rub, b[0].credit.doll, b[0].credit.pcs, b[0].credit.rec,
		b[1].debit.rub, b[1].debit.doll, b[1].debit.pcs, b[1].debit.rec,
		b[1].credit.rub, b[1].credit.doll, b[1].credit.pcs, b[1].credit.rec,
		b[2].debit.rub, b[2].debit.doll, b[2].debit.pcs, b[2].debit.rec,
		b[2].credit.rub, b[2].credit.doll, b[2].credit.pcs, b[2].credit.rec,
		b[3].debit.rub, b[3].debit.doll, b[3].debit.pcs, b[3].debit.rec,
		b[3].credit.rub, b[3].credit.doll, b[3].credit.pcs, b[3].credit.rec,
		b[4].debit.rub, b[4].debit.doll, b[4].debit.pcs, b[4].debit.rec,
		b[4].credit.rub, b[4].credit.doll, b[4].credit.pcs, b[4].credit.rec,
		b[5].debit.rub, b[5].debit.doll, b[5].debit.pcs, b[5].debit.rec,
		b[5].credit.rub, b[5].credit.doll, b[5].credit.pcs, b[5].credit.rec,
		b[6].debit.rub, b[6].debit.doll, b[6].debit.pcs, b[6].debit.rec,
		b[6].credit.rub, b[6].credit.doll, b[6].credit.pcs, b[6].credit.rec,
		b[7].debit.rub, b[7].debit.doll, b[7].debit.pcs, b[7].debit.rec,
		b[7].credit.rub, b[7].credit.doll, b[7].credit.pcs, b[7].credit.rec,
		b[8].debit.rub, b[8].debit.doll, b[8].debit.pcs, b[8].debit.rec,
		b[8].credit.rub, b[8].credit.doll, b[8].credit.pcs, b[8].credit.rec,
		b[9].debit.rub, b[9].debit.doll, b[9].debit.pcs, b[9].debit.rec,
		b[9].credit.rub, b[9].credit.doll, b[9].credit.pcs, b[9].credit.rec,
		b[10].debit.rub, b[10].debit.doll, b[10].debit.pcs, b[10].debit.rec,
		b[10].credit.rub, b[10].credit.doll, b[10].credit.pcs, b[10].credit.rec,
		b[11].debit.rub, b[11].debit.doll, b[11].debit.pcs, b[11].debit.rec,
		b[11].credit.rub, b[11].credit.doll, b[11].credit.pcs, b[11].credit.rec);
}

static PyObject *deas_journal_first (PyObject *self, PyObject *args)
{
	entryinfo *e;

	if (! clnt.u || ! PyArg_ParseTuple (args, ":deas.journal_first"))
		return 0;
	e = journal_first (&clnt);
	if (e)
		return Py_BuildValue ("(llll(llll)slls)", e->eid, e->date,
			e->debit, e->credit, e->amount.rub, e->amount.doll,
			e->amount.pcs, e->amount.rec, e->descr, e->deleted,
			e->rmask, e->user);
	Py_INCREF (Py_None);
	return Py_None;
}

static PyObject *deas_journal_next (PyObject *self, PyObject *args)
{
	entryinfo *e;

	if (! clnt.u || ! PyArg_ParseTuple (args, ":deas.journal_next"))
		return 0;
	e = journal_next (&clnt);
	if (e)
		return Py_BuildValue ("(llll(llll)slls)", e->eid, e->date,
			e->debit, e->credit, e->amount.rub, e->amount.doll,
			e->amount.pcs, e->amount.rec, e->descr, e->deleted,
			e->rmask, e->user);
	Py_INCREF (Py_None);
	return Py_None;
}

static PyObject *deas_journal_info (PyObject *self, PyObject *args)
{
	long eid;
	entryinfo *e;

	if (! clnt.u || ! PyArg_ParseTuple (args, "l:deas.journal_info", &eid))
		return 0;
	e = journal_info (&clnt, eid);
	if (e)
		return Py_BuildValue ("(llll(llll)slls)", e->eid, e->date,
			e->debit, e->credit, e->amount.rub, e->amount.doll,
			e->amount.pcs, e->amount.rec, e->descr, e->deleted,
			e->rmask, e->user);
	Py_INCREF (Py_None);
	return Py_None;
}

static PyObject *deas_journal_delete (PyObject *self, PyObject *args)
{
	long eid;

	if (! clnt.u || ! PyArg_ParseTuple (args, "l:deas.journal_delete", &eid))
		return 0;
	journal_delete (&clnt, eid);
	Py_INCREF (Py_None);
	return Py_None;
}

static PyObject *deas_journal_add (PyObject *self, PyObject *args)
{
	entryinfo info, *e = &info;
	char *dp, *up;

	if (! clnt.u || ! PyArg_ParseTuple (args, "llll(llll)slls:deas.journal_add",
	    &e->eid, &e->date, &e->debit, &e->credit,
	    &e->amount.rub, &e->amount.doll, &e->amount.pcs, &e->amount.rec,
	    &dp, &e->deleted, &e->rmask, &up))
		return 0;
	strncpy (e->descr, dp, sizeof (e->descr) - 1);
	strncpy (e->user, up, sizeof (e->user) - 1);
	journal_add (&clnt, e);
	Py_INCREF (Py_None);
	return Py_None;
}

static PyObject *deas_journal_edit (PyObject *self, PyObject *args)
{
	entryinfo info, *e = &info;
	char *dp, *up;

	if (! clnt.u || ! PyArg_ParseTuple (args, "llll(llll)slls:deas.journal_edit",
	    &e->eid, &e->date, &e->debit, &e->credit,
	    &e->amount.rub, &e->amount.doll, &e->amount.pcs, &e->amount.rec,
	    &dp, &e->deleted, &e->rmask, &up))
		return 0;
	strncpy (e->descr, dp, sizeof (e->descr) - 1);
	strncpy (e->user, up, sizeof (e->user) - 1);
	journal_edit (&clnt, e);
	Py_INCREF (Py_None);
	return Py_None;
}

static PyObject *deas_menu_first (PyObject *self, PyObject *args)
{
	menuinfo *m;

	if (! clnt.u || ! PyArg_ParseTuple (args, ":deas.menu_first"))
		return 0;
	m = menu_first (&clnt);
	if (m)
		return Py_BuildValue ("(lllss)",
			m->debit, m->credit, m->currency, m->line, m->descr);
	Py_INCREF (Py_None);
	return Py_None;
}

static PyObject *deas_menu_next (PyObject *self, PyObject *args)
{
	menuinfo *m;

	if (! clnt.u || ! PyArg_ParseTuple (args, ":deas.menu_next"))
		return 0;
	m = menu_next (&clnt);
	if (m)
		return Py_BuildValue ("(lllss)",
			m->debit, m->credit, m->currency, m->line, m->descr);
	Py_INCREF (Py_None);
	return Py_None;
}

static PyObject *deas_user_first (PyObject *self, PyObject *args)
{
	userinfo *u;

	if (! clnt.u || ! PyArg_ParseTuple (args, ":deas.user_first"))
		return 0;
	u = user_first (&clnt);
	if (u)
		return Py_BuildValue ("(sssl)",
			u->logname, u->user, u->descr, u->gmask);
	Py_INCREF (Py_None);
	return Py_None;
}

static PyObject *deas_user_next (PyObject *self, PyObject *args)
{
	userinfo *u;

	if (! clnt.u || ! PyArg_ParseTuple (args, ":deas.user_next"))
		return 0;
	u = user_next (&clnt);
	if (u)
		return Py_BuildValue ("(sssl)",
			u->logname, u->user, u->descr, u->gmask);
	Py_INCREF (Py_None);
	return Py_None;
}

static PyObject *deas_ymd_to_date (PyObject *self, PyObject *args)
{
	int y, m, d;

	if (! clnt.u || ! PyArg_ParseTuple (args, "iii:deas.ymd_to_date",
	    &y, &m, &d))
		return 0;
	return Py_BuildValue ("l", ymd2date (y, m, d));
}

static PyObject *deas_date_to_ymd (PyObject *self, PyObject *args)
{
	long date;
	int y, m, d;

	if (! clnt.u || ! PyArg_ParseTuple (args, "l:deas.date_to_ymd", &date))
		return 0;
	date2ymd (date, &y, &m, &d);
	return Py_BuildValue ("(iii)", y, m, d);
}

/* List of functions defined in the module */

static PyMethodDef deas_methods[] = {
	{ "register",           deas_reguser,           1 },
	{ "delflag",            deas_delflag,           1 },
	{ "date",               deas_date,              1 },
	{ "chart_first",        deas_chart_first,       1 },
	{ "chart_next",         deas_chart_next,        1 },
	{ "chart_info",         deas_chart_info,        1 },
	{ "chart_add",          deas_chart_add,         1 },
	{ "chart_edit",         deas_chart_edit,        1 },
	{ "chart_delete",       deas_chart_delete,      1 },
	{ "account_first",      deas_account_first,     1 },
	{ "account_next",       deas_account_next,      1 },
	{ "account_balance",    deas_account_balance,   1 },
	{ "account_report",     deas_account_report,    1 },
	{ "journal_first",      deas_journal_first,     1 },
	{ "journal_next",       deas_journal_next,      1 },
	{ "journal_info",       deas_journal_info,      1 },
	{ "journal_add",        deas_journal_add,       1 },
	{ "journal_edit",       deas_journal_edit,      1 },
	{ "journal_delete",     deas_journal_delete,    1 },
	{ "user_first",         deas_user_first,        1 },
	{ "user_next",          deas_user_next,         1 },
	{ "menu_first",         deas_menu_first,        1 },
	{ "menu_next",          deas_menu_next,         1 },
	{ "ymd_to_date",        deas_ymd_to_date,       1 },
	{ "date_to_ymd",        deas_date_to_ymd,       1 },
{0}};

void initdeas ()
{
	PyObject *m, *d;

	/* Create the module and add the functions */
	m = Py_InitModule ("deas", deas_methods);

	/* Add some symbolic constants to the module */
	d = PyModule_GetDict (m);

	/* Check for errors */
	if (PyErr_Occurred ())
		Py_FatalError ("can't initialize module deas");
}

