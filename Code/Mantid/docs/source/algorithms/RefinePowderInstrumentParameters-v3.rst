.. algorithm::

.. summary::

.. alias::

.. properties::

Description
-----------

This algorithm refines the instrumental geometry parameters for powder
diffractomers. The parameters that can be refined are Dtt1, Zero, Dtt1t,
Dtt2t, Zerot, Width and Tcross.

It serves as the second step to fit/refine instrumental parameters that
will be introduced in Le Bail Fit. It uses the outcome from algorithm
FitPowderDiffPeaks().


Introduction
============

In order to do Rietveld refinement to experimental data, the diffractometer’s profile should be calibrated by the standards, such as LaB6 or Ni, 
with known crystal structure and lattice parameters.  

For POWGEN and NOMAD, the type of the instrument profile is back-to-back exponential function convoluted with 
pseudo voigt of thermal neutron and epithermal neutron.
It means that each diffraction peak is a back-to-back exponential, 

.. math:: I\frac{AB}{2(A+B)}\left[ \exp \left( \frac{A[AS^2+2(x-X0)]}{2}\right) \mbox{erfc}\left( \frac{AS^2+(x-X0)}{S\sqrt{2}} \right) + \exp \left( \frac{B[BS^2-2(x-X0)]}{2} \right) \mbox{erfc} \left( \frac{[BS^2-(x-X0)]}{S\sqrt{2}} \right) \right].

with peak parameter :math:`A`, :math:`B`, :math:`X_0` and :math:`S`

And their corresponding peak parameters are functions described as 
.. math::

   n_{cross} = \frac{1}{2} erfc(Width(xcross\cdot d^{-1}))

   TOF_e = Zero + Dtt1\cdot d

   TOF_t = Zerot + Dtt1t\cdot d - Dtt2t \cdot d^{-1}

Final Time-of-flight is calculated as:

.. math:: TOF = n_{cross} TOF_e + (1-n_{cross}) TOF_t

Formular for calculating :math:`A(d)`, :math:`B(d)`, :math:`\sigma(d)` and :math:`\gamma(d)`
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

:math:`\alpha(d)`:

.. math::

   \alpha^e(d) = \alpha_0^e + \alpha_1^e d_h

   \alpha^t(d) = \alpha_0^t - \frac{\alpha_1^t}{d_h}

   \alpha(d)   = \frac{1}{n\alpha^e + (1-n)\alpha^t}

:math:`\beta(d)`:

.. math::

   \beta^e(d) = \beta_0^e + \beta_1^e d_h

   \beta^t(d) = \beta_0^t - \frac{\beta_1^t}{d_h}

   \beta(d)   = \frac{1}{n\alpha^e + (1-n)\beta^t}

For :math:`\sigma_G` and :math:`\gamma_L`, which represent the standard deviation for pseudo-voigt

.. math::

   \sigma_G^2(d_h) = \sigma_0^2 + (\sigma_1^2 + DST2(1-\zeta)^2)d_h^2 + (\sigma_2^2 + Gsize)d_h^4

   \gamma_L(d_h) = \gamma_0 + (\gamma_1 + \zeta\sqrt{8\ln2DST2})d_h + (\gamma_2+F(SZ))d_h^2

The analysis formula for the convoluted peak at :math:`d_h`

.. math:: \Omega(TOF(d_h)) = (1-\eta(d_h))N\{e^uerfc(y)+e^verfc(z)\} - \frac{2N\eta}{\pi}\{\Im[e^pE_1(p)]+\Im[e^qE_1(q)]\}

where

.. math::

   erfc(x) = 1-erf(x) = 1-\frac{2}{\sqrt{\pi}}\int_0^xe^{-u^2}du

   E_1(z) = \int_z^{\infty}\frac{e^{-t}}{t}dt

   u = \frac{1}{2}\alpha(d_h)(\alpha(d_h)\sigma^2(d_h)+2x)

   y = \frac{\alpha(d_h)\sigma^2(d_h)+x}{\sqrt{2\sigma^2(d_h)}}

   p = \alpha(d_h)x + \frac{i\alpha(d_h)H(d_h)}{2}

   v = \frac{1}{2}\beta(d_h)(\beta(d_h)\sigma^2(d_h)-2x)

   z = \frac{\beta(d_h)\sigma^2(d_h)-x}{\sqrt{2\sigma^2(d_h)}}

   q = -\beta(d_h)x + \frac{i\beta(d_h)H(d_h)}{2}

:math:`erfc(x)` and :math:`E_1(z)` will be calculated numerically.


Break down the problem
======================

If we can do the single peak fitting on each single diffraction peak in a certain range, 
then we can divide the optimization problem into 4 sub problems for :math:`X_0`, :math:`A`,
:math:`B` and :math:`S`, with the constraint on :math:`n`, the ratio between thermal 
and epi thermal neutrons. 

The function to fit is

:math:`X_0`:

.. math:: TOF\_h = n(Zero + Dtt1\cdot d) + (1-n)(Zerot + Dtt1t\cdot d + Dtt2t/d)

:math:`A`:

.. math::  \alpha(d)   = \frac{1}{n\alpha^e + (1-n)\alpha^t}

:math:`B`:

.. math:: \beta(d)   = \frac{1}{n\alpha^e + (1-n)\beta^t}

:math:`S`:

.. math:: \sigma_G^2(d_h) = \sigma_0^2 + (\sigma_1^2 + DST2(1-\zeta)^2)d_h^2 + (\sigma_2^2 + Gsize)d_h^4

with constraint:

.. math:: n = 1/2 erfc(W\cdot (1-Tcross/d))

The coefficients in this function are strongly correlated to each other.

Current Implementation
======================

Only the parameters of the function for :math:`X_0` are fitted in 
present implementation. 

Refinement Algorithm
====================

Two refinement algorithms, DirectFit and MonteCarlo, are provided.

DirectFit
^^^^^^^^^

This is a simple one step fitting. If there is one parameter to fit,
Levenberg Marquart minimizer is chosen. As its coefficients are strongly
correlated to each other, Simplex minimizer is used if there are more
than 1 parameter to fit.

MonteCarlo
^^^^^^^^^^

This adopts the concept of Monte Carlo random walk in the parameter
space. In each MC step, one parameter will be chosen, and a new value is
proposed for it. A constraint fitting by Simplex minimizer is used to
fit the coefficients in new configuration.

Simulated annealing will be tried as soon as it is implemented in
Mantid.

Constraint
^^^^^^^^^^
In future, constaint will be considered.


How to use algorithm with other algorithms
==========================================

This algorithm is designed to work with other algorithms to do Le Bail
fit. The introduction can be found in the wiki page of
:ref:`algm-LeBailFit`.

Usage
-----


.. categories::
