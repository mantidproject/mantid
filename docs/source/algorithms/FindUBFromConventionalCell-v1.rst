.. algorithm::

.. summary::

.. relatedalgorithms::

.. properties::

Description
-----------

Given a :ref:`PeaksWorkspace <PeaksWorkspace>` of **unindexed** peaks together with the
known **conventional-cell** lattice parameters (``a``, ``b``, ``c``, ``alpha``, ``beta``,
``gamma``) and ``Centering``, this algorithm determines a **UB matrix** describing the
orientation of the crystal and sets it on the workspace's oriented lattice.

The measured peak positions fix the shape and size of the crystal's reciprocal lattice but
not how it is oriented in the instrument. The algorithm treats that orientation as the
unknown and searches for it:

#. The conventional cell is reduced to its **primitive** setting so the search works on a
   lattice with a single lattice point per cell.
#. Candidate lab-frame directions are scored by how strongly the peak Q-vectors, projected
   onto a direction, cluster near a regular lattice spacing (a *periodic-alignment* score,
   the magnitude of the mean complex phase :math:`\left|\langle e^{2\pi i\,d\,(q\cdot\hat{n})}\rangle\right|`).
   A coarse hemisphere grid (``NumAzimuth`` × ``NumPolar``) selects an **anchor** axis; a
   random spherical-cap refinement (``CapAngleDeg``, ``CapSamples``) sharpens its direction;
   and an azimuthal *cone-pair* search (``NumPsi``) places the two remaining axes at the
   correct interaxial angles.
#. Near-degenerate axis lengths are disambiguated (``AxisDegeneracyTolerance``) by comparing
   the global fractional-index fit, the primitive UB is converted back to the conventional
   setting, and the resulting UB is assigned to the workspace.

Two diagnostic outputs are produced: ``DiagnosticTable`` (a table of fit quality metrics —
the fractional-index RMS, the periodicity and centering scores, the recovered primitive cell,
whether an axis swap was applied, and the centering counts) and ``ProjectionHistograms``
(histograms of the peak Q-vectors projected onto the fitted *a*, *b*, *c* axes). Because the
search always returns a UB, these diagnostics should be inspected to judge whether the result
is trustworthy.

.. note::

   This algorithm is suited to the case of a **known cell** with relatively **few peaks**,
   including centered lattices. The direction search uses a random-number generator seeded by
   ``RandomSeed``; results are reproducible for a fixed seed but may change if the seed changes.

Usage
-----

**Example - recovering the orientation of a face-centred cubic cell:**

.. testcode:: ExFindUBFromConventionalCell

    import numpy as np
    from mantid.kernel import V3D

    # A known orientation (U) of a face-centred cubic cell with a = 8 Angstrom
    a = 8.0
    B = np.diag([1.0 / a, 1.0 / a, 1.0 / a])
    theta = np.deg2rad(10.0)
    U = np.array([[np.cos(theta), -np.sin(theta), 0.0],
                  [np.sin(theta),  np.cos(theta), 0.0],
                  [0.0,            0.0,           1.0]])
    UB_true = U @ B

    # Build a few unindexed peaks from that orientation
    hkls = [[2, 0, 0], [2, 2, 0], [1, 1, 1], [2, 0, 2], [1, 3, 1]]
    peaks = CreatePeaksWorkspace(NumberOfPeaks=0, OutputType="LeanElasticPeak")
    SetUB(peaks, UB=UB_true)
    for hkl in hkls:
        peaks.addPeak(peaks.createPeakHKL(V3D(*hkl)))

    # Recover the orientation from just the peaks and the known conventional cell
    FindUBFromConventionalCell(PeaksWorkspace=peaks, a=a, b=a, c=a,
                               alpha=90, beta=90, gamma=90, Centering="F",
                               CapSamples=1000, NumPsi=720)

    UB_est = peaks.sample().getOrientedLattice().getUB()
    q = np.array([UB_true @ np.array(hkl, dtype=float) for hkl in hkls])
    hkl_est = np.linalg.solve(UB_est, q.T).T
    rms = np.sqrt(np.mean((hkl_est - np.rint(hkl_est)) ** 2))
    print("Peaks indexed by recovered UB:", bool(rms < 0.3))

Output:

.. testoutput:: ExFindUBFromConventionalCell

    Peaks indexed by recovered UB: True

.. categories::

.. sourcelink::
