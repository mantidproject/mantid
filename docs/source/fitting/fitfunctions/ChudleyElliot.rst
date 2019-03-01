.. _func-ChudleyElliot:

==============
Chudley-Elliot
==============

.. index:: Chudley-Elliot

Description
-----------

Models the Q dependence of the QENS line width (Gamma (hwhm)), diffusion
coefficients (D), residence times (tau) and jump lengths (length) to extract the
associated long range diffusive motions of molecules.

The Chudley-Elliot Jump diffusion model [1]_ has the form:

.. math:: HWHM(Q) = \frac{\hbar}{\tau} \cdot \left(1 - \frac{sin(Ql)}{Ql}\right)

Units of :math:`l` are inverse units of :math:`Q`.

Units of :math:`HWHM` are :math:`meV` if units of :math:`\tau` are *ps*.
Alternatively, units of :math:`HWHM` are :math:`\mu eV` if units of
:math:`\tau` are *ns*.

.. attributes::

.. properties::

References
----------

.. [1] C T Chudley and R J Elliott `1961 Proc. Phys. Soc. 77 353 <http://dx.doi.org/10.1088/0370-1328/77/2/319>`__

.. categories::

.. sourcelink::
