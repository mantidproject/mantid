.. algorithm::

.. summary::

.. alias::

.. properties::

Description
-----------

This algorithm is not meant to be used directly by users. Please see :ref:`algm-ReflectometryReductionOneAuto`
which is a facade over this algorithm.

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
already reduced using this algorithm. If the input workspace is in TOF, monitors, detectors of
interest and region of direct beam are extracted by running :ref:`algm-GroupDetectors` with
``ProcessingInstructions`` as input, :ref:`algm-GroupDetectors` with ``RegionOfDirectBeam`` as input,
and :ref:`algm-CropWorkspace` with ``I0MonitorIndex`` as input respectively, and each of
the resulting workspaces is converted to wavelength (note that :literal:`AlignBins` is set
to :literal:`True` in all the three cases). Note that the normalization by a direct beam
is optional, and only happens if ``RegionOfDirectBeam`` is provided. In the same way,
monitor normalization is also optional, and only takes place if ``I0MonitorIndex``,
``MonitorBackgroundWavelengthMin`` and ``MonitorBackgroundWavelengthMax`` are all
specified. Detectors can be normalized by integrated monitors by setting
:literal:`NormalizeByIntegratedMonitors` to true, in which case
:literal:`MonitorIntegrationWavelengthMin` and :literal:`MonitorIntegrationWavelengthMax` are
used as the integration range. If monitors are not integrated, detectors are rebinned to
monitors using :ref:`algm-RebinToWorkspace` so that the normalization by monitors can take place.
Finally, the resulting workspace is cropped in wavelength according to :literal:`WavelengthMin`
and :literal:`WavelengthMax`, which are both mandatory properties. A summary of the steps
is shown in the workflow diagram below. For the sake of clarity, all possible steps are illustrated, even if some of them are optional.

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
:literal:`MonitorIntegrationWavelengthMax`, and :literal:`ProcessingInstructions`.
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

Finally, the output workspace in wavelength is converted to momentum transfer (Q) using
:ref:`algm-ConvertUnits`. Note that the output workspace in Q is therefore a workspace
with native binning, and no rebin step is applied to it.

.. diagram:: ReflectometryReductionOne_ConvertToMomentum-v2_wkflw.dot

If you wish to obtain a rebinned workspace in Q you should consider using algorithm
:ref:`algm-ReflectometryReductionOneAuto` instead, which is a facade over this algorithm
and has two extra steps (:ref:`algm-Rebin` and :ref:`algm-Scale`) to produce an additional
workspace in Q with specified binning and scale factor. Please refer to :ref:`algm-ReflectometryReductionOneAuto`
for more information.

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

   print "%.4f" % (IvsLam.readY(0)[533])
   print "%.4f" % (IvsLam.readY(0)[534])
   print "%.4f" % (IvsQ.readY(0)[327])
   print "%.4f" % (IvsQ.readY(0)[328])


Output:

.. testoutput:: ExReflRedOneSimple

   0.0003
   0.0003
   0.0003
   0.0003


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

   print "%.4f" % (IvsLam.readY(0)[480])
   print "%.4f" % (IvsLam.readY(0)[481])
   print "%.4f" % (IvsQ.readY(0)[107])
   print "%.4f" % (IvsQ.readY(0)[108])


Output:

.. testoutput:: ExReflRedOneTrans

   0.4588
   0.4655
   0.7336
   1.0156

.. categories::

.. sourcelink::
