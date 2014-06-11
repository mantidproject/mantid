.. algorithm::

.. summary::

.. alias::

.. properties::

Description
-----------

FFTSmooth uses the FFT algorithm to create a Fourier transform of a
spectrum, applies a filter to it and transforms it back. The filters
remove higher frequencies from the spectrum which reduces the noise.

The second version of the FFTSmooth algorithm has two filters:

Zeroing
#######

-  Filter: "Zeroing"
-  Params: "n" - an integer greater than 1 meaning that the Fourier
   coefficients with frequencies outside the 1/n of the original range
   will be set to zero.

Butterworth
###########

-  Filter: "Butterworth"
-  Params: A string containing two positive integer parameters separated
   by a comma, such as 20,2.

"n"- the first integer, specifies the cutoff frequency for the filter,
in the same way as for the "Zeroing" filter. That is, the cutoff is at
m/n where m is the original range. "n" is required to be strictly more
than 1.

"order"- the second integer, specifies the order of the filter. For low
order values, such as 1 or 2, the Butterworth filter will smooth the
data without the strong "ringing" artifacts produced by the abrupt
cutoff of the "Zeroing" filter. As the order parameter is increased, the
action of the "Butterworth" filter will approach the action of the
"Zeroing" filter.

For both filter types, the resulting spectrum has the same size as the
original one.

Previous Versions
-----------------

Version 1
#########

Version 1 did not support the Butterworth Filter and did not offer the
options to ignore X bins or smooth all spectra.

.. categories::
