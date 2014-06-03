.. algorithm::

.. summary::

.. alias::

.. properties::

Description
-----------

This algorithm takes a list of spectra and for each spectrum calculates
an average count rate in the given region, usually a region when there
are only background neutrons. This count rate is then subtracted from
the counts in all the spectrum's bins. However, no bin will take a
negative value as bins with count rates less than the background are set
to zero (and their error is set to the backgound value).

The average background count rate is estimated in one of two ways. When
Mode is set to 'Mean' it is the sum of the values in the bins in the
background region divided by the width of the X range. Selecting 'Linear
Fit' sets the background value to the height in the centre of the
background region of a line of best fit through that region.

The error on the background value is only calculated when 'Mean' is
used. It is the errors in all the bins in the background region summed
in quadrature divided by the number of bins. This background error value
is added in quadrature to the errors in each bin.

ChildAlgorithms used
####################

The `Linear <Linear>`__ algorithm is used when the Mode = Linear Fit.
From the resulting line of best fit a constant value taken as the value
of the line at the centre of the fitted range.

.. categories::
