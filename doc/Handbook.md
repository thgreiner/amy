# Installation

## Introduction

The author releases Amy in source code only. Precompiled packages are
available for FreeBSD and Windows systems. If you want to use Amy on a
different operating system (e.g. Linux) or want to enable certain
runtime features (e.g. parallel search on multiprocessor systems) you
will want to compile Amy yourself.

## Unix / Linux systems

### Prerequisites

Amy uses the Berkeley DB library ([](http://www.sleepycat.com)) software
to store the opening book. If you plan to use Amy with an opening book
(which is quite likely), you should install the Berkeley DB software
prior to compiling Amy. Note that there are several revisions of the
Berkeley DB software. Amy is known to work with revision 2 software
(tested with versions 2.7.3 and 2.7.7).

### Compiling from source

Amy uses the autoconf utility to configure operating system dependant
details. The standard procedure to build Amy is to issue the commands
`./configure && make && make install`. This will configure Amy for your
platform, compile and install it in `/usr/local`.

| Option | Description |
|----|----|
| `--with-dbpath=path` | Allows you to specify the directory where the Berkeley DB software is installed. |
| `--enable-mt` | Enable multithreaded search. |

autoconf parameters

### Using the FreeBSD port

On FreeBSD there is an Amy port located in `/usr/ports/games/amy`. To
install Amy issue the command `cd /usr/ports/games/amy && make install`.

## Mac OS X

Joshua Shriver reported that Amy works 'out of the box' using the usual
`./configure` and make approach on OS X. He recommends setting `CFLAGS`
to "-O3 -faltivec -mcpu=7450" on a G4 Mac.

## Windows systems

Thanks to Dann Corbit there are several precompiled versions of Amy for
Windows operating systems available on his ftp site.

# Configuration

## Introduction

Several runtime options of Amy can be configured by a resource file. Amy
looks for the resource file in the current directory. The resource file
is named `.amyrc` (typically used on Unix/Linux platforms) or `Amy.ini`
(typically used on Windows).

A typical resource file might look like this:

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

| Option | Description |
|----|----|
| autosave | If set to `true` games played by Amy will be automatically saved. This also enables booklearning. |
| cpu | Specifies the number of cpu to use for parallel search. |
| ht | Determines the size of the hashtable. Use the suffixes `k` to specify the size in kilobytes or `m` to specify the size in megabyes. |
| tbpath | Specifies the path were the endgame tablebases are located. |

Resource file entries

# Using the command line interface

## Introduction

Several features of Amy (e.g. opening book creation, running test
suites) are accessible only via the command line interface.

## Creating an opening book

Opening books are created or updated using the `bookup` command. It
expects the filename of an PGN file containing one or more games in the
PGN format. While parsing the PGN file Amy will output a dot for every
hundred games red as a progress indicator.

    White(1): bookup ClassicGames.pgn
       Parsing PGN file PGN/ClassicGames.pgn. '.'= 100 Games
    ......................................................................(7000)
    ...........................................................(12936)

The opening book created is stored in a file name `Book.db`.

## Flattening an opening book

An opening book created from a PGN file will contain many lines which
will never be played by Amy since they occur only once in the book. You
can remove these lines by flattening the book. This process removes all
entries which occur less frequently than some threshold.

    White(1): flatten 5
    Flattening book with threshold 5
    Read 183189 entries, wrote 5272 entries

The `flatten` command produces a file `Book2.db`. This file can be used
instead of the standard opening book `Book.db`.

## Running test suites

You can run a test suite in EPD format with the `test` command.
Typically you will first set a time limit using the `level` as in the
following example:

    White(1): level fixed/5
    White(1): test EPD/WAC.epd 
    …
    solved 293 out of 300  (BT2630 = 2609, LCT2 = 10690, BS2830 = 2794)
    -----------------------------------------------

The output reproduced above is from a real test run on a Intel(R)
Core(TM) i7-1165G7 @ 2.80GHz laptop. Out of the 300 test positions
included in the "Win At Chess" test suite Amy solved 293 at 1 second per
test.

To simplify the evaluation of several standard test suites Amy outputs
the scores as calculated by the formulae given for the test suites
BT2630, LCT2 and BS2830.

# Using a graphical user interface

Amy supports the `xboard` chess engine interface which is used by
`xboard` and `winboard`. This includes the `feature` (since 0.8.3) to
inform `xboard` about the features Amy supports.

To use Amy with `xboard` use the `-fcp` like this: `xboard -fcp Amy`.
This example assumes that your `PATH` variable contains the path to the
Amy executable.
