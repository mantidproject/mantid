.. algorithm::

.. summary::

.. relatedalgorithms::

.. properties::

Description
-----------

This Algorithm indexes a PeaksWorkspace with peaks occurring at specific
fractional offsets from h,k,or l values.
The order can be 1 for +/-1 or 2 for +/-1 and +/-2, etc.
See 
:ref:`ModulatedStructure <ModulatedStructure>` for equations.

The input PeaksWorkspace must contain an orientation matrix.


Usage
-----

**Example:**

.. testcode:: TopazExample

   peaks = LoadIsawPeaks("TOPAZ_3007.peaks")
   LoadIsawUB(peaks,"TOPAZ_3007.mat")
   IndexPeaks(peaks)

   fractional_peaks = PredictSatellitePeaks(peaks, ModVector1=[-0.5,0,0.0], MaxOrder=1)
   IndexPeaksWithSatellites(fractional_peaks, ModVector1=[-0.5,0,0.0], MaxOrder=1)
   fractional_peaks = FilterPeaks(fractional_peaks, FilterVariable='h^2+k^2+l^2', FilterValue=0, Operator='>')
   print("Number of fractional peaks: {}".format(fractional_peaks.getNumberPeaks()))

.. testoutput:: TopazExample

   Number of fractional peaks: 122

.. categories::

.. sourcelink::
