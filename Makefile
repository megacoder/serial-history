CC	=gcc
CFLAGS	=-O6
LDFLAGS	=
LDLIBS	=

BINDIR	=${HOME}/bin

HFILES	=serial-history.h
CFILES	=serial-history.c
OBS	=${CFILES:.c=.o}

all:	serial-history

serial-history: ${OBS}
	${CC} ${LDFLAGS} -o serial-history ${OBS} ${LDLIBS}

clean:
	${RM} *.o a.out lint core tags Makefile.tmp

distclean clobber: clean
	${RM} serial-history Makefile.orig

install: serial-history uart
	install -d ${BINDIR}
	install -c -s serial-history ${BINDIR}
	install -c uart ${BINDIR}

uninstall:
	${RM} ${BINDIR}/serial-history
	${RM} ${BINDIR}/uart

tags:	${HFILES} ${CFILES}
	ctags ${HFILES} ${CFILES}

depend:
	[ -f Makefile.orig ] || cp Makefile Makefile.orig
	@sed -n '1,/^# DO NOT DELETE/p' Makefile	>  Makefile.tmp
	@echo '#'					>> Makefile.tmp
	@mkdep -o ${CFILES}				>> Makefile.tmp
	mv Makefile.tmp Makefile
#
# DO NOT DELETE THIS LINE, "make depend" REQUIRES IT
#
serial-history.o: serial-history.h serial-history.c
