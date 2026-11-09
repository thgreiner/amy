bin_PROGRAMS = Amy
noinst_PROGRAMS = nn_crosscheck

# NB: these lists cannot use $(wildcard)/$(filter): automake 1.19 tries to
# extract concrete file names from _SOURCES values (for the dist list and
# object list) and mangles nested make functions.
# Adding a .c file here is the price of using automake for this.
amy_core_sources = attack_map.c \
                   bitboard.c bpd.c bookup.c commands.c dbase.c eco.c \
                   hashtable.c heap.c \
                   intset_sorted.c init.c learn.c magic.c mates.c movedata.c mytb.cpp \
                   neural_net.c next.c nn_extract.c pgn_read.c pgn_write.c probe.c \
                   random.c recog.c search.c search_io.c state_machine.c swap.c \
                   time_ctl.c tree.c utils.c

amy_test_sources = test_attack_map.c test_bpd.c \
                   test_dbase.c test_intset_sorted.c test_neural_net.c \
                   test_nn_extract.c test_pgn_write.c test_utils.c

Amy_SOURCES = $(amy_core_sources) $(amy_test_sources) main.c
nn_crosscheck_SOURCES = $(amy_core_sources) $(amy_test_sources) nn_crosscheck.c

if HAVE_LINENOISE
Amy_SOURCES += linenoise.c
nn_crosscheck_SOURCES += linenoise.c
endif

# ctypes feature-extraction library for the Python training pipeline
# (see nn_extract.c; PLAN-attack-map-features.md, T2).  Links the
# already-compiled core objects (without the test code) into a shared
# library that the Python side loads with ctypes.  Hand-rolled instead
# of libtool so the same objects can feed both the Amy binary and the
# library; on macOS arm64 plain objects are position-independent.
# Built only on macOS Apple Silicon (see BUILD_NN_EXTRACT in configure.ac).
if BUILD_NN_EXTRACT
nn_extract_objects = $(addsuffix .o, \
	$(basename $(filter-out test_%.c, $(amy_core_sources))))

if HAVE_LINENOISE
nn_extract_objects += linenoise.o
endif

libamy_nn_extract.dylib: $(nn_extract_objects)
	$(CXX) $(CXXFLAGS) $(AM_CFLAGS) -shared -o $@ $(nn_extract_objects)

all-local: libamy_nn_extract.dylib
endif

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

# Note: in out-of-tree builds make vpath-qualifies the source names on
# the command line, so -MM emits qualified targets (src/foo.o); the sed
# normalizes them back to plain object names.
Makefile.deps: $(Amy_SOURCES) nn_crosscheck.c
	$(CC) $(CFLAGS) $(AM_CFLAGS) $(DEFAULT_INCLUDES) $(Amy_SOURCES) \
        nn_crosscheck.c -MM | sed 's|^[^ ]*/\([^:]*\):|\1:|' > Makefile.deps

$(OBJS): Makefile

.PHONY: format
format:
	clang-format -i $(Amy_SOURCES) nn_crosscheck.c $(EXTRA_Amy_SOURCES)

.PHONY: clean-local
clean-local:
	rm -f Makefile.deps libamy_nn_extract.dylib

.PHONY: distclean-local
distclean-local:
	rm -f Eco.db Book.db Amy.log Makefile.deps

.PHONY: check-local
check-local:
	./Amy --test
