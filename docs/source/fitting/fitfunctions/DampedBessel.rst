.. _func-DampedBessel:

============
DampedBessel
============

.. index:: DampedBessel

Description
-----------

A bessel function with damped oscillation that could apply to incommensurate magnetic structures or spin density waves.

.. math:: A(t)= A_0e^{-\lambda_\text{L}t}\left( (1-f_L)e^{-\lambda_\text{T}t}J_0(\omega_\mu t + \phi) + f_L\right)

where,
 
:math:`A_0` is the amplitude of asymmetry,

:math:`J_0(x)` is the Bessel function of the first kind,

:math:`\lambda_\text{T}` is the damping of the oscillation,

:math:`\lambda_\text{L}` is the dynamic longitudinal spin relaxation rate,

:math:`B` (G) is the B-field,

and :math:`\phi` is the phase.

.. plot::
	
   from mantid.simpleapi import FunctionWrapper
   import matplotlib.pyplot as plt
   import numpy as np
   x = np.arange(0.1,16,0.1)
   y = FunctionWrapper("DampedBessel")
   fig, ax=plt.subplots()
   ax.plot(x, y(x))
   ax.set_xlabel('t($\mu$s)')
   ax.set_ylabel('A(t)')

.. attributes::

.. properties::

References
----------

[1]  `D.E. MacLaughlin, PRB 89 144419 (2014) <https://journals.aps.org/prb/pdf/10.1103/PhysRevB.89.144419>`_.

.. categories::

.. sourcelink::
