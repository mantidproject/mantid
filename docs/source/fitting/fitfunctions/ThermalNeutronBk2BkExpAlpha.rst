.. _func-ThermalNeutronBk2BkExpAlpha:

===========================
ThermalNeutronBk2BkExpAlpha
===========================

.. index:: ThermalNeutronBk2BkExpAlpha

Description
-----------

ThermalNeutronBk2BkExpAlpha is a function to calculate :math:`\alpha(d_h)` of :ref:`ThermalNeutronBk2BkExpConvPVoigt <func-ThermalNeutronBk2BkExpConvPVoigt>`.
It is defined as

.. math:: \alpha(d_h)  = \left(n\alpha_0 + \alpha_1 d_h + (1-n)\alpha_0^t - \frac{\alpha_1^t}{d_h}\right)^{-1}

where

.. math:: n = \frac{1}{2} \mathit{erfc}(\text{Width}(t_\text{cross} - d^{-1}))

and

.. math:: erfc(x) = 1-erf(x) = 1-\frac{2}{\sqrt{\pi}}\int_0^xe^{-u^2}du.

.. attributes::

.. properties::

.. categories::

.. sourcelink::
