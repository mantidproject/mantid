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

Approximation
-------------

Native peak parameters
======================

- A: Value of the Lorentzian amplitude
- position :math:`x_0`: Position of the Lorentzian peak
- Lorentizian FWHM :math:`\Gamma_L`: Value of the full-width half-maximum for the Lorentzian
- Gaussian FWHM :math:`\Gamma_G`: Value of the full-width half-maximum for the Gaussian


Effective peak parameters
=========================




- FWHM: return (getParameter(LORENTZ_FWHM) + getParameter(GAUSSIAN_FWHM));
- center: double Voigt::centre() const { return getParameter(LORENTZ_POS); }
- height: Y(center)
- intensity: return M_PI * getParameter(LORENTZ_AMP) * getParameter(LORENTZ_FWHM) / 2.0;


""" V(X, Y) = 
    const double xoffset = xValues[i] - lorentzPos;

    const double X = xoffset * 2.0 * rtln2oGammaG;
    const double Y = gamma_L * rtln2oGammaG;

    double fx(0.0), dFdx(0.0), dFdy(0.0);
    for (size_t j = 0; j < NLORENTZIANS; ++j) {
      const double ymA(Y - COEFFA[j]);
      const double xmB(X - COEFFB[j]);
      const double alpha = COEFFC[j] * ymA + COEFFD[j] * xmB;
      const double beta = ymA * ymA + xmB * xmB;
      const double ratioab = alpha / beta;
      fx += ratioab;
      dFdx += (COEFFD[j] / beta) - 2.0 * (X - COEFFB[j]) * ratioab / beta;
      dFdy += (COEFFC[j] / beta) - 2.0 * (Y - COEFFA[j]) * ratioab / beta;
    }
    if (functionValues) {
      functionValues[i] = prefactor * fx;



"""



.. attributes::

.. properties::

.. categories::

.. sourcelink::
