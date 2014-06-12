.. algorithm::

.. summary::

.. alias::

.. properties::

Description
-----------

This algorithm corrects the data and error values on a workspace by the
value of an exponential function of the form
:math:`{\rm C0} e^{-{\rm C1} x}`. This formula is calculated for each
data point, with the value of *x* being the mid-point of the bin in the
case of histogram data. The data and error values are either divided or
multiplied by the value of this function, according to the setting of
the Operation property.

.. categories::
