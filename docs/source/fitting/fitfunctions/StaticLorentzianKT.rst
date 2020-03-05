.. _func-StaticLorentzianKT:

==================
StaticLorentzianKT
==================

.. index:: StaticLorentzianKT

Description
-----------

Static Lorentzian Kubo-Toyabe function:

.. math:: g_{z}^{L}(t,B_L) = A_0 \{1 - \frac{a}{\omega_{L}}j_1(\omega_{L}t)e^{-at}-\left(\frac{a}{\omega_L}\right)^2(j_o(\omega_{L}t)e^{-at}-1)-\left(1+\left(\frac{a}{\omega_L}\right)^2\right)a\int_{0}^{t}j_0(\omega_{L}\tau)e^{-a\tau}d\tau\}

where,

:math:`L` refers to Lorentzian,

:math:`B_L` refers to the longitudinal field applied to the z-axis,

:math:`j_{i}` are the spheical Bessel functions of the First Kind,

:math:`\omega_L` is is the precessing angular frequency and its relationship is given by :math:`B_L= \omega_{L} / \gamma_{\mu}`,

:math:`\gamma_{\mu}` is the gyromagnetic ratio of muons,

and :math:`a (\mu s^{-1})` is the half-width at half maximum of the Lorentzian distribution.

.. plot::
	
   from mantid.simpleapi import FunctionWrapper
   import matplotlib.pyplot as plt
   import numpy as np
   x = np.arange(0.1,16,0.1)
   y = FunctionWrapper("StaticLorentzianKT")
   fig, ax=plt.subplots()
   ax.plot(x, y(x))
   ax.set_xlabel('t($\mu$s)')
   ax.set_ylabel('A(t)')

.. attributes::

.. properties::

References
----------

[1]  `Y. Uemura et al., Phys. Rev. B 31 546 (1985) <https://journals.aps.org/prb/pdf/10.1103/PhysRevB.31.546>`_.

.. categories::

.. sourcelink::
