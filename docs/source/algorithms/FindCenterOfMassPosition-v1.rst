.. algorithm::

.. summary::

.. relatedalgorithms::

.. properties::

Description
-----------

Finds the beam center using the direct beam method. 
This algorithm is used by the EQSANS and HFIR SANS reduction.

The position of the beam center *p* is given by

:math:`\vec{p}=\frac{\sum_iI_i\vec{d}_i}{\sum_iI_i}`


where i runs over all pixels within the largest square detector area centered on the 
initial guess for the beam center position. The initial guess is the center of the detector. 
:math:`I_i` is the detector count for pixel i, and :math:`\vec{d}_i` is the pixel coordinates. The calculation above 
is repeated iteratively by replacing the initial guess with the position found with the 
previous iteration. The process stops when the difference between the positions found 
with two consecutive iterations is smaller than 0.25 pixel.

If *DirectBeam* is set to False, the beam center will be found using the 
scattered beam method. The process is identical to the direct beam method, with the 
only difference being that the pixels within a distance R (the beam radius) of the 
beam center guess are excluded from the calculation. The direct beam is thus 
excluded and only the scattered data is used.

If the *Output* property is set, the beam centre will be placed in a
table workspace. Otherwise, the result is placed in an ArrayProperty
named *CenterOfMass*.


Usage
-----

.. include:: ../usagedata-note.txt

**Example - Find the beam center for a BioSANS data file:**

.. testcode:: ExBeamCenter

   # Load your data file
   workspace = LoadSpice2D('BioSANS_empty_cell.xml')
   
   # Compute the center position, which will be put in a table workspace
   FindCenterOfMassPosition('workspace', Output='center', Version=1)
   
.. categories::

.. sourcelink::
