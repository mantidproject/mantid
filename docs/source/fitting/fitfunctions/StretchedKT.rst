.. _func-StretchedKT:

===========
StretchedKT
===========

.. index:: StretchedKT

Description
-----------

For observation of muon spin relaxation MuSR spectra with lineshapes lying between a Guassian and a Lorentzian,

.. math:: A(t)=A_0\left(\frac{1}{3}+\frac{2}{3}\left(1-(\sigma t)^\beta\right)e^{-\frac{(\sigma t)^\beta}{\beta}}\right)

where,

:math:`\sigma` is KuboToyabe decay rate,

:math:`A_0` is the amplitude,

:math:`\beta` is the stretching exponential and it ranges from 1 (neglible nuclear field) and 2 (strong nuclear field). 

.. attributes::

.. properties::

.. plot::
	
   from mantid.simpleapi import FunctionWrapper
   import matplotlib.pyplot as plt
   import numpy as np
   x = np.arange(0.1,16,0.1)
   y = FunctionWrapper("StretchedKT")
   fig, ax=plt.subplots()
   ax.plot(x, y(x))
   ax.set_xlabel('t($\mu$s)')
   ax.set_ylabel('A(t)')

References
----------

[1]  `Crook et al, J. Phys.: Condens. Matter 9 1149-1158 (1997) <https://iopscience.iop.org/article/10.1088/0953-8984/9/5/018/pdf>`_.

[2]  `Lord J.S., J. Phys.: Conf. Ser. 17 81 (2005) <https://iopscience.iop.org/article/10.1088/1742-6596/17/1/014/pdf>`_.


.. categories::

.. sourcelink::
