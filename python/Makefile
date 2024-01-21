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
