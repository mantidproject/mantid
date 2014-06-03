.. algorithm::

.. summary::

.. alias::

.. properties::

Description
-----------

It is intended that the input white beam vanadium workspaces are from
the same instrument and were collected before and after an experimental
run of interest. First the ratios of the total number of counts in
corresponding histograms from each input workspace are calculated and
then the median ratio is calculated. Each ratio is compared to the
median and a histogram will fail when any of the following conditions
are true:

-  (sum1/sum2)/median(sum1/sum2) > Variation
-  (sum1/sum2)/median(sum1/sum2) < 1/Variation

where sum1 is the sum of the counts in a histogram in the workspace
WhiteBeamBase and sum2 is the sum of the counts in the equivalent
histogram in WhiteBeamCompare. The above equations only make sense for
identifying bad detectors if Variation > 1. If a value of less than one
is given for Variation then Variation will be set to the reciprocal.

The output workspace contains a MaskWorkspace where those spectra that
fail the tests are masked and those that pass them are assigned a single
positive value.

Child algorithms used
#####################

Uses the :ref:`algm-Integration` algorithm to sum the spectra.

.. categories::
