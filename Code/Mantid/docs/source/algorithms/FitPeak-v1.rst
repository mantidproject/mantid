.. algorithm::

.. summary::

.. alias::

.. properties::

Description
-----------

This algorithm is used to fit a single peak with some checking mechanism
to ensure its fitting result is physical.

The output `TableWorkspace <TableWorkspace>`__ contains the following
columns...

Subalgorithms used
##################

-  Fit

Treating weak peaks vs. high background
#######################################

FindPeaks uses a more complicated approach to fit peaks if
**HighBackground** is flagged. In this case, FindPeak will fit the
background first, and then do a Gaussian fit the peak with the fitted
background removed. This procedure will be repeated for a couple of
times with different guessed peak widths. And the parameters of the best
result is selected. The last step is to fit the peak with a combo
function including background and Gaussian by using the previously
recorded best background and peak parameters as the starting values.

Criteria To Validate Peaks Found
################################

FindPeaks finds peaks by fitting a Guassian with background to a certain
range in the input histogram. :ref:`algm-Fit` may not give a correct
result even if chi^2 is used as criteria alone. Thus some other criteria
are provided as options to validate the result

1. Peak position. If peak positions are given, and trustful, then the
fitted peak position must be within a short distance to the give one.

2. Peak height. In the certain number of trial, peak height can be used
to select the best fit among various starting sigma values.

Fit Window and Peak Range
#########################

If FitWindows is defined, then a peak's range to fit (i.e., x-min and
x-max) is confined by this window.

If PeakRange is defined and starting peak centre given by user is not
within this range, then the situation is considered illegal. In future,
FitPeak might be able to estimate the peak centre in this situation by
locating the X-value whose corresponding Y-value is largest within
user-defined peak range.

What's new
----------

.. categories::
