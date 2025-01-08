bin_PROGRAMS = Amy

Amy_SOURCES = bitboard.c bookup.c commands.c dbase.c eco.c hashtable.c \
              heap.c \
              init.c learn.c magic.c mates.c movedata.c \
              mytb.cpp next.c pgn.c probe.c random.c recog.c score.c \
              score_config.c search.c \
              search_io.c state_machine.c swap.c time_ctl.c utils.c tree.c \
              version.c yaml.c test_yaml.c test_dbase.c main.c

Amy_DEPENDENCIES = bitboard.c bookup.o commands.o dbase.o eco.o \
              heap.o \
              hashtable.o init.o learn.o magic.o mates.o movedata.o mytb.o \
              next.o pgn.o probe.o random.o recog.o score.o \
              score_config.o search.o \
              search_io.o state_machine.o swap.o time_ctl.o utils.o tree.o \
              version.o yaml.o test_yaml.o test_dbase.c main.o

AM_CFLAGS=-I$(top_srcdir)/include

pkgdata_DATA = Eco.db Book.db

EXTRA_Amy_SOURCES = tbindex.cpp tbdecode.c

EXTRA_DIST = bitboard_x86.s

Eco.db: ${srcdir}/../PGN/eco.pgn
	echo "eco ${srcdir}/../PGN/eco.pgn" | ./Amy

Book.db: Eco.db ${srcdir}/../PGN/ClassicGames.pgn
	-rm Book.db
	echo "bookup ${srcdir}/../PGN/ClassicGames.pgn" | ./Amy

bookup.o: ${srcdir}/bookup.c
	$(CC) $(CFLAGS) $(AM_CFLAGS) $(DEFAULT_INCLUDES) -DBOOKDIR=\"${pkgdatadir}\" \
        ${srcdir}/bookup.c -c -o bookup.o

eco.o: ${srcdir}/eco.c
	$(CC) $(CFLAGS) $(AM_CFLAGS) $(DEFAULT_INCLUDES) -DECODIR=\"${pkgdatadir}\" \
        ${srcdir}/eco.c -c -o eco.o

-include Makefile.deps

Makefile.deps: $(Amy_SOURCES)
	$(CC) $(CFLAGS) $(AM_CFLAGS) $(DEFAULT_INCLUDES) $(Amy_SOURCES) -MM > Makefile.deps

$(OBJS): Makefile

.PHONY: format
format:
	clang-format -i ${Amy_SOURCES} ${EXTRA_Amy_SOURCES} ${noinst_HEADERS}

.PHONY: clean-local
	rm -f Makefile.deps

.PHONY: distclean-local
distclean-local:
	rm -f Eco.db Book.db Amy.log Makefile.deps

.PHONE: check-local
check-local:
	./Amy --test
