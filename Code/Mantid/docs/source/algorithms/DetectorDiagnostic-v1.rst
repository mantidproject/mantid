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

.. categories::
