.. _func-MuH:

===
MuH
===

.. index:: MuH

Description
-----------

Fitting function for use by Muon scientists defined by:

.. math:: A(t)=\frac{A_0(t)}{6}e^{-\lambda t}e^{-\frac{(\sigma t)^2}{2}}\left(1+\cos(\omega_{D}t + \phi)+2\cos\left(\frac{\omega_D}{2}t+\phi\right)+2\cos\left(\frac{3\omega_D}{2}t+\phi\right)\right)

:math:`A_0` is the amplitude,

:math:`\lambda` is the exponential decay rate,

:math:`\sigma` is the gaussian decay rate,

:math:`\omega_D = 2 \pi \nu` where :math:`\nu` (MHz) is the oscillating frequency,

and :math:`\phi` (rad) is the phase at time :math:`t=0`.

.. figure:: /images/MuH.png

.. attributes::

.. properties::

References
----------

[1]  `T. Lancaster et al., J. Phys.: Condens. Matter 21 346004 (2009) <https://iopscience.iop.org/article/10.1088/0953-8984/21/34/346004/pdf>`_.

.. categories::

.. sourcelink::
