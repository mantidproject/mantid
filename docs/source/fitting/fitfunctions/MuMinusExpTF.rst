.. _func-MuMinusExpTF:

============
MuMinusExpTF
============

.. index:: MuMinusExpTF

Description
-----------

A relatxation function for negative muons in Traverse field.

.. math:: N(t)=N_0e^{-frac{t}{\tau}}\left(1+Ae^{-\lambda t}\cos(2\pi\nu t + \phi)\right)

where,

:math:`N_O` is the count at :math:`t=0` ,

:math:`\tau` is the muon decay life time,

:math:`A` is the relative amplitude of the oscillatory term, 

:math:`\lambda` (MHz) is the relaxation rateo f the oscillatory term,

:math:`\nu` is the oscillating frequency,

and :math:`\phi` is the phase.

.. plot::
	
   from mantid.simpleapi import FunctionWrapper
   import matplotlib.pyplot as plt
   import numpy as np
   x = np.arange(0.1,16,0.1)
   y = FunctionWrapper("MuMinusExpTF")
   fig, ax=plt.subplots()
   ax.plot(x, y(x))
   ax.set_xlabel('t($\mu$s)')
   ax.set_ylabel('A(t)')

.. attributes::

.. properties::

References
----------

[1]  `PSI MUSRFIT documentation <http://lmu.web.psi.ch/musrfit/user/html/user-manual.html#n12>`_.

.. categories::

.. sourcelink::
