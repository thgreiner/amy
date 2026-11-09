# All public headers are distributed; new files are picked up
# automatically, no list to maintain.  $(srcdir) keeps this
# correct for out-of-tree builds (make distcheck).
noinst_HEADERS = $(notdir $(wildcard $(srcdir)/*.h))

.PHONY: format
format:
	clang-format -i $(wildcard $(srcdir)/*.h)
