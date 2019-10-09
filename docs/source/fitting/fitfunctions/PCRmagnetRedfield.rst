.. _func-PCRmagnetRedfield:

=================
PCRmagnetRedfield
=================

.. index:: PCRmagnetRedfield

Description
-----------

ZF signal from PCR ordered magnet with Redfield Fluctuations.

.. math:: A(t)= \begin{cases} A_0\left(\frac13+\frac23e^{-\frac{\Delta^4}{q}t}\cos{\omega t}\right), & q\geq 0 \\ A_0\left(\frac13+\frac23\cos{\omega t}\right), & \text{otherwise} \end{cases}

where,

:math:`q=\nu(\nu^2+\Delta^2)` ,

:math:`\omega = 2\pi\Delta` ,

:math:`A_0` is the amplitude, 

:math:`\lambda` (MHz) is the relaxation rate,

and :math:`\Delta` is the precession frequency.

.. plot::
	
   from mantid.simpleapi import FunctionWrapper
   import matplotlib.pyplot as plt
   import numpy as np
   x = np.arange(0.1,16,0.1)
   y = FunctionWrapper("PCRmagnetRedfield")
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
