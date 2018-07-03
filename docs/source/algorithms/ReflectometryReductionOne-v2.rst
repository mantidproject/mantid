.. algorithm::

.. summary::

.. relatedalgorithms::

.. properties::

Description
-----------

This algorithm is not meant to be used directly by users. Please see
:ref:`algm-ReflectometryReductionOneAuto` which is a facade over this
algorithm.

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
steps taking place in the reduction. For the sake of clarity, all possible
steps are illustrated, even if some of them are optional.

.. diagram:: ReflectometryReductionOne_HighLvl-v2_wkflw.dot

First, the algorithm checks the X units of the input workspace. If the input
workspace is already in wavelength, summation and normalization by monitors and
direct beam are not performed, as it is considered that the input run was
already reduced using this algorithm.

If summation is to be done in wavelength, then this is done first. The the
conversion to wavelength and normalisation by monitors and direct beam is done,
followed by the transmission correction. Transmission correction is always
done, even if the input was already in wavelength. The resulting workspace is
cropped in wavelength according to :literal:`WavelengthMin` and
:literal:`WavelengthMax`, which are both mandatory properties.

If summation is to be done in Q, this is done after the normalisations and
cropping, but again, only if the reduction has not already been done.

Finally, the output workspace in wavelength is converted to momentum transfer
(Q).

Conversion to Wavelength
########################

If summing in wavelength, detectors of interest are extracted and summed in TOF
using :ref:`algm-GroupDetectors` with ``ProcessingInstructions`` as input. (If
summing in Q, summation is not done yet as it is done in a later step after all
normalisations have been done.) The workspace is then converted to wavelength
with :literal:`AlignBins` set to :literal:`True`.

Next, normalization by direct beam and monitors is optionally done using
:ref:`algm-Divide`.  A summary of the steps is shown in the workflow diagram
below.

.. diagram:: ReflectometryReductionOne_ConvertToWavelength-v2_wkflw.dot

Create Direct Beam Workspace
############################

Direct Beam and Monitor corrections can be applied to the workspace. These are
both optional steps and will only take place if the required inputs are
provided - otherwise, these steps will be skipped.

The region of direct beam is extracted from the input workspace in TOF using
:ref:`algm-GroupDetectors` with ``RegionOfDirectBeam`` as input. This is only
done if ``RegionOfDirectBeam`` is specified. The resulting workspace is
converted to wavelength with :literal:`AlignBins` set to :literal:`True`.

.. diagram:: ReflectometryReductionOne_DirectBeamCorrection-v2_wkflw.dot

Create Monitor Workspace
########################

Monitors are extracted from the input workspace in TOF using
:ref:`algm-CropWorkspace` with ``I0MonitorIndex`` as input. The resulting
workspace is converted to wavelength with :literal:`AlignBins` set to
:literal:`True`. Monitor normalisation is only done if ``I0MonitorIndex``,
``MonitorBackgroundWavelengthMin`` and ``MonitorBackgroundWavelengthMax`` are
all specified.

Normalisation can be done by integrated monitors by setting
:literal:`NormalizeByIntegratedMonitors` to true, in which case
:literal:`MonitorIntegrationWavelengthMin` and
:literal:`MonitorIntegrationWavelengthMax` are used as the integration
range. If monitors are not integrated, detectors are rebinned to monitors using
:ref:`algm-RebinToWorkspace` so that the normalization by monitors can take
place.

.. diagram:: ReflectometryReductionOne_MonitorCorrection-v2_wkflw.dot

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

Sum in Q
########

If summing in Q, the summation is done now, after all normalisations and
cropping have been done. As with summation in :math:`\lambda`, the summation is
only done if the reduction has not already been done.

The summation is done using the algorithm proposed by Cubitt et al
(J. Appl. Crystallogr., 48 (6) (2015)). This involves a projection to an
arbitrary reference angle, :math:`2\theta_R`, with a "virtual" wavelength,
:math:`\lambda_v`. This is the wavelength the neutron would have had if it had
arrived at :math:`2\theta_R` with the same momentum transfer (:math:`Q`).

Counts are considered to be spread evenly over the input pixel, and the
top-left and bottom-right corner of the pixel are projected onto
:math:`2\theta_R` giving a range in :math:`\lambda_v` to project onto. Counts
are shared out proportionally into the output bins that overlap this range, and
the projected counts from all pixels are summed into the appropriate output
bins.

The resulting 1D workspace in :math:`\lambda_v` at :math:`2\theta_R` becomes
the output workspace in wavelength.

.. diagram:: ReflectometryReductionOne_SumInQ-v2_wkflw.dot

The ``IncludePartialBins`` property specifies how the :math:`\lambda_v` range
should be calculated from the input range :math:`\lambda_1, \lambda_2` (which
corresponds to ``WavelengthMin``, ``WavelengthMax``). If ``IncludePartialBins``
is ``false`` (default) then we use the projection to the strictly-cropped range
:math:`\lambda_{c_1},\lambda_{c_2}`. This excludes any counts from the
orange-shaded triangles shown in the figure, for which we may only have partial
information because counts from the red shaded triangles are outside the
specified lambda range.

If ``IncludePartialBins`` is ``true`` then the algorithm will use the full
projected range :math:`\lambda_{f_1},\lambda_{f_2}`. This will include all
counts from the input range :math:`\lambda_1,\lambda_2`, but may result in
partially-filled bins for counts contributed from the orange-shaded regions if
data is not available in the red-shaded regions. Note however that if the red
regions do contain counts then they will still be included, e.g. if you have
narrowed the range ``WavelengthMin``, ``WavelengthMax`` from the available
range for the instrument then the red regions may contain valid counts.

.. figure:: /images/ReflectometryReductionOneDetectorPositions.png
    :width: 400px
    :align: center


Conversion to Momentum Transfer (Q)
###################################

Finally, the output workspace in wavelength is converted to momentum transfer
(:math:`Q`) using :ref:`algm-ConvertUnits`. The equation used is
:math:`Q=4\pi sin(\theta_R)/\lambda_v` in the non-flat sample case or
:math:`Q=4\pi sin(2\theta_R-\theta_0)/\lambda_v` in the divergent beam
case. This is because the latter needs to take into account the divergence of
the beam from the assumed direct beam direction.

.. diagram:: ReflectometryReductionOne_ConvertToMomentum-v2_wkflw.dot

Note that the output workspace in Q is a workspace with native binning, and no
rebin step is applied to it. If you wish to obtain a rebinned workspace in Q
you should consider using algorithm :ref:`algm-ReflectometryReductionOneAuto`
instead, which is a facade over this algorithm and has two extra steps
(:ref:`algm-Rebin` and :ref:`algm-Scale`) to produce an additional workspace in
Q with specified binning and scale factor. Please refer to
:ref:`algm-ReflectometryReductionOneAuto` for more information.

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

   print("{:.4f}".format(IvsLam.readY(0)[533]))
   print("{:.4f}".format(IvsLam.readY(0)[534]))
   print("{:.4f}".format(IvsQ.readY(0)[327]))
   print("{:.4f}".format(IvsQ.readY(0)[328]))


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

   print("{:.4f}".format(IvsLam.readY(0)[480]))
   print("{:.4f}".format(IvsLam.readY(0)[481]))
   print("{:.4f}".format(IvsQ.readY(0)[107]))
   print("{:.4f}".format(IvsQ.readY(0)[108]))


Output:

.. testoutput:: ExReflRedOneTrans

   0.4597
   0.4654
   0.7203
   1.0512

.. categories::

.. sourcelink::
