
.. algorithm::

.. summary::

.. relatedalgorithms::

.. properties::

Description
-----------

Do a brute force search for the goniometer rotation angles that maximize the number of peaks indexed by the UB on the peaks workspace.  The peaks must have peaks from only one run.  The search will start with the goniometer position set on the PeaksWoskpace and deviations from that goniometer up to MaxAngle will be tried, seaching for the gonomater angles that maximize the number of peaks indexed and minimize the total indexing error. This algorithm was ported from ISAW.

The best goniometer angles found will be returned in terms of phi, chi, omega ("YZY") and if Apply is selected then the goniometer will be updated on the PeaksWorkspace and the Peaks.


Usage
-----
..  Try not to use files in your examples,
    but if you cannot avoid it then the (small) files must be added to
    autotestdata\UsageData and the following tag unindented
    .. include:: ../usagedata-note.txt

**Example - FindGoniometerAngles**

.. testcode:: FindGoniometerAnglesExample

   # create a peaks workspace with particular lattice and goniometer
   ws = LoadEmptyInstrument(InstrumentName="TOPAZ")
   SetGoniometer(ws, Axis0="45,0,0,1,1")
   SetUB(ws, a=5, b=5, c=5, alpha=90, beta=90, gamma=90)
   peaks = PredictPeaks(ws)
   num_peaks = peaks.getNumberPeaks()

   # create new peaks workspace with wrong goniometer
   new_peaks = CreatePeaksWorkspace(ws, 0)
   SetGoniometer(new_peaks, Axis0="42,0,0,1,1")
   SetUB(new_peaks, a=5, b=5, c=5, alpha=90, beta=90, gamma=90)

   # copy peak q_lab vectors, q_sample will be wrong since wrong goniometer
   from mantid.kernel import SpecialCoordinateSystem
   for i in range(num_peaks):
       new_peaks.addPeak(peaks.getPeak(i).getQLabFrame(), SpecialCoordinateSystem.QLab)

   results = IndexPeaks(new_peaks, 0.05)
   print(f"Starting with {results.NumIndexed} out of {num_peaks} peaks indexed")
   angles = FindGoniometerAngles(new_peaks, Apply=True)
   results = IndexPeaks(new_peaks, 0.05)
   print(f"Found Chi={angles.Chi:.2f} and there are now {results.NumIndexed} out of {num_peaks} peaks indexed")

Output:

.. testoutput:: FindGoniometerAnglesExample

  Starting with 0 out of 65 peaks indexed
  Found Chi=44.99 and there are now 65 out of 65 peaks indexed

.. categories::

.. sourcelink::
