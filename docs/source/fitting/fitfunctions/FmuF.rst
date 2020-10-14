.. _func-FmuF:

====
FmuF
====

.. index:: FmuF

Description
-----------

A fitting function for muon-fluorine bonds in ionic crystals.

.. math:: A(t)=A_0e^{-\frac{(\sigma t)^2}{2}}e^{-\lambda t}G(t)

where,

:math:`\lambda` is the exponential decay rate,

:math:`\sigma` is the Gaussian decay rate,

:math:`\omega_D` (rad/s) is the dipolar interaction angular frequency,

and the expression for :math:`G(t)` and :math:`\omega_D` are given by

.. math:: G(t)=\frac{1}{6}\left[3+\cos(\sqrt{3} \omega_\text{D} t)+\left(1-\frac{1}{\sqrt{3}}\right)\cos(\frac{3-\sqrt{3}}{2}\omega_\text{D} t)+\left(1+\frac{1}{\sqrt{3}}\right)\cos(\frac{3+\sqrt{3}}{2}\omega_\text{D} t)\right]

and

.. math:: \omega_D = 2\pi\nu

where :math:`\nu` (MHz) is the dipolar interaction frequency.

.. figure:: /images/FmuF.png

.. attributes::

.. properties::

References
----------

[1]  `J.H.Brewer et al., PRB 33 11 (1986) <https://journals.aps.org/prb/pdf/10.1103/PhysRevB.33.7813>`_.

.. categories::

.. sourcelink::
