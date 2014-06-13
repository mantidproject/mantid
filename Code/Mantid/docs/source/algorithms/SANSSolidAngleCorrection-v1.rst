.. algorithm::

.. summary::

.. alias::

.. properties::

Description
-----------

Performs a solid angle correction on all detector pixels.
This algorithm is usually called by
:ref:`SANSReduction <algm-SANSReduction>` or :ref:`HFIRSANSReduction <algm-HFIRSANSReduction>`.

The solid angle correction is applied as follows:

:math:`I'(x,y)=\frac{I(x,y)}{\cos^3(2\theta)}`

:math:`\sigma_{I'(x,y)}=\frac{\sigma_{I(x,y)}}{\vert\cos^3(2\theta)\vert}`

If *DetectorTubes* is set to True, the angle :math:`2\theta` will be replaced
by the angle between the Z-axis and the projection of the sample-to-pixel vector on
the plane defined by the beam (Z) axis and the Y-axis.


.. categories::
