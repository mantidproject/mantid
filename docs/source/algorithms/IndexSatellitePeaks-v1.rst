.. algorithm::

.. summary::

.. alias::

.. properties::

Description
-----------

IndexSatellitePeaks is used to find the higher dimensional miller indicies that index a collection of "satellite" fractional single crystal peaks. This algorithm takes a PeaksWorkspace output from the algorithm RefineSatellitePeaks and returns a new table with integer indicies for each peak.

Usage
-----

.. include:: ../usagedata-note.txt


**Example - calling IndexSatellitePeaks:**

.. testcode:: IndexSatellitePeaks

    import numpy as np
    np.random.seed(1)

    nuclear_peaks = Load('WISH_peak_hkl_small.nxs')
    satellite_peaks = Load("refine_satellites_fixed_q_test.nxs")
    indexed_peaks = IndexSatellitePeaks(nuclear_peaks, satellite_peaks, tolerance=0.1, NumOfQs=2)
    index_values = np.array(indexed_peaks.column("m1"))

    for peak in indexed_peaks:
        print("H: {h:>5} K: {k:>5} L: {l:>5} M1: {m1:>5}".format(**peak))

Output:

.. testoutput:: IndexSatellitePeaks

    H:   2.0 K:  -1.0 L:  -3.0 M1:  -1.0
    H:   2.0 K:  -1.0 L:   0.0 M1:  -1.0
    H:   2.0 K:  -1.0 L:  -3.0 M1:   1.0
    H:   2.0 K:  -1.0 L:   0.0 M1:   1.0

.. categories::

.. sourcelink::
  :cpp: None
  :h: None
