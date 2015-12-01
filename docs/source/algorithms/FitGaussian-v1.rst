.. algorithm::

.. summary::

.. alias::

.. properties::

Description
-----------

This algorithm fits a single peak with a Gaussian function.

It assumes that the peak centre is at or near the point with the highest value
and the peak is at least three samples wide above the half maximum. It estimates
the peak height, sigma and range, and calls the :ref:`algm-Fit` algorithm to
fit the peak curve.

Input and Output
################

The input parameters are a histogram workspace and the index of the histogram.
The ouput parameters are the fitted peak centre and sigma. If the data could not
be fitted, 0.0 is returned as both the peak centre and sigma values and a warning
message is logged. Errors in parameters raise RuntimeError.

ChildAlgorithms used
####################

Uses the :ref:`algm-Fit` algorithm to fit the peak curve with a Gaussian function.

.. warning::

.. categories::

.. sourcelink::
