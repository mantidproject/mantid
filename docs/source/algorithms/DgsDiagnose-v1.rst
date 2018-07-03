.. algorithm::

.. summary::

.. relatedalgorithms::

.. properties::

Description
-----------

This algorithm is responsible for setting up the necessary workspaces to hand off
to the :ref:`DetectorDiagnostic <algm-DetectorDiagnostic>`
algorithm. The diagram below shows the manipulations
done by this algorithm. Workspaces that have dashed lines are optional. Parameters
in italics are retrieved from the
`instrument parameter file (IPF) <http://www.mantidproject.org/InstrumentParameterFile>`_
unless they are provided to the algorithm via a property manager. The mappings for
these parameters are shown below.

+-----------------------------+----------------------+--------------------------------------------------------------+
| Parameter                   | IPF Mapping          | :ref:`DetectorDiagnostic <algm-DetectorDiagnostic>` Mapping  |
+=============================+======================+==============================================================+
| RejectZeroBackground        | diag\_samp\_zero     | \-                                                           |
+-----------------------------+----------------------+--------------------------------------------------------------+
| BackgroundCheck             | check\_background    | \-                                                           |
+-----------------------------+----------------------+--------------------------------------------------------------+
| PsdBleed                    | diag\_bleed\_test    | \-                                                           |
+-----------------------------+----------------------+--------------------------------------------------------------+
| BackgroundTofStart          | bkgd-range-min       | \-                                                           |
+-----------------------------+----------------------+--------------------------------------------------------------+
| BackgroundTofEnd            | bkgd-range-max       | \-                                                           |
+-----------------------------+----------------------+--------------------------------------------------------------+
| DetVanRatioVariation        | diag\_variation      | DetVanRatioVariation                                         |
+-----------------------------+----------------------+--------------------------------------------------------------+

The open circles represent groups of parameters. They are detailed in the tables
below. All parameters given here act like italicized parameters.

**Detectors Outside Limits Parameters**

+-----------------------------+----------------------+-------------------------------------------------------------+
| Parameter                   | IPF Mapping          | :ref:`DetectorDiagnostic <algm-DetectorDiagnostic>` Mapping |
+=============================+======================+=============================================================+
| HighCounts                  | diag\_huge           | HighThreshold                                               |
+-----------------------------+----------------------+-------------------------------------------------------------+
| LowCounts                   | diag\_tiny           | LowThreshold                                                |
+-----------------------------+----------------------+-------------------------------------------------------------+

**Median Detector Test Parameters**

+-----------------------------+----------------------+-------------------------------------------------------------+
| Parameter                   | IPF Mapping          | :ref:`DetectorDiagnostic <algm-DetectorDiagnostic>` Mapping |
+=============================+======================+=============================================================+
| HighOutlier                 | diag\_van\_out\_hi   | HighOutlier                                                 |
+-----------------------------+----------------------+-------------------------------------------------------------+
| LowOutlier                  | diag\_van\_out\_lo   | LowOutlier                                                  |
+-----------------------------+----------------------+-------------------------------------------------------------+
| MedianTestHigh              | diag\_van\_hi        | HighThresholdFraction                                       |
+-----------------------------+----------------------+-------------------------------------------------------------+
| MedianTestLow               | diag\_van\_lo        | LowThresholdFraction                                        |
+-----------------------------+----------------------+-------------------------------------------------------------+
| ErrorBarCriterion           | diag\_van\_sig       | SignificanceTest                                            |
+-----------------------------+----------------------+-------------------------------------------------------------+

**Sample Background Parameters**

+-----------------------------+----------------------+-------------------------------------------------------------+
| Parameter                   | IPF Mapping          | :ref:`DetectorDiagnostic <algm-DetectorDiagnostic>` Mapping |
+=============================+======================+=============================================================+
| SamBkgMedianTestHigh        | diag\_samp\_hi       | SampleBkgHighAcceptanceFactor                               |
+-----------------------------+----------------------+-------------------------------------------------------------+
| SamBkgMedianTestLow         | diag\_samp\_lo       | SampleBkgLowAcceptanceFactor                                |
+-----------------------------+----------------------+-------------------------------------------------------------+
| SamBkgErrorBarCriterion     | diag\_samp\_sig      | SampleBkgSignificanceTest                                   |
+-----------------------------+----------------------+-------------------------------------------------------------+

**PsdBleed Parameters**

+-----------------------------+----------------------+-------------------------------------------------------------+
| Parameter                   | IPF Mapping          | :ref:`DetectorDiagnostic <algm-DetectorDiagnostic>` Mapping |
+=============================+======================+=============================================================+
| MaxFrameRate                | diag\_bleed\_maxrate | MaxTubeFramerate                                            |
+-----------------------------+----------------------+-------------------------------------------------------------+
| IgnoredPixels               | diag\_bleed\_pixels  | NIgnoredCentralPixels                                       |
+-----------------------------+----------------------+-------------------------------------------------------------+

Workflow
########

.. figure:: /images/DgsDiagnoseWorkflow.png
   :alt: DgsDiagnoseWorkflow.png

Usage
-----

.. warning::

    This algorithm is not really intented for use at the command line, but is used
    within :ref:`DgsReduction <algm-DgsReduction>`.

.. categories::

.. sourcelink::
