.. _func-TFMuonium:

=========
TFMuonium
=========

.. index:: TFMuonium

Description
-----------

General case TF muonium rotation

.. math:: A(t)=\frac{A_0}{4}\{(1+\delta)a_{12}\cos(\omega_{12}+\phi)+ (1-\delta)a_{14}\cos(\omega_{14}+\phi)+(1+\delta)a_{34}\cos(\omega_{34}+\phi)+(1-\delta)a_{23}\cos(\omega_{23}+\phi)\}

and,

.. math:: \delta= \frac{\chi}{\sqrt{1+\chi^2}},

.. math:: \chi = (g_\mu+g_e)\frac{B}{A},

.. math:: d = \frac{(g_e-g_\mu)}{g_e+g_\mu},

.. math:: E_1=\frac{A}{4}(1+2d\chi) \qquad E_2=\frac{A}{4}(-1+2\sqrt{1+\chi^2})

.. math:: E_3=\frac{A}{4}(1-2d\chi) \qquad E_4=\frac{A}{4}(-1-2\sqrt{1+\chi^2}),

.. math:: \omega_{ij}= 2 \pi (E_i - E_j),

.. math:: a_{ij}=\frac{1}{(1+(\omega_{ij}/(2\pi f_\text{cut}))^2)},

where,

:math:`A_0` is the amplitude,

A (MHz) is the isotropic hyperfine coupling constant,

:math:`\phi` (rad) is the phase at time :math:`t=0`,

:math:`g_\mu = 0.01355342` , the gyromagnetic ratio of muon,

:math:`g_e = 2.8024` , the gyromagnetic ratio of electron,

and :math:`f_\text{cut} = 10^{32}`.

.. plot::
	
   from mantid.simpleapi import FunctionWrapper
   import matplotlib.pyplot as plt
   import numpy as np
   x = np.arange(0.1,16,0.1)
   y = FunctionWrapper("TFMuonium")
   fig, ax=plt.subplots()
   ax.plot(x, y(x))
   ax.set_xlabel('t($\mu$s)')
   ax.set_ylabel('A(t)')


.. attributes::

.. properties::

References
----------

[1]  `P. Percival, TRIUMF Summer Institute 2011 <http://www.triumf.info/hosted/TSI/TSI11/lectures/L9-Muonium.pdf>`_.

.. categories::

.. sourcelink::
