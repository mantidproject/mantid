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

:math:`\lambda` (MHz) is the exponential decay rate,

:math:`\sigma` (MHz) is the Gaussian decay rate,

:math:`\omega_D` (MHz) is the dipolar interaction frequency,

and the expression for :math:`G(t)` is given by

.. math:: G(t)=\frac{1}{6}\left[3+\cos(\sqrt{3} \omega_\text{D} t)+(1-\frac{1}{\sqrt{3}})\cos(\frac{3-\sqrt{3}}{2}\omega_\text{D} t)+(1+\frac{1}{\sqrt{3}})\cos(\frac{3+\sqrt{3}}{2}\omega_\text{D} t)\right].

.. figure:: /images/FmuF.png

.. attributes::

.. properties::

References
----------

[1]  `J.H.Brewer et al., PRB 33 11 (1986) <https://journals.aps.org/prb/pdf/10.1103/PhysRevB.33.7813>`_.

.. categories::

.. sourcelink::
