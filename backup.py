#!/usr/local/bin/python

#
# Сброс всех данных базы в текстовом виде.
#
import deas, time, string, regsub
from posix import environ

#
# Функция преобразования номера счета в строку с отбрасыванием ведущей цифры
#
def cvt (val):
	str = "%d" % val
	return str[1:]

#
# Функция преобразования маски доступа в текстовую шкалу
#
def cvtmask (val):
	list = []
	i = 0
	while i<32 and val:
		if val & 1: list.append (chr (ord ('0') + i%10))
		else:       list.append ('-')
		val = val >> 1
		i = i+1
	list.append ('-')
	return string.joinfields (list, "")

#
# Функция преобразования даты в гг.мм.дд
#
def cvtdate (date):
	(y, m, d) = deas.date_to_ymd (date)
	return "%02d.%02d.%02d" % (y % 100, m, d)

#
# Определяем имя сервера, имя пользователя, порт
#
try:    user = environ["USER"]
except: user = environ["LOGNAME"]

try:    host = environ["DEASHOST"]
except: host = "localhost"

password = ""
port = 0

#
# Печатаем заголовок файла
#
print "# Сброс базы:",
if port:
	print "%s:%d" % (host, port)
else:
	print host
print "# Дата:", time.ctime (time.time ())
print "# Исполнитель:", user
print ""

#
# Открываем базу
#
deas.register (user, password, host, port)
deas.delflag (1)        # Удаленные счета/проводки тоже нужны
deas.date (99999)       # Ставим дату на 300 лет вперед

#
# Сохраняем план счетов
#
c = deas.chart_first ()
while c:
	(acn, pacn, passive, anal, descr, deleted, rmask, wmask) = c
	if pacn: print "sub",
	else:    print "acc",
	print cvt (acn),
	if pacn: print cvt (pacn),
	print "\"%s\"" % regsub.gsub ('"', "'", descr),
	print cvtmask (rmask), cvtmask (wmask),
	if passive: print "passive",
	if anal:    print "anal",
	if deleted: print "deleted",
	print ""
	c = deas.chart_next ()

print ""

#
# Выдаем все проводки
#
c = deas.journal_first ()
while c:
	(eid, date, debit, credit, amount, descr, deleted, rmask, user) = c
	(rub, doll, pcs, rec) = amount
	print "ent", eid, cvtdate (date), cvt (debit), cvt (credit),
	if rub: print rub,
	else:   print "$%d" % doll,
	if pcs: print pcs,
	print "\"%s\"" % user, "\"%s\"" % regsub.gsub ('"', "'", descr),
	if deleted: print "deleted",
	print ""
	c = deas.journal_next ()

print ""
print "# Все."
