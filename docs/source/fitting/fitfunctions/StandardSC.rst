.. _func-StandardSC:

==========
StandardSC
==========

.. index:: StandardSC

Description
-----------

Sinusoidal function damped with Gaussian relaxation and a non-decaying oscillation:

.. math:: A(t)=A_0e^{-\frac{\sigma^2t^2}{2}}\cos(\omega_\text{SC}t+\phi) + A_\text{bg}\cos(\omega_\text{BG}t+\phi)

where,

:math:`A_\text{bg}` is the amplitude of the background field oscillation,

:math:`A_0` is the amplitude of the decaying oscillation due to internal field,

:math:`\omega_\text{SC}` (rad/s) is the angular frequency of internal field,

:math:`\omega_\text{BG}` (rad/s) is the angular frequency of background field,

:math:`\sigma` is the depolarization rate,

and :math:`\phi` is the phase.

The relationship between :math:`\omega` and magnetic field B is given by:

.. math:: \omega = 2 \pi \nu = \gamma_\mu \text{B}

where, 

:math:`\gamma_\mu` is the gyromagnetic ratio of muon.

.. plot::
	
   from mantid.simpleapi import FunctionWrapper
   import matplotlib.pyplot as plt
   import numpy as np
   x = np.arange(0.1,16,0.1)
   y = FunctionWrapper("StandardSC")
   fig, ax=plt.subplots()
   ax.plot(x, y(x))
   ax.set_xlabel('t($\mu$s)')
   ax.set_ylabel('A(t)')


.. attributes::

.. properties::

References
----------

[1]  `J.A.T. Barker et al., Phys. Rev. Lett. 115 267001 (2015) <https://journals.aps.org/prl/pdf/10.1103/PhysRevLett.115.267001>`_.

.. categories::

.. sourcelink::
