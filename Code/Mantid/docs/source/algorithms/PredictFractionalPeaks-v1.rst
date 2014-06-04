.. algorithm::

.. summary::

.. alias::

.. properties::

Description
-----------

This Algorithm creates a PeaksWorkspace with peaks occurring at specific
fractional offsets from h,k,or l values.

There are options to create Peaks offset from peaks from the input
PeaksWorkspace, or to create peaks offset from h,k, and l values in a
range. Zero offsets are allowed if some or all integer h,k, or l values
are desired

The input PeaksWorkspace must contain an orientation matrix and have
been INDEXED by THIS MATRIX when the new peaks are not created from a
range of h ,k, and l values

Example usage
#############

from mantidsimple import \* PeaksWrkSpace=mtd["PeaksQa"]

#. Can be created via PredictPeaks( then do NOT use next line)

FindUBUsingFFT(PeaksWrkSpace,3,15,.12) IndexPeaks(PeaksWrkSpace,.12,1)
PredictFractionalPeaks(PeaksWrkSpace,"FracPeaks","-.5,0,.5","-.5,.5","0")

#. NOTE: There are editing options on PeaksWorkspaces, like combining 2
   PeaksWorkspaces.

.. categories::
