.. _func-TeixeiraWater:

=============
TeixeiraWater
=============

.. index:: TeixeiraWater

Description
-----------

Teixeira's model for water [1]_.

Models the Q dependence of the QENS line width (Gamma (hwhm)), diffusion
coefficients (D), residence times (tau) and jump lengths (length) to extract
the associated long range diffusive motions of molecules.

This model (1961) has the form:

.. math::
    HWHM(Q) = \frac{\hbar}{\tau} \cdot \frac{(QL)^2}{6 + (QL)^2}

    D = \frac{L^2}{6 \tau}

Units of :math:`L` are inverse units of :math:`Q`.

Units of :math:`HWHM` are :math:`meV` if units of :math:`\tau` are *ps*.
Alternatively, units of :math:`HWHM` are :math:`\mu eV` if units of
:math:`\tau` are *ns*.

.. attributes::

.. properties::

References
----------

.. [1] J. Teixeira, M.-C. Bellissent-Funel, S. H. Chen, and A. J. Dianoux. `Phys. Rev. A, 31:1913–1917 <http://dx.doi.org/10.1103/PhysRevA.31.1913>`__

.. categories::

.. sourcelink::
