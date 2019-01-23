.. _ConvFitConcept:

Conv Fit
========

The measured data :math:`I(Q, \omega)` is proportional to the convolution of the
scattering law :math:`S(Q, \omega)` with the resolution function :math:`R(Q,
\omega)` of the spectrometer via :math:`I(Q, \omega) = S(Q, \omega) ⊗  R(Q,
\omega)`. The traditional method of analysis has been to fit the measured
:math:`I(Q, \omega)` with an appropriate set of functions related to the form of
:math:`S(Q, \omega)` predicted by theory.

* In quasielastic scattering the simplest form is when both the :math:`S(Q,
  \omega)` and the :math:`R(Q, \omega)` have the form of a Lorentzian - a
  situation which is almost correct for reactor based backscattering
  spectrometers such as IN10 & IN16 at ILL. The convolution of two Lorentzians
  is itself a Lorentzian so that the spectrum of the measured and resolution
  data can both just be fitted with Lorentzians. The broadening of the sample
  spectrum is then just the  difference of the two widths.
* The next easiest case is when both :math:`S(Q, \omega)` and :math:`R(Q,
  \omega)` have a simple functional form and the convolution is also a function
  containing the parameters of the :math:`S(Q, \omega)` and R(Q,  \omega) functions.
  The convoluted function may then be fitted to the data to provide the
  parameters. An example would be the case where the :math:`S(Q, \omega)` is a
  Lorentzian and the :math:`R(Q, \omega)` is a Gaussian.
* For diffraction, the shape of the peak in time is a convolution of a Gaussian
  with a decaying exponential and this function can be used to fit the Bragg
  peaks.
* The final case is where :math:`R(Q, \omega)` does not have a simple function
  form so that the measured data has to be convoluted numerically with the
  :math:`S(Q, \omega)` function to provide an estimate of the sample scattering.
  The result is least-squares fitted to the measured data to provide values for
  the parameters in the :math:`S(Q, \omega)` function.

This latter form of peak fitting is provided by SWIFT. It employs a
least-squares algorithm which requires the derivatives of the fitting function
with respect to its parameters in order to be faster and more efficient than
those algorithms which calculate the derivatives numerically. To do this the
assumption is made that the derivative of a convolution is equal to the
convolution of the derivative-as the derivative and the convolution are
performed over different variables (function parameters and energy transfer
respectively) this should be correct. A flat background is subtracted from the
resolution data before the convolution is performed.

Four types of sample function are available for :math:`S(Q, \omega)`:

Quasielastic
  This is the most common case and applies to both translational (diffusion) and
  rotational modes, both of which have the form of a Lorentzian. The fitted
  function is a set of Lorentzians centred at the origin in energy transfer.

Elastic
  Comprising a central elastic peak together with a set of quasi-elastic
  Lorentzians also centred at the origin. The elastic peak is taken to be the
  un-broadened resolution function.

Shift
  A central Lorentzian with pairs of energy shifted Lorentzians. This was
  originally used for crystal field splitting data but more recently has been
  applied to quantum tunnelling peaks. The fitting function assumes that the
  peaks are symmetric about the origin in energy transfer both in position and
  width. The widths of the central and side peaks may be different.

Polymer
  A single quasi-elastic peak with 3 different forms of shape. The theory behind
  this is described elsewhere [1,2]. Briefly, polymer theory predicts 3 forms
  of the :math:`I(Q,t)` in the form of :math:`exp(-at2/b)` where :math:`b` can
  be 2, 3 or 4. The Full Width Half-Maximum (FWHM) then has a Q-dependence
  (power law) of the form :math:`Qb`. The :math:`I(Q,t)` has been numerically
  Fourier transformed into :math:`I(Q, \omega)` and the :math:`I(Q, \omega)`
  have been fitted with functions of the form of a modified Lorentzian. These
  latter functions are used in the energy fitting procedures.

References:

1. J S Higgins, R E Ghosh, W S Howells & G Allen, `JCS Faraday II 73 40 (1977) <http://dx.doi.org/10.1039/F29777300040>`_
2. J S Higgins, G Allen, R E Ghosh, W S Howells & B Farnoux, `Chem Phys Lett 49 197 (1977) <http://dx.doi.org/10.1016/0009-2614(77)80569-1>`_


.. categories:: Concepts
