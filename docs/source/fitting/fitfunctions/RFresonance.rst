.. _func-RFresonance:

===========
RFresonance
===========

.. index:: RFresonance

Description
-----------

General off resonance case of an applied RF field.

.. math:: A(t)= A_0\{1+\left(\cos(\gamma_\mu Bt)e^{-{\Delta t}^2-1}-1\right)\frac{B^2}{B_\text{eff}^2}\}

with

.. math:: B_\text{eff} = \sqrt{B^2 + (B - B_0)^2}

,where,

:math:`A_0` is the amplitude,

:math:`B` (G) is applied B field,

:math:`B_0` (G) is the central B field of the resonance,

and :math:`\Delta` (MHz) is the decay rate.

.. plot::
	
   from mantid.simpleapi import FunctionWrapper
   import matplotlib.pyplot as plt
   import numpy as np
   x = np.arange(0.1,16,0.1)
   y = FunctionWrapper("RFresonance")
   fig, ax=plt.subplots()
   ax.plot(x, y(x))
   ax.set_xlabel('t($\mu$s)')
   ax.set_ylabel('A(t)')


.. attributes::

.. properties::

References
----------

[1]  `S.R. Kreitzman, Hyperfine Interactions 65 1058 (1990) <https://link.springer.com/content/pdf/10.1007%2FBF02397762.pdf>`_.

.. categories::

.. sourcelink::
