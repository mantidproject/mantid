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


Usage
-----

**Example:**

.. testcode:: TopazExample

   peaks = LoadIsawPeaks("TOPAZ_3007.peaks")
   LoadIsawUB(peaks,"TOPAZ_3007.mat")
   IndexPeaks(peaks)

   fractional_peaks = PredictFractionalPeaks(peaks, HOffset=[-0.5,0,0.5],KOffset=0,LOffset=0.2)
   print "Number of fractional peaks:",fractional_peaks.getNumberPeaks()

.. testoutput:: TopazExample

   Number of fractional peaks: 117
