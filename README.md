![main branch](https://github.com/thgreiner/amy/actions/workflows/c-cpp.yml/badge.svg)

What is Amy?
============

Amy is a chess playing program.  It is compatible with xboard, uses endgame
table bases and an opening book.

Copyright
=========

Amy is distributed under the BSD 2-Clause License. You should find a file
'LICENSE' in the Amy distribution containing this license.

Please note that Amy uses table base code developed and copyrighted by
Eugene Nalimov. This code is *not* under the BSD License.

Building Amy
============

On Un*x or Linux systems this is very simple:

	./configure
	make

should do it. Make sure to specify compiler options with good optimization, I
typically use something like (assuming a bash or ksh shell):

	export CFLAGS='-O2 -march=native'
	export CXXFLAGS=$CFLAGS
	./configure
	make

On Windows systems:

Thanks to Dann Corbit (dcorbit@solutionsiq.com) Amy will now compile under
Windows. Please do not ask me any details about how to get this going.


Building a parallel version
===========================

Amy uses ABDADA as a parallel search algorithm. Please see 'search.c' for
reference. To enable ABDADA pass the "--enable-mt" to configure.

Note that you need 'POSIX THREADS' on your system to use the parallel search.


Invoking Amy
============

Just type 'Amy'. You can also specify a hashtable size:

	Amy -ht 10m
	
will use 10 MB of hashtables. If you build a parallel version, you can supply
the number of processors (or threads rather) Amy should use:

	Amy -ht 10m -cpu 2

Note that you can specify these options via an '.amyrc' file, too. See below.
	
 

Creating and using opening books
================================

To create a book from a PGN file, first create the ECO database. At Amy's
prompt, type 'eco PGN/eco.pgn'. Verify that it works: Type 

	new
	force
	e4
	e

the output should be 

	Eco code is B00 King's pawn opening

You can now use the 'bookup' command to create an opening book from a PGN file:

	bookup ClassicGames.pgn

This command will create an opening book from the file "ClassicGames.pgn".

Since book files tend to become very large, you can make them smaller by using
the 'flatten' command. Typing

	flatten 1

will create a file 'Book2.db' which contains all positions from 'Book.db' which
occured more than one time. This typically reduces book size to 1/10th! To use
the new book, simply rename it to 'Book.db' and restart Amy.


Setting opening book preferences
================================

You want Amy to play King's gambit no longer? No problem! Just create a file
containing the lines

	e4 e5 f4?

and use 

	prefs filename

to read it in. Amy comes complete with a file 'Preferences' which avoids some
opening traps and known bad lines. Just use

	prefs Preferences

to set these.


Using book learning
===================

Starting with version 0.8.4 Amy features a simple version of book learning.
It uses the autosave files created by Amy to update the opening book statistics.
For this to work, game autosaving must be activated (see section "Setting 
options via .amyrc").


Using endgame tablebases
========================

Create a directory called 'TB' in the distribution directory. Put your 
tablebase files there. They will be recognized automagically. See also the
section on '.amyrc' below.


Setting options via .amyrc / Amy.ini
====================================

Amy supports an .amyrc file to set several options. The .amyrc file should be
in the current directory when starting amy. Here is a sample .amyrc:

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

Since people using Windows have reported that they have to resort to DOS mode
for creating a .amyrc file, Amy also looks for Amy.ini.


THANKS
======

- to Dann Corbit for porting Amy to Win32

- to Allen Lake for tuning Amy's timing algorithm

