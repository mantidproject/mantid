.. _func-Meier:

=====
Meier
=====

.. index:: Meier

Description
-----------

Time dependence of the polarization function for a static muon interacting with nuclear spin [1].

.. math:: A(t)=\frac13(2P_x+P_z)

,where

.. math:: P_z(t) = \frac{1}{2J+1}\left\{1+\sum^J_{m=-J+1}[\cos^2(2\alpha_m)+\sin^2(2\alpha_m)\cos(\lambda^+_m-\lambda^-_m)t]\right\},

.. math:: P_x(t) = \frac{1}{2J+1}\sum^J_{m=-J} \{ \cos^2\alpha_{m+1}\sin^2\alpha_m\cos(\lambda_{m+1}^+-\lambda_m^+)t +\cos^2\alpha_{m+1}\cos^2\alpha_m\cos(\lambda_{m+1}^+-\lambda_m^-)t +\sin^2\alpha_{m+1}\sin^2\alpha_m\cos(\lambda_{m+1}^--\lambda_m^+)t +\sin^2\alpha_{m+1}\cos^2\alpha_m\cos(\lambda_{m+1}^--\lambda_m^-)t\},
	
.. math:: \lambda_m^\pm = \frac{1}{2}[\omega_Q(2m^2-2m+1)+\omega_D\pm W_m],

.. math:: W_m = \{(\omega_D+\omega_Q)^2(2m-1)^2+\omega_D^2[J(J+1)-m(m-1)]\}^\frac{1}{2},

.. math:: tan(2\alpha_m)=\frac{\omega_D[J(J+1)-m(m-1)]^\frac{1}{2}}{(1-2m)(\omega_D+\omega_Q)},

:math:`\omega_D` is the angular frequency due to dipolar coupling,

:math:`\omega_Q` is the angular frequency due to quadrupole interaction of the nuclear spin :math:`J` due to a field gradient exerted by the presence of the muon,

:math:`J` is the total angular momentum quantum number,

and :math:`m` is the z-component of the total orbital quantum number.

.. plot::
	
   from mantid.simpleapi import FunctionWrapper
   import matplotlib.pyplot as plt
   import numpy as np
   x = np.arange(0.1,16,0.1)
   y = FunctionWrapper("Meier")
   fig, ax=plt.subplots()
   ax.plot(x, y(x))
   ax.set_xlabel('t($\mu$s)')
   ax.set_ylabel('A(t)')


.. attributes::

References
----------

[1]  `P.H. Meier, HFI 17-19 427-434 (1984) <https://link.springer.com/content/pdf/10.1007%2FBF02064848.pdf>`_.

.. properties::

.. categories::

.. sourcelink::
