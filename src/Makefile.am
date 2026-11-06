bin_PROGRAMS = Amy
noinst_PROGRAMS = nn_crosscheck

amy_core_sources = attack_map.c \
                   bitboard.c blunder.c bpd.c bookup.c commands.c dbase.c eco.c \
                   evaluation.c evaluation_config.c filter.c hashtable.c heap.c \
                   intset_sorted.c init.c learn.c magic.c mates.c movedata.c mytb.cpp \
                   neural_net.c next.c nn_extract.c pgn_read.c pgn_write.c probe.c \
                   random.c recog.c search.c search_io.c state_machine.c swap.c \
                   time_ctl.c tree.c utils.c yaml.c

amy_test_sources = test_attack_map.c test_blunder.c test_bpd.c \
                   test_dbase.c test_intset_sorted.c test_neural_net.c \
                   test_nn_extract.c test_pgn_write.c test_utils.c test_yaml.c

Amy_SOURCES = $(amy_core_sources) $(amy_test_sources) main.c
nn_crosscheck_SOURCES = $(amy_core_sources) $(amy_test_sources) nn_crosscheck.c

if HAVE_LINENOISE
Amy_SOURCES += linenoise.c
nn_crosscheck_SOURCES += linenoise.c
endif

Amy_DEPENDENCIES = attack_map.o \
                   bitboard.o blunder.o bpd.o bookup.o commands.o dbase.o eco.o \
                   evaluation.o evaluation_config.o filter.c hashtable.o \
                   heap.o init.o learn.o magic.o main.o mates.o movedata.o \
                   mytb.o neural_net.o next.o nn_extract.o pgn_read.o pgn_write.o probe.o \
                   random.o recog.o search.o search_io.o state_machine.o swap.o \
                   test_attack_map.o test_blunder.o test_bpd.o test_dbase.o \
                   test_intset_sorted.o test_neural_net.o test_nn_extract.o \
                   test_pgn_write.o test_utils.o test_yaml.o time_ctl.o \
                   tree.o utils.o yaml.o

# ctypes feature-extraction library for the Python training pipeline
# (see nn_extract.c; PLAN-attack-map-features.md, T2).  Links the
# already-compiled core objects (without the test code) into a shared
# library that the Python side loads with ctypes.  Hand-rolled instead
# of libtool so the same objects can feed both the Amy binary and the
# library; on macOS arm64 plain objects are position-independent.
nn_extract_objects = $(addsuffix .o, \
	$(basename $(filter-out test_%.c, $(amy_core_sources))))

if HAVE_LINENOISE
nn_extract_objects += linenoise.o
endif

libamy_nn_extract.dylib: $(nn_extract_objects)
	$(CXX) $(CXXFLAGS) $(AM_CFLAGS) -shared -o $@ $(nn_extract_objects)

all-local: libamy_nn_extract.dylib

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

Makefile.deps: $(Amy_SOURCES) nn_crosscheck.c
	$(CC) $(CFLAGS) $(AM_CFLAGS) $(DEFAULT_INCLUDES) $(Amy_SOURCES) \
        nn_crosscheck.c -MM > \
        Makefile.deps

$(OBJS): Makefile

.PHONY: format
format:
	clang-format -i ${Amy_SOURCES} nn_crosscheck.c ${EXTRA_Amy_SOURCES} ${noinst_HEADERS}

.PHONY: clean-local
clean-local:
	rm -f Makefile.deps libamy_nn_extract.dylib

.PHONY: distclean-local
distclean-local:
	rm -f Eco.db Book.db Amy.log Makefile.deps

.PHONE: check-local
check-local:
	./Amy --test
