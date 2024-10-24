.. _func-Hall-RossSQE:

===========
HallRossSQE
===========

.. index:: HallRossSQE

Description
-----------

This fitting function models the dynamic structure factor
for a particle undergoing jump diffusion using the Hall-Ross Jump diffusion model [1]_.

.. math::

   S(Q,E) = Height \cdot \frac{1}{\pi} \frac{\Gamma}{\Gamma^2+(E-Centre)^2}

   \Gamma = \frac{\hbar}{\tau} \cdot \left(1-\exp(-\frac{l^2 Q^2}{2})\right)

where:

-  :math:`Height` - Intensity scaling, a fit parameter
-  :math:`Centre` - Centre of peak, a fit parameter
-  :math:`Tau` - Residence time, a fit parameter
-  :math:`l` - Jump lengths, a fit parameter
-  :math:`Q` - Momentum transfer, an attribute (non-fitting)

Units of :math:`l` are inverse units of :math:`Q`.

Units of :math:`HWHM` are :math:`meV` if units of :math:`\tau` are *ps*.
Alternatively, units of :math:`HWHM` are :math:`\mu eV` if units of
:math:`\tau` are *ns*.

.. attributes::

.. properties::

References
----------

.. [1] P. L. Hall and D. K. Ross, Mol. Phys. 42, 673 (1981) <http://dx.doi.org/10.1080/00268978100100521>

.. categories::

.. sourcelink::
