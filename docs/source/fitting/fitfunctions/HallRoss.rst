.. _func-Hall-Ross:

=========
Hall-Ross
=========

.. index:: Hall-Ross

Description
-----------

Models the Q dependence of the QENS line width (Gamma (hwhm)), diffusion
coefficients (D), residence times (tau) and jump lengths (l) to extract the
associated long range diffusive motions of molecules.

The Hall-Ross Jump diffusion model [1]_ has the form:

.. math:: Gamma(Q) = \frac{\hbar}{\tau} \cdot (1-\exp(-\frac{l^2 Q^2}{2}))

Units of :math:`l` are inverse units of :math:`Q`.

Units of :math:`Gamma` are meV if units of :math:`\tau` are ps.
Alternatively, units of :math:`Gamma` are :math:`\mu`eV if units of
:math:`\tau` are ns.

Mean square diffusion jump length :math:`= 3 l^2 \ \AA^2`.

Diffusion coefficient :math:`D = \frac{l^2}{2 \tau} \ \AA^2 ps^{-1}`.

.. attributes::

.. properties::

References
----------

.. [1] P. L. Hall and D. K. Ross, Mol. Phys. 42, 673 (1981) <http://dx.doi.org/10.1080/00268978100100521>

.. categories::

.. sourcelink::
