.. _func-Bessel:

==============================
Bessel
==============================

.. index:: Bessel

Description
-----------

Fitting function for use by Muon scientists defined by:

.. math:: 
	\begin{align} A(t)&=A_0J_0(2\pi\nu t+\phi),\\ \text{where} \qquad J_0(x)&=\sum_{l=0}^{\infty}\frac{(-1)^l}{2^{2l}(l!)^2}x^{2l} \end{align}

.. figure:: /images/Bessel.png
   :alt: Bessel.png

.. attributes::

.. properties::
:math:`J_0(x)` is the Bessel Function of the first kind
:math:`A_0` is the amplitude at :math:`t=0`
:math:`\nu` is the wavenumber
:math:`\phi` is the phase

References
----------
[1]`Savici et al., PRB 66 (2002) <https://journals.aps.org/prb/pdf/10.1103/PhysRevB.66.014524>`_.
[2]`MathWorks Documentation besselj <https://www.mathworks.com/help/symbolic/besselj.html>`

.. categories::

.. sourcelink::
