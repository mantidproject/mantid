.. algorithm::

.. summary::

.. alias::

.. properties::

Description
-----------

The algorithm calculates the weighted mean of two workspaces. This is
useful when working with distributions rather than histograms,
particularly when counting statistics are poor and it is possible that
the value of one data set is statistically insignificant but differs
greatly from the other. In such a case simply calculating the average of
the two data sets would produce a spurious result. This algorithm will
eventually be modified to take a list of workspaces as an input.

:math:`\displaystyle y=\frac{\sum\frac{x_i}{\sigma^{2}_i}}{\sum\frac{1}{\sigma^{2}_i}}`

.. categories::
