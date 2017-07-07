.. _func-GramCharlier:

============
GramCharlier
============

.. index:: GramCharlier

Description
-----------

This function implements the
`Gram-Charlier Series A <https://www.encyclopediaofmath.org/index.php/Gram-Charlier_series>`_ expansion.
It finds its main usage in fitting broad mass peaks in Y space within Neutron Compton Scattering experiments such
as on the `Vesuvio <http://www.isis.stfc.ac.uk/instruments/vesuvio/vesuvio4837.html>`_ instrument at ISIS. As such
the expansion includes only the even numbered Hermite polynomials, up to order 10, with the exception of the 3rd order term where
it is useful to include a differnt amplitude factor.

The function defintion is given by:

.. math::

   f(x) = A\frac{exp(-z^2)}{\sqrt{2\pi\sigma^2}}(1 + \frac{C4}{(2^4(4/2)!)}H_4(z) +
     \frac{C6}{(2^6(6/2)!)}H_6(z) + \frac{C8}{(2^8(8/2)!)}H_8(z) + \frac{C10}{(2^10(10/2)!)}H_{10}(z)) +
          Afse\frac{\sigma\sqrt{2}}{12\sqrt{2\pi\sigma^2}}exp(-z^2)H_3(z)

where :math:`z=\frac{(x-X_0)}{\sqrt{2\sigma^2}}`, :math:`H_n(z)` is the nth-order
`Hermite polynomial <http://mathworld.wolfram.com/HermitePolynomial.html>`_ and the other parameters are
defined in the properties table below.

.. attributes::

.. properties::

.. categories::

.. sourcelink::
