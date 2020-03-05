.. _func-ZFelectronDipole:

================
ZFelectronDipole
================

.. index:: ZFelectronDipole

Description
-----------

ZF PCR signal from interaction with a single dipole.

.. math:: A(t)=\frac{A_0}{6}\left(1+e^{-\lambda t}\left(\cos(g_\mu B_\text{D} t)+ 2\cos(\frac{3}{2}g_\mu B_\text{D} t)+2\cos(\frac{1}{2}g_\mu B_\text{D} t)\right)\right)

.. math:: B_\text{D}= \begin{cases} \frac{8290}{r^3}, & r > 0 \\ 0 , & r = 0 \end{cases}

where,

:math:`A_0` is the amplitude, 

:math:`\lambda` (MHz) is the relaxation rate,

:math:`B_\text{D}` (G) is the dipolar field,

:math:`r` (Armstrong) is the radius,

:math:`\mu_p` is the proton moment,

and :math:`\mu_n` is the proton moment.

.. plot::
	
   from mantid.simpleapi import FunctionWrapper
   import matplotlib.pyplot as plt
   import numpy as np
   x = np.arange(0.1,16,0.1)
   y = FunctionWrapper("ZFelectronDipole")
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
