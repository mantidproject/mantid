.. _func-ModOsc:

======
ModOsc
======

.. index:: ModOsc

Description
-----------

Traverse Field sinususodally modulated oscillation. Sinusoidal distribution of precession frequencies about bias e.g. from SDW.

.. math:: A(t)=A_0 \cos(\omega t + \phi)J_0(\omega_\text{mod}t)

where,

:math:`A_0` is the amplitude, 

:math:`\omega` is the oscillation frequency,

:math:`phi` is the phase,

and :math:`\omega_\text{mod}` (G) is the modulation frequency.

.. plot::
	
   from mantid.simpleapi import FunctionWrapper
   import matplotlib.pyplot as plt
   import numpy as np
   x = np.arange(0.1,16,0.1)
   y = FunctionWrapper("ModOsc")
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
