.. algorithm::

.. summary::

.. alias::

.. properties::

Description
-----------

This algorithm is a C++ replacement for the Python diagnostics.diagnose
function located in the scripts/inelastic directory. The algorithm
expects processed workspaces as input just as the other function did.
The main functionality of the algorithm is to determine issues with
detector vanadium runs and mask out bad pixels. The algorithms that are
run on the detector vanadium are :ref:`algm-FindDetectorsOutsideLimits` and
:ref:`algm-MedianDetectorTest`. It also performs a second set of diagnostics on
another detector vanadium run and :ref:`algm-DetectorEfficiencyVariation` on the
two. The algorithm also checks processed sample workspaces (total counts
and background) for bad pixels as well. The total counts workspace is
tested with :ref:`algm-FindDetectorsOutsideLimits`. The background workspace is run
through :ref:`algm-MedianDetectorTest`. A processed sample workspace can be given
to perform and :ref:`algm-CreatePSDBleedMask` will be run on it.

Workflow
########

Parameters for the child algorithms are not shown due to the sheer number.
They are passed onto child algorithms that under the same name, except

* LowThreshold and HighThresold are not passed onto :ref:`algm-FindDetectorsOutsideLimits`, but are set as 1.0e-10 and 1.0e100 respectively.

* LowOutLier, HighOutlier and ExcludeZeroesFromMedian are not passed onto :ref:`algm-MedianDetectorTest`, but are set as 0.0, 1.0e100 and true respectively.

* DetVanRatioVariation is passed onto :ref:`algm-DetectorEfficiencyVariation` as Variation.

* SampleBkgLowAcceptanceFactor, SampleBkgHighAcceptanceFactor, SampleBkgSignificanceTest and SampleCorrectForSolidAngle are passed onto :ref:`algm-MedianDetectorTest` as LowThreshold, HighThreshold, SignicanceTest and CorrectForSolidAngle respectively.

Numerous uses of :ref:`algm-MaskDetectors` are not shown and can be taken be be executed whenever appropriate. 
Also the output property NumberOfFailures from the executed child algorithms are added together 
to form the NumberOfFailures output by the main algorithm.


.. diagram:: DetectorDiagnostic-v1_wkflw.dot



.. categories::
