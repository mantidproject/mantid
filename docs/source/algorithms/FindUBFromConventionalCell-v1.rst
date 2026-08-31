.. algorithm::

.. summary::

.. relatedalgorithms::

.. properties::

Description
-----------

Given a :py:obj:`PeaksWorkspace <mantid.dataobjects.PeaksWorkspace>` of **unindexed** peaks together with the
known **conventional-cell** lattice parameters (``a``, ``b``, ``c``, ``alpha``, ``beta``,
``gamma``) and ``Centering``, this algorithm determines a **UB matrix** describing the
orientation of the crystal and sets it on the workspace's oriented lattice.

The input lattice parameters and centering condition describe the expected reciprocal
lattice positions up to a rotation corresponding to how the sample is oriented in the
instrument. The algorithm treats that orientation as the unknown and searches for it:

#. The conventional cell is reduced to its **primitive** setting so the search works on a
   lattice with a single lattice point per cell.
#. Candidate lab-frame directions are scored by how strongly the peak Q-vectors, projected
   onto a direction, cluster near a regular lattice spacing (a *periodic-alignment* score,
   the magnitude of the mean complex phase
   :math:`\left|\langle e^{i 2\pi d\,(\overrightarrow{Q}\cdot\hat{n})}\rangle\right|`).
   A coarse hemisphere grid (``NumAzimuth`` × ``NumPolar``) selects an **anchor** axis; a
   random spherical-cap refinement (``CapAngleDeg``, ``CapSamples``) sharpens its direction;
   and an azimuthal *cone-pair* search (``NumPsi``) places the two remaining axes at the
   correct interaxial angles.
#. Near-degenerate primitive-axis lengths are disambiguated
   (``AxisDegeneracyTolerance``) by trying the relevant axis swaps and selecting
   the assignment with the best global fractional-index fit.
#. The selected primitive UB is converted back to the conventional-cell setting,
   and the resulting UB is assigned to the workspace.

.. note::

   This algorithm requires a **known cell**. Unlike FFT-based indexing it does not require many
   peaks: as few as three non-coplanar peaks can be sufficient, including for centered lattices.
   The direction search uses a random-number generator seeded by ``RandomSeed``; results are
   reproducible for a fixed seed but may change if the seed changes.

Input requirements
##################

The input peaks and the conventional cell must satisfy the following, which are checked before
execution:

- At least three peaks are required, and their Q-vectors must not all point in nearly the same
  direction. The check counts consecutive peak pairs in the workspace whose Q-vectors differ in
  direction by more than 10 degrees, and requires at least two such pairs.
- Each lattice angle must satisfy 10 < angle < 170 degrees.
- ``Centering="R"`` expects the rhombohedral cell in its conventional **hexagonal** setting,
  that is ``a`` = ``b``, ``alpha`` = ``beta`` = 90 and ``gamma`` = 120. A rhombohedral cell
  given in its primitive setting should instead be passed with ``Centering="P"``.

Diagnostic outputs
##################

The search always returns a UB, whether or not the peaks are consistent with the given cell, so
the two diagnostic outputs should be inspected to judge whether the result is trustworthy.

``DiagnosticTable`` contains one ``Metric``/``Value`` row per entry below. All values are stored
as doubles, so the categorical entries are encoded numerically.

.. list-table::
   :header-rows: 1

   * - Metric
     - Meaning
   * - ``rms_hkl``
     - RMS deviation of the fractional Miller indices from the nearest integers, over all peaks,
       for the chosen axis assignment. This is the primary measure of fit quality.
   * - ``centering_score``
     - Fraction of the near-integer reflections that also satisfy the centering condition of
       ``Centering``. 1 indicates full consistency; 0 means no reflection was near-integer.
   * - ``joint_score``
     - Product of the periodic-alignment scores of the two non-anchor axes at the accepted
       solution. Each factor lies in [0, 1] and larger values indicate better alignment.
   * - ``psi_best_deg``
     - Azimuthal angle, in degrees, around the anchor axis that produced ``joint_score``.
   * - ``branch``
     - Which azimuthal-offset branch of the cone-pair search kept the *a*, *b*, *c* frame
       right-handed: 1 for the :math:`+\delta` branch, -1 for the :math:`-\delta` branch.
   * - ``axis_swap_applied``
     - Which pair of near-degenerate axes was swapped: 0 = none, 1 = *a* and *b*,
       2 = *a* and *c*, 3 = *b* and *c*.
   * - ``primitive_a``, ``primitive_b``, ``primitive_c``
     - Lengths, in angstrom, of the primitive cell that was searched.
   * - ``primitive_alpha_deg``, ``primitive_beta_deg``, ``primitive_gamma_deg``
     - Angles, in degrees, of the primitive cell that was searched.
   * - ``n_total``
     - Number of peaks in the input workspace.
   * - ``n_near_integer``
     - Number of peaks whose fractional indices are all within ``Tolerance`` of an integer.
   * - ``n_centering_ok``
     - Number of those near-integer peaks that also satisfy the centering condition.

``Tolerance`` affects only ``centering_score``, ``n_near_integer`` and ``n_centering_ok``. It is
not used by the direction search and does not change the UB that is found.

``ProjectionHistograms`` contains three spectra, labelled ``h``, ``k`` and ``l``, holding the
peak Q-vectors projected onto the fitted **primitive** axis directions and scaled by the
corresponding primitive lattice lengths. Counts concentrated at integer positions indicate a
good fit. These projections are therefore primitive indices, whereas the UB set on the workspace
is indexed on the conventional cell.

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
