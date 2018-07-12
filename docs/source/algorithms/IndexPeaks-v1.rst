.. algorithm::

.. summary::

.. relatedalgorithms::

.. properties::

Description
-----------

Given a PeaksWorkspace with a :ref:`UB matrix <Lattice>` stored with
the sample, this algorithm will use UB inverse to index the peaks. If
there are peaks from multiple runs in the workspace, the stored
:ref:`UB matrix <Lattice>` will be used to get an initial indexing for
the peaks from each individual run. Subsequently, a temporary
:ref:`UB matrix <Lattice>` will be optimzed for the peaks from each
individual run, and used to index the peaks from that run. In this way,
a consistent indexing of the peaks from multiple runs will be obtained,
which indexes the largest number of peaks, although one
:ref:`UB matrix <Lattice>` may not produce exactly that indexing for all
peaks, within the specified tolerance.

To not optimize the UB for each run, use the option, CommonUBForAll.  In
this case, the same UB stored in the workspace will be used to index
all the peaks with no optimization.

Any peak with any Miller index more than the specified tolerance away
from an integer will have its (h,k,l) set to (0,0,0). The calculated
Miller indices can either be rounded to the nearest integer value, or
can be left as decimal fractions.

Usage
-----

**Example - a simple example of IndexPeaks**

.. include:: ../usagedata-note.txt

.. testcode:: ExIndexPeaksSimple

   # Load Peaks found in SXD23767.raw
   Load(Filename='SXD23767.peaks',OutputWorkspace='peaks_qLab')

   #Set orientated lattice
   ubMatrix = [-0.06452276,  0.2478114,  -0.23742194, 0.29161678, -0.00914316, -0.12523779, 0.06958942, -0.1802702,  -0.14649001]
   SetUB('peaks_qLab',UB=ubMatrix)

   # Run Algorithm
   indexed =IndexPeaks(PeaksWorkspace='peaks_qLab',Tolerance=0.12,RoundHKLs=1)

   print("Number of Indexed Peaks: {:d}".format(indexed[0]))

Output:

.. testoutput:: ExIndexPeaksSimple

   Number of Indexed Peaks: 147

Related Algorithms
------------------

:ref:`IndexSXPeaks <algm-IndexSXPeaks>`
will index peaks given a PeaksWorkspace and lattice parameters

.. categories::

.. sourcelink::
