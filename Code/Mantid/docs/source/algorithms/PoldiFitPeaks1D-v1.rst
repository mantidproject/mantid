.. algorithm::

.. summary::

.. alias::

.. properties::

Description
-----------

PoldiFitPeaks1D takes a TableWorkspace with peaks (for example from
:ref:`algm-PoldiPeakSearch`) and a spectrum from
:ref:`algm-PoldiAutoCorrelation` and tries to fit a
Gaussian peak profile to the spectrum for each peak. Usually, the peaks
are accompanied by a quadratic background, so this is fitted as well.

The implementation is very close to the original POLDI analysis software
(using the same profile function). One point where this routine differs
is error calculation. In the original program the parameter errors were
adjusted by averaging :math:`\chi^2`-values, but this does not work
properly if there is an outlier caused by a bad fit for one of the
peaks.

.. categories::
