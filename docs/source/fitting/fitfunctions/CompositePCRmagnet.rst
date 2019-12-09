.. _func-CompositePCRmagnet:

==================
CompositePCRmagnet
==================

.. index:: CompositePCRmagnet

Description
-----------

ZF KuboToyabe when B = 0. A damped cosine precession when :math:`B_int>0` .

When B = 0,

.. math:: A(t)= A_0\left(\frac{1}{3}+\frac{2}{3}\cos(g_\mu B_0t)e^{-\sigma^2 t^2+\frac{2}{5}\Delta^2 t^2}\right) .

When :math:`B > 0` ,

.. math:: A(t) = A_0\left(\frac13+\frac23(1-(\Delta t)^2)e^{-(\Delta t)^2/2}\right)

where,

:math:`A_0` is the amplitude, 

:math:`\sigma` (MHz) is the gaussian relaxation rate,

:math:`B_0` (G) is the gaussian relaxation rate,

and :math:`\Delta` is the relaxation rate of the extra term when :math:`B>0` .

.. plot::
	
   from mantid.simpleapi import FunctionWrapper
   import matplotlib.pyplot as plt
   import numpy as np
   x = np.arange(0.1,16,0.1)
   y = FunctionWrapper("CompositePCRmagnet")
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
