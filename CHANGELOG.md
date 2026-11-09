# Change Log

## [0.9.8]

* Replace the classical heuristic evaluation with a quantized neural
  network (NNUE): 256-wide accumulator network with half-mirrored
  piece framing, separate pawn and attack features, and 4 per-phase
  hidden/output branches (selected by total piece count of knights,
  bishops, rooks, and queens of both sides)
* New `network.quant` weights file format (LZAV compressed); a
  retrained network is required. The engine now identifies as
  "Amy <version>-neural"
* Remove the classical evaluation, its parameter configuration
  (`conf`, `conf-save`, `-conf`) and the YAML parser
* Training pipeline support: log root position evaluations to
  `evaluation_log.bin` (new `evallog` rc option, binary BPD format),
  new `bpd-test` command to write test positions in the same format,
  `nn_crosscheck` tool and `nn_extract` library to verify and
  reproduce feature generation against the Python reference
* New `self` command: play N games of selfplay, appended to
  `selfplay.pgn`
* New `fuzz` rc option: adds randomness to book move selection and
  search
* Hashtables: use R/W-locks instead of mutexes, store the full
  position key in entries, fix CPU time increment in MP mode
* Use linenoise for interactive line editing on POSIX systems
* New logo
* Bug fix: xboard mode
* Convert eco.pgn to UTF-8
* Remove obsolete `--with-dbpath` (rocksdb) configure option


## [0.9.7] 2025-01-08

* Refactor connected passed pawn scoring
* Bug fix: opposite colored bishop evaluation
* Bug fix: promotion piece parsing


## [0.9.6] 2025-01-07

* Refactored search move generation
* Replaced several lookup tables by small functions
* Fixed: "new" command should reset time ([#11](https://github.com/thgreiner/amy/issues/11))
* Fixed several bugs in the evaluation function detected by symmetry testing
* Started work on reading evaluation parameters from YAML file (unfinished)
* Changes for Windows compatibility contributed by Jim Ablett


## [0.9.5] 2024-12-10

* Fixed a lot of compiler warnings
* Refactored MP hashtable code
* Attack generation now uses magic bitboards
* Replace the docbook version of the handbook with a markdown version


## [0.9.4] 2024-11-27

* Removed BerkeleyDB and implemented AVL tree for opening book
  and ECO database


## [0.9.3] 2021-03-14

* Several bug fixes and clean ups
* Modernized autoconf / automake setup
* Refactored bitbanging code


## [0.9.2]

* Added selfplay
* Use clang-format to format code


## [0.9.1]

* Several bug fixes and clean ups
* split single mutex into multiple ones to improve MP performance


## [0.8.7]

* Bug fixes and clean up in the evaluation function
* Revert changes to search extensions made between 0.8.4 and 0.8.6


## [0.8.6]

* Try to really fix the XBoard/WinBoard bug. Thanks to Leo Dijksmann for
  pointing me in the right direction.


## [0.8.5]

* Try to work around a bug in XBoard/WinBoard related to long PV output


## [0.8.4]

* Added 'perft' command
* Added simple book learning
* Truly randomize book selection
* Add documentation in docbook format


## [0.8.3]

* Changes to evaluation function
* Removed --enable-asm option, since I am too stupid to configure 
  automake/autoconf to support it in a sensible way.


## [0.8.2]

* Merged timing changes by Allen Lake
* Use xboard "feature" command


## [0.8.1]

* Bug fix in timing code
* Bug fix in analysis mode
* README fixes


## [0.8]

* Now recognizes Amy.ini *and* .amyrc
* IMPORTANT: Copyright notice for Eugene Nalimov's tablebase code


## [0.7]

* Fixed a couple of minor glitches which were discovered by Dann Corbit when
  porting Amy to Windows
* Fixed several bugs in the parallel search (EGTB code, pawnFacts)
* Added analyze mode
* Added .amyrc
* New time allocation strategy
* Changes to book handling
* Several minor changes to the evaluation function

