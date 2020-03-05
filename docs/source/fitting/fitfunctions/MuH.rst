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

:math:`\omega_D = 2 \pi \nu_D` where :math:`\nu_D` (MHz) is the oscillating frequency,

and :math:`\phi` (rad) is the phase at time :math:`t=0`.

.. plot::
	
   from mantid.simpleapi import FunctionWrapper
   import matplotlib.pyplot as plt
   import numpy as np
   x = np.arange(0.1,16,0.1)
   y = FunctionWrapper("MuH", A0 = 0.5, NuD = 0.5, Lambda = 0.3, Sigma = 0.05, Phi = 0.0)
   fig, ax=plt.subplots()
   ax.plot(x, y(x))
   ax.set_xlabel('t($\mu$s)')
   ax.set_ylabel('A(t)')

.. attributes::

.. properties::

References
----------

[1]  `T. Lancaster et al., J. Phys.: Condens. Matter 21 346004 (2009) <https://iopscience.iop.org/article/10.1088/0953-8984/21/34/346004/pdf>`_.

.. categories::

.. sourcelink::
