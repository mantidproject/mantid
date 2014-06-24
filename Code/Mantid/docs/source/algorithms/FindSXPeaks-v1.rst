.. algorithm::

.. summary::

.. alias::

.. properties::

Description
-----------

Detector-space, single crystal peak finding. Finds peaks by searching
through each spectra and looking for high intensity bins. If a bin has
high intensity it is a candidate for a peak.

Notable points:

-  The highest intensity bin is taken to be the peak, so only finds one
   peak per spectra
-  Peaks that are not above the background are culled. The background is
   calculated as the average of the start and end intensity multiplied
   by the provided SignalBackground parameter.
-  Calculated Qlab follows the Busy, Levy 1967 convention.

Usage
-----

**Example**

.. testcode:: ExFindSXPeaksSimple

   # create histogram workspace
   ws=CreateSampleWorkspace()
   
   wsPeaks = FindSXPeaks(ws)

   print "Peaks found: " + str(wsPeaks.getNumberPeaks())

Output:

.. testoutput:: ExFindSXPeaksSimple

   Peaks found: 174

.. categories::
