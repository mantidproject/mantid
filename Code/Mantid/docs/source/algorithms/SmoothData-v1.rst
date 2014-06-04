.. algorithm::

.. summary::

.. alias::

.. properties::

Description
-----------

Smooths out statistical jitter in a workspace's data by making each
point the mean average of itself and one or more points lying
symmetrically either side of it. The statistical error on each point
will be reduced by sqrt(npts) because more data is now contributing to
it. For points at the end of each spectrum, a reduced number of
smoothing points will be used. For example, if NPoints is 5 the first
value in the spectrum will be smoothed by making it the average of the
first 3 values, the next will use the first 4 and then the third and
onwards will use the full 5 points in the averaging.

.. categories::
