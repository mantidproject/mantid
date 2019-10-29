.. _func-ZFMuonium:

=========
ZFMuonium
=========

.. index:: ZFMuonium

Description
-----------

ZF rotation for axial muonium

.. math:: A(t)=\frac{A_0}{6}\{A_1\cos(\omega_{1}+\phi)+A_2\cos(\omega_{2}+\phi)+A_{3}\cos(\omega_{3}+\phi)\}

and,

.. math:: \delta= \frac{\chi}{\sqrt{1+\chi^2}},

.. math:: \omega_{1}= 2\pi(\text{FreqA} - \text{FreqD}),

.. math:: \omega_{2}= 2\pi\left(\text{FreqA} + \frac{\text{FreqD}}{2}\right),

.. math:: \omega_{3}= 3\pi \text{FreqD},

.. math:: A_{i}=\frac{1}{(1+(\omega_{i}/(2\pi F_\text{cut}))^2)}, 0<i<4

where,

:math:`A_0` is the amplitude,

FreqA (MHz) is the isotropic hyperfine coupling constant,

FreqD (MHz) is the anisotropic hyperfine coupling constant,

:math:`F_\text{cut}` is the frequency cut,

and :math:`\phi` (rad) is the phase at time :math:`t=0` .

.. plot::
	
   from mantid.simpleapi import FunctionWrapper
   import matplotlib.pyplot as plt
   import numpy as np
   x = np.arange(0.1,16,0.1)
   y = FunctionWrapper("ZFMuonium")
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
