.. algorithm::

.. summary::

.. alias::

.. properties::

Description
-----------

This algorithm is the equivalent of the COLETTE "DISPLAY H/V" command.
It firsts integrates the input workspace, which must contain all the
spectra from the detector of interest - no more and no less (so 128x128
or 192x192), between the X values given. Then each row or column is
summed between the H/V\_Min/Max values, if given, and the result is a
single spectrum of row or column number against total counts.

ChildAlgorithms used
####################

The :ref:`algm-Integration` algorithm is used to sum up each
spectrum between XMin & XMax.

.. categories::
