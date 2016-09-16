.. algorithm::

.. summary::

.. alias::

.. properties::

Description
-----------

Detector-space, single crystal peak finding. Finds peaks by searching
through each spectra and looking for the highest intensity bin within
a given x range.

Notable points:

-  The highest intensity bin is taken to be the peak, so the algorithm only finds one
   peak per spectra.
-  Peaks that are not above the background are culled. The background is
   calculated as the average of the start and end intensity multiplied
   by the provided SignalBackground parameter.
-  Calculated Qlab follows the Busy, Levy 1967 convention.
-  The resolution parameter defines a tolerance which is compared to the absolute difference
   between the parameters :math:`\phi`, :math:`2\theta`, and :math:`t` of two found peaks. 
   If the absolute difference between any of the parameters for two peaks is greater than the 
   product of the tolerance value and the parameter value then the two peaks are classed as 
   not the same. i.e. if :math:`|\phi_1 - \phi_2| > tolerance * \phi_1` then peaks 1 & 2 are 
   not the same (as well as similar definitions for :math:`2\theta` and :math:`t`).

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

.. sourcelink::
