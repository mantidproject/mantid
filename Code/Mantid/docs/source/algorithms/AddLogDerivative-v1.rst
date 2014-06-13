.. algorithm::

.. summary::

.. alias::

.. properties::

Description
-----------

This algorithm performs a simple numerical derivative of the values in a
sample log.

The 1st order derivative is simply: dy = (y1-y0) / (t1-t0), which is
placed in the log at t=(t0+t1)/2

Higher order derivatives are obtained by performing the equation above N
times. Since this is a simple numerical derivative, you can expect the
result to quickly get noisy at higher derivatives.

If any of the times in the logs are repeated, then those repeated time
values will be skipped, and the output derivative log will have fewer
points than the input.

.. categories::
