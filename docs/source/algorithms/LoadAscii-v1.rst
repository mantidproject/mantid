.. algorithm::

.. summary::

.. relatedalgorithms::

.. properties::

Description
-----------

The LoadAscii algorithm reads in spectra data from a text file and
stores it in a :ref:`Workspace2D <Workspace2D>` as data points. The data in
the file must be organized in columns separated by commas, tabs, spaces,
colons or semicolons. Only one separator type can be used throughout the
file; use the "Separator" property to tell the algorithm which to use.
The algorithm :ref:`algm-SaveAscii` is normally able to produce such
a file.

By default the algorithm attempts to guess which lines are header lines
by trying to see where a contiguous block of numbers starts. This can be
turned off by specifying the "SkipNumLines" property, which will then
tell the algorithm to simply use that as the number of header lines.

The format can be one of:

-  Two columns: 1st column=X, 2nd column=Y, E=0
-  For a workspace of *n* spectra, 2\ *n*\ +1 columns: 1\ *st* column=X,
   2i\ *th* column=Y, 2i+1\ *th* column =E
-  Four columns: 1st column=X, 2nd column=Y, 3rd column=E, 4th column=DX
   (X error)

The number of bins is defined by the number of rows.

The resulting workspace will have common X binning for all spectra.

This algorithm cannot load a file created by :ref:`algm-SaveAscii`
if it has X errors written and several spectra.

.. categories::

.. sourcelink::
