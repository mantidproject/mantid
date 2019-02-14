.. algorithm::

.. summary::

.. relatedalgorithms::

.. properties::

Description
-----------

This algorithm basically optimizes h,k, and l offsets from an integer by
varying the parameters sample positions for each run in the peaks workspace.

The crystal orientation matrix, :ref:`UB matrix <Lattice>`, from the
PeaksWorkspace should index all the runs "very well". Otherwise iterations
that slowly build a :ref:`UB matrix <Lattice>` with corrected sample
orientations may be needed.


Usage
-----

**Example:**

.. testcode:: ExOptimizeCrystalPlacementByRun

   ws=LoadIsawPeaks("calibrated.peaks")
   FindUBUsingFFT(PeaksWorkspace=ws,MinD=2,MaxD=20,Tolerance=0.12)
   IndexPeaks(PeaksWorkspace='ws',Tolerance=0.12)
   wsd = OptimizeCrystalPlacementByRun(InputWorkspace=ws,OutputWorkspace='wsd',Tolerance=0.12)
   print('Optimized %s sample position: %s'%(mtd['wsd'].getPeak(0).getRunNumber(),mtd['wsd'].getPeak(0).getSamplePos()))
   print('Optimized %s sample position: %s'%(mtd['wsd'].getPeak(8).getRunNumber(),mtd['wsd'].getPeak(8).getSamplePos()))

Output:

.. testoutput:: ExOptimizeCrystalPlacementByRun

   Optimized 71907 sample position: [-0.000678629,-2.16033e-05,0.00493278]
   Optimized 72007 sample position: [-0.0027929,-0.00105681,0.00497094]


.. categories::

.. sourcelink::
