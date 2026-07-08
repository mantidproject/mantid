.. algorithm::

.. summary::

.. relatedalgorithms::

.. properties::

Description
-----------

The purpose of this algorithm is to improve the indexing of peaks in special cases where ``FindUBUsingIndexedPeaks`` does not index all peaks due to sample misorientation.

The example below demonstrates a case where ``FindUBUsingIndexedPeaks`` is insufficient:

.. code-block:: python

    from mantid.simpleapi import *
    import matplotlib.pyplot as plt
    import numpy as np

    from scipy.spatial.transform import Rotation
    import scipy.optimize

    filename = '/SNS/TOPAZ/IPTS-33878/shared/RFMBA2PbI4/RFMBA2PbI4_mantid_295K_find_peaks/RFMBA2PbI4_Monoclinic_P_5sig.integrate'
    LoadIsawPeaks(Filename=filename, OutputWorkspace='peaks')

    FindUBUsingIndexedPeaks(PeaksWorkspace='peaks', Tolerance=0.12)
    IndexPeaks(PeaksWorkspace='peaks', Tolerance=0.12)

This code will only index about half of the peaks.

The solution is to use this algorithm that refines the UB-matrix and goniometer offsets simultaneously.

.. code-block:: python

    from mantid.simpleapi import mtd, RefineSingleCrystalGoniometer, LoadIsawPeaks, FindUBUsingIndexedPeaks, IndexPeaks

    filename = "/SNS/TOPAZ/IPTS-33878/shared/RFMBA2PbI4/RFMBA2PbI4_mantid_295K_find_peaks/RFMBA2PbI4_Monoclinic_P_5sig.integrate"

    LoadIsawPeaks(Filename=filename, OutputWorkspace="peaks")
    FindUBUsingIndexedPeaks(PeaksWorkspace="peaks")
    IndexPeaks(PeaksWorkspace="peaks", CommonUBForAll=True)

    RefineSingleCrystalGoniometer("peaks", tol=0.25, cell="Monoclinic", n_iter=5)
    IndexPeaks(PeaksWorkspace="peaks", CommonUBForAll=True)

This will result in a better indexing of the peaks.

Large or unreliable goniometer offsets
#######################################

When the goniometer offset is large enough (or the motor readback is unreliable enough) that no
single UB can index more than a handful of peaks across all runs, set ``LargeOffset=True``. Each
run is then indexed independently with ``FindUBUsingFFT`` (which does not depend on the existing
orientation) before the UB and goniometer offsets are refined jointly as above. Runs whose
independently-indexed lattice constants (a, b, c, alpha, beta, gamma) are inconsistent with the
other runs are logged and excluded, since an inconsistent cell usually indicates a bad per-run
indexing rather than a real difference in the sample.

.. code-block:: python

    from mantid.simpleapi import RefineSingleCrystalGoniometer, LoadIsawPeaks, IndexPeaks

    filename = "/SNS/TOPAZ/IPTS-33878/shared/unreliable_motors/peaks.nxs"

    LoadIsawPeaks(Filename=filename, OutputWorkspace="peaks")

    RefineSingleCrystalGoniometer(
        Peaks="peaks",
        Tolerance=0.12,
        Cell="Triclinic",
        NumIterations=8,
        LargeOffset=True,
        MinD=5,
        MaxD=15,
    )
    IndexPeaks(PeaksWorkspace="peaks", CommonUBForAll=True)

.. categories::

.. sourcelink::
