.. algorithm::

.. summary::

.. relatedalgorithms::

.. properties::

Description
-----------

This is a wrapper for :ref:`algm-StripPeaks` algorithm used with the peak positions for Vanadium.

-  A list of vanadium peak positions in d-spacing is used for the
   central peak positions:
   0.5044,0.5191,0.5350,0.5526,0.5936,0.6178,0.6453,0.6768,0.7134,0.7566,0.8089,0.8737,0.9571,1.0701,1.2356,1.5133,2.1401

-  The vanadium peaks are fit to a function combined from Gaussian and
   linear/quadratic background.

Usage
-----

.. include:: ../usagedata-note.txt

**Example:**

.. testcode:: ExStripPeak

    ws = Load("PG3_733.nxs")
    wsOut = StripVanadiumPeaks(ws,BackgroundType="Linear",WorkspaceIndex=2)
    i = 1529
    print("This peak at %.4f Angstroms has been reduced from %.0f to a background level of %.0f" % (wsOut.readX(2)[i],ws.readY(2)[i], wsOut.readY(2)[i]))


Output:

.. testoutput:: ExStripPeak

    This peak at 0.8116 Angstroms has been reduced from 11569 to a background level of 10845

.. categories::

.. sourcelink::
