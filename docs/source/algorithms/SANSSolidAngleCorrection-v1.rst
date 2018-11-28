.. algorithm::

.. summary::

.. relatedalgorithms::

.. properties::

Description
-----------

Performs a solid angle correction on all detector pixels.
This algorithm is usually called by
:ref:`SANSReduction <algm-SANSReduction>` or :ref:`HFIRSANSReduction <algm-HFIRSANSReduction>`.

The solid angle correction is applied as follows:

:math:`I'(x,y)=\frac{I(x,y)}{\cos^3(2\theta)}`

:math:`\sigma_{I'(x,y)}=\frac{\sigma_{I(x,y)}}{\vert\cos^3(2\theta)\vert}`

If *DetectorTubes* is set to True, the correction is calculated according to a tube geometry. The cosine term above then becomes:
    
:math:`\cos^3(2\theta) \rightarrow \cos^2(2\theta) \cos(\alpha)`

where :math:`\alpha`: is the angle between the sample-to-pixel vector and its projection on the X-Z plane.


.. categories::

.. sourcelink::
