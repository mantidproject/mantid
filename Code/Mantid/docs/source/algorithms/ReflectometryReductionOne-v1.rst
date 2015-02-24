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

Workflow
########

.. diagram:: ReflectometryReductionOne-v1_wkflw.dot

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

   print "The first four IvsLam Y values are: [", str(IvsLam.readY(0)[0]),",", str(IvsLam.readY(0)[1]),",", str(IvsLam.readY(0)[2]),",", str(IvsLam.readY(0)[3]),"]"
   print "The first four IvsQ Y values are: [", str(IvsQ.readY(0)[0]),",", str(IvsQ.readY(0)[1]),",", str(IvsQ.readY(0)[2]),",", str(IvsQ.readY(0)[3]),"]"
   print "Theta out is the same as theta in:",thetaOut


Output:

.. testoutput:: ExReflRedOneSimple

   The first four IvsLam Y values are: [ 0.0 , 0.0 , 0.0 , 1.19084981351e-06 ]
   The first four IvsQ Y values are: [ 3.26638386884e-05 , 5.41802219385e-05 , 4.89364938612e-05 , 5.50890537024e-05 ]
   Theta out is the same as theta in: 0.7


.. categories::
