.. algorithm::

.. summary::

.. relatedalgorithms::

.. properties::

Description
-----------

This algorithm is a facade over :ref:`algm-ReflectometryReductionOne` (see :ref:`algm-ReflectometryReductionOne`
for more information on the wrapped algorithm). It optionally corrects the detector position and then pulls numeric
parameters out of the instrument parameter file where possible. These automatically applied defaults
can be overriden by providing your own values. In addition, it outputs a rebinned workspace in Q, and it optionally
performs polarization analysis if the input workspace is a workspace group.

First, if :literal:`ThetaIn` is given the algorithm will try to correct the detector position. For this, it uses
:literal:`ProcessingInstructions`, which corresponds to the grouping pattern of workspace indices that define the
detectors of interest. Only the detectors of interest will be corrected, the rest of the instrument components
will remain in the original position. Note that when :literal:`ProcessingInstructions` is not set, its value
is inferred from other properties, depending on the value of :literal:`AnalysisMode`:

* If :literal:`AnalysisMode = PointDetectorAnalaysis` the algorithm will search for :literal:`PointDetectorStart` and :literal:`PointDetectorStop` in the parameter file, and :literal:`ProcessingInstructions` will be set to :literal:`PointDetectorStart:PointDetectorEnd`.
* If :literal:`AnalysisMode = MultiDetectorAnalaysis` the algorithm will search for :literal:`MultiDetectorStart` in the parameter file and all of the spectra from this value onwards will be used.

Note that ProcessingInstructions are workspace indices, not detector IDs. The first few workspaces may correspond
to monitors, rather than detectors of interest. For the syntax of this property, see :ref:`algm-GroupDetectors`.

:literal:`theta` is calcualted using :literal:`SpecularReflectionCalculateTheta`. This is passed through to :literal:`ReflectometryReductionOne` and :literal:`2 * theta` is passed through to :literal:`CalculateResolution`. :literal:`theta` can be overridden by setting :literal:`ThetaIn` or :literal:`ThetaLogName` (:literal:`ThetaIn` takes precedence if both are given). If :literal:`CorrectDetectors` is also true, then the algorithm corrects the positions of the detectors of interest to :literal:`2 * theta` using :ref:`algm-SpecularReflectionPositionCorrect`. The detectors are moved either by shifting them vertically, or by rotating them around the sample position, as specified by :literal:`DetectorCorrectionType`.

Next, the algorithm will try to populate input properties which have not been set. Specifically, it will search for
:literal:`LambdaMin`, :literal:`LambdaMax`, :literal:`I0MonitorIndex`, :literal:`MonitorBackgroundMin`, :literal:`MonitorBackgroundMax`,
:literal:`MonitorIntegralMin` and :literal:`MonitorIntegralMin` in the parameter file to populate :literal:`WavelengthMin`,
:literal:`WavelengthMax`, :literal:`I0MonitorIndex`, :literal:`MonitorBackgroundWavelengthMin`, :literal:`MonitorBackgroundWavelengthMax`,
:literal:`MonitorIntegrationWavelengthMin` and :literal:`MonitorIntegrationWavelengthMax` respectively. The first two properties
will be used by :ref:`algm-ReflectometryReductionOne` to crop the workspace in wavelength, whereas the rest of the properties
refer to monitors and are used to create a temporary monitor workspace by which the detectors of interest will be normalized.
Note that there is an additional property referring to monitors, :literal:`NormalizeByIntegratedMonitors`, which can be used
to specify whether or not integrated monitors should be considered.

The rest of the input properties are not inferred from the parameter file, and must be specified manually. :literal:`RegionOfDirectBeam` is an optional
property that allows users to specify a region of direct beam that will be used to normalize
the detector signal. The region of direct beam is specified by workspace indices. For instance, :literal:`RegionOfDirectBeam='2-3'`
means that spectra with workspace indices :literal:`2` and :literal:`3` will be summed and the resulting
workspace will be used as the direct beam workspace.

Transmission corrections can be optionally applied by providing either one or
two transmission runs or polynomial corrections. Polynomial correction is enabled by setting the
:literal:`CorrectionAlgorithm` property. If set to :literal:`AutoDetect`, the algorithm looks at the instrument
parameters for the :literal:`correction` parameter. If this parameter is set to
:literal:`polynomial`, then polynomial correction is performed using the
:ref:`algm-PolynomialCorrection` algorithm, with the polynomial string taken
from the instrument's :literal:`polystring` parameter. If the
:literal:`correction` parameter is set to :literal:`exponential` instead, then
the :Ref:`algm-ExponentialCorrection` algorithm is used, with C0 and C1 taken
from the instrument parameters, :literal:`C0` and :literal:`C1`. All these values
can be specified manually by setting the :literal:`CorrectionAlgorithm` to either
:literal:`PolynomialCorrection` or :literal:`ExponentialCorrection` and setting
:literal:`Polynomial` or :literal:`C0` and :literal:`C1` properties accordingly.
Note that when using a correction algorithm, monitors will not be integrated, even if
:literal:`NormalizeByIntegratedMonitors` was set to true.

Finally, properties :literal:`MomentumTransferMin`, :literal:`MomentumTransferStep` and :literal:`MomentumTransferMax` are
used to rebin the output workspace in Q, and :literal:`ScaleFactor` is used to scale the rebinned workspace. When they
are not provided the algorithm will attempt to determine the bin width using :ref:`algm-NRCalculateSlitResolution` (note that, for
the latter to run successfully, a :literal:`slit` component with a :literal:`vertical gap` must be defined in the
instrument definition file).

See :ref:`algm-ReflectometryReductionOne` for more information
on how the input properties are used by the wrapped algorithm.

Workspace Groups
################

If a workspace group is provided as input, each workspace in the group will be
reduced independently and sequentially using :ref:`algm-ReflectometryReductionOne`. Each of these
individual reductions will produce three output workspaces: an output workspace in
wavelength, an output workspace in Q with native binning, and a rebinned workspace in Q.
Output workspaces in wavelength will be grouped together to produce an output workspace group in wavelength, and output
workspaces in Q will be grouped together to produce an output workspace group in Q.
The diagram below illustrates this process (note that, for the sake of clarity, the rebinned output
workspace in Q, :literal:`OutputWorkspaceBinned`, is not represented but it is handled analogously to
:literal:`OutputWorkspace` and :literal:`OutputWorkspaceWavelength`):

.. diagram:: ReflectometryReductionOneAuto-v2-Groups_noPA_wkflw.dot

Polarization Analysis Off
~~~~~~~~~~~~~~~~~~~~~~~~~

If :literal:`PolarizationAnalysis = None` the reduction stops here. Note that if
transmission runs are given in the form of a workspace group, each member in the
transmission group will be associated to the corresponding member in the input
workspace group, i.e., the first item in the transmission group will be used as the
transmission run for the first workspace in the input workspace group, the second
element in the transmission group will be used as the transmission run for the
second workspace in the input workspace group, etc. This is also illustrated
in the diagram above, where :literal:`[0]` represents the first element in a
workspace group, :literal:`[1]` the second element, etc. If transmission runs
are provided as matrix workspaces the specified runs will be used for all members
of the input workspace group.

Polarization Analysis On
~~~~~~~~~~~~~~~~~~~~~~~~

If :literal:`PolarizationAnalysis` is set to :literal:`PA` or :literal:`PNR`
the reduction continues and polarization corrections will be applied to
the output workspace in wavelength. The algorithm will use the properties :literal:`PolarizationAnalysis`,
:literal:`CPp`, :literal:`CAp`, :literal:`CRho` and :literal:`CAlpha` to run :ref:`algm-PolarizationCorrection`.
The result will be a new workspace in wavelength, which will override the previous one, that will
be used as input to :ref:`algm-ReflectometryReductionOne` to calculate the new output workspaces in Q, which
in turn will override the existing workspaces in Q. Note that if transmission runs are provided in the form of workspace
groups, the individual workspaces will be summed to produce a matrix workspace that will be used as the
transmission run for all items in the input workspace group, as illustrated in the diagram below
(note that, for the sake of clarity, the rebinned output workspace in Q, :literal:`OutputWorkspaceBinned`, is not
represented but it is handled analogously to :literal:`OutputWorkspace`).

.. diagram:: ReflectometryReductionOneAuto-v2-Groups_PA_wkflw.dot

Previous Versions
-----------------

This is version 2 of the algorithm. For version 1, please see `here. <ReflectometryReductionOneAuto-v1.html>`_

Usage
-----

.. include:: ../usagedata-note.txt

**Example - Basic reduction with no transmission run, polynomial corrections will be automatically applied**

.. testcode:: ExReflRedOneAutoSimple

    run = Load(Filename='INTER00013460.nxs')
    IvsQ, IvsQ_unbinned, IvsLam = ReflectometryReductionOneAuto(InputWorkspace=run, ThetaIn=0.7)

    print("{:.5f}".format(IvsLam.readY(0)[175]))
    print("{:.5f}".format(IvsLam.readY(0)[176]))
    print("{:.5f}".format(IvsQ_unbinned.readY(0)[106]))
    print("{:.5f}".format(IvsQ_unbinned.readY(0)[107]))
    print("{:.5f}".format(IvsQ.readY(0)[13]))
    print("{:.5f}".format(IvsQ.readY(0)[14]))

Output:

.. testoutput:: ExReflRedOneAutoSimple

    0.00441
    0.00462
    0.63441
    0.41079
    0.44792
    0.23703

**Example - Basic reduction with a transmission run**

.. testcode:: ExReflRedOneAutoTrans

    run = Load(Filename='INTER00013460.nxs')
    trans = Load(Filename='INTER00013463.nxs')
    IvsQ, IvsQ_unbinned, IvsLam = ReflectometryReductionOneAuto(InputWorkspace=run, FirstTransmissionRun=trans, ThetaIn=0.7)

    print("{:.5f}".format(IvsLam.readY(0)[164]))
    print("{:.5f}".format(IvsLam.readY(0)[164]))
    print("{:.5f}".format(IvsQ_unbinned.readY(0)[96]))
    print("{:.5f}".format(IvsQ_unbinned.readY(0)[97]))
    print("{:.5f}".format(IvsQ.readY(0)[5]))
    print("{:.5f}".format(IvsQ.readY(0)[6]))

Output:

.. testoutput:: ExReflRedOneAutoTrans

    0.00338
    0.00338
    1.16756
    0.89144
    1.46655
    1.41327

**Example - Reduction overriding some default values**

.. testcode:: ExReflRedOneAutoOverload

    run = Load(Filename='INTER00013460.nxs')
    IvsQ, IvsQ_unbinned, IvsLam = ReflectometryReductionOneAuto(InputWorkspace=run, ThetaIn=0.7, DetectorCorrectionType="RotateAroundSample", MonitorBackgroundWavelengthMin=0.0, MonitorBackgroundWavelengthMax=1.0)

    print("{:.5f}".format(IvsLam.readY(0)[175]))
    print("{:.5f}".format(IvsLam.readY(0)[176]))
    print("{:.5f}".format(IvsQ_unbinned.readY(0)[106]))
    print("{:.5f}".format(IvsQ_unbinned.readY(0)[107]))
    print("{:.5f}".format(IvsQ.readY(0)[5]))
    print("{:.5f}".format(IvsQ.readY(0)[6]))

Output:

.. testoutput:: ExReflRedOneAutoOverload

    0.00441
    0.00462
    0.64231
    0.41456
    0.51029
    0.52241

.. categories::

.. sourcelink::
