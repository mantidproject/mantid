.. _func-SilverBaseline:

==============
SilverBaseline
==============

.. index:: SilverBaseline

Description
-----------

.. math:: A(t)=A_0e^{-0.0015t}

where,

:math:`A_0` is the amplitude at :math:`t=0`.

.. plot::
	
   from mantid.simpleapi import FunctionWrapper
   import matplotlib.pyplot as plt
   import numpy as np
   x = np.arange(0.1,16,0.1)
   y = FunctionWrapper("SilverBaseline", A0 = 0.5, NuD = 0.5, Lambda = 0.3, Sigma = 0.05, Phi = 0.0)
   fig, ax=plt.subplots()
   ax.plot(x, y(x))
   ax.set_xlabel('t($\mu$s)')
   ax.set_ylabel('A(t)')


.. attributes::

.. properties::

.. categories::

.. sourcelink::
