noinst_HEADERS = amy.h bitboard.h blunder.h bookup.h commands.h dbase.h eco.h \
                 evaluation.h evaluation_config.h filter.h hashtable.h heap.h \
                 init.h inline.h learn.h magic.h mates.h movedata.h next.h \
                 pgn_read.h pgn_write.h probe.h random.h recog.h \
                 safe_malloc.h search.h search_io.h state_machine.h swap.h \
                 test_blunder.h test_dbase.h test_pgn_write.h test_utils.h test_yaml.h time_ctl.h tree.h \
                 types.h utils.h yaml.h

.PHONY: format
format:
	clang-format -i ${noinst_HEADERS}
