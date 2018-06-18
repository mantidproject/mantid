.. algorithm::

.. summary::

.. relatedalgorithms::

.. properties::

Description
-----------

Calculates the reflection angle theta for a Magnetism Reflectometer workspace.
There are two options:

**Use SANGLE**

This option simply reads in the SANGLE value and returns it in radians. SANGLE is read from an encoder on the sample stage.

**Use DANGLE**

DANGLE tracks the angle between the incoming beam and the detector relative to an offset (DANGLE0).
When everything is aligned, :math:`\theta=(DANGLE-DANLGE0)/2`.

The reflected beam should be aligned to fall on the detector pixel stored in the DIRPIX log. When it's not the case, the theta angle
will be corrected accordingly. If the ``SpecularPixel`` property is set, the reflection angle will be given by:

:math:`\theta=(DANGLE-DANLGE0)/2 + (DIRPIX - SpecularPixel) * W / 2L`

where ``W`` is the pixel width (0.7 mm) and ``L`` is the sample-to-detector distance.

With both options, an offset can be added to the resulting angle by setting the ``AngleOffset`` property.

.. categories::

.. sourcelink::
