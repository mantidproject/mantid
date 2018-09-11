.. algorithm::

.. summary::

.. relatedalgorithms::

.. properties::

Description
-----------

Compute the resolution in Q according to Mildner-Carpenter. The algorithm uses the
wavelength information, the apertures and the source, sample and detector positions
to compute the Q resolution and add it to the workspace.

This algorithm is generally not called directly. It's called by 
:ref:`SANSAzimuthalAverage1D <algm-SANSAzimuthalAverage1D>`
after the calculation of I(Q). It can only be applied to an I(Q) workspace.

.. categories::

.. sourcelink::
