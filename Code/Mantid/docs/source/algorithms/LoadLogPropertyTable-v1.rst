.. algorithm::

.. summary::

.. alias::

.. properties::

Description
-----------

Creates a table workspace of the average values of log values against
the run number.

There are special cases for:

-  beamlog\_(counts, frames, etc): last few points end up in next run's
   log. Find Maximum.
-  comment (separate function)
-  time series, take average for t>0 (if available)

It should:

#. Load any file type that :ref:`algm-Load` can handle.
#. Not crash with multiperiod data - although values will be from period
   1
#. Handle gaps in the file structure (although this can be slow over a
   network if you choose a range of 100s)
#. Load only a single spectra of the data (if the file loader supports
   this).
#. Print out the list of acceptable log names if one is entered
   incorrectly.
#. Use a hidden workspace for the temporary loaded workspaces, and clean
   up after itself.

.. categories::
