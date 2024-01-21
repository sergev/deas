PROG    = deasmodule
CFLAGS  = -O -fpic -Wall -I.. -Ipython -I../pgplib
LDFLAGS = -Bshareable

LIBS    = ../libdeas.ra ../pgplib/libpgp.ra

$(PROG).so: deasmod.o $(LIBS)
	$(LD) $(LDFLAGS) deasmod.o $(LIBS) -o $(PROG).so

clean:
	rm -f *.[ob] z $(PROG).so

depend:
	mkdep $(CFLAGS) *.c &&\
	sed '/^###$$/,$$d' Makefile > Makefile~ &&\
	echo '###' >> Makefile~ &&\
	cat .depend >> Makefile~ &&\
	rm .depend &&\
	mv Makefile~ Makefile

##
###
deasmod.o: deasmod.c python/Python.h python/allobjects.h /usr/include/stdio.h \
  /usr/include/sys/types.h /usr/include/sys/cdefs.h \
  /usr/include/machine/endian.h /usr/include/machine/ansi.h \
  /usr/include/machine/types.h /usr/include/string.h /usr/include/errno.h \
  python/myproto.h python/object.h python/objimpl.h python/accessobject.h \
  python/intobject.h python/longobject.h python/floatobject.h \
  python/rangeobject.h python/stringobject.h python/tupleobject.h \
  python/listobject.h python/mappingobject.h python/methodobject.h \
  python/moduleobject.h python/funcobject.h python/classobject.h \
  python/fileobject.h python/errors.h python/mymalloc.h python/modsupport.h \
  /usr/include/varargs.h python/ceval.h python/abstract.h ../deas.h
