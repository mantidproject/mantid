.. algorithm::

.. summary::

.. alias::

.. properties::

Description
-----------

Corrects the data and error values on a workspace by the value of a
polynomial function:

.. math:: {\rm C0} + {\rm C1} x + {\rm C2} x^2 + ...

which is evaluated at the *x* value of each data point (using the
mid-point of the bin as the *x* value for histogram data. The data and
error values are multiplied or divided by the value of this function.
The order of the polynomial is determined by the length of the
Coefficients property, which can be of any length.

.. categories::
