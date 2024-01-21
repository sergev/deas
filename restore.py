#!/usr/local/bin/python

#
# Загрузка данных в базу
#
import deas, sys, string

#
# Функция преобразования текстовой шкалы в маску доступа
#
def getmode (str):
	gmask = 1
	for i in range (len(str)):
		if str[i] in string.digits:
			gmask = gmask | 1 << i
	return gmask

#
# Функция преобразования гг.мм.дд в дату
#
def getdate (str):
	list = string.splitfields (str, '.')
	return deas.ymd_to_date (string.atoi (list[0]), string.atoi (list[1]),
		string.atoi (list[2]))

deas.register ()

print "Loading",
naccounts = nentries = 0
acntab = {}
while 1:
	line = sys.stdin.readline ()
	if not line: break
	if line[0] == '\n' or line[0] == '#': continue
	list = string.split (line)

	if list[0] == "acc" or list[0] == "sub":
		type = list[0]
		del list[0]
		acn = string.atoi ('1' + list[0])
		del list[0]

		pacn = 0
		if type == "sub":
			pacn = string.atoi ('1' + list[0])
			del list[0]

		last = list[0]
		del list[0]
		descr = [last]
		while len(list)>0 and last[len(last)-1] != '"':
			last = list[0]
			del list[0]
			descr.append (last)
		descr = string.join (descr)
		descr = descr[1:-1]

		rmode = getmode (list[0])
		del list[0]
		wmode = getmode (list[0])
		del list[0]

		deleted = passive = anal = 0
		while len(list) > 0:
			if list[0] == "passive":   passive = 1
			elif list[0] == "anal":    anal = 1
			elif list[0] == "deleted": deleted = 1
			else:
				print "Invalid flag:", list[0]
			del list[0]

		deas.chart_add (acn, pacn, passive, anal, descr, deleted,
			rmode, wmode)
		naccounts = naccounts + 1
		sys.stdout.write (".")
		sys.stdout.flush ()

	elif list[0] == "ent":
		del list[0]
		eid = string.atoi (list[0])
		del list[0]
		date = getdate (list[0])
		del list[0]
		debit = string.atoi ('1' + list[0])
		del list[0]
		credit = string.atoi ('1' + list[0])
		del list[0]

		rub = doll = 0
		if list[0][0] == '$': doll = string.atoi (list[0][1:])
		else:                 rub = string.atoi (list[0])
		del list[0]

		pcs = 0
		if list[0][0] != '"':
			pcs = string.atoi (list[0])
			del list[0]

		last = list[0]
		del list[0]
		user = [last]
		while len(list)>0 and last[len(last)-1] != '"':
			last = list[0]
			del list[0]
			user.append (last)
		user = string.join (user)
		user = user[1:-1]

		last = list[0]
		del list[0]
		descr = [last]
		while len(list)>0 and last[len(last)-1] != '"':
			last = list[0]
			del list[0]
			descr.append (last)
		descr = string.join (descr)
		descr = descr[1:-1]

		deleted = 0
		if len(list) > 0 and list[0] == "deleted":
			deleted = 1

		acntab[eid] = (date, debit, credit, rub, doll, pcs,
			descr, deleted, user)
	else:
		print "Error:", line[0]

eidlist = acntab.keys()
eidlist.sort()
for eid in eidlist:
	(date, debit, credit, rub, doll, pcs, descr, deleted, user) = acntab[eid]
	deas.journal_add (eid, date, debit, credit, (rub, doll, pcs, 1),
		descr, deleted, 0, user)
	nentries = nentries + 1
	sys.stdout.write ("-")
	sys.stdout.flush ()

print " done"
print "Loaded %d accounts, %d entries" % (naccounts, nentries)
