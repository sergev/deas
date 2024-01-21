#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>
#include <netdb.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>
#include "deas.h"
#include "pgplib.h"

#define SYSNAME "deas"

#define RPC_SUCCESS             0
#define RPC_CANTENCODEARGS      -1
#define RPC_CANTDECODEARGS      -2
#define RPC_CANTSEND            -3
#define RPC_CANTRECV            -4

#define clear(x) memset (&x, 0, sizeof(x))

typedef int SOCKET;

#define NULLSOCK -1
#define sock_close close
#define TODOS(x)
#define FROMDOS(x)

struct deas_req req;
struct deas_reply reply;

static char key [16];

static SOCKET sock = NULLSOCK;
static void (*deas_fatal) (char*);
static short counter;

int clnt_call (SOCKET s, int op, int len, void *arg, int lenfrom, void *rep)
{
	char data [8192];

	if (len > sizeof (req.data))
		return RPC_CANTENCODEARGS;
	req.counter = ++counter;
	req.op = op;
	req.len = len;
	if (len)
		memcpy (req.data, arg, len);

	len = CALIGN (len + sizeof (req) - sizeof (req.data));
	block_encrypt (&req, data, len, key);

	send_packet (s, data, len);

	len = receive_packet (s, data, sizeof (data));
	if (len < 0 || len % CBLK)
		return RPC_CANTRECV;

	block_decrypt (data, &reply, len, key);
	if (reply.len != lenfrom)
		return RPC_CANTDECODEARGS;

	if (reply.len)
		memcpy (rep, reply.data, reply.len);

	return 0;
}

void clnt_close ()
{
	if (sock != NULLSOCK)
		sock_close (sock);
	sock = NULLSOCK;
}

int clnt_connect (char *host, int port)
{
	int val = 2;
	struct sockaddr_in server;
	struct hostent *hp;
	struct hostent hbuf;
	struct in_addr hbufaddr;

	if (! host)
		host = "127.0.0.1";
	if (*host>='0' && *host<='9' &&
	    (hbufaddr.s_addr = inet_addr (host)) != INADDR_NONE) {
		/* raw ip address */
		hp = &hbuf;
#ifdef h_addr
		{
		static char *alist [1];
		hp->h_addr_list = alist;
		}
#endif
		hp->h_name = host;
		hp->h_addrtype = AF_INET;
		hp->h_addr = (void*) &hbufaddr;
		hp->h_length = sizeof (struct in_addr);
		hp->h_aliases = 0;
	} else {
		hp = gethostbyname (host);
		if (! hp)
			return NULLSOCK;
	}
	if (! port)
		port = DEASPORT;

	server.sin_family = hp->h_addrtype;
	memcpy (&server.sin_addr, hp->h_addr, hp->h_length);
	server.sin_port = htons (port);

	sock = socket (AF_INET, SOCK_STREAM, 0);
	if (sock == NULLSOCK)
		return NULLSOCK;

	if (connect (sock, (struct sockaddr*) &server, sizeof (server)) < 0) {
		sock_close (sock);
		return NULLSOCK;
	}
	setsockopt (sock, SOL_SOCKET, SO_RCVLOWAT, &val, sizeof (val));
	setsockopt (sock, IPPROTO_TCP, TCP_NODELAY, &val, sizeof (val));
	atexit (clnt_close);
	return sock;
}

static void badcall (char *prog)
{
	sock_close (sock);
	sock = NULLSOCK;
	(*deas_fatal) (prog);
}

int deas_init (char *host, int port, void (*fatal)(char*))
{
	static char errbuf[80];

	deas_fatal = fatal;
	sock = clnt_connect (host, port);
	if (sock != NULLSOCK)
		return 1;
	snprintf (errbuf, sizeof(errbuf), "нет связи с сервером %s", host ? host : "");
	deas_fatal (errbuf);
	return 0;
}

/*
 * 1. Создаем одноразовый пароль, 16 байт.  Используем:
 *    - параметр seed
 *    - текущее время
 *    - номер процесса
 *    - пароль пользователя
 * 2. Шифруем его секретным ключом пользователя,
 *    используя параметр password. Получаем 128 байт.
 * 3. Шифруем полученный массив открытым ключом системы.
 *    Получаем 256 байт.
 */
userdata *user_register (char *logname, char *password, long seed)
{
	char data [256];
	authinfo auth;
	long result;
	int len, i;

	static char filename[128], *p;
	p = getenv ("HOME");
	if (! p)
		p = "/";
	strcpy (filename, p);
	strcat (filename, "/.deasdat");
	crypt_pubfile = crypt_secfile = filename;

	/* Формируем одноразовый пароль. */
	*(long*) key = seed;
	*(long*) (key+4) = time(NULL);
	*(short*) (key+8) = getpid ();
	memcpy (key+10, password, 6);
	for (i=0; i<16; ++i)
		key[i] ^= 0x55 ^ i;

	/* Шифруем пароль секретным ключом пользователя. */
	memset (&auth, 0, sizeof (auth));
	strncpy (auth.logname, logname, sizeof (auth.logname) - 1);
	len = private_encrypt (key, auth.data, 16, logname, password);
	if (len != sizeof (auth.data))
		return 0;

	/* Шифруем открытым ключом системы. */
	len = public_encrypt (&auth, data, sizeof (auth), SYSNAME);
	if (len <= 0)
		return 0;

	send_packet (sock, (char*) &data, len);
	if (receive_packet (sock, (char*) &result, sizeof (result)) != sizeof (result))
		badcall ("user_register");
	if (! result)
		return 0;

	counter = time(NULL);
	return (userdata*) sock;
}

long option_date (clientinfo *c, long date)
{
	static long res;

	clear (res);
	if (clnt_call (sock, OPTION_DATE, sizeof (long), &date, sizeof (long), &res) != 0)
		badcall ("option_date");
	return res;
}

long option_deleted (clientinfo *c, long on)
{
	static long res;

	clear (res);
	if (clnt_call (sock, OPTION_DELETED, sizeof (long), &on, sizeof (long), &res) != 0)
		badcall ("option_deleted");
	return res;
}

accountinfo *chart_first (clientinfo *c)
{
	static account_reply res;

	clear (res);
	if (clnt_call (sock, CHART_FIRST, 0, 0, sizeof (account_reply), &res) != 0)
		badcall ("chart_first");
	if (res.error) return 0;
	TODOS (res.info.descr);
	return &res.info;
}

accountinfo *chart_next (clientinfo *c)
{
	static account_reply res;

	clear (res);
	if (clnt_call (sock, CHART_NEXT, 0, 0, sizeof (account_reply), &res) != 0)
		badcall ("chart_next");
	if (res.error) return 0;
	TODOS (res.info.descr);
	return &res.info;
}

accountinfo *chart_info (clientinfo *c, long acn)
{
	static account_reply res;

	clear (res);
	if (clnt_call (sock, CHART_INFO, sizeof (long), &acn, sizeof (account_reply), &res) != 0)
		badcall ("chart_info");
	if (res.error) return 0;
	TODOS (res.info.descr);
	return &res.info;
}

entryinfo *account_first (clientinfo *c, long acn)
{
	static entry_reply res;

	clear (res);
	if (clnt_call (sock, ACCOUNT_FIRST, sizeof (long), &acn, sizeof (entry_reply), &res) != 0)
		badcall ("account_first");
	if (res.error) return 0;
	TODOS (res.info.descr);
	TODOS (res.info.user);
	return &res.info;
}

entryinfo *account_next (clientinfo *c)
{
	static entry_reply res;

	clear (res);
	if (clnt_call (sock, ACCOUNT_NEXT, 0, 0, sizeof (entry_reply), &res) != 0)
		badcall ("account_next");
	if (res.error) return 0;
	TODOS (res.info.descr);
	TODOS (res.info.user);
	return &res.info;
}

balance *account_balance (clientinfo *c, long acn)
{
	static balance_reply res;

	clear (res);
	if (clnt_call (sock, ACCOUNT_BALANCE, sizeof (long), &acn, sizeof (balance_reply), &res) != 0)
		badcall ("account_balance");
	return res.error ? 0 : &res.info;
}

balance *account_report (clientinfo *c, long acn)
{
	static report_reply res;

	clear (res);
	if (clnt_call (sock, ACCOUNT_REPORT, sizeof (long), &acn, sizeof (report_reply), &res) != 0)
		badcall ("account_balance");
	return res.error ? 0 : res.info;
}

entryinfo *journal_first (clientinfo *c)
{
	static entry_reply res;

	clear (res);
	if (clnt_call (sock, JOURNAL_FIRST, 0, 0, sizeof (entry_reply), &res) != 0)
		badcall ("journal_first");
	if (res.error) return 0;
	TODOS (res.info.descr);
	TODOS (res.info.user);
	return &res.info;
}

entryinfo *journal_next (clientinfo *c)
{
	static entry_reply res;

	clear (res);
	if (clnt_call (sock, JOURNAL_NEXT, 0, 0, sizeof (entry_reply), &res) != 0)
		badcall ("journal_next");
	if (res.error) return 0;
	TODOS (res.info.descr);
	TODOS (res.info.user);
	return &res.info;
}

entryinfo *journal_info (clientinfo *c, long eid)
{
	static entry_reply res;

	clear (res);
	if (clnt_call (sock, JOURNAL_INFO, sizeof (long), &eid, sizeof (entry_reply), &res) != 0)
		badcall ("journal_info");
	if (res.error) return 0;
	TODOS (res.info.descr);
	TODOS (res.info.user);
	return &res.info;
}

menuinfo *menu_first (clientinfo *c)
{
	static menu_reply res;

	clear (res);
	if (clnt_call (sock, MENU_FIRST, 0, 0, sizeof (menu_reply), &res) != 0)
		badcall ("menu_first");
	if (res.error) return 0;
	TODOS (res.info.descr);
	TODOS (res.info.line);
	return &res.info;
}

menuinfo *menu_next (clientinfo *c)
{
	static menu_reply res;

	clear (res);
	if (clnt_call (sock, MENU_NEXT, 0, 0, sizeof (menu_reply), &res) != 0)
		badcall ("menu_next");
	if (res.error) return 0;
	TODOS (res.info.descr);
	TODOS (res.info.line);
	return &res.info;
}

userinfo *user_first (clientinfo *c)
{
	static user_reply res;

	clear (res);
	if (clnt_call (sock, USER_FIRST, 0, 0, sizeof (user_reply), &res) != 0)
		badcall ("user_first");
	if (res.error) return 0;
	TODOS (res.info.descr);
	TODOS (res.info.user);
	TODOS (res.info.logname);
	return &res.info;
}

userinfo *user_next (clientinfo *c)
{
	static user_reply res;

	clear (res);
	if (clnt_call (sock, USER_NEXT, 0, 0, sizeof (user_reply), &res) != 0)
		badcall ("user_next");
	if (res.error) return 0;
	TODOS (res.info.descr);
	TODOS (res.info.user);
	TODOS (res.info.logname);
	return &res.info;
}

void chart_edit (clientinfo *c, accountinfo *i)
{
	FROMDOS (i->descr);
	if (clnt_call (sock, CHART_EDIT, sizeof (accountinfo), i, 0, 0) != 0)
		badcall ("chart_edit");
}

void chart_add (clientinfo *c, accountinfo *i)
{
	FROMDOS (i->descr);
	if (clnt_call (sock, CHART_ADD, sizeof (accountinfo), i, 0, 0) != 0)
		badcall ("chart_add");
}

void chart_delete (clientinfo *c, long acn)
{
	if (clnt_call (sock, CHART_DELETE, sizeof (long), &acn, 0, 0) != 0)
		badcall ("chart_delete");
}

void journal_add (clientinfo *c, entryinfo *i)
{
	FROMDOS (i->descr);
	FROMDOS (i->user);
	if (clnt_call (sock, JOURNAL_ADD, sizeof (entryinfo), i, 0, 0) != 0)
		badcall ("journal_add");
}

void journal_edit (clientinfo *c, entryinfo *i)
{
	FROMDOS (i->descr);
	FROMDOS (i->user);
	if (clnt_call (sock, JOURNAL_EDIT, sizeof (entryinfo), i, 0, 0) != 0)
		badcall ("journal_edit");
}

void journal_delete (clientinfo *c, long eid)
{
	if (clnt_call (sock, JOURNAL_DELETE, sizeof (long), &eid, 0, 0) != 0)
		badcall ("journal_delete");
}
