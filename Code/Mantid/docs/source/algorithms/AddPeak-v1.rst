.. algorithm::

.. summary::

.. alias::

.. properties::

Description
-----------

Adds a peak to a :ref:`PeaksWorkspace <PeaksWorkspace>`.

Usage
-----

**Example - Add a peak to a PeaksWorkspace.**

.. testcode:: AddPeak

    ws = CreateSampleWorkspace("Histogram","Multiple Peaks")
    # Find the peaks in a 2D workspace.
    peaks_ws = FindSXPeaks(ws)

    print "The number of peaks before adding a peak is: " + str(peaks_ws.getNumberPeaks())
    # Add a peak to the peaks workspace.
    AddPeak(PeaksWorkspace=peaks_ws,RunWorkspace=ws,DetectorID=101,TOF=3819,Height=10.3,BinCount=2)

    print "The number of peaks after adding a peak is: " + str(peaks_ws.getNumberPeaks())

Output:

.. testoutput:: AddPeak

    The number of peaks before adding a peak is: 174
    The number of peaks after adding a peak is: 175

.. categories::
