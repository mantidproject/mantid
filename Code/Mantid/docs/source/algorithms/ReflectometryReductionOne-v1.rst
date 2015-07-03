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

Analysis Modes
##############

The default analysis mode is *PointDetectorAnalysis*. Only this mode
supports Transmission corrections (see below). For PointAnalysisMode the
analysis can be roughly reduced to IvsLam = DetectorWS / sum(I0) /
TransmissionWS / sum(I0). The normalization by tranmission run(s) is
optional. If necessary, input workspaces are converted to *Wavelength*
first via :ref:`algm-ConvertUnits`.

IvsQ is calculated via :ref:`algm-ConvertUnits` into units of
*MomentumTransfer*. Corrections may be applied prior to the
transformation to ensure that the detectors are in the correct location
according to the input Theta value. Corrections are only enabled when a
Theta input value has been provided.

Transmission Runs
#################

Transmission correction is a normalization step, which may be applied to
*PointDetectorAnalysis* reduction.

Transmission runs are expected to be in TOF. The spectra numbers in the
Transmission run workspaces must be the same as those in the Input Run
workspace. If two Transmission runs are provided then the Stitching
parameters associated with the transmission runs will also be required.
If a single Transmission run is provided, then no stitching parameters
will be needed.


Polynomial Correction
#####################

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

Workflow
########

.. diagram:: ReflectometryReductionOne-v1_wkflw.dot

Usage
-----

**Example - Reduce a Run**

.. testcode:: ExReflRedOneSimple

   run = Load(Filename='INTER00013460.nxs')
   # Basic reduction with no transmission run
   IvsQ, IvsLam, thetaOut = ReflectometryReductionOne(InputWorkspace=run, ThetaIn=0.7, I0MonitorIndex=2, ProcessingInstructions='3:4', 
   WavelengthMin=1.0, WavelengthMax=17.0, 
   MonitorBackgroundWavelengthMin=15.0, MonitorBackgroundWavelengthMax=17.0, 
   MonitorIntegrationWavelengthMin=4.0, MonitorIntegrationWavelengthMax=10.0 )

   print "The first four IvsLam Y values are: [ %.4e, %.4e, %.4e, %.4e ]" % (IvsLam.readY(0)[0], IvsLam.readY(0)[1], IvsLam.readY(0)[2], IvsLam.readY(0)[3])
   print "The first four IvsQ Y values are: [ %.4e, %.4e, %.4e, %.4e ]" % (IvsQ.readY(0)[0], IvsQ.readY(0)[1], IvsQ.readY(0)[2], IvsQ.readY(0)[3])
   print "Theta out is the same as theta in:",thetaOut


Output:

.. testoutput:: ExReflRedOneSimple

   The first four IvsLam Y values are: [ 0.0000e+00, 0.0000e+00, 4.9588e-07, 1.2769e-06 ]
   The first four IvsQ Y values are: [ 2.1435e-05, 5.0384e-05, 5.2332e-05, 5.2042e-05 ]
   Theta out is the same as theta in: 0.7


.. categories::

.. sourcelink::
