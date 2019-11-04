.. _func-AFM_LF:

======
AFM_LF
======

.. index:: AFM_LF

Description
-----------

A pair of frequencies for aligned Anti-ferrormagnetic magnetism in Longitudinal Fields.

.. math:: A(t)  = \frac{A_0}{2}((1-a_1)+a_1\cos(\omega_1t+\phi))+(1-a_2)+a_2\cos(\omega_2t+\phi))

where, 

.. math:: a_1 =\frac{(f_a\sin\theta)^2}{(f_b+f_a\cos\theta)^2+(f_a\sin\theta)^2} ,

.. math:: a_2 =\frac{(f_a\sin\theta)^2}{((f_b-f_a\cos\theta)^2+(f_a\sin\theta)^2)} ,

.. math:: \omega_1 = 2\pi\sqrt{f_a^2+f_b^2+2f_af_b\cos\theta} ,

.. math:: \omega_2 = 2\pi\sqrt{f_a^2+f_b^2-2f_af_b\cos\theta} ,

:math:`f_a` is the ZF frequency (MHz),

:math:`f_b = 0.01355 B` for B is the applied field,

:math:`\theta` is the angle of internal field w.r.t. to applied field,

and :math:`\phi` is the phase.

.. plot::
	
   from mantid.simpleapi import FunctionWrapper
   import matplotlib.pyplot as plt
   import numpy as np
   x = np.arange(0.1,16,0.1)
   y = FunctionWrapper("AFM_LF")
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
