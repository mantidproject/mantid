.. _func-ThermalNeutronBk2BkExpBeta:

==========================
ThermalNeutronBk2BkExpBeta
==========================

.. index:: ThermalNeutronBk2BkExpBeta

Description
-----------

ThermalNeutronBk2BkExpBeta is a function to calculate :math:`\beta(d_h)` of :ref:`ThermalNeutronBk2BkExpConvPVoigt <func-ThermalNeutronBk2BkExpConvPVoigt>`.
It is defined as

.. math:: \beta(d_h)  = \left(n\beta_0 + \beta_1 d_h + (1-n)\beta_0^t - \frac{\beta_1^t}{d_h}\right)^{-1}

where

.. math:: n = \frac{1}{2} \mathit{erfc}(\text{Width}(t_\text{cross} - d^{-1}))

and

.. math:: erfc(x) = 1-erf(x) = 1-\frac{2}{\sqrt{\pi}}\int_0^xe^{-u^2}du.

.. attributes::

.. properties::

.. categories::

.. sourcelink::
