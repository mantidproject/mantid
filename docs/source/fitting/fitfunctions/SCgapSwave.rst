.. _func-SCgapSwave:

==========
SCgapSwave
==========

.. index:: SCgapSwave

Description
-----------

Calculation of temperature dependence of :math:`\frac{1}{\lambda^2}` of S wave gap symmetry.

.. math:: \tilde{I}(T) = 1-\frac{E_\text{c}}{2k_\text{B}T}\int^1_0\frac{1}{\cosh^2(\sqrt{(E_\text{c}E)^2+ \Delta^2(T)}/2k_\text{B}T)} dE

and,

.. math:: \Delta(T) = \Delta_0\tanh\bigg[1.82\left(1.018\frac{T_\text{c}}{T}-1\right)^{0.51}\bigg]

where,

:math:`\tilde{I}(T)` is the integral of \frac{1}{\lambda^2},

:math:`E_\text{c}` (meV) is the cutoff limit of :math:`E` ,

:math:`k_\text{B}` is the Boltzman constant,

:math:`\Delta_0` (meV) is the value of the superconducting gap,

and :math:`T_\text{c}` is the critical temperature.

.. plot::
	
   from mantid.simpleapi import FunctionWrapper
   import matplotlib.pyplot as plt
   import numpy as np
   x = np.arange(1.0,10,0.1)
   y = FunctionWrapper("SCgapSwave")
   fig, ax=plt.subplots()
   ax.plot(x, y(x))
   ax.set_xlabel('t($\mu$s)')
   ax.set_ylabel('A(t)')

.. attributes::

.. properties::

References
----------

[1]  `A. Carrington et al., Physica C 385 1-2 205 (2003) <https://www.sciencedirect.com/science/article/pii/S0921453402023195>`_.

[2]  `Pabitra. K. Biswas et al., PRB 98 180501 (2018) <https://journals.aps.org/prb/pdf/10.1103/PhysRevB.98.180501>`_.

.. categories::

.. sourcelink::
