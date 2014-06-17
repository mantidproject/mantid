.. algorithm::

.. summary::

.. alias::

.. properties::

Description
-----------

This is an algorithm for Fourier transfom of real data. It uses the GSL
routines gsl\_fft\_real\_transform and gsl\_fft\_halfcomplex\_inverse.
The result of a forward transform is a two-spectra workspace with the
real and imaginary parts of the transform in position 0 and 1
correspondingly. Only positive frequencies are given and as a result the
output spectra are twice as short as the input one.

An input workspace for backward transform must have the form of the
output workspace of the forward algorithm, i.e. have two spectra with
the real part in the first spectrum and the imaginary part in the second
one. The output workspace contains a single spectrum with the real
inverse transform.

.. categories::
