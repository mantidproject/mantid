.. algorithm::

.. summary::

.. alias::

.. properties::

Description
-----------

**Parameters Note:** Note that it is possible to use both of the
optional 'spectrum' properties (i.e. a range and a list) together if so
desired.

Load ISIS log file(s)
#####################

Assumes that a log file originates from a PC (not VMS) environment, i.e.
the log files to be loaded are assumed to have the extension .txt. Its
filename is assumed to starts with the raw data file identifier followed
by the character '\_', and a log file is assumed to have a format of two
columns, where the first column consists of data-time strings of the ISO
8601 form and the second column consists of either numbers or strings
that may contain spaces.

Parent algorithm
################

LoadLog is also a child algorithm of :ref:`algm-LoadRaw`, i.e. it gets
called whenever LoadRaw is executed.

Load SNS text log file
######################

If the file is determined to be a SNS text log file it should be of the
form

| ``655747325.450625   0.000000    24.000000   26.000000   0.000000``
| ``655747325.716250   0.296875    24.000000   26.000000   0.000000``
| ``655747325.997500   0.593750    24.000000   26.000000   0.000000``
| ``655747326.263125   0.906250    24.000000   26.000000   0.000000``
| ``655747326.544375   1.093750    24.000000   26.000000   0.000000``
| ``655747326.825625   1.406250    24.000000   26.000000   0.000000``
| ``655747327.091250   1.703125    24.000000   26.000000   0.000000``
| ``655747327.372500   2.000000    24.000000   26.000000   0.000000``
| ``655747327.638125   2.203125    24.000000   26.000000   0.000000``
| ``655747327.919375   2.500000    24.000000   26.000000   0.000000``
| ``655747328.200625   2.796875    24.000000   26.000000   0.000000``
| ``655747328.466250   3.093750    24.000000   26.000000   0.000000``

The first column is the number of seconds since January 1, 1990, then
the other columns (space delimited) are the log values. For this mode
the *name* and *units* parameters must be specified.

.. categories::
