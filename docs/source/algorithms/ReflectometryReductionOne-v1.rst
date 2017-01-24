.. algorithm::

.. summary::

.. alias::

.. properties::

Description
-----------

Reduces a single TOF reflectometry run into a mod Q vs I/I0 workspace.
Performs transmission corrections. Handles both point detector and
multidetector cases. The algorithm can correct detector locations based
on an input theta value.

Historically the work performed by this algorithm was known as the Quick
script.

If :literal:`MonitorBackgroundWavelengthMin` and
:literal:`MonitorBackgroundWavelengthMax` are both set to :literal:`0`, then
background normalization will not be performed on the monitors.

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

.. diagram:: ReflectometryReductionOne_HighLvl-v1_wkflw.dot

Low-Level Workflow
------------------

Conversion to Wavelength
########################

The following diagram describes the steps taken in converting the input
workspace into units of wavelength and dividing its constituent detectors by
monitors.

.. diagram:: ReflectometryReductionOne_ConvertToWavelength-v1_wkflw.dot

The default analysis mode is *PointDetectorAnalysis*. For PointAnalysisMode the
analysis can be roughly reduced to IvsLam = DetectorWS / sum(I0) /
TransmissionWS / sum(I0). For MultiDetectorAnalysis the analysis can be roughly
reduced to IvsLam = DetectorWS / RegionOfDirectBeamWS / sum(I0) / TransmissionWS
/ sum(I0).

Transmission Correction
#######################

This diagram shows how the resultant workspace of the previous step is corrected
by either by provided transmission runs or by a specific correction algorithm.

.. diagram:: ReflectometryReductionOne_TransmissionCorrection-v1_wkflw.dot

Transmission correction is a normalization step, which may be applied to both
*PointDetectorAnalysis* and *MultiDetectorAnalysis* reduction.

Transmission runs are expected to be in TOF. The spectra numbers in the
Transmission run workspaces must be the same as those in the Input Run
workspace. If two Transmission runs are provided then the Stitching
parameters associated with the transmission runs will also be required.
If a single Transmission run is provided, then no stitching parameters
will be needed.

The normalization by tranmission run(s) is optional.

The input workspace provided to the workflow in this instance is the original
:literal:`InputWorkspace` after conversion to wavelength and normalization by
monitors, as shown in the previous :literal:`Conversion To Wavelength` diagram.

The output workspace given is not the output to the whole algorithm. Rather it
will serve as the input workspace to the :literal:`Polynomial Correction`
workflow, where further steps will be applied to it.

Polynomial Correction
=====================

If no Transmission runs are provided, then polynomial correction can be
performed instead. Polynomial correction is enabled by setting the
:literal:`CorrectionAlgorithm` property. If set to
:literal:`PolynomialCorrection` it runs the :ref:`algm-PolynomialCorrection`
algorithm, with this algorithms :literal:`Polynomial` property used as its
:literal:`Coefficients` property.

If the :literal:`CorrectionAlgorithm` property is set to
:literal:`ExponentialCorrection`, then the :Ref:`algm-ExponentialCorrection`
algorithm is used, with C0 and C1 taken from the :literal:`C0` and :literal:`C1`
properties.

Detector Position Correction
############################

The diagram below describes how the input workspace is then corrected by
detector positions after transmission correction.

.. diagram:: ReflectometryReductionOne_CorrectDetectorPositions-v1_wkflw.dot

Detector Position Correction is used for when the position of the detector
is not aligned with the reflected beamline. The correction algorithm used is
:ref:`algm-SpecularReflectionPositionCorrect-v1` which is a purely vertical
position correction.

The detector positions in this process are corrected in terms of
:literal:`ThetaIn`. In general, the detector posistions should always be
corrected unless the :literal:`InputWorkspace` already has the detectors in the
right positions. This can be achieved by running
:literal:`MoveInstrumentComponent` before :literal:`ReflectometryReductionOne`.

Convert To Momentum Transfer (Q)
################################

The last diagram describes the steps involved in converting the input workspace
from units of wavelength into momentum transfer (Q).

.. diagram:: ReflectometryReductionOne_ConvertToMomentum-v1_wkflw.dot

ReflectometryReductionOne contains 2 post-processing options that will be
applied to the IvsQ workspace. These two options are `Rebin` and `Scale`.

Rebinning
=========

To Rebin your IvsQ workspace you will have to provide values for the following
properties: `MomentumTransferMinimum`, `MomentumTransferStep` and
`MomentumTransferMaximum`. These values will be appended to each other to form
your :ref:`algm-Rebin` Params. These values correspond to your `MinimumExtent`,
`BinWidth` and `MaximumExtent` respectively.

If you provide a positive `MomentumTransferStep` value then the algorithm will
automatically negate this value which will allow for Logarithmic Rebinning.
Alternatively, a negative `MomentumTransferStep` will result in Linear
Rebinning. More details about the Rebinning process can be found in the
documentation for :ref:`algm-Rebin`.

If no values are provided for `MomentumTransferMinimum` and
`MomentumTransferMaximum` then the algorithm will attempt to calculate these
values by using the equations below:

    :math:`Q_{min} = 2 \, k \, sin \, \theta = \frac{4 \pi sin \theta}{\lambda_{max}}`

    :math:`Q_{max} = 2 \, k \, sin \, \theta = \frac{4 \pi sin \theta}{\lambda_{min}}`

Where :math:`\lambda_{min}` is the minimum extent of the `IvsLambda` Workspace
and :math:`\lambda_{max}` is the maximum extent of the `IvsLambda` Workspace.

If you have not provided a value for `MomentumTransferStep` then the algorithm
will use :ref:`algm-CalculateResolution` to calculate this value for you.

Scaling
=======

To apply a scaling to the IvsQ workspace that has been produced by the
reduction, you will need to provide a value for the `ScaleFactor` property in
the algorithm. The default for this value is 1.0 and thus no scaling is applied
to the workspace. The scaling of the IvsQ workspace is performed in-place by the
:ref:`algm-Scale` algorithm and your IvsQ workspace will be set to the product
of this algorithm.

Source Rotation
===============

In the workflow diagram above, after we produce the IvsLambda workspace, it may
be necessary to rotate the position of the source to match the value of
ThetaOut (:math:`\theta_f`).

Below we see the typical experimental setup for a Reflectometry instrument. The
source direction (Beam vector) is along the horizon. This setup is defined in
the Instrument Defintion File and this instrument setup will be attached to any
workspaces associated with that instrument. When we pass the IvsLambda workspace
to :ref:`algm-ConvertUnits` to produce an IvsQ workspace,
:ref:`algm-ConvertUnits` will assume that :math:`2\theta` is the angle between
the Beam vector and the sample-to-detector vector. When we have the typical
setup seen below, :math:`2\theta` will be exactly half the value we wish it to
be.

.. figure:: /images/CurrentExperimentSetupForReflectometry.png
    :width: 650px
    :height: 250px
    :align: center

We rotate the position of the Source (and therefore the Beam vector) in the
Instrument Defintion associated with the IvsLambda workspace until the condition
:math:`\theta_i = \theta_f` is satisfied. This will achieve the desired result
for :math:`2\theta` (see below for rotated source diagram). After
:ref:`algm-ConvertUnits` has produced our IvsQ workspace, we will rotate the
position of the source back to its original position so that the experimental
setup remains unchanged for other algorithms that may need to manipulate/use it.

.. figure:: /images/RotatedExperimentSetupForReflectometry.png
    :width: 650px
    :height: 250px
    :align: center


Processing Instructions
#######################

These enable a grouping pattern on workspace indices to yield only the detectors of interest. It allows usage of the operators :literal:`,:+-` to specify or exclude specific indices or to add
spectra together. See :literal:`Grouping Pattern` from :Ref:`algm-GroupDetectors` for further details on their usage.

Usage
-----

**Example - Reduce a Run**

.. testcode:: ExReflRedOneSimple

   run = Load(Filename='INTER00013460.nxs')
   # Basic reduction with no transmission run
   IvsQ, IvsLam, thetaOut = ReflectometryReductionOne(InputWorkspace=run, ThetaIn=0.7, I0MonitorIndex=2, ProcessingInstructions='3:4',
   WavelengthMin=1.0, WavelengthMax=17.0,
   MonitorBackgroundWavelengthMin=15.0, MonitorBackgroundWavelengthMax=17.0,
   MonitorIntegrationWavelengthMin=4.0, MonitorIntegrationWavelengthMax=10.0, Version=1)

   print "The first four IvsLam Y values are: [ %.4e, %.4e, %.4e, %.4e ]" % (IvsLam.readY(0)[0], IvsLam.readY(0)[1], IvsLam.readY(0)[2], IvsLam.readY(0)[3])
   print "The first four IvsQ Y values are: [ %.4e, %.4e, %.4e, %.4e ]" % (IvsQ.readY(0)[0], IvsQ.readY(0)[1], IvsQ.readY(0)[2], IvsQ.readY(0)[3])
   print "Theta out is the same as theta in:",thetaOut


Output:

.. testoutput:: ExReflRedOneSimple

   The first four IvsLam Y values are: [ 0.0000e+00, 0.0000e+00, 7.8118e-07, 1.9346e-06 ]
   The first four IvsQ Y values are: [ 6.2615e-04, 7.8498e-04, 9.2155e-04, 1.1042e-03 ]
   Theta out is the same as theta in: 0.7


.. categories::

.. sourcelink::
