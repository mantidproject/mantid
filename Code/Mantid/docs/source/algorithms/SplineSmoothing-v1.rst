.. algorithm::

.. summary::

.. alias::

.. properties::

Description
-----------

The algorithm performs a smoothing of the input data using a cubic
spline. The algorithm takes a 2D workspace and generates a spline for
each of the spectra to approximate a fit of the data.

Optionally, this algorithm can also calculate the first and second
derivatives of each of the interpolated points as a side product.
Setting the DerivOrder property to zero will force the algorithm to
calculate no derivatives.

For Histogram Workspaces
########################

If the input workspace contains histograms, rather than data points,
then SplineInterpolation will automatically convert the input to point
data. The output returned with be in the same format as the input.

.. categories::
