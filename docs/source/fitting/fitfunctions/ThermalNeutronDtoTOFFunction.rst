.. _func-ThermalNeutronDtoTOFFunction:

============================
ThermalNeutronDtoTOFFunction
============================

.. index:: ThermalNeutronDtoTOFFunction

Description
-----------

ThermalNeutronDtoTOFFunction is a function to calculate TOF from d-spacing values for :ref:`ThermalNeutronBk2BkExpConvPVoigt <func-ThermalNeutronBk2BkExpConvPVoigt>`.
It is defined as

.. math:: TOF = n(zero + dtt1 \cdot dh)+(1-n)(zero_t + dtt1t\cdot dh - \frac{dtt2t}{dh})

where

.. math:: n = \frac{1}{2} \mathit{erfc}(\text{Width}(t_\text{cross} - d^{-1})).

.. attributes::

.. properties::

.. categories::

.. sourcelink::
