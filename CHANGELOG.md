# Change Log

## [0.9.5]

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

