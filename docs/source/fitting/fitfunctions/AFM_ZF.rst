.. _func-AFM_ZF:

======
AFM_ZF
======

.. index:: AFM_ZF

Description
-----------

ZF oscillation for aligned Anti-ferromagnetic magnetic sample with Gaussian dephasing

.. math:: A(t)  = \frac{A_0}{2}((1-a_1)+a_1\cos(\omega_1t+\phi))+(1-a_2)+a_2\cos(\omega_2t+\phi))

where,

.. math:: a_1 =\sin^2\theta ,

.. math:: \omega = 2\pi\nu ,

:math:`\nu` is the ZF frequency (MHz),

:math:`\sigma` is the Gaussian relaxation for oscillatory component,

:math:`\theta` is the angle of internal field w.r.t. to applied field,

and :math:`\phi` is the phase.

.. plot::
	
   from mantid.simpleapi import FunctionWrapper
   import matplotlib.pyplot as plt
   import numpy as np
   x = np.arange(0.1,16,0.1)
   y = FunctionWrapper("AFM_ZF")
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
