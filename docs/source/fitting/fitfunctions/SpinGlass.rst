.. _func-SpinGlass:

=========
SpinGlass
=========

.. index:: SpinGlass

Description
-----------

Fitting function for use by Muon scientists defined by:

.. math:: A(t) = A_0\left(\frac{1}{3}e^{-\sqrt{\Omega t}}+\frac{2}{3}\left(1-\frac{Qa^2t^2}{\sqrt{\Omega t+Qa^2t^2}}\right)e^{-\sqrt{\Omega t + Qa^2t^2}}\right)

.. math:: \Omega = \frac{4(1-Q)a^2}{\nu}

where,

:math:`A_0` is the amplitude,

:math:`Q` is the order parameter,

:math:`\nu` is the rate of Markovian modulation,

and :math:`a` is the half-width half maximum of the local field Lorentzian Distribution.

Note that :math:`0<q<1` and :math:`\gamma>0`

.. plot::
	
   from mantid.simpleapi import FunctionWrapper
   import matplotlib.pyplot as plt
   import numpy as np
   x = np.arange(0.0,16,0.01)
   y = FunctionWrapper("SpinGlass")
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
