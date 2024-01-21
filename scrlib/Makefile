SHELL   = /bin/sh
LD      = g++ -g
SRCS    = Screen.cc Tty.cc Key.cc Box.cc Popup.cc Dialog.cc Menu.cc curses.c ctype.c Termgoto.cc $(CAPSRC)
OBJS    = Screen.o  Tty.o  Key.o  Box.o  Popup.o  Dialog.o  Menu.o  curses.o ctype.o Termgoto.o  $(CAPOBJ)
CAPSRC  = Termcap.cc
CAPOBJ  = Termcap.o
LIB     = libscr.a

# For Interactive Unix 386
#-------------------------
#CC      = gcc -DISC -O -fstrength-reduce -finline-functions #-g0
#CPLUS   = g++ -DISC -O -fstrength-reduce -finline-functions #-g0
#CFLAGS  = -DTERMIO

# For Xenix 386
#--------------
#CC      = gcc -O -fstrength-reduce -finline-functions #-g0
#CPLUS   = g++ -O -fstrength-reduce -finline-functions #-g0
#CFLAGS  = -DTERMIO

# For Ultrix
#-----------
#CC      = gcc -pipe -O -fstrength-reduce -finline-functions #-g0
#CPLUS   = g++ -pipe -O -fstrength-reduce -finline-functions #-g0
#CFLAGS  =

# For FreeBSD
#-----------
CPLUS   = g++ -Wall -g
CC      = gcc -Wall -g
CFLAGS  = -O -DTERMIOS

# For SunOS 4.1 with GCC
#-----------
#CPLUS   = g++ -Wall
#CC      = gcc -Wall
#CFLAGS  = -g -DTERMIOS

# Use standard libtermcap, instead of internal termcap library
#-------------------------------------------------------------
#CAPSRC  = Termlib.cc
#CAPOBJ  = Termlib.o
#LIBS    = -ltermcap

$(LIB): $(OBJS)
	[ "$?" != "" ] && ar r $(LIB) $? && ranlib $(LIB)

tests:  hello hv hellcurs

hello:  hello.o $(LIB)
	$(CPLUS) $(CFLAGS) hello.o -o hello $(LIB) $(LIBS)

hellcurs: hellcurs.o $(LIB)
	$(CPLUS) $(CFLAGS) hellcurs.o -o hellcurs $(LIB) $(LIBS)

hv:     hv.o $(LIB)
	$(CPLUS) $(CFLAGS) hv.o -o hv $(LIB) $(LIBS)

depend:
	mkdep $(CFLAGS) *.cc &&\
	sed '/^###$$/,$$d' Makefile > Makefile~ &&\
	echo '###' >> Makefile~ &&\
	cat .depend >> Makefile~ &&\
	rm .depend &&\
	mv Makefile~ Makefile

clean:
	rm -f *.o *.b *.a .,* core a.out hello hv hellcurs

install: $(LIB)
	-mkdir /usr/local/include
	-mkdir /usr/local/lib
	install -c $(LIB) /usr/local/lib/$(LIB)
	ranlib /usr/local/lib/$(LIB)
	chmod 444 /usr/local/lib/$(LIB)
	install -c -m 444 Screen.h /usr/local/include/Screen.h
	install -c -m 444 curses.h /usr/local/include/curses.h
	install -c -m 444 ctype.h /usr/local/include/ctype.h

dos:
	rm -rf scrdos scrdos.zip
	mkdir scrdos
	cp -p *.c *.cc *.h *.txt scrdos
	cp -p Makefile.dos scrdos/Makefile
	cp -p Makefile scrdos/Makefile.unx
	find scrdos -type f -print | xargs todos
	zip -rm scrdos scrdos

.SUFFIXES: .cc

.cc.o:
	$(CPLUS) $(CFLAGS) -c $<

###
Box.o: Box.cc Screen.h extern.h /usr/include/stdlib.h \
  /usr/include/machine/ansi.h /usr/include/sys/cdefs.h
Dialog.o: Dialog.cc /usr/include/stdarg.h extern.h /usr/include/stdlib.h \
  /usr/include/machine/ansi.h /usr/include/sys/cdefs.h Screen.h Dialog.h
Key.o: Key.cc /usr/include/setjmp.h /usr/include/sys/cdefs.h \
  /usr/include/signal.h /usr/include/sys/types.h \
  /usr/include/machine/endian.h /usr/include/machine/ansi.h \
  /usr/include/machine/types.h /usr/include/sys/signal.h \
  /usr/include/machine/signal.h /usr/include/machine/trap.h \
  /usr/include/errno.h Screen.h Termcap.h extern.h /usr/include/stdlib.h
Menu.o: Menu.cc Screen.h Menu.h extern.h /usr/include/stdlib.h \
  /usr/include/machine/ansi.h /usr/include/sys/cdefs.h
Popup.o: Popup.cc Screen.h Popup.h extern.h /usr/include/stdlib.h \
  /usr/include/machine/ansi.h /usr/include/sys/cdefs.h /usr/include/stdarg.h
Screen.o: Screen.cc /usr/include/signal.h /usr/include/sys/types.h \
  /usr/include/sys/cdefs.h /usr/include/machine/endian.h \
  /usr/include/machine/ansi.h /usr/include/machine/types.h \
  /usr/include/sys/signal.h /usr/include/machine/signal.h \
  /usr/include/machine/trap.h Screen.h Termcap.h extern.h \
  /usr/include/stdlib.h
Termcap.o: Termcap.cc Screen.h Termcap.h extern.h /usr/include/stdlib.h \
  /usr/include/machine/ansi.h /usr/include/sys/cdefs.h
Termgoto.o: Termgoto.cc Screen.h Termcap.h extern.h /usr/include/stdlib.h \
  /usr/include/machine/ansi.h /usr/include/sys/cdefs.h
Termlib.o: Termlib.cc Screen.h Termcap.h extern.h /usr/include/stdlib.h \
  /usr/include/machine/ansi.h /usr/include/sys/cdefs.h
Tty.o: Tty.cc /usr/include/sys/termios.h /usr/include/sys/cdefs.h \
  /usr/include/sys/ttycom.h /usr/include/sys/ioccom.h \
  /usr/include/sys/ttydefaults.h /usr/include/sys/ioctl.h \
  /usr/include/sys/filio.h /usr/include/sys/sockio.h Screen.h
curses.o: curses.cc /usr/include/stdlib.h /usr/include/machine/ansi.h \
  /usr/include/sys/cdefs.h /usr/include/string.h /usr/include/unistd.h \
  /usr/include/sys/types.h /usr/include/machine/endian.h \
  /usr/include/machine/types.h /usr/include/sys/unistd.h Screen.h curses.h \
  /usr/include/stdio.h
hello.o: hello.cc Screen.h
hv.o: hv.cc /usr/include/sys/types.h /usr/include/sys/cdefs.h \
  /usr/include/machine/endian.h /usr/include/machine/ansi.h \
  /usr/include/machine/types.h /usr/include/sys/stat.h \
  /usr/include/sys/time.h /usr/include/time.h /usr/include/stdio.h \
  /usr/include/stdlib.h /usr/include/string.h /usr/include/unistd.h \
  /usr/include/sys/unistd.h /usr/include/sys/wait.h /usr/include/signal.h \
  /usr/include/sys/signal.h /usr/include/machine/signal.h \
  /usr/include/machine/trap.h Screen.h Menu.h
