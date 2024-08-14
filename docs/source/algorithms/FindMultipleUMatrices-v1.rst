.. algorithm::

.. summary::

.. relatedalgorithms::

.. properties::

Description
-----------

FindMultipleUMatrices will use the lattice parameters and spacegroup provided to optimise a number (``NumberOfUBs``)
of UB matrices (B is hard-coded due to the lattice parameters provided) and returns a group of peak workspaces
one for each UB, containing the peaks that are indexed most accurately by that UB.

This algorithm is useful for finding a single UB in the presence of spurious peaks, or finding multiple UBs when there
are multiple domains.

The algorithm proceeds by looping over each pair of peaks in ``PeaksWorkspace`` within the d-spacing limits
(``MinDSpacing`` and ``MaxDSpacing``), trying to index them based on their d-spacing and angle between the
peaks (within tolerances given by ``DSpacingTolerance`` and ``AngleTolerance``). It then calculates the HKL
errors (difference in HKL residuals squared) for all peaks in ``PeaksWorkspace`` and identifies the UBs that
index the most peaks with the highest accuracy (i.e. better than any other UB found)

Note that although the user can provide modulation vectors, these are not currently used to index pairs of
peaks but are indexed by each candidate UB found and used to determine the best UBs to return.

Peaks with relatively higher d-spacing are easier to index correctly, try using a smaller number of higher
d-spacing peaks to start.


Useage
------

**Example:**

.. testcode:: exampleFindMultipleUMatrices

    from mantid.simpleapi import *
    from scipy.spatial.transform import Rotation as rot

    ws = LoadEmptyInstrument(InstrumentName='SXD', OutputWorkspace='empty_SXD')
    axis = ws.getAxis(0)
    axis.setUnit("TOF")

    # create a peak tables of orthorhombic domains with lattice parameters a=4, b=5, c=10
    alatt = {'a': 4, 'b': 5, 'c': 10, 'alpha': 90, 'beta': 90, 'gamma': 90}
    ubs = [np.diag([1/alatt['a'], 1/alatt['b'], 1/alatt['c']])]
    ubs.append(rot.from_rotvec([0,0,90], degrees=True).as_matrix()  @ ubs[0])

    peaks = CreatePeaksWorkspace(InstrumentWorkspace=ws, NumberOfPeaks=0, OutputWorkspace=f"peaks")
    for iub, ub in enumerate(ubs):
        SetUB(peaks, UB=ub)
        for h in range(1, 3):
            for k in range(1, 3):
                for l in range(2,4):
                    pk = peaks.createPeakHKL([h, k ,l])
                    if pk.getDetectorID() > 0:
                        peaks.addPeak(pk)

    peaks_out = FindMultipleUMatrices(PeaksWorkspace=peaks, OutputWorkspace='peaks_out', **alatt,
                                      MinDSpacing=1.25, MaxDSpacing=3.5, Spacegroup='P m m m',
                                      NumberOfUBs=2)

    for ipk, pks in enumerate(peaks_out):
        print("HKL along x-axis of QLab = ", abs(np.round(pks.sample().getOrientedLattice().getvVector())))


**Output:**

.. testoutput:: exampleFindMultipleUMatrices

    HKL along x-axis of QLab =  [ 4.  0.  0.]
    HKL along x-axis of QLab =  [ 0.  5.  0.]


.. categories::

.. sourcelink::
