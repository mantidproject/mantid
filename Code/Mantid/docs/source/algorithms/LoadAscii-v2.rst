.. algorithm::

.. summary::

.. alias::

.. properties::

Description
-----------

The LoadAscii2 algorithm reads in spectra data from a text file and
stores it in a `Workspace2D <Workspace2D>`__ as data points. The data in
the file must be organized in columns separated by commas, tabs, spaces,
colons or semicolons. Only one separator type can be used throughout the
file; use the "Separator" property to tell the algorithm which to use.
The algorithm `SaveAscii2 <SaveAscii2>`__ is normally able to produce
such a file.

The format must be:

-  A single integer or blank line to denote a new spectra
-  For each bin, between two and four columns of delimted data in the
   following order: 1st column=X, 2nd column=Y, 3rd column=E, 4th
   column=DX (X error)
-  Comments can be included by prefixing the line with a non-numerical
   character which must be consistant throughout the file and specified
   when you load the file. Defaults to "#"
-  The number of bins is defined by the number of rows and must be
   identical for each spectra

The following is an example valid file of 4 spectra of 2 bins each with
no X error

#. X , Y , E

1 2.00000000,2.00000000,1.00000000 4.00000000,1.00000000,1.00000000 2
2.00000000,5.00000000,2.00000000 4.00000000,4.00000000,2.00000000 3
2.00000000,3.00000000,1.00000000 4.00000000,0.00000000,0.00000000 4
2.00000000,0.00000000,0.00000000 4.00000000,0.00000000,0.00000000

.. categories::
