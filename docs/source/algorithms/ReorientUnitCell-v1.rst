.. algorithm::

.. summary::

.. relatedalgorithms::

.. properties::

Description
-----------

ReorientUnitCell finds the orientation of the reciprocal lattice vectors :math:`(\vec{a^{*}}, \vec{b^{*}}, \vec{c^{*}})`
such that :math:`\vec{a^{*}}` is aligned as closely as possible with the beam direction
(Z-axis in the laboratory frame of reference).
Thinking of the orthonormal reciprocal basis :math:`(\vec{u^{*}}, \vec{v^{*}}, \vec{w^{*}})`
resulting from transforming the reciprocal lattice vectors by the Busing matrix :math:`B`,
the algorithm will try the following alignment:

- align :math:`\vec{u^{*}} \equiv \vec{a^{*}}` with the neutron beam ( :math:`Z` axis)
- align :math:`\vec{v^{*}}` with the horizontal axis ( :math:`X` axis)
- align :math:`\vec{w^{*}}` with the vertical axis ( :math:`Y` axis)

Matrix :math:`U` describes the rotation transforming the orthonormal reciprocal basis
to the laboratory frame of reference :math:`(\vec{x}, \vec{y}, \vec{z})`.
For the previous alignment, we have:

.. math::

   (\vec{u^{*}}, \vec{v^{*}}, \vec{w^{*}})
   =
   (\vec{x}, \vec{y}, \vec{z})
   \begin{pmatrix}
   0 & 1 & 0 \\
   0 & 0 & 1 \\
   1 & 0 & 0
   \end{pmatrix}
    =
    (\vec{x}, \vec{y}, \vec{z}) \cdot R

where matrix :math:`R` is the particular :math:`U` matrix representing this alignment.

Starting from some initial orientation of the crystal, denoted by transformation matrix :math:`U_0 \cdot B`,
the algorithm will try to find an orientation :math:`U \cdot B` such that
:math:`U \cdot B \approx R \cdot B`.
This is accomplished by applying the proper point symmetry operations :math:`S` (rotations)
of the lattice system.

.. math::

   U \cdot B \equiv U_0 \cdot B \cdot S \\
   U = U_0 \cdot B \cdot S \cdot B^{-1} \\

The algorithm selects the rotation :math:`S` such that composite rotation :math:`R^{-1} \cdot U` is as close as possible
to the identity matrix :math:`I`.
The similarity to the identity matrix is quantified by the angle of rotation :math:`\theta`,
which can be calculated from its trace:

.. math::

   Tr(R^{-1} \cdot U) = Tr(R^{-1} \cdot U_0 \cdot B \cdot S \cdot B^{-1}) = 1 + 2\cos{\theta} \\

Minimizing :math:`\theta` is therefore equivalent to maximizing this trace.

**Note:** The triclinic crystal system has no symmetry operations other than the identity,
so no reorientation is possible. Hence, this algorithm does not support triclinic crystals.

Usage
-----

.. include:: ../usagedata-note.txt

**Example - ReorientUnitCell:**

.. testcode:: ReorientUnitCellExample

    import numpy as np

    def getuVector():
        ol = mtd['peaks'].sample().getOrientedLattice()
        return "(" + ', '.join([f"{x:.1f}" for x in ol.getuVector()]) + ")"

    filename = 'TOPAZ_Monoclinic_P_5sig.integrate'
    LoadIsawPeaks(Filename=filename, OutputWorkspace='peaks')
    FindUBUsingIndexedPeaks(PeaksWorkspace='peaks')
    ol = mtd['peaks'].sample().getOrientedLattice()
    # U represents a 120 degree rotation around axis <1 1 -1>
    U = np.array([[0, 1, 0], [0, 0, -1], [-1, 0, 0]], dtype=float)
    SetUB(Workspace='peaks', UB=(U @ ol.getB()).flatten().tolist())
    IndexPeaks(PeaksWorkspace='peaks', CommonUBForAll=True)
    print(f"(before) u = {getuVector()}")
    ReorientUnitCell(PeaksWorkspace='peaks', CrystalSystem='Monoclinic')
    print(f"(after) u = {getuVector()}")

Output:

.. testoutput:: ReorientUnitCellExample

    (before) u = (-9.2, 0.0, 0.0)
    (after ) u = (9.2, 0.0, 0.0)

.. categories::

.. sourcelink::
        :filename: ReorientUnitCell
