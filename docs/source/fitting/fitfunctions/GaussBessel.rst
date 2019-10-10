.. _func-GaussBessel:

===========
GaussBessel
===========

.. index:: GaussBessel

Description
-----------

Bessel function oscillation with Gaussian damp and :math:`\frac{1}{3}`component. Example: Spin Density Wave.

.. math:: A(t) = A_0\left(\frac{1}{3}+\frac{2}{3}J_0(\omega t + \phi)e^{-\frac{(\sigma t)^2}{2}}\right)

where,

:math:`N_O` is the count at :math:`t=0` ,

:math:`\sigma` (MHz) is the Gaussian relaxation rate,

:math:`\omega = 2\pi \nu` is the oscillating frequency,

:math:`\nu` (MHz) is the oscillation frequency,

and :math:`\phi` is the phase.

.. plot::
	
   from mantid.simpleapi import FunctionWrapper
   import matplotlib.pyplot as plt
   import numpy as np
   x = np.arange(0.1,16,0.1)
   y = FunctionWrapper("GaussBessel")
   fig, ax=plt.subplots()
   ax.plot(x, y(x))
   ax.set_xlabel('t($\mu$s)')
   ax.set_ylabel('A(t)')

.. attributes::

.. properties::

References
----------

[1]  `F.L. Pratt, Physica B 289-290, 710 (2000) <http://shadow.nd.rl.ac.uk/wimda/>`_.

.. categories::

.. sourcelink::
