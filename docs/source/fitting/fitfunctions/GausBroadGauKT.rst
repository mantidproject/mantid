.. _func-GauBroadGauKT:

=============
GauBroadGauKT
=============

.. index:: GauBroadGauKT

Description
-----------

Gaussian-Broadened Gaussian Kubo-Toyabe relaxation function given by:

.. math:: A(t)=A_0\left(\frac{1}{3}+\frac{2}{3}\left(\frac{1+R^2}{1+R^2+R^2\Delta^2_\text{eff}t^2}\right)^{\frac{3}{2}}\left(1- \frac{\Delta^2_\text{eff}t^2}{1+R^2+R^2\Delta^2_\text{eff}t^2}\right)exp\left(\frac{-\Delta^2_\text{eff}t^2}{2(1+R^2+R^2\Delta^2_\text{eff}t^2)}\right)\right)

where

.. math:: \Delta^2_\text{eff} = \Delta^2_0 + \omega^2,

.. math:: R = \frac{\omega}{\Delta_0},

:math:`A_0`is the amplitude,

:math:`R` is the Broadening ratio,

:math:`Delta_0` is the central width,

:math:`\omega` is the rms width,

and :math:`Delta_\text{eff}` is the effective width.

.. plot::
	
   from mantid.simpleapi import FunctionWrapper
   import matplotlib.pyplot as plt
   import numpy as np
   x = np.arange(0.1,16,0.1)
   y = FunctionWrapper("GauBroadGauKT")
   fig, ax=plt.subplots()
   ax.plot(x, y(x))
   ax.set_xlabel('t($\mu$s)')
   ax.set_ylabel('A(t)')

.. attributes::

.. properties::

References
----------

[1]  `D.R. Noakes et al, PRB 56 2352 (1997) <https://journals.aps.org/prb/pdf/10.1103/PhysRevB.56.2352>`_.

[2]  `D.E. Maclaughlin et al, PRB 89 144419 (2014) <https://journals.aps.org/prb/pdf/10.1103/PhysRevB.89.144419>`_.

.. categories::

.. sourcelink::
