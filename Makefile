CFLAGS  = -O -g -Wall -Iscrlib -Ipgplib
CRFLAGS = $(CFLAGS) -fpic
LDFLAGS = -g
CRYPT   = pgplib/libpgp.a
#CRYPT   = cryptnul.o
LIBS    = scrlib/libscr.a pgplib/libpgp.a
OBJS    = main.o help.o users.o accounts.o journal.o entries.o lib.o util.o
ROBJS   = main.o help.o users.o accounts.o journal.o entries.o lib$(PROG).a
DOBJS   = server.o lib.o util.o $(CRYPT)
LOBJS   = client.o util.o
LROBJS  = client.ro util.ro
PROG    = deas

all:    l$(PROG) $(PROG) $(PROG)d $(PROG)ctl lib$(PROG).a lib$(PROG).ra python/$(PROG)module.so

l$(PROG): $(OBJS) $(LIBS)
	$(CC) $(LDFLAGS) $(OBJS) -o l$(PROG) $(LIBS)

$(PROG): $(ROBJS) $(LIBS)
	$(CC) $(LDFLAGS) $(ROBJS) -o $(PROG) $(LIBS)

$(PROG)d: $(DOBJS)
	$(CC) $(LDFLAGS) $(DOBJS) -o $(PROG)d

lib$(PROG).a: $(LOBJS)
	[ "$?" != "" ] && ar r lib$(PROG).a $? && ranlib lib$(PROG).a

lib$(PROG).ra: $(LROBJS)
	[ "$?" != "" ] && ar r lib$(PROG).ra $? && ranlib lib$(PROG).ra

$(PROG)ctl: $(PROG)ctl.o $(LIBS)
	$(CC) $(CFLAGS) $(PROG)ctl.o $(LIBS) -o $(PROG)ctl

$(PROG).pgp:
	rm -f $(PROG).pgp
	pgp -kx vak $(PROG).pgp $(HOME)/.pgp/secring.pgp
	pgp -kx vsk $(PROG).pgp $(HOME)/.pgp/secring.pgp
	pgp -kx sak $(PROG).pgp $(HOME)/.pgp/secring.pgp
	pgp -kx vav $(PROG).pgp $(HOME)/.pgp/secring.pgp
	pgp -kx andrew $(PROG).pgp $(HOME)/.pgp/secring.pgp
	pgp -kx group1 $(PROG).pgp $(HOME)/.pgp/secring.pgp
	pgp -kx group2 $(PROG).pgp $(HOME)/.pgp/secring.pgp
	pgp -kx group3 $(PROG).pgp $(HOME)/.pgp/secring.pgp
	pgp -kx $(PROG) $(PROG).pgp $(HOME)/.pgp/secring.pgp

clean:
	rm -f *.[oba] *.r[oa] z $(PROG) l$(PROG) r$(PROG) $(PROG)d $(PROG)ctl
	cd scrlib && make clean
	cd pgplib && make clean
	cd python && make clean

depend:
	mkdep $(CFLAGS) *.c *.cc &&\
	sed '/^###$$/,$$d' Makefile > Makefile~ &&\
	echo '###' >> Makefile~ &&\
	cat .depend >> Makefile~ &&\
	rm .depend &&\
	mv Makefile~ Makefile

scrlib/libscr.a:
	cd scrlib && make

pgplib/libpgp.a:
	cd pgplib && make

python/$(PROG)module.so:
	cd python && make

install: /usr/local/etc/$(PROG)d /usr/local/bin/$(PROG) /usr/local/bin/l$(PROG)\
	/usr/local/lib/python/$(PROG)module.so /usr/local/etc/$(PROG)ctl\
	/usr/local/etc/chk$(PROG) /usr/local/bin/$(PROG)backup\
	/usr/local/bin/$(PROG)restore

/usr/local/bin/$(PROG): $(PROG)
	install -c -s $(PROG) /usr/local/bin/$(PROG)

/usr/local/bin/$(PROG)backup: backup.py
	install -c backup.py /usr/local/bin/$(PROG)backup

/usr/local/bin/$(PROG)restore: restore.py
	install -c restore.py /usr/local/bin/$(PROG)restore

/usr/local/bin/l$(PROG): l$(PROG)
	install -c -s l$(PROG) /usr/local/bin/l$(PROG)

/usr/local/etc/$(PROG)d: $(PROG)d
	install -c -s $(PROG)d /usr/local/etc/$(PROG)d

/usr/local/etc/$(PROG)ctl: $(PROG)ctl
	install -c -s $(PROG)ctl /usr/local/etc/$(PROG)ctl

/usr/local/etc/chk$(PROG): chk$(PROG)
	install -c chk$(PROG) /usr/local/etc/chk$(PROG)

/usr/local/lib/python/$(PROG)module.so: python/$(PROG)module.so
	install -c python/$(PROG)module.so /usr/local/lib/python/$(PROG)module.so

.SUFFIXES: .ro

.c.ro:
	$(CC) $(CRFLAGS) -c $< -o $*.ro

dos:
	rm -rf $(PROG)dos $(PROG)dos.zip
	mkdir $(PROG)dos
	find . -type f ! -path ./python\* -print | tar cfT - - | (cd $(PROG)dos; tar xf -)
	tar czf $(PROG)dos/python.tgz python
	for i in $(PROG)dos/Makefile $(PROG)dos/*/Makefile; do\
		mv $$i $$i.unx; done
	for i in $(PROG)dos/Makefile.dos $(PROG)dos/*/Makefile.dos; do\
		mv $$i `dirname $$i`/`basename $$i .dos`; done
	 find $(PROG)dos -type f ! -name \*.dat ! -name \*.pgp ! -name \*.lib ! -name \*.tgz ! -name \*.exe -print | xargs todos
	 zip -rm $(PROG)dos $(PROG)dos

###
client.o: client.c /usr/include/unistd.h /usr/include/sys/cdefs.h \
  /usr/include/sys/types.h /usr/include/machine/endian.h \
  /usr/include/machine/ansi.h /usr/include/machine/types.h \
  /usr/include/sys/unistd.h /usr/include/string.h /usr/include/stdio.h \
  /usr/include/netdb.h /usr/include/time.h /usr/include/sys/socket.h \
  /usr/include/netinet/in.h /usr/include/netinet/tcp.h \
  /usr/include/arpa/inet.h deas.h pgplib/pgplib.h
lib.o: lib.c /usr/include/stdio.h /usr/include/sys/types.h \
  /usr/include/sys/cdefs.h /usr/include/machine/endian.h \
  /usr/include/machine/ansi.h /usr/include/machine/types.h \
  /usr/include/stdlib.h /usr/include/string.h /usr/include/fcntl.h \
  /usr/include/unistd.h /usr/include/sys/unistd.h /usr/include/sys/stat.h \
  /usr/include/sys/time.h /usr/include/time.h deas.h
util.o: util.c /usr/include/sys/types.h /usr/include/sys/cdefs.h \
  /usr/include/machine/endian.h /usr/include/machine/ansi.h \
  /usr/include/machine/types.h /usr/include/sys/uio.h /usr/include/unistd.h \
  /usr/include/sys/unistd.h deas.h
server.o: server.c /usr/include/stdio.h /usr/include/sys/types.h \
  /usr/include/sys/cdefs.h /usr/include/machine/endian.h \
  /usr/include/machine/ansi.h /usr/include/machine/types.h \
  /usr/include/stdlib.h /usr/include/unistd.h /usr/include/sys/unistd.h \
  /usr/include/string.h /usr/include/errno.h /usr/include/time.h \
  /usr/include/sys/socket.h /usr/include/netinet/in.h \
  /usr/include/netinet/tcp.h deas.h pgplib/pgplib.h
accounts.o: accounts.cc /usr/include/stdio.h /usr/include/sys/types.h \
  /usr/include/sys/cdefs.h /usr/include/machine/endian.h \
  /usr/include/machine/ansi.h /usr/include/machine/types.h \
  /usr/include/stdlib.h /usr/include/string.h scrlib/Screen.h scrlib/Dialog.h \
  deas.h
entries.o: entries.cc
help.o: help.cc scrlib/Screen.h
journal.o: journal.cc /usr/include/stdio.h /usr/include/sys/types.h \
  /usr/include/sys/cdefs.h /usr/include/machine/endian.h \
  /usr/include/machine/ansi.h /usr/include/machine/types.h \
  /usr/include/stdlib.h /usr/include/string.h scrlib/Screen.h scrlib/Dialog.h \
  deas.h
main.o: main.cc /usr/include/stdio.h /usr/include/sys/types.h \
  /usr/include/sys/cdefs.h /usr/include/machine/endian.h \
  /usr/include/machine/ansi.h /usr/include/machine/types.h \
  /usr/include/stdlib.h /usr/include/string.h /usr/include/unistd.h \
  /usr/include/sys/unistd.h /usr/include/signal.h /usr/include/sys/signal.h \
  /usr/include/machine/signal.h /usr/include/machine/trap.h \
  /usr/include/sys/wait.h /usr/include/rpc/rpc.h /usr/include/rpc/types.h \
  /usr/include/sys/time.h /usr/include/time.h /usr/include/netinet/in.h \
  /usr/include/rpc/xdr.h /usr/include/rpc/auth.h /usr/include/rpc/clnt.h \
  /usr/include/rpc/rpc_msg.h /usr/include/rpc/auth_unix.h \
  /usr/include/rpc/svc.h /usr/include/rpc/svc_auth.h scrlib/Screen.h \
  scrlib/Popup.h scrlib/Dialog.h deas.h
users.o: users.cc /usr/include/string.h /usr/include/machine/ansi.h \
  /usr/include/sys/cdefs.h scrlib/Screen.h deas.h
deasctl.o: deasctl.c /usr/include/stdio.h /usr/include/sys/types.h \
  /usr/include/sys/cdefs.h /usr/include/machine/endian.h \
  /usr/include/machine/ansi.h /usr/include/machine/types.h \
  /usr/include/ctype.h /usr/include/runetype.h /usr/include/unistd.h \
  /usr/include/sys/unistd.h pgplib/mpilib.h /usr/include/string.h pgplib/usuals.h pgplib/mpiio.h \
  pgplib/idea.h pgplib/md5.h
