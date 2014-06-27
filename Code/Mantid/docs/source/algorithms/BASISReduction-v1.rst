.. algorithm::

.. summary::

.. alias::

.. properties::

Description
-----------

This algorithm is meant to temporarily deal with letting BASIS reduce
lots of files via Mantid. The syntax for the run number designation will
allow groups of runs to be joined. Examples:

1. 2144-2147,2149,2156
2. 2144-2147;2149;2156

Example 1 will be summed into a single run. Example 2 will have three run groups.

Usage
-----

.. warning::

    This algorithm is not meant to be run from the command line.

.. categories::
