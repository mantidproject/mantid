.. _func-Activation:

==========
Activation
==========

.. index:: Activation

Description
-----------

An activation fitting function, when using Kelvin, may be used to describe:

.. math:: y = A_R\times e^{-\frac{b}{x}}

where:
- A_R - Attempt Rate
- b - Barrier energy


When fitting to meV instead it can be described as:

.. math:: y = A_R\times e^{-\frac{E\times b}{1000 k_B x}}

where:
E = energy spin
k_B = Boltzmann Constant



Examples
--------

An example of when this might be used is for examining the muonium state of CdS at low temperatures[1].



.. attributes::

.. properties::

References
----------
[1] Gil, J.M et al (1999). Novel Muonium State in CdS. Phys. Rev. Lett., Vol 83 Issue 25, 5294-5297 `doi: 10.1103/PhysRevLett.83.5294 <https://doi.org/10.1103/PhysRevLett.83.5294>`.

.. categories::

.. sourcelink::
