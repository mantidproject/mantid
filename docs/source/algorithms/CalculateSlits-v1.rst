.. algorithm::

.. summary::

.. relatedalgorithms::

.. properties::

Description
-----------

This algorithm can be used to calculate the slit dimensions to use for
reflectometry instruments. It is effectively the inverse of :ref:`algm-NRCalculateSlitResolution`.

CalculateSlits uses nothing but the input properties to calculate the output, specifically:

.. math::

   Slit2 = (Footprint \times sin\alpha) - (2 \times Slit2SA \times tan(\alpha \times Resolution))

   Slit1 = (2 \times Slit1Slit2 \times tan(\alpha \times Resolution)) - Slit2

where :math:`\alpha` is the angle in radians (conversion between degrees and radians is implemented by the algorithm).

Footprint
---------

The footprint of the experiment is not necessarily instrument specific and can be thought of as the area of the sample
that is reach by the beam during the experiment. When providing the value of the footprint to the algorithm it should be
based on the footprint that you would wish to use for the experiment as this information cannot be taken directly from
the instrument definition and is experiment-dependent.

Usage
-----

.. testcode::

  s1, s2 = CalculateSlits(Slit1Slit2=1940.5, Slit2SA=364, Angle=0.7, Footprint=50, Resolution=0.03)
  print("Slit 1: {:.3f} mm".format(s1))
  print("Slit 2: {:.3f} mm".format(s2))

.. testoutput::

  Slit 1: 1.078 mm
  Slit 2: 0.344 mm

.. categories::

.. sourcelink::
