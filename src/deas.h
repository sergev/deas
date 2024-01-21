/*
 * Generic Management Accounting System.
 * Протокол взаимодействия с базой данных сервера.
 *
 * Copyright (C) 1996 Cronyx Ltd.
 * All rights reserved.
 */
#ifdef __cplusplus
extern "C" {
#endif

#define DEASPORT        4726            /* порт по умолчанию */

#define MAXUSERS        16              /* макс. количество пользователей */
#define MAXACCOUNTS     4096            /* макс. количество счетов в базе */
#define MAXMENU         128             /* макс. размер меню проводок */

#define GRMAX           16              /* макс. количество групп доступа */
#define ANAMSZ          52              /* длина имени счета */
#define ENAMSZ          84              /* длина комментария проводки */
#define UNAMSZ          20              /* длина имени пользователя */
#define UDESCRSZ        84              /* длина полного имени пользователя */
#define LOGNSZ          12              /* длина регистрационного имени */
#define KEYSZ           (256+8)         /* макс. размер ключа */

struct deas_req {                       /* пакет-запрос */
	short counter;                  /* порядковый номер */
	short op;                       /* операция */
	short len;                      /* длина данных */
	char data [8192];               /* данные */
};

struct deas_reply {                     /* пакет-ответ */
	short counter;                  /* порядковый номер */
	short len;                      /* длина данных */
	char data [8192];               /* данные */
};

typedef struct {                        /* пакет аутентификации */
	char logname [LOGNSZ];          /* регистрационное имя */
	char data [64];                 /* зашифрованный пароль */
} authinfo;

/*
 * Сумма денег/штук.
 */
typedef struct {
	long rub;               /* рубли */
	long doll;              /* доллары */
	long pcs;               /* штуки для аналитических счетов */
	long rec;               /* всего записей */
} val;

/*
 * Информация о счете в плане счетов
 */
typedef struct {
	char descr [ANAMSZ];    /* название, описание */
	long acn;               /* номер счета */
	long pacn;              /* для субсчета - номер основного счета */
	long passive;           /* тип актив/пассив */
	long anal;              /* признак аналитического счета */
	long deleted;           /* счет удален */
	long rmask;             /* маска групп доступа на чтение */
	long wmask;             /* маска групп доступа на изменение */
} accountinfo;

/*
 * Информация о проводке в журнале
 */
typedef struct {
	char descr [ENAMSZ];    /* описание */
	char user [UNAMSZ];     /* автор */
	long eid;               /* номер проводки */
	long date;              /* дата, дни от 1.1.90 */
	long debit;             /* дебетуемый счет */
	long credit;            /* кредитуемый счет */
	val amount;             /* сумма */
	long deleted;           /* проводка удалена */
	long rmask;             /* маска групп доступа на чтение */
} entryinfo;

/*
 * Баланс по счету:
 * суммарный актив для активных счетов и пассив для пассивных.
 */
typedef struct {
	val debit;              /* сальдо по дебету */
	val credit;             /* сальдо по кредиту */
} balance;

/*
 * Информация о пользователе
 */
typedef struct {
	char logname [LOGNSZ];  /* регистрационное имя */
	char user [UNAMSZ];     /* учетное имя */
	char descr [UDESCRSZ];  /* подробная информация */
	long gmask;             /* список групп */
} userinfo;

typedef struct {
	userinfo info;          /* информация о пользователе */
	char E [KEYSZ];         /* открытый ключ */
	char N [KEYSZ];         /* открытый ключ */
} userdata;

/*
 * Состояние клиента.
 */
struct _account_t;
typedef struct {
	userdata *u;            /* информация о пользователе */
	char key [16];          /* открытый ключ */
	int delflg;             /* показывать удаленные счета/проводки */
	int cidx;               /* текущее положение для chart_next */
	int aidx;               /* текущее положение для account_next */
	int jidx;               /* текущее положение для journal_next */
	int uidx;               /* текущее положение для user_next */
	int midx;               /* текущее положение для menu_next */
	struct _account_t *ap;  /* текущий счет для account_next */
} clientinfo;

/*
 * Строка меню проводок
 */
typedef struct {
	char descr [ENAMSZ];    /* заготовка комментария проводки */
	char line [ANAMSZ];     /* строка меню */
	long credit;
	long debit;
	long currency;
} menuinfo;

typedef struct {
	long error;
	accountinfo info;
} account_reply;

typedef struct {
	long error;
	entryinfo info;
} entry_reply;

typedef struct {
	long error;
	balance info;
} balance_reply;

typedef struct {
	long error;
	balance info [12];
} report_reply;

typedef struct {
	long error;
	userinfo info;
} user_reply;

typedef struct {
	long error;
	menuinfo info;
} menu_reply;

#define OPTION_DATE     1       /* long        -> long          */
#define OPTION_DELETED  2       /* long        -> long          */

#define CHART_FIRST     11      /* void        -> account_reply */
#define CHART_NEXT      12      /* void        -> account_reply */
#define CHART_INFO      13      /* long        -> account_reply */
#define CHART_ADD       14      /* accountinfo -> void          */
#define CHART_EDIT      15      /* accountinfo -> void          */
#define CHART_DELETE    16      /* long        -> void          */

#define ACCOUNT_FIRST   21      /* long        -> entry_reply   */
#define ACCOUNT_NEXT    22      /* void        -> entry_reply   */
#define ACCOUNT_BALANCE 24      /* long        -> balance_reply */
#define ACCOUNT_REPORT  25      /* long        -> report_reply  */

#define JOURNAL_FIRST   31      /* void        -> entry_reply   */
#define JOURNAL_NEXT    32      /* void        -> entry_reply   */
#define JOURNAL_INFO    33      /* long        -> entry_reply   */
#define JOURNAL_ADD     34      /* entryinfo   -> void          */
#define JOURNAL_EDIT    35      /* entryinfo   -> void          */
#define JOURNAL_DELETE  36      /* long        -> void          */

#define USER_FIRST      41      /* void        -> user_reply    */
#define USER_NEXT       42      /* void        -> user_reply    */

#define MENU_FIRST      51      /* void        -> menu_reply    */
#define MENU_NEXT       52      /* void        -> menu_reply    */

extern char deas_E [];
extern char deas_D [];
extern char deas_P [];
extern char deas_Q [];
extern char deas_U [];
extern char deas_N [];

int deas_init (char *hostdir, int port, void(*)(char*));
userdata *user_register (char *logname, char *password, long seed);
char *user_E (clientinfo *c);
char *user_N (clientinfo *c);

long option_deleted (clientinfo *c, long on);
long option_date (clientinfo *c, long date);

userinfo *user_first (clientinfo *c);
userinfo *user_next (clientinfo *c);

menuinfo *menu_first (clientinfo *c);
menuinfo *menu_next (clientinfo *c);

accountinfo *chart_first (clientinfo *c);
accountinfo *chart_next (clientinfo *c);
accountinfo *chart_info (clientinfo *c, long acn);
void chart_edit (clientinfo *c, accountinfo *i);
void chart_add (clientinfo *c, accountinfo *i);
void chart_delete (clientinfo *c, long acn);

entryinfo *account_first (clientinfo *c, long acn);
entryinfo *account_next (clientinfo *c);
balance *account_balance (clientinfo *c, long acn);
balance *account_report (clientinfo *c, long acn);

entryinfo *journal_first (clientinfo *c);
entryinfo *journal_next (clientinfo *c);
entryinfo *journal_info (clientinfo *c, long eid);
void journal_add (clientinfo *c, entryinfo *i);
void journal_edit (clientinfo *c, entryinfo *i);
void journal_delete (clientinfo *c, long eid);
void date2ymd (long date, int *year, int *mon, int *day);
long ymd2date (int year, int mon, int day);

int receive_packet (int sock, char *data, int len);
void send_packet (int sock, char *data, short len);

#ifdef __cplusplus
};
#endif
