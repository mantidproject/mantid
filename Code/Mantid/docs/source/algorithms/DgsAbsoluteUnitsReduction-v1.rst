.. algorithm::

.. summary::

.. alias::

.. properties::

Description
-----------

This algorithm is responsible for taking an absolute units sample and
converting it to an integrated value (one value for entire workspace)
for that sample. A corresponding detector vanadium can be used in
conjunction with the data reduction. The diagram below shows the
workflow. The AbsUnitsIncidentEnergy parameter needs to be passed via a
property manager since the absolute units sample may have been measured
at an energy different from the sample of interest. Parameters in
italics are controlled by the
`instrument parameter file (IPF) <http://www.mantidproject.org/InstrumentParameterFile>`_
unless provided to the algorithm via a property manager. The mappings are given
below.

+-------------------------+----------------------+
| Parameter               | IPF Mapping          |
+=========================+======================+
| VanadiumMass            | vanadium-mass        |
+-------------------------+----------------------+
| AbsUnitsMinimumEnergy   | monovan-integr-min   |
+-------------------------+----------------------+
| AbsUnitsMaximumEnergy   | monovan-integr-max   |
+-------------------------+----------------------+

The last two parameters are used to create a single bin for the :ref:`Rebin <algm-Rebin>`
algorithm. The dashed oval parameter, VanadiumRmm, is taken from the
atomic information for the molecular mass of Vanadium. The open circle
represents detector diagnostic parameters and they are detailed in the
table below.

+-----------------------------+----------------------+-------------------------------------------------------------+
| Parameter                   | IPF Mapping          | :ref:`DetectorDiagnostic <algm-DetectorDiagnostic>` Mapping |
+=============================+======================+=============================================================+
| HighCounts                  | diag\_huge           | HighThreshold                                               |
+-----------------------------+----------------------+-------------------------------------------------------------+
| LowCounts                   | diag\_tiny           | LowThreshold                                                |
+-----------------------------+----------------------+-------------------------------------------------------------+
| AbsUnitsLowOutlier          | monovan\_lo\_bound   | LowOutlier                                                  |
+-----------------------------+----------------------+-------------------------------------------------------------+
| AbsUnitsHighOutlier         | monovan\_hi\_bound   | HighOutlier                                                 |
+-----------------------------+----------------------+-------------------------------------------------------------+
| AbsUnitsMedianTestLow       | monovan\_lo\_frac    | LowThresholdFraction                                        |
+-----------------------------+----------------------+-------------------------------------------------------------+
| AbsUnitsMedianTestHigh      | monovan\_hi\_frac    | HighThresholdFraction                                       |
+-----------------------------+----------------------+-------------------------------------------------------------+
| AbsUnitsErrorBarCriterion   | diag\_samp\_sig      | SignificanceTest                                            |
+-----------------------------+----------------------+-------------------------------------------------------------+

If a detector vanadium is used, the processed sample workspace is
multiplied by a factor containing the sample mass (SampleMass), sample
molecular mass (SampleRmm) and the cross-section (Scattering XSec) given
by:
:math:`\frac{(\sigma^{V}_{incoherent}+\sigma^{V}_{coherent})\times10^{3}}{4\pi}`
with the cross-section units of :math:`millibarns/steradian`.

Workflow
########

.. diagram:: DgsAbsoluteUnitsReduction-v1_wkflw.dot

Usage
-----

.. warning::

    This algorithm is not really intented for use at the command line, but is used
    within :ref:`DgsReduction <algm-DgsReduction>`.

.. categories::
