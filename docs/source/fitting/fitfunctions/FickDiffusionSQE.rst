.. _func-FickDiffusionSQE:

================
FickDiffusionSQE
================

.. index:: FickDiffusionSQE

Description
-----------

This fitting function models the dynamic structure factor
for a particle undergoing jump diffusion using Fick's law for diffusion [1]_.

.. math::

   S(Q,E) = Height \cdot \frac{1}{\pi} \frac{\Gamma}{\Gamma^2+(E-Centre)^2}

   \Gamma = DiffCoeff \cdot Q^{2}

where:

-  :math:`Height` - Intensity scaling, a fit parameter
-  :math:`DiffCoeff` - diffusion coefficient, a fit parameter
-  :math:`Centre` - Centre of peak, a fit parameter
-  :math:`Q` - Momentum transfer, an attribute (non-fitting)

.. attributes::

.. properties::

References
----------

.. [1] M. Bée, Chemical Physics 292, 121-141 (2003) <https://www.sciencedirect.com/science/article/abs/pii/S030101040300257X>

.. categories::

.. sourcelink::
