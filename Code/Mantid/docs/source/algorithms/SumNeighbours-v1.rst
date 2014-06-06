.. algorithm::

.. summary::

.. alias::

.. properties::

Description
-----------

The algorithm looks through the `Instrument <Instrument>`__ to find all
the `RectangularDetectors <RectangularDetector>`__ defined. For each
detector, the SumX\*SumY neighboring event lists are summed together and
saved in the output workspace as a single spectrum. Therefore, the
output workspace will have 1/(SumX\*SumY) \* the original number of
spectra.

.. categories::
