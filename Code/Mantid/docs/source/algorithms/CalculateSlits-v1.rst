.. algorithm::

.. summary::

.. alias::

.. properties::

Description
-----------

This algorithm can be used to calculate the slit dimensions to use for
reflectometry instruments. It is effectively the inverse of :ref:`algm-CalculateResolution`.

CalculateSlits uses nothing but the input properties to calculate the output, specifically:

.. math::

   Slit2 = (Footprint \times sin\alpha) - (2 \times Slit2SA \times tan(\alpha \times Resolution))

   Slit1 = (2 \times Slit1Slit2 \times tan(\alpha \times Resolution)) - Slit2

where :math:`\alpha` is the angle in radians.

Usage
-----

.. testcode::

  s1, s2 = CalculateSlits(Slit1Slit2=1940.5, Slit2SA=364, Angle=0.7, Footprint=50, Resolution=0.03)
  print("Slit 1: %.3f mm" % s1)
  print("Slit 2: %.3f mm" % s2)

.. testoutput::

  Slit 1: 1.078 mm
  Slit 2: 0.344 mm

.. categories::
