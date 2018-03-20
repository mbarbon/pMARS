# generic UNIX makefile
#CC = gcc			# req. for linux
CC = cc				# if you don't have gcc
# Configuration options:
#
# No.   Name            Incompatible with   Description
# (1)   -DSERVER        2                   disables cdb debugger (koth server 
#                                           version)
# (2)   -DGRAPHX        1                   enables platform specific core 
#                                           graphics
# (3)   -DKEYPRESS                          only for curses display on SysV:
#                                           enter cdb upon keypress (use if
#                                           Ctrl-C doesn't work)
# (4)   -DEXT94                             ICWS'94 + SEQ,SNE,NOP,*,{,}
# (5)   -DSMALLMEM                          16-bit addresses, less memory
# (6)   -DXWINGRAPHX    1                   X-Windows graphics (UNIX)

CFLAGS = -O -DEXT94
LFLAGS = -x
LIB = -lcurses -ltermlib		# enable this one for curses display
# LIB = -lvgagl -lvga			# enable this one for Linux/SVGA
# LIB = -lX11				# enable this one for X11

.SUFFIXES: .o .c .c~ .man .doc .6
MAINFILE = pmars
ARCHIVE = pmars08s

HEADER = global.h config.h asm.h sim.h 
OBJ1 = pmars.o asm.o eval.o disasm.o cdb.o sim.o pos.o
OBJ2 = clparse.o global.o token.o 
OBJ3 = str_eng.o

ALL: flags $(MAINFILE) man doc

flags:
	@echo Making $(MAINFILE) with compiler flags $(CFLAGS)

$(MAINFILE): $(OBJ1) $(OBJ2) $(OBJ3)
	@echo Linking $(MAINFILE)
	@$(CC) -o $(MAINFILE) $(OBJ1) $(OBJ2) $(OBJ3) $(LIB)
	@strip $(MAINFILE)
	@echo done

token.o asm.o disasm.o: asm.h

sim.o cdb.o pos.o disasm.o: sim.h

sim.o: curdisp.c uidisp.c lnxdisp.c xwindisp.c

xwindisp.c: xwindisp.h pmarsicn.h

lnxdisp.c: lnxdisp.h

$(OBJ1) $(OBJ2) $(OBJ3): makefile config.h global.h

.c.o:
	@echo Compiling $*.o 
	@$(CC) $(CFLAGS) -c $*.c

man: pmars.man
doc: pmars.doc
pmars.doc: pmars.man
pmars.man: pmars.6

.6.man:
	@echo Building $*.man - online manual
	@nroff -e -man $*.6 > $*.man

.man.doc:
	@echo Building $*.doc - printable manual
	@cp $*.man $*.doc
	@ex $*.doc < makedoc.ex > /dev/null

zip:
	@echo Building zip archive
	@zip -@ $(ARCHIVE) < filelist
	@echo done

tar:
	@echo Building tar archive
	@tar cf $(ARCHIVE).tar -I filelist
	@compress $(ARCHIVE).tar
	@echo done
 
clean:
	rm -f $(OBJ1) $(OBJ2) $(OBJ3) core

