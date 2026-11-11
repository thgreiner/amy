![main branch](https://github.com/thgreiner/amy/actions/workflows/c-cpp.yml/badge.svg)
![License](https://img.shields.io/github/license/thgreiner/amy)

What is Amy?
============

Amy is a chess engine. It speaks the XBoard protocol and uses endgame
tablebases and an opening book. Since version 0.9.8 Amy uses a neural net
for evaluation.

Copyright
=========

Amy is distributed under the BSD 2-Clause License. You should find a file
`LICENSE` in the Amy distribution containing this license.

Please note that Amy uses table base code developed and copyrighted by
Eugene Nalimov. This code is *not* under the BSD License.

Building Amy
============

On Un*x or Linux systems this is very simple:

	./configure
	make


Specify optimization when configuring: compile with `-O3` (ideally together
with `-march=native`) so that the neural net arithmetic can be vectorized to
AVX (x86) / NEON (ARM) instructions; otherwise performance will be much
lower:

	export CFLAGS='-O3 -march=native'
	export CXXFLAGS=$CFLAGS
	./configure
	make

On Windows systems:

Thanks to Dann Corbit (dcorbit@solutionsiq.com) Amy will now compile under
Windows. Please do not ask me any details about how to get this going.


Building a parallel version
===========================

Amy uses ABDADA as a parallel search algorithm. Please see `search.c` for
reference. To enable ABDADA pass `--enable-mt` to configure.

Note that you need POSIX threads (pthreads) on your system to use the parallel search.


Invoking Amy
============

Just type `Amy`. To use a 10 MB hashtable:

	Amy -ht 10m

If you build a parallel version, you can supply the number of threads
Amy should use:

	Amy -ht 10m -cpu 2

Note that you can specify these options via an `.amyrc` file, too. See below.

Creating and using opening books
================================

To create a book from a PGN file, first create the ECO database. At Amy's
prompt, type `eco PGN/eco.pgn`. Verify that it works: type

	new
	force
	e4
	e

the output should be

	Eco code is B00 King's pawn opening

You can now use the `bookup` command to create an opening book from a PGN file:

	bookup PGN/ClassicGames.pgn

Since book files tend to become very large, you can make them smaller by using
the `flatten` command. Typing

	flatten 1

will create a file `Book2.db` which contains all positions from `Book.db` which
occurred more than one time. This typically reduces the book size by a factor
of 10! To use the new book, simply rename it to `Book.db` and restart Amy.


Setting opening book preferences
================================

Want Amy to stop playing the King's Gambit? No problem! Just create a file
containing the lines

	e4 e5 f4?

and use

	prefs filename

to read it in. Amy comes complete with a file `Preferences` which avoids some
opening traps and known bad lines. Just use

	prefs Preferences

to set these.


Using book learning
===================

Starting with version 0.8.4 Amy has a simple book learning feature.
It uses the autosave files created by Amy to update the opening book statistics.
For this to work, game autosaving must be activated (see section "Setting
options via .amyrc").


Using endgame tablebases
========================

Create a directory called `TB` in the distribution directory. Put your
tablebase files there. They will be recognized automatically. See also the
section on `.amyrc` below.


Setting options via .amyrc / Amy.ini
====================================

Amy supports an `.amyrc` file to set several options. The `.amyrc` file
should be in the current directory when starting Amy. Here is a sample
`.amyrc`:

```
#
# Sample .amyrc
#
# Use 20 MB of hashtables:
ht=20m
#
# Look for tablebases on /space/TB
tbpath=/space/TB
#
# Use 2 processors for parallel search
cpu=2
#
# Enable game autosaving to allow booklearning
autosave=true
#
# Log root position evaluations to evaluation_log.bin
# (also done unconditionally during selfplay)
evallog=true
```

Since people using Windows have reported that they have to resort to DOS-style
line endings (CRLF) for creating a .amyrc file, Amy also looks for Amy.ini.


NNUE evaluation
===============

The trained network is loaded at startup. The compressed format is
preferred, so the following paths are tried in order:

1. `nets/amy.net.z` in the working directory — dev workflow, and an
   override for installed binaries;
2. the installed `amy.net.z` in `$(pkgdatadir)` (e.g.
   `/usr/local/share/amy/amy.net.z` after `make install`);
3. `nets/amy.net` in the working directory (uncompressed);
4. the installed `amy.net` in `$(pkgdatadir)`.

If no net is found, Amy prints "No net found." and keeps running, but
with zeroed weights the evaluation is a constant (bias only) that does
not depend on the position.

The training pipeline (Python/MLX) lives in `training-mlx/` — see
`training-mlx/README.md` and `training-mlx/AGENTS.md` for setup, usage,
and the bit-exact verification against the C engine.


Thanks
======

- to Dann Corbit for porting Amy to Win32

- to Allen Lake for tuning Amy's timing algorithm
