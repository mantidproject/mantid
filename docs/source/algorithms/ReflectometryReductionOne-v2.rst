.. algorithm::

.. summary::

.. alias::

.. properties::

Description
-----------

This algorithm reduces a single reflectometry run into a mod Q vs I/I0 workspace.
The mandatory input properties, :literal:`WavelengthMin`, :literal:`WavelengthMax`
and :literal:`ProcessingInstructions`, must be manually set by the user. In addition, for
the algorithm to be able to convert from wavelength to momentum transfer correctly,
instrument components, specially detectors, must be located at the correct positions.
The expected experimental setup for the algorithm to produce correct results is
shown below, where the angle between the beam direction and the sample-to-detector
vector must be :math:`2\theta`.

.. figure:: /images/ReflectometryReductionOneDetectorPositions.png
    :width: 400px
    :align: center


The figure below displays a high-level workflow diagram illustrating the main
steps taking place in the reduction.

.. diagram:: ReflectometryReductionOne_HighLvl-v2_wkflw.dot


Conversion to Wavelength
########################

First, the algorithm checks the X units of
the input workspace. If the input workspace is already in wavelength, normalization by
monitors and direct beam are not performed, as it is considered that the input run was
already reduced using this algorithm. If the input workspace is in TOF, it will be
converted to wavelength (note that :literal:`AlignBins` will be set to :literal:`True` for this in 
:ref:`algm-ConvertUnits`), cropped according to
:literal:`WavelengthMin` and :literal:`WavelengthMax`, which are both mandatory properties, and the detectors
of interest, specified via :literal:`ProcessingInstructions`, will be grouped together using
:ref:`algm-GroupDetectors`. **Optionally**, the algorithm will perform direct beam
normalization (if :literal:`RegionOfDirectBeam` is specified) dividing the detectors of
interest by the direct beam, and monitor normalization (if :literal:`I0MonitorIndex` and
:literal:`MonitorBackgroundWavelengthMin` and :literal:`MonitorBackgroundWavelengthMax` are all specified),
in which case the detectors of interest will be divided by monitors. Detectors can be normalized
by integrated monitors by setting :literal:`NormalizeByIntegratedMonitors` to true, in which case
:literal:`MonitorIntegrationWavelengthMin` and :literal:`MonitorIntegrationWavelenthMax` will
be used as the integration range. A summary of the
steps is shown in the workflow diagram below.

.. diagram:: ReflectometryReductionOne_ConvertToWavelength-v2_wkflw.dot


Transmission Correction
#######################

Transmission corrections can be optionally applied to the workspace resulting
from the previous step. Transmission corrections can be either specified via
transmission runs or specific correction algorithms.

.. diagram:: ReflectometryReductionOne_TransmissionCorrection-v2_wkflw.dot


When normalizing by transmission runs, i.e. when one or two transmission runs
are given, the spectrum numbers in the
transmission workspaces must be the same as those in the input run
workspace. If spectrum numbers do not match, the algorithm will throw and exception
and execution of the algorithm will be stopped. This behaviour can be optionally
switched off by setting :literal:`StrictSpectrumChecking` to false, in which case
a warning message will be shown instead.

When normalizing by transmission run, this algorithm will run
:ref:`algm-CreateTransmissionWorkspace` as a child algorithm, with properties :literal:`WavelengthMin`,
:literal:`WavelengthMax`, :literal:`I0MonitorIndex`, :literal:`MonitorBackgroundWavelengthMin`,
:literal:`MonitorBackgroundWavelengthMax`, :literal:`MonitorIntegrationWavelengthMin`,
:literal:`MonitorIntegrationWavelengthMax`, and :literal:`ProcessingCommands`. 
In addition, when both :literal:`FirstTransmissionRun` and :literal:`SecondTransmissionRun`
are provided the stitching parameters :literal:`Params`, as well as :literal:`StartOverlap` and
:literal:`EndOverlap` will be used by :ref:`algm-CreateTransmissionWorkspace` to create the
transmission workspace that will be used for the normalization.

If no transmission runs are provided, then algorithmic corrections can be
performed instead by setting :literal:`CorrectionAlgorithm` to either
:literal:`PolynomialCorrection` or :literal:`ExponentialCorrection`, the two
possible types of corrections at the moment. If :literal:`PolynomialCorrection`,
is selected, :ref:`algm-PolynomialCorrection` algorithm will be run, with this
algorithm's :literal:`Polynomial` property used as its :literal:`Coefficients`
property. If the :literal:`CorrectionAlgorithm` property is set to
:literal:`ExponentialCorrection`, then the :Ref:`algm-ExponentialCorrection`
algorithm is used, with *C0* and *C1* taken from the :literal:`C0` and :literal:`C1`
properties.

Conversion to Momentum Transfer (Q)
###################################

Finally, the output workspace in wavelength is converted to momentum transfer (Q).
Optionally, this workspace can be rebinned according to :literal:`MomentumTransferMin`,
:literal:`MomentumTransferStep` and :literal:`MomentumTransferMax`, and scaled if
:literal:`ScaleFactor` is given.

.. diagram:: ReflectometryReductionOne_ConvertToMomentum-v2_wkflw.dot

Previous Versions
-----------------

This is version 2 of the algorithm. For version 1, please see `here. <ReflectometryReductionOne-v1.html>`_

Usage
-----

**Example - Reduce a run**

.. testcode:: ExReflRedOneSimple

   run = Load(Filename='INTER00013460.nxs')
   # Basic reduction with no transmission run
   IvsQ, IvsLam = ReflectometryReductionOne(InputWorkspace=run,
                                            WavelengthMin=1.0,
                                            WavelengthMax=17.0,
                                            ProcessingInstructions='3:4',
                                            I0MonitorIndex=2,
                                            MonitorBackgroundWavelengthMin=15.0,
                                            MonitorBackgroundWavelengthMax=17.0,
                                            MonitorIntegrationWavelengthMin=4.0,
                                            MonitorIntegrationWavelengthMax=10.0)

   print "%.4f" % (IvsLam.readY(0)[173])
   print "%.4f" % (IvsLam.readY(0)[174])
   print "%.4f" % (IvsQ.readY(0)[2])
   print "%.4f" % (IvsQ.readY(0)[3])


Output:

.. testoutput:: ExReflRedOneSimple

   0.0014
   0.0014
   0.0117
   0.0214


**Example - Reduce a run and normalize by transmission workspace**

.. testcode:: ExReflRedOneTrans

   run = Load(Filename='INTER00013460.nxs')
   trans1 = Load(Filename='INTER00013463.nxs')
   trans2 = Load(Filename='INTER00013464.nxs')
   # Basic reduction with two transmission runs
   IvsQ, IvsLam = ReflectometryReductionOne(InputWorkspace=run,
                                            WavelengthMin=1.0,
                                            WavelengthMax=17.0,
                                            ProcessingInstructions='3-4',
                                            I0MonitorIndex=2,
                                            MonitorBackgroundWavelengthMin=15.0,
                                            MonitorBackgroundWavelengthMax=17.0,
                                            MonitorIntegrationWavelengthMin=4.0,
                                            MonitorIntegrationWavelengthMax=10.0,
					    FirstTransmissionRun=trans1,
					    SecondTransmissionRun=trans2)

   print "%.4f" % (IvsLam.readY(0)[170])
   print "%.4f" % (IvsLam.readY(0)[171])
   print "%.4f" % (IvsQ.readY(0)[107])
   print "%.4f" % (IvsQ.readY(0)[108])


Output:

.. testoutput:: ExReflRedOneTrans

   0.4897
   0.5468
   0.6144
   0.5943

.. categories::

.. sourcelink::
