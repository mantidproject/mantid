.. algorithm::

.. summary::

.. alias::

.. properties::

Description
-----------

The measured spectrum :math:`I(Q, \omega)` is proportional to the four
dimensional convolution of the scattering law :math:`S(Q, \omega)` with the
resolution function :math:`R(Q, \omega)` of the spectrometer via :math:`I(Q,
\omega) = S(Q, \omega) ⊗ R(Q, \omega)`, so :math:`S(Q, \omega)` can be obtained,
in principle, by a deconvolution in :math:`Q` and :math:`\omega`. The method
employed here is based on the Fourier Transform (FT) technique [6,7]. On Fourier
transforming the equation becomes :math:`I(Q, t) = S(Q, t) x R(Q, t)` where the
convolution in :math:`\omega`-space is replaced by a simple multiplication in
:math:`t`-space. The intermediate scattering law :math:`I(Q, t)` is then
obtained by simple division and the scattering law :math:`S(Q, \omega)` itself
can be obtained by back transformation. The latter however is full of pitfalls
for the unwary. The advantage of this technique over that of a fitting procedure
such as SWIFT is that a functional form for :math:`I(Q, t)` does not have to be
assumed. On IRIS the resolution function is close to a Lorentzian and the
scattering law is often in the form of one or more Lorentzians. The FT of a
Lorentzian is a decaying exponential, :math:`exp(-\alpha t)` , so that plots of
:math:`ln(I(Q, t))` against t would be straight lines thus making interpretation
easier.

In general, the origin in energy for the sample run and the resolution run need
not necessarily be the same or indeed be exactly zero in the conversion of the
RAW data from time-of-flight to energy transfer. This will depend, for example,
on the sample and vanadium shapes and positions and whether the analyser
temperature has changed between the runs. The procedure takes this into account
automatically, without using an arbitrary fitting procedure, in the following
way. From the general properties of the FT, the transform of an offset
Lorentzian has the form :math:`(cos(\omega_{0}t) + isin(\omega_{0}t))exp(-\Gamma
t)` , thus taking the modulus produces the exponential :math:`exp(-\Gamma t)`
which is the required function. If this is carried out for both sample and
resolution, the difference in the energy origin is automatically removed. The
results of this procedure should however be treated with some caution when
applied to more complicated spectra in which it is possible for :math:`I(Q, t)`
to become negative, for example, when inelastic side peaks are comparable in
height to the elastic peak.

The interpretation of the data must also take into account the propagation of
statistical errors (counting statistics) in the measured data as discussed by
Wild et al [1]. If the count in channel :math:`k` is :math:`X_{k}` , then
:math:`X_{k}=<X_{k}>+\Delta X_{k}` where :math:`<X_{k}>` is the mean value and
:math:`\Delta X_{k}` the error. The standard deviation for channel :math:`k` is
:math:`\sigma k` :math:`2=<\Delta X_{k}>2` which is assumed to be given by
:math:`\sigma k=<X_{k}>`. The FT of :math:`X_{k}` is defined by
:math:`X_{j}=<X_{j}>+\Delta X_{j}` and the real and imaginary parts denoted by
:math:`X_{j} I` and :math:`X_{j} I` respectively. The standard deviations on
:math:`X_{j}` are then given by :math:`\sigma 2(X_{j} R)=1/2 X0 R + 1/2 X2j R`
and :math:`\sigma 2(X_{j} I)=1/2 X0 I - 1/2 X2j I`.

Note that :math:`\sigma 2(X_{0} R) = X_{0} R` and from the properties of FT
:math:`X_{0} R = X_{k}`.  Thus the standard deviation of the first coefficient
of the FT is the square root of the integrated intensity of the spectrum. In
practice, apart from the first few coefficients, the error is nearly constant
and close to :math:`X_{0} R`.  A further point to note is that the errors make
the imaginary part of :math:`I(Q, t)` non-zero and that, although these will be
distributed about zero, on taking the modulus of :math:`I(Q, t)`, they become
positive at all times and are distributed about a non-zero positive value. When
:math:`I(Q, t)` is plotted on a log-scale the size of the error bars increases
with time (coefficient) and for the resolution will reach a point where the
error on a coefficient is comparable to its value. This region must therefore be
treated with caution. For a true deconvolution by back transforming, the data
would be truncated to remove this poor region before back transforming. If the
truncation is severe the back transform may contain added ripples, so an
automatic back transform is not provided.

References:

1. U P Wild, R Holzwarth & H P Good, Rev Sci Instr 48 1621 (1977)

.. categories::
