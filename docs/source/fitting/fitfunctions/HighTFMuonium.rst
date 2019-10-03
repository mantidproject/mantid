.. _func-HighTFMuonium:

==============
HighTFMuonium
==============

.. index:: HighTFMuonium

Description
-----------

Pair of high TF muonium rotation frequencies.

.. math:: A(t)=\frac{A_0}{2}(\cos(\omega_1t+\phi)+\cos(\omega_2t+\phi))

where,

:math:`A_0` is the amplitude at time :math:`t=0` ,

:math:`g_\mu = 0.01355342` ,

:math:`g_e = 2.8024` ,

:math:`\omega_2 = \Omega - \omega_-` ,

:math:`\omega_1 = \omega_2 + 2\pi\nu` ,

:math:`\omega_- = 2\pi(g_e - g_m)B` ,

:math:`\Omega = 2\pi\left(\frac{1}{2}\sqrt{(\nu^2+((g_e+g_m)B)^2)} - \frac{1}{2}\nu\right)`

and :math:`\nu` is the frequency (MHz).

.. plot::
	
   from mantid.simpleapi import FunctionWrapper
   import matplotlib.pyplot as plt
   import numpy as np
   x = np.arange(0.1,16,0.1)
   y = FunctionWrapper("HighTFMuonium")
   fig, ax=plt.subplots()
   ax.plot(x, y(x))
   ax.set_xlabel('t($\mu$s)')
   ax.set_ylabel('A(t)')


.. attributes::

.. properties::

References
----------

[1]  `P. Percival, Radiochimica Acta 26 1-14 (1979) <https://core.ac.uk/download/pdf/85213318.pdf>`_.

.. categories::

.. sourcelink::
