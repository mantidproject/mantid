.. algorithm::

.. summary::

.. relatedalgorithms::

.. properties::

Description
-----------

This Algorithm creates a PeaksWorkspace with peaks occurring at specific
fractional offsets from h,k,or l values.

There are options to create Peaks offset from peaks from the input
PeaksWorkspace, or to create peaks offset from h,k, and l values in a
range. Zero offsets are allowed if some or all integer h,k, or l values
are desired. The order can be 1 for +/-1 or 2 for +/-1 and +/-2, etc.

The input PeaksWorkspace must contain an orientation matrix and have
been INDEXED by THIS MATRIX when the new peaks are not created from a
range of h ,k, and l values

Convention: if includePeaksInRange = true, each new discovered peak's
goniometer matrix will be the goniometer orientation matrix from the
PeaksWorkspace. Each new discovered peak's run number will be the run number
from the PeaksWorkspace.

If includePeaksInRange = false, each new discovered peak's
goniometer matrix and run number will be set to the same values as that
of the peak it was discovered to be a satellite of.


Usage
-----

**Example:**

.. testcode:: TopazExample

   peaks = LoadIsawPeaks("TOPAZ_3007.peaks")
   LoadIsawUB(peaks,"TOPAZ_3007.mat")
   IndexPeaks(peaks)

   fractional_peaks = PredictSatellitePeaks(peaks, ModVector1=[-0.5,0,0.0], MaxOrder=1)
   print("Number of fractional peaks: {}".format(fractional_peaks.getNumberPeaks()))

.. testoutput:: TopazExample

   Number of fractional peaks: 122

.. categories::

.. sourcelink::
