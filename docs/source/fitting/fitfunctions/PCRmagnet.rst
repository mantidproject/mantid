.. _func-PCRmagnet:

=========
PCRmagnet
=========

.. index:: PCRmagnet

Description
-----------

ZF signal from PCR ordered magnet

.. math:: A(t)=A_0\left(\frac{1}{3}+\frac{2}{3}e^{-(\sigma\omega t')^2/2}\cos(\omega t)\right)

where,

:math:`\omega=g_\mu H_0` ,

:math:`t'=t-t_\text{off}` ,

:math:`A_0` is the amplitude,

:math:`\sigma` (MHz) is the relaxation rate,

H0 (G) is the local magnetic field,

:math:`g_\mu` is the gyromagnetic ratio of muons,

and :math:`t_\text{off}` (microsecs) is the time offset.

.. plot::
	
   from mantid.simpleapi import FunctionWrapper
   import matplotlib.pyplot as plt
   import numpy as np
   x = np.arange(0.1,16,0.1)
   y = FunctionWrapper("PCRmagnet")
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
