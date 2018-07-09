.. algorithm::

.. summary::

.. properties::

Description
-----------

IndexSatellitePeaks is used to find the higher dimensional miller indices that
index a collection of "satellite" fractional single crystal peaks. This
algorithm takes a PeaksWorkspace output from the algorithm RefineSatellitePeaks
and returns a new table with integer indices for each peak.

This algorithm works by first attempting to finding the minimum number of
distinct modulation (`q`) vectors that are required to fully index the
collection of peaks. The full list of q vectors is found using the same method
described in the :ref:`RefineSatellitePeaks <algm-RefineSatellitePeaks-v1>`
algorithm. The basis set is then chosen from this list. If there are multiple
choices of vectors the algorithm will always choose the smallest one. This
defines the number of additional dimensions required to index the crystal with
integer indices. 

Once a basis of `q` vectors has been chosen for indexing the system all integer
multiples of the basis set are generated. Each `q` vector is then compared with
the set of all possible integer reflections and is assigned to the closest one
found within the radius of the `Tolerance` parameter (measured by Euclidean
distance in HKL space). Reflections which cannot be indexed within the
tolerance are set to (0,0,0).

.. warning:: The current version of the algorithm returns a
    :ref:`TableWorkspace <Table Workspaces>` and not a :ref:`PeaksWorkspace
    <PeaksWorkspace>`. This means that the workspace cannot be overlaid on the
    slice viewer or the instrument view. 

.. seealso:: As well as being able to export the data to the nexus file format,
    saving the data to the Jana format is supported via the :ref:`SaveReflections
    <algm-SaveReflections-v1>` algorithm.

For more information on superspace crystallography see:

- Van Smaalen, Sander. "An elementary introduction to superspace 
  crystallography." Zeitschrift für Kristallographie-Crystalline Materials 
  219, no. 11 (2004): 681-691. 

- Van Smaalen, Sander. "Incommensurate crystal structures." Crystallography 
  Reviews 4, no. 2 (1995): 79-202. 

Related Algorithms
------------------

- :ref:`RefineSatellitePeaks <algm-RefineSatellitePeaks-v1>` can be used to
  obtain a workspace of satellite peak positions in fractional HKL that is
  required for input to this algorithm.

- :ref:`SaveReflections <algm-SaveReflections-v1>` algorithm can be used to save
  the table workspace output from this algorithm to the Jana text file format.

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
