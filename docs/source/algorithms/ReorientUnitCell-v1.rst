.. algorithm::

.. summary::

.. relatedalgorithms::

.. properties::

Description
-----------

ReorientUnitCell finds the unit cell most aligned with the goniometer axes by finding rotation :math:`U`
most similar to the identity matrix. The similarity is quantified by the angle of rotation :math:`\theta`,
which can be calculated from the trace of the rotation matrix:

.. math::

   Tr(U) = 1 + 2\cos{\theta} \\
   Tr(UB) \leq Tr(U) Tr(B)

Minimizing :math:`\theta` is therefore equivalent to maximizing :math:`Tr(UB)`. Here, :math:`B` is the
matrix transforming the lattice vectors to the reciprocal orthonormal frame of reference.

Starting from an initial guess for the orientation matrix :math:`UB`, the algorithm applies all
possible symmetry operations :math:`S` of the crystal's point group to find matrix :math:`U'`
most similar to the identity matrix, where

.. math::

   U'B = UBS \\
   Tr(U'B) = Tr(UBS) \leq Tr(B) Tr(SU)

Thus, :math:`S` is the symmetry operation most similar to the inverse of :math:`U`.

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

The triclinic crystal system has no symmetry operations other than the identity, so no reorientation is possible.
Hence, this algorithm does not support triclinic crystals.

Usage
-----

.. include:: ../usagedata-note.txt

**Example - ReorientUnitCell:**

.. testcode:: ReorientUnitCellExample

    import numpy as np

    def rotation_angle(workspace):
        oriented_lattice = mtd[str(workspace)].sample().getOrientedLattice()
        trace = oriented_lattice.getU().diagonal().sum()
        cos_theta = (trace - 1) / 2.
        return int(np.degrees(np.arccos(cos_theta)))

    filename = 'RFMBA2PbI4_Monoclinic_P_5sig.integrate'
    LoadIsawPeaks(Filename=filename, OutputWorkspace='peaks')
    FindUBUsingIndexedPeaks(PeaksWorkspace='peaks')
    IndexPeaks(PeaksWorkspace='peaks', CommonUBForAll=True)
    print(f"(before) rotation angle {rotation_angle('peaks')}")

    ReorientUnitCell(PeaksWorkspace='peaks', CrystalSystem='Monoclinic')
    IndexPeaks(PeaksWorkspace='peaks', CommonUBForAll=True)
    print(f"(after ) rotation angle {rotation_angle('peaks')}")

Output:

.. testoutput:: ReorientUnitCellExample

    (before) rotation angle 157
    (after ) rotation angle 132

.. categories::

.. sourcelink::
        :filename: ReorientUnitCell
