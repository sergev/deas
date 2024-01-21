#ifdef unix
#   include <sys/types.h>
#   include <sys/uio.h>
#   include <unistd.h>
#else
#   include "tcp.h"
#endif
#include "deas.h"

#define DYSIZE(y)       ((y) % 4 ? 365 : 366)
#define DAYS_PER_400YRS 146097L
#define DAYS_PER_4YRS   1461L
#define YRNUM(c, y)     (DAYS_PER_400YRS*(c)/4 + DAYS_PER_4YRS*(y)/4)
#define DAYNUM(c,y,m,d) (YRNUM((c), (y)) + mdays[m] + (d))

void date2ymd (long date, int *year, int *mon, int *day)
{
	static dmsize [12] = { 31,28,31,30,31,30,31,31,30,31,30,31 };

	if (date >= 0)
		for (*year=1990; date >= DYSIZE (*year); ++*year)
			date -= DYSIZE (*year);
	else
		for (*year=1990; date<0; --*year)
			date += DYSIZE (*year-1);
	if (DYSIZE (*year) == 366)
		dmsize[1] = 29;
	for (*mon=0; date >= dmsize[*mon]; ++*mon)
		date -= dmsize[*mon];
	dmsize[1] = 28;
	*day = date+1;
	++*mon;
}

long ymd2date (int year, int mon, int day)
{
	static nmdays[] = { 0,31,28,31,30,31,30,31,31,30,31,30,31 };
	static mdays[] = { 0,31,61,92,122,153,184,214,245,275,306,337 };
	int century;

	if (year < 90)
		year += 2000;
	else if (year < 100)
		year += 1900;

	if (day > nmdays[mon])
		if (mon != 2 || (year % 4 == 0 &&
		    (year % 100 != 0 || year % 400 == 0) && day > 29))
			return -1;      /* нет такой даты в этом месяце */

	century = year / 100;
	year %= 100;
	if (mon > 2)
		mon -= 3;
	else {
		mon += 9;
		if (year == 0) {
			--century;
			year = 99;
		} else
			--year;
	}
	return DAYNUM (century, year, mon, day) - DAYNUM (19, 89, 10, 1);
}

/*
 * Принимаем данные по сети.  Вначале идет двухбайтная длина
 * пакета, затем собственно пакет.
 * Возвращает длину принятого пакета (возможно 0),
 * или -1 при ошибке.
 */
#ifdef unix
int receive_packet (int sock, char *data, int maxlen)
{
	short len;
	int cnt, res;

	res = read (sock, &len, 2);
	if (res <= 0)
		return -1;
	if (res == 1) {
		res = read (sock, 1 + (char*)&len, 1);
		if (res <= 0)
			return -1;
	}
	if (len < 0 || len > maxlen)
		return -1;
	for (cnt=0; cnt<len; cnt+=res) {
		res = read (sock, data + cnt, len - cnt);
		if (res <= 0)
			return -1;
	}
	return len;
}
#else
int receive_packet (void *sock, char *data, int maxlen)
{
	short len, res;

	res = sock_read (sock, (unsigned char*)&len, 2);
	if (res != 2 || len < 0 || len > maxlen)
		return -1;
	res = sock_read (sock, (unsigned char*)data, len);
	if (res != len)
		return -1;
	return len;
}
#endif

/*
 * Передаем данные по сети: двухбайтная длина плюс пакет.
 */
#ifdef unix
void send_packet (int sock, char *data, short len)
{
	struct iovec vec[2];

	if (! len) {
		write (sock, &len, 2);
		return;
	}
	vec[0].iov_base = (void*) &len;
	vec[0].iov_len = 2;
	vec[1].iov_base = data;
	vec[1].iov_len = len;
	writev (sock, vec, 2);
}
#else
void send_packet (void *sock, char *data, short len)
{
	if (len) {
		sock_write (sock, (unsigned char*)&len, 2);
		sock_flushnext (sock);
		sock_write (sock, (unsigned char*)data, len);
	} else {
		sock_flushnext (sock);
		sock_write (sock, (unsigned char*)&len, 2);
	}
}
#endif
