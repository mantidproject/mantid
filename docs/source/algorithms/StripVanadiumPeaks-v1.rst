.. algorithm::

.. summary::

.. relatedalgorithms::

.. properties::

Description
-----------

-  A list of vanadium peak positions in d-spacing is used for the
   central peak positions:
   0.5044,0.5191,0.5350,0.5526,0.5936,0.6178,0.6453,0.6768,0.7134,0.7566,0.8089,0.8737,0.9571,1.0701,1.2356,1.5133,2.1401

   -  You can specify AlternativePeakPositions to use other value (e.g.
      in other units).

-  The PeakWidthPercent value is used to estimate the width of the peak
   (as a percentage of the d-spacing value).
-  The algorithm performs a simple linear fit of the background exluding
   the peak.

   -  It uses two use averaging regions of 1/2 width, centered at +-
      width/2 from the center, and interpolates the linear background
      from it.
   -  The values between the average regions are replaced with the
      interpolated linear background drawn as a straight line.

Usage
-----

.. include:: ../usagedata-note.txt

**Example:**

.. testcode:: ExStripPeak

    ws = Load("PG3_733.nxs")
    wsOut = StripVanadiumPeaks(ws,WorkspaceIndex=2,PeakWidthPercent=3,Version=1)
    i = 1529
    print("This peak at %.4f Angstroms has been reduced from %.0f to a background level of %.0f" % (wsOut.readX(2)[i],ws.readY(2)[i], wsOut.readY(2)[i]))


Output:

.. testoutput:: ExStripPeak

    This peak at 0.8116 Angstroms has been reduced from 11569 to a background level of 10869




.. categories::

.. sourcelink::
