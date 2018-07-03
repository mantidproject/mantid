.. algorithm::

.. summary::

.. relatedalgorithms::

.. properties::

Description
-----------

If instrument geometry information is available the
:ref:`algm-SolidAngle` algorithm is used to calculate the number of
counts per unit solid angle, otherwise numbers of counts are used
without correction. First the median number of counts in all the bins is
calculated. Then the ratio of the total number of counts and the median
number is calculated for each histogram. This ratio is compared against
the user defined upper and lower thresholds and if the ratio is outside
the limits the statistical significance test is done.

In the statistical significance test the difference between the number
of counts in each spectrum and the median number is compared to the
spectrum's error value. Any spectra where the ratio of the its deviation
from the mean and the its error is less than the value of the property
SignificanceTest will not be labelled bad. This test is particularly
important when the number of counts is low, for example when examining
the low count "background" parts of spectra.

Optionally, some might want to do median on a tube, or a bank. Fot that,
use the LevelsUp input. For example, in the CNCS instrument, the
detector is called a pixel. The parent of a pixel is a tube, while an
eightpack contains 8 tubes. To calculate the median of a tube, use
LevelsUp=1, for an eightpack use LevelsUp=2. LevelsUp=0 will calculate
the median over the whole instrument.

The output workspace contains a MaskWorkspace where those spectra that
fail the tests are masked and those that pass them are assigned a single
positive value.

ChildAlgorithms used
####################

Uses the :ref:`algm-SolidAngle`, :ref:`algm-Integration` and
:ref:`algm-ConvertToDistribution` algorithms.

.. categories::

.. sourcelink::
