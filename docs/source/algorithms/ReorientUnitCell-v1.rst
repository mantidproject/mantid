.. algorithm::

.. summary::

.. relatedalgorithms::

.. properties::

Description
-----------

ReorientUnitCell finds the unit cell most aligned with the goniometer axes by finding rotation :math:`U`
most similar to the identity matrix. Rotation :math:`U` transforms the reciprocal orthonormal
vectors :math:`u^{*}, v^{*}, w^{*}` such that:

- :math:`u^{*}` aligns with the beam ( :math:`Z` axis)
- :math:`v^{*}` aligns with the horizontal ( :math:`X` axis)
- :math:`w^{*}` aligns with the vertical ( :math:`Y` axis)

Thus, the algorithm tries to align the reciprocal lattice vectors as closely as possible
to these goniometer axes.
The similarity of :math:`U` to the identity matrix is quantified by the angle of rotation :math:`\theta`,
which can be calculated from its trace:

.. math::

   Tr(U) = 1 + 2\cos{\theta} \\

Minimizing :math:`\theta` is therefore equivalent to maximizing :math:`Tr(U)`.

Starting with an initial guess for matrices :math:`U` and :math:`B`, where :math:`B` is the Busing matrix
transforming the reciprocal-lattice vectors to the reciprocal orthonormal frame of reference,
the algorithm applies all possible rotations :math:`S` of the crystal's point group
to find matrix :math:`U'` most similar to the identity matrix, where

.. math::

   U' \cdot B = U \cdot B \cdot S \\
   Tr(U') = Tr(U \cdot (B \cdot S \cdot B^{-1}))

Thus, :math:`S` is the rotation of the point group such that :math:`B \cdot S \cdot B^{-1}`
is most similar to the inverse of :math:`U`.

The figure belows shows a special case in the hexagonal lattice
where rotation :math:`S` and Busing matrix :math:`B` commute,
so that :math:`S` is that rotation most similar to the inverse of :math:`U`.

.. figure:: ../images/ReorientUnitCell_fig1.png
   :class: screenshot
   :width: 600px
   :align: center

In the image, lattice vectors :math:`\{a_1^*, a_2^*\}` transform to reciprocal orthonormal vectors
:math:`\{u^*, v^*\}` by matrix :math:`B`, then transform to goniometer axes :math:`\{u, v\}`
by rotation matrix :math:`U`.
For equivalent lattice vectors :math:`\{{a'}_{1}^{*}, {a'}_{2}^{*}\}`,
matrix :math:`B` directly transforms to goniometer axes, thus :math:`U'` is the identity matrix.
Notice that matrix :math:`S \equiv U^{-1}` transforms :math:`\{{a'}_{1}^{*}, {a'}_{2}^{*}\}`
to :math:`\{a_1^*, a_2^*\}`.

**Note:** The triclinic crystal system has no symmetry operations other than the identity, so no reorientation is possible.
Hence, this algorithm does not support triclinic crystals.

Usage
-----

.. include:: ../usagedata-note.txt

**Example - ReorientUnitCell:**

.. testcode:: ReorientUnitCellExample

    import numpy as np

    def rotation_angle(workspace):
        R = np.array(
            [[0,1,0],
             [0,0,1],
             [1,0,0]])
        oriented_lattice = mtd[str(workspace)].sample().getOrientedLattice()
        trace = (R @ oriented_lattice.getU()).diagonal().sum()
        cos_theta = (trace - 1) / 2.0
        return round(np.degrees(np.arccos(cos_theta)))

    filename = 'RFMBA2PbI4_Monoclinic_P_5sig.integrate'
    LoadIsawPeaks(Filename=filename, OutputWorkspace='peaks')
    FindUBUsingIndexedPeaks(PeaksWorkspace='peaks')
    ol = mtd['peaks'].sample().getOrientedLattice()
    # U represents a 120 degree rotation around axis <1 -1 -1>
    U = np.array([[0, 0, -1], [-1, 0, 0], [0, 1, 0]], dtype=float)
    SetUB(Workspace='peaks', UB=(U @ ol.getB()).flatten().tolist())
    IndexPeaks(PeaksWorkspace='peaks', CommonUBForAll=True)
    print(f"(before) rotation angle {rotation_angle('peaks')}")
    ReorientUnitCell(PeaksWorkspace='peaks', CrystalSystem='Monoclinic')
    print(f"(after ) rotation angle {rotation_angle('peaks')}")

Output:

.. testoutput:: ReorientUnitCellExample

    (before) rotation angle 180
    (after ) rotation angle 0

.. categories::

.. sourcelink::
        :filename: ReorientUnitCell
