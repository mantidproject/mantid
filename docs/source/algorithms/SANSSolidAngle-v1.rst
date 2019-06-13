
.. algorithm::

.. summary::

.. relatedalgorithms::

.. properties::

Description
-----------

Computes the solid angle for all detector pixels.

Depending on the option selected, the solid angle is calculated as follows:

**Normal**

.. math:: I'(x,y)=\frac{I(x,y)}{\cos^3(2\theta)}

**Tube**

.. math:: I'(x,y)=\frac{I(x,y)}{\cos^2(2\theta)\cos(\alpha)}

**Wing**

.. math:: I'(x,y)=\frac{I(x,y)}{\cos^3(\alpha)}

where :math:`\alpha` is the angle between the sample-to-pixel vector and its projection on the X-Z plane.
