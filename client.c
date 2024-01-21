#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#ifdef unix
#   include <unistd.h>
#   include <netdb.h>
#   include <sys/socket.h>
#   include <netinet/in.h>
#   include <netinet/tcp.h>
#   include <arpa/inet.h>
#else
#   include "tcp.h"
#endif
#include "deas.h"
#include "pgplib.h"

#define SYSNAME "deas"

#define RPC_SUCCESS             0
#define RPC_CANTENCODEARGS      -1
#define RPC_CANTDECODEARGS      -2
#define RPC_CANTSEND            -3
#define RPC_CANTRECV            -4

#define clear(x) memset (&x, 0, sizeof(x))

#ifdef unix
typedef int SOCKET;
#   define NULLSOCK -1
#   define sock_close close
#   define TODOS(x)
#   define FROMDOS(x)
#else
typedef void *SOCKET;
#   define NULLSOCK 0
#   define TODOS(x)	recode (x, sizeof(x), todos)
#   define FROMDOS(x)	recode (x, sizeof(x), fromdos)
#endif

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

#ifdef unix
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
#else
SOCKET clnt_connect (char *host, int port)
{
	static tcp_Socket sockbuf;
	int status;
	long addr;

	addr = resolve (host);
	if (! port)
		port = DEASPORT;
	if (! addr || ! tcp_open (&sockbuf, 0, addr, port, 0))
		return NULLSOCK;
	sock_wait_established (&sockbuf, sock_delay, 0, &status);
	sock_mode (&sockbuf, TCP_MODE_NONAGLE);
	return &sockbuf;
sock_err:
	return NULLSOCK;
}
#endif

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
	sprintf (errbuf, "нет связи с сервером %s", host ? host : "");
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

#ifdef unix
	{
	static char filename[128], *p;
	p = getenv ("HOME");
	if (! p)
		p = "/";
	strcpy (filename, p);
	strcat (filename, "/.deasdat");
	crypt_pubfile = crypt_secfile = filename;
	}
#else
	crypt_pubfile = crypt_secfile = "deas.dat";
#endif
	/* Формируем одноразовый пароль. */
	*(long*) key = seed;
	*(long*) (key+4) = time (0);
#ifdef unix
	*(short*) (key+8) = getpid ();
#else
	*(short*) (key+8) = rand ();
#endif
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

	counter = time ((time_t) 0);
	return (userdata*) sock;
}

#ifndef unix
static char fromdos[] = {
	0xe1, 0xe2, 0xf7, 0xe7, 0xe4, 0xe5, 0xf6, 0xfa,
	0xe9, 0xea, 0xeb, 0xec, 0xed, 0xee, 0xef, 0xf0,
	0xf2, 0xf3, 0xf4, 0xf5, 0xe6, 0xe8, 0xe3, 0xfe,
	0xfb, 0xfd, 0xff, 0xf9, 0xf8, 0xfc, 0xe0, 0xf1,
	0xc1, 0xc2, 0xd7, 0xc7, 0xc4, 0xc5, 0xd6, 0xda,
	0xc9, 0xca, 0xcb, 0xcc, 0xcd, 0xce, 0xcf, 0xd0,
	0x90, 0x91, 0x92, 0x81, 0x87, 0xb2, 0xb4, 0xa7,
	0xa6, 0xb5, 0xa1, 0xa8, 0xae, 0xad, 0xac, 0x83,
	0x84, 0x89, 0x88, 0x86, 0x80, 0x8a, 0xaf, 0xb0,
	0xab, 0xa5, 0xbb, 0xb8, 0xb1, 0xa0, 0xbe, 0xb9,
	0xba, 0xb6, 0xb7, 0xaa, 0xa9, 0xa2, 0xa4, 0xbd,
	0xbc, 0x85, 0x82, 0x8d, 0x8c, 0x8e, 0x8f, 0x8b,
	0xd2, 0xd3, 0xd4, 0xd5, 0xc6, 0xc8, 0xc3, 0xde,
	0xdb, 0xdd, 0xdf, 0xd9, 0xd8, 0xdc, 0xc0, 0xd1,
	0xb3, 0xa3, 0x99, 0x98, 0x93, 0x9b, 0x9f, 0x97,
	0x9c, 0x95, 0x9e, 0x96, 0xbf, 0x9d, 0x94, 0x9a
};

static char todos[] = {
	0xc4, 0xb3, 0xda, 0xbf, 0xc0, 0xd9, 0xc3, 0xb4,
	0xc2, 0xc1, 0xc5, 0xdf, 0xdc, 0xdb, 0xdd, 0xde,
	0xb0, 0xb1, 0xb2, 0xf4, 0xfe, 0xf9, 0xfb, 0xf7,
	0xf3, 0xf2, 0xff, 0xf5, 0xf8, 0xfd, 0xfa, 0xf6,
	0xcd, 0xba, 0xd5, 0xf1, 0xd6, 0xc9, 0xb8, 0xb7,
	0xbb, 0xd4, 0xd3, 0xc8, 0xbe, 0xbd, 0xbc, 0xc6,
	0xc7, 0xcc, 0xb5, 0xf0, 0xb6, 0xb9, 0xd1, 0xd2,
	0xcb, 0xcf, 0xd0, 0xca, 0xd8, 0xd7, 0xce, 0xfc,
	0xee, 0xa0, 0xa1, 0xe6, 0xa4, 0xa5, 0xe4, 0xa3,
	0xe5, 0xa8, 0xa9, 0xaa, 0xab, 0xac, 0xad, 0xae,
	0xaf, 0xef, 0xe0, 0xe1, 0xe2, 0xe3, 0xa6, 0xa2,
	0xec, 0xeb, 0xa7, 0xe8, 0xed, 0xe9, 0xe7, 0xea,
	0x9e, 0x80, 0x81, 0x96, 0x84, 0x85, 0x94, 0x83,
	0x95, 0x88, 0x89, 0x8a, 0x8b, 0x8c, 0x8d, 0x8e,
	0x8f, 0x9f, 0x90, 0x91, 0x92, 0x93, 0x86, 0x82,
	0x9c, 0x9b, 0x87, 0x98, 0x9d, 0x99, 0x97, 0x9a
};

static void recode (void *ptr, int len, char map[])
{
	register char *p;

	for (p=ptr; --len>=0 && *p; ++p)
		if (*p & 0x80)
			*p = map [*p & 0x7f];
}
#endif

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
