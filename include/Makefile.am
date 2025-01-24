noinst_HEADERS = amy.h bitboard.h bookup.h commands.h dbase.h eco.h evaluation_config.h \
                 heap.h learn.h magic.h tree.h \
                 hashtable.h init.h inline.h mates.h movedata.h next.h evaluation.h pgn.h \
                 probe.h recog.h random.h search.h search_io.h state_machine.h swap.h \
                 test_dbase.h test_yaml.h time_ctl.h types.h utils.h yaml.h

.PHONY: format
format:
	clang-format -i ${noinst_HEADERS}
