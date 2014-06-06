.. algorithm::

.. summary::

.. alias::

.. properties::

Description
-----------

This algorithm calculates the weighted mean from all the spectra in a
given workspace. Monitors and masked spectra are ignored. Also,
individual bins with IEEE values will be excluded from the result. The
weighted mean calculated by the following:

:math:`\displaystyle y=\frac{\sum\frac{x_i}{\sigma^{2}_i}}{\sum\frac{1}{\sigma^{2}_i}}`

and the variance is calculated by:

:math:`\displaystyle \sigma^{2}_y=\frac{1}{\sum\frac{1}{\sigma^{2}_i}}`

.. categories::
