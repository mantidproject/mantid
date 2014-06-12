.. algorithm::

.. summary::

.. alias::

.. properties::

Description
-----------

This algorithm requires a workspace that is both in d-spacing, but has
also been preprocessed by the :ref:`algm-CrossCorrelate`
algorithm. In this first step you select one spectrum to be the
reference spectrum and all of the other spectrum are cross correlated
against it. Each output spectrum then contains a peak whose location
defines the offset from the reference spectrum.

The algorithm iterates over each spectrum in the workspace and fits a
`Gaussian <Gaussian>`__ function to the reference peaks. The fit is used
to calculate the centre of the fitted peak, and the offset is then
calculated as:

:math:`-peakCentre*step/(dreference+PeakCentre*step)`

This is then written into a `.cal file <CalFile>`__ for every detector
that contributes to that spectrum. All of the entries in the cal file
are initially set to both be included, but also to all group into a
single group on :ref:`algm-DiffractionFocussing`. The
:ref:`algm-CreateCalFileByNames` algorithm can be used to
alter the grouping in the cal file.

.. categories::
