.. algorithm::

.. summary::

.. relatedalgorithms::

.. properties::

Description
-----------

Given a set of peaks at least three of which have been assigned Miller
indices, this algorithm will find the :ref:`UB matrix <Lattice>`, that
best maps the integer (h,k,l) values to the corresponding Q vectors. The set of
indexed peaks must include three linearly independent Q vectors. The
(h,k,l) values from the peaks are first rounded to form integer (h,k,l)
values. The algorithm then forms a possibly over-determined linear
system of equations representing the mapping from (h,k,l) to Q for each
indexed peak. The system of linear equations is then solved in the least
squares sense, using QR factorization.
This algorithm also indexes the satellite peaks.

Usage
-----

**Example:**

.. testcode:: ExFindUBUsingIndexedPeakswithSatellites

    peaks = LoadIsawPeaks("TOPAZ_3007.peaks")
    LoadIsawUB(peaks,"TOPAZ_3007.mat")
    IndexPeaks(peaks)
 
    fractional_peaks = PredictSatellitePeaks(peaks, ModVector1=[-0.5,0,0.0], MaxOrder=1)
    IndexPeakswithSatellites(fractional_peaks, ModVector1=[-0.5,0,0.0], MaxOrder=1)
    FindUBUsingIndexedPeakswithSatellites(fractional_peaks)
    print("After FindUBUsingIndexedPeakswithSatellites does the workspace have an orientedLattice: %s" % fractional_peaks.sample().hasOrientedLattice())

    print(fractional_peaks.sample().getOrientedLattice().getUB())


Output:

.. testoutput:: ExFindUBUsingIndexedPeakswithSatellites

    After FindUBUsingIndexedPeakswithSatellites does the workspace have an orientedLattice: True
    [[-0.04538476  0.04055514 -0.0125774 ]
     [ 0.00133894 -0.00316533  0.1168664 ]
     [ 0.05738488  0.03214582  0.02736221]]


.. categories::

.. sourcelink::
