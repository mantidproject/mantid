.. algorithm::

.. summary::

.. alias::

.. properties::

Description
-----------

This algorithm starts with a PeaksWorkspace containing the expected
positions of peaks in detector space. It calculates the centroid of the
peak by calculating the average of the coordinates of all events within
a given radius of the peak, weighted by the weight (signal) of the event
for event workspaces or the intensity for histogrammed workspaces.

Usage
-----

.. code-block:: python

    # Load a SCD data set from systemtests Data and find the peaks
    LoadEventNexus(Filename='/home/vel/workspace/TOPAZ_3132_event.nxs', OutputWorkspace='TOPAZ_3132_nxs')
    ConvertToDiffractionMDWorkspace(InputWorkspace='TOPAZ_3132_nxs', OutputWorkspace='TOPAZ_3132_md', LorentzCorrection=True)
    peaks = FindPeaksMD(InputWorkspace='TOPAZ_3132_md', PeakDistanceThreshold=0.14999999999999999, MaxPeaks=100)
    FindUBUsingFFT(PeaksWorkspace='peaks', MinD=2, MaxD=16)
    IndexPeaks(PeaksWorkspace='peaks', NumIndexed=100, AverageError=0.013759860303255647)
    peak = peaks.getPeak(0)
    print peak.getBinCount()
    peaks = CentroidPeaks(InPeaksWorkspace='peaks', InputWorkspace='TOPAZ_3132_nxs')
    peak = peaks.getPeak(0)
    print peak.getBinCount()


.. categories::
