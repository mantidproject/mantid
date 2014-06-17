.. algorithm::

.. summary::

.. alias::

.. properties::

Description
-----------

Given a PeaksWorkspace with a UB matrix stored with the sample, this
algorithm will use UB inverse to index the peaks. If there are peaks
from multiple runs in the workspace, the stored UB will be used to get
an initial indexing for the peaks from each individual run.
Subsequently, a temporary UB will be optimzed for the peaks from each
individual run, and used to index the peaks from that run. In this way,
a consistent indexing of the peaks from multiple runs will be obtained,
which indexes the largest number of peaks, although one UB may not
produce exactly that indexing for all peaks, within the specified
tolerance.

Any peak with any Miller index more than the specified tolerance away
from an integer will have its (h,k,l) set to (0,0,0). The calculated
Miller indices can either be rounded to the nearest integer value, or
can be left as decimal fractions.

.. categories::
