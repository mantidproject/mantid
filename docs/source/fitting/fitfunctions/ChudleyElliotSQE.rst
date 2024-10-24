.. _func-Chudley-ElliotSQE:

================
ChudleyElliotSQE
================

.. index:: ChudleyElliotSQE

Description
-----------

This fitting function models the dynamic structure factor
for a particle undergoing jump diffusion using the Chudley-Elliot Jump diffusion model [1]_.

.. math::

   S(Q,E) = Height \cdot \frac{1}{\pi} \frac{\Gamma}{\Gamma^2+(E-Centre)^2}

   \Gamma = \frac{\hbar}{\tau} \cdot \left(1 - \frac{sin(Ql)}{Ql}\right)

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

.. [1] C T Chudley and R J Elliott `1961 Proc. Phys. Soc. 77 353 <http://dx.doi.org/10.1088/0370-1328/77/2/319>`__

.. categories::

.. sourcelink::
