.. algorithm::

.. summary::

.. alias::

.. properties::

Description
-----------

The algorithm performs interpolation of points onto a cubic spline. The
algorithm takes two input workspaces: one that is used to define the
spline and one that contains a number of spectra to be interpolated onto
the spline.

If multiple spectra are defined in the WorkspaceToInterpolate workspace,
they will all be interpolated against the first spectra in
WorkspaceToMatch.

Optionally, this algorithm can also calculate the first and second
derivatives of each of the interpolated points as a side product.
Setting the DerivOrder property to zero will force the algorithm to
calculate no derivatives.

For Histogram Workspaces
########################

If the input workspace contains histograms, rather than data points,
then SplineInterpolation will automatically convert the input to point
data. The output returned with be in the same format as the input.

Histogram workspaces being interpolated will show a warning when the
range of the data is equal to the size of the workspace to match, but
has finer bin boundaries. This is because histogram data is converted to
point data using the average of the bin boundaries. This will cause some
values to fall outside of the range of the spline when fine bin
boundaries are used.

.. categories::
