.. algorithm::

.. summary::

.. relatedalgorithms::

.. properties::

Description
-----------

This algorithm will search through the input workspace for values less
than zero and make them positive according to the properties. If
``AddMinimum`` is "true" then all values will have :math:`-1*min` for the
spectrum added to them if the minimum is less than zero. Otherwise all
values that are less than zero will be set to ``ResetValue`` which has a
default of 0.

.. categories::

.. sourcelink::
