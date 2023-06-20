.. algorithm::

.. summary::

.. relatedalgorithms::

.. properties::

Description
-----------

Finds the beam center using the direct beam method.
This algorithm is used by the EQSANS and HFIR SANS reduction.

The position of the beam center :math:`\vec{p}` is given by

.. math::

   \vec{p}=\frac{\sum_iI_i\vec{d}_i}{\sum_iI_i}


where :math:`i` runs over all pixels within the largest square detector area centered on the initial guess for the beam center position.
The initial guess is (``CenterX``, ``CenterY``) which defaults to the center of the detector.
:math:`I_i` is the detector count for pixel :math:`i`, and :math:`\vec{d}_i` is x-y projection of the pixel position.
The range of pixels considered is limited to be those symmetric (in x and y separately) around the previous step's found beam center.
The calculation above is repeated iteratively by replacing the initial guess with the position found with the previous iteration.
The process stops when the difference between the positions found with two consecutive iterations is smaller than ``Tolerance`` in meters.

The integration range within the symmetric area of the detector can be controlled using ``BeamRadius`` (when ``DirectBeam=False``) and ``IntegrationRadius``.
When both are in effect the pixels integrated are those that meet the equation

.. math::

   BeamRadius < |\vec{d}_i - \vec{p}| < IntegrationRadius

where :math:`\vec{p}` is the beam center from the previous iteration.
Again, when ``DirectBeam=True`` is specified, the lower bound is ignored.
When ``IntegrationRadius`` is not specified, the upper bound is ignored.
Not specifying any of these three parameters uses all data within the symmetric x/y region.

If the ``Output`` property is set, the beam centre will be placed in a table workspace.
Otherwise, the result is placed in an ArrayProperty named ``CenterOfMass``.

Usage
-----

.. include:: ../usagedata-note.txt

**Example - Find the beam center for a BioSANS data file:**

.. testcode:: ExBeamCenter

   # Load your data file
   LoadSpice2D('BioSANS_empty_cell.xml', OutputWorkspace='empty_cell')

   # Compute the center position, which will be put in a table workspace
   center = FindCenterOfMassPosition('empty_cell', Output='center')
   x, y = center.column(1)
   print(f"(x, y) = ({x:.4f}, {y:.4f})")

Output:

.. testoutput:: ExBeamCenter

   (x, y) = (-0.0066, 0.0091)


.. categories::

.. sourcelink::
