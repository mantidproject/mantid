.. algorithm::

.. summary::

.. alias::

.. properties::

Description
-----------

This algorithm will correct detector efficiency according to the ILL INX
program for time-of-flight data reduction.

A formula named "formula\_eff" must be defined in the instrument
parameters file. The input workspace must be in DeltaE units.

The output data will be corrected as:

:math:`y = \frac{y}{eff}`

where :math:`eff` is

:math:`eff = \frac{f(Ei - \Delta E)}{f(E_i)}`

The function :math:`f` is defined as "formula\_eff" in the IDF. To date
this has been implemented at the ILL for ILL IN4, IN5 and IN6.

.. categories::
