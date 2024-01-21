SHELL   = /bin/sh
LD      = g++ -g
SRCS    = Screen.cc Tty.cc Key.cc Box.cc Popup.cc Dialog.cc Menu.cc curses.c Termgoto.cc $(CAPSRC)
OBJS    = Screen.o  Tty.o  Key.o  Box.o  Popup.o  Dialog.o  Menu.o  curses.o Termgoto.o  $(CAPOBJ)
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
CPLUS   = g++ -Wall -g -std=c++20
CC      = gcc -Wall -g
CFLAGS  = -O -DTERMIOS

# For SunOS 4.1 with GCC
#-----------
#CPLUS   = g++ -Wall
#CC      = gcc -Wall
#CFLAGS  = -g -DTERMIOS

# Use standard libtermcap, instead of internal termcap library
#-------------------------------------------------------------
CAPSRC  = Termlib.cc
CAPOBJ  = Termlib.o
LIBS    = -ltermcap

$(LIB): $(OBJS)
	[ "$?" != "" ] && ar r $(LIB) $? && ranlib $(LIB)

tests:  hello hv hellcurs

hello:  hello.o $(LIB)
	$(CPLUS) $(CFLAGS) hello.o -o hello $(LIB) $(LIBS)

hellcurs: hellcurs.o $(LIB)
	$(CPLUS) $(CFLAGS) hellcurs.o -o hellcurs $(LIB) $(LIBS)

hv:     hv.o $(LIB)
	$(CPLUS) $(CFLAGS) hv.o -o hv $(LIB) $(LIBS)

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

.SUFFIXES: .cc

.cc.o:
	$(CPLUS) $(CFLAGS) -c $<

###
hellcurs.o: hellcurs.c curses.h
Box.o: Box.cc Screen.h extern.h
Dialog.o: Dialog.cc extern.h Screen.h Dialog.h
Key.o: Key.cc Screen.h Captab.h KeyPrivate.h extern.h
Menu.o: Menu.cc Screen.h Menu.h extern.h
Popup.o: Popup.cc Screen.h Popup.h extern.h
Screen.o: Screen.cc Screen.h Captab.h TtyPrivate.h KeyPrivate.h extern.h
Termcap.o: Termcap.cc Screen.h Captab.h extern.h
Termgoto.o: Termgoto.cc Screen.h Captab.h extern.h
Termlib.o: Termlib.cc Screen.h Captab.h extern.h
Tty.o: Tty.cc Screen.h TtyPrivate.h
curses.o: curses.cc Screen.h curses.h
hello.o: hello.cc Screen.h
hv.o: hv.cc Screen.h Menu.h
