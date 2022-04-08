
pyL04 -- a graphical user interface to RuG/L04 written in Python


Usage
=====

Start demo...

    Windows: 

            double-click on pyL04.py or pyL04.exe

    Unix:

            Make sure you have the other RuG/L04 programs in your PATH

            chmod +x pyL04.py

            ./pyL04.py

    OS X:   ?

From the program menu...

    select: File -> Open -> demo -> Project.ini

    double-click on a line starting with EPS


Requirements
============

Python: a recent version (at least 2.3), standard install. The GUI is
written on Linux with Python 2.3.4, tested on Windows98 with Python
2.4a3.2, and tested on Linux, Windows98 and Windows XP with Python 2.4.1.
If you don't want pyL04 to hang while it is doing calculations, make
sure you have at least Python 2.4.1.

On Windows: you don't need Python if you downloaded pyL04exe.zip.

    [ http://www.python.org/ ]


A program to view PostScript figures. On Unix: gv in your PATH. ON
Windows: through file type association.

    [ http://www.cs.wisc.edu/~ghost/index.html ]


Issues
======

On Windows: do NOT rename pyL04.py to pyL04.pyw. The program needs the
terminal for output.

Using threads causes gv to hang in Python 2.3.4 (and older?). This
doesn't happen in Python 2.4.1. So I disabled threads in pyL04 when run
with a Python older than 2.4.1. This means that the program stops
responding while it is running a make.

Currently, there are two languages supported: English and Dutch. If a
local setting is detected for Dutch, the program will run in Dutch,
otherwise it will run in English. Some dialog texts are built-in within
Python/Tkinter, and beyond the control of the program. Detection of
language preference under Windows is done by querying the registry. This
was only tested with Windows98 and Windows XP. It might not work with
other Windows. On other platforms, language preference is determined by
the LANG environment variable.


To do
=====

- Document code

- More output tasks:
  * CC Map
  * MDS/cluster plot

- Additional tools:
  * Defining maps
  * Data conversion: import from spreadsheet 
  * View alignments
