.. algorithm::

.. summary::

.. alias::

.. properties::

Description
-----------

Reduces a single TOF reflectometry run into a mod Q vs I/I0 workspace.
Performs transmission corrections. Handles both point detector and
multidetector cases.

Historically the work performed by this algorithm was known as the Quick
script.

The properties of this algorithm should be manually selected by the user. If you
wish to use the default values (found in the Instrument Defintion File) for the
properties of this algorithm, you may want to consider using
:ref:`algm-ReflectometryReductionOneAuto`.

:ref:`algm-ReflectometryReductionOneAuto` also performs extra processing steps
such as Background subtraction and :ref:`algm-PolarizationCorrection`. If you
want to know how these processing steps are used, please refer to the
:ref:`algm-ReflectometryReductionOneAuto` documentation.

High-Level Workflow
-------------------

The diagram below displays a high-level version of the algorithm workflow,
illustrating the main steps taking place in the ReflectometryReductionOne
algorithm. These individual steps are described in more detail in the next
sections.

.. diagram:: ReflectometryReductionOne_HighLvl-v2_wkflw.dot

Low-Level Workflow
------------------

Conversion to Wavelength
########################

The following diagram describes the steps taken in converting the input
workspace into units of wavelength and dividing its constituent detectors by
monitors.

.. diagram:: ReflectometryReductionOne_ConvertToWavelength-v2_wkflw.dot

The default analysis mode is *PointDetectorAnalysis*. For PointAnalysisMode the
analysis can be roughly reduced to IvsLam = DetectorWS / sum(I0) /
TransmissionWS / sum(I0). For MultiDetectorAnalysis the analysis can be roughly
reduced to IvsLam = DetectorWS / RegionOfDirectBeamWS / sum(I0) / TransmissionWS
/ sum(I0).

Transmission Correction
#######################

Polynomial Correction
=====================

Convert To Momentum Transfer (Q)
################################

Rebinning
=========

Scaling
=======

Usage
-----

.. categories::

.. sourcelink::
