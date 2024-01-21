CFLAGS  = -O -g -DNEEDSWAP -DIDEA32 -DUNIT32
CRFLAGS = $(CFLAGS) -fpic
LDFLAGS = -g
LIB     = libpgp.a
RLIB    = libpgp.ra
OBJS    = libpgp.o mpilib.o idea.o md5.o randpool.o mpiio.o rsaglue1.o
ROBJS   = libpgp.ro mpilib.ro idea.ro md5.ro randpool.ro mpiio.ro rsaglue1.ro

all:    $(LIB) $(RLIB)

$(LIB): $(OBJS)
	[ "$?" != "" ] && ar r $(LIB) $? && ranlib $(LIB)

$(RLIB): $(ROBJS)
	[ "$?" != "" ] && ar r $(RLIB) $? && ranlib $(RLIB)

depend:
	mkdep $(CFLAGS) *.c &&\
	sed '/^###$$/,$$d' Makefile > Makefile~ &&\
	echo '###' >> Makefile~ &&\
	cat .depend >> Makefile~ &&\
	rm .depend &&\
	mv Makefile~ Makefile

test.o: libpgp.c
	$(CC) $(CFLAGS) -DTEST -c libpgp.c -o test.o

test:   test.o $(LIB)
	$(CC) $(CFLAGS) test.o $(LIB) -o test

clean:
	rm -f *.[oba] *.r[oa] test

.SUFFIXES: .ro

.c.ro:
	$(CC) $(CRFLAGS) -c $< -o $*.ro

###
idea.o: idea.c /usr/include/string.h /usr/include/machine/ansi.h \
  /usr/include/sys/cdefs.h idea.h usuals.h randpool.h
libnul.o: libnul.c pgplib.h
libpgp.o: libpgp.c /usr/include/stdio.h /usr/include/sys/types.h \
  /usr/include/sys/cdefs.h /usr/include/machine/endian.h \
  /usr/include/machine/ansi.h /usr/include/machine/types.h \
  /usr/include/ctype.h /usr/include/runetype.h /usr/include/unistd.h \
  /usr/include/sys/unistd.h mpilib.h /usr/include/string.h usuals.h mpiio.h \
  idea.h md5.h pgplib.h randpool.h
md5.o: md5.c /usr/include/string.h /usr/include/machine/ansi.h \
  /usr/include/sys/cdefs.h md5.h
mpiio.o: mpiio.c /usr/include/stdio.h /usr/include/sys/types.h \
  /usr/include/sys/cdefs.h /usr/include/machine/endian.h \
  /usr/include/machine/ansi.h /usr/include/machine/types.h mpilib.h \
  /usr/include/string.h usuals.h mpiio.h
mpilib.o: mpilib.c mpilib.h /usr/include/string.h /usr/include/machine/ansi.h \
  /usr/include/sys/cdefs.h usuals.h
randpool.o: randpool.c /usr/include/stdlib.h /usr/include/machine/ansi.h \
  /usr/include/sys/cdefs.h /usr/include/string.h randpool.h usuals.h md5.h
rsaglue1.o: rsaglue1.c /usr/include/string.h /usr/include/machine/ansi.h \
  /usr/include/sys/cdefs.h mpilib.h usuals.h mpiio.h
