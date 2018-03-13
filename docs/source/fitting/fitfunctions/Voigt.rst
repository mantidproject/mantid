.. _func-Voigt:

=====
Voigt
=====

.. index:: Voigt

Description
-----------

A Voigt function is a convolution between a Lorentzian and Gaussian and
is defined as:

.. math:: V(X,Y) = \frac{Y}{\pi}\int_{-\infty}^{+\infty}dz\frac{exp^{-z^2}}{Y^2 + (X - z)^2}

where

-  X - Normalized line separation width;
-  Y - Normalized collision separation width.

Generally, the Voigt function involves a numerical integral and is
therefore a computational intensive task. However, several
approximations to the Voigt function exist making it palatable for
fitting in a least-squares algorithm. The approximation used here is
described in

-  A.B. McLean, C.E.J. Mitchell, and D.M. Swanston. *Implementation of an Efficient Analytical Approximation to the Voigt Function for Photoemission Lineshape Analysis.* Journal of Electron Spectroscopy and Related Phenomena **69.2** (1994): 125–132 
   `doi:10.1016/0368-2048(94)02189-7  <http://dx.doi.org/10.1016/0368-2048(94)02189-7>`__

The approximation uses a combination of 4 Lorentzians in two variables
to generate good approximation to the true function.

.. attributes::

.. properties::

.. categories::

.. sourcelink::
