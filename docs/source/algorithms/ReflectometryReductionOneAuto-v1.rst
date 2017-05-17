.. algorithm::

.. summary::

.. alias::

.. properties::

Description
-----------

Facade over :ref:`algm-ReflectometryReductionOne`.

Pulls numeric parameters out of the instrument parameters where possible. You can override any of these automatically applied defaults by providing your own value for the input.

See :ref:`algm-ReflectometryReductionOne` for more information on the wrapped algorithm.

ProcessingInstructions
######################

If ProcessingInstructions is not set its value is inferred from other properties:

* If AnalysisMode = PointDetectorAnalaysis and PointDetectorStart = PointDetectorStop then the spectrum specified by PointDetectorStart is used.
* If AnalysisMode = PointDetectorAnalaysis and PointDetectorStart ≠ PointDetectorStop then the sum of the spectra from PointDetectorStart to PointDetectorStop is used.
* If AnalysisMode = MultiDetectorAnalaysis then all of the spectra from MultiDetectorStart onwards are used.

Note, the ProcessingInstructions are workspace indicies, not detector IDs. The first few workspaces may correspond to monitors, rather than detectors of interest.
For the syntax of this property, see :ref:`algm-GroupDetectors`.

Workflow for WorkspaceGroups
############################

If a WorkspaceGroup is provided to ReflectometryReductionOneAuto, it will follow the steps shown in the diagram below to produce its output.

.. diagram:: ReflectometryReductionOneAuto-v1-Groups_wkflw.dot

Workflow for Polarization Correction
####################################

If polarization correction is enabled, it is performed as an additional step once the main processing has completed.
The following diagram shows how the :ref:`algm-PolarizationCorrection` algorithm is used.

.. diagram:: ReflectometryReductionOneAuto-v1-PolarizationCorrection_wkflw.dot

Polynomial Correction
#####################

If no Transmission runs are provided, then polynomial correction can be
performed instead. Polynomial correction is enabled by setting the
:literal:`CorrectionAlgorithm` property.

If set to :literal:`AutoDetect`, it looks at the instrument
parameters for the :literal:`correction` parameter. If it is set to
:literal:`polynomial`, then polynomial correction is performed using the
:ref:`algm-PolynomialCorrection` algorithm, with the polynomial string taken
from the instrument's :literal:`polynomial` parameter. If the
:literal:`correction` parameter is set to :literal:`exponential` instead, then
the :Ref:`algm-ExponentialCorrection` algorithm is used, with C0 and C1 taken
from the instrument parameters, :literal:`C0` and :literal:`C1`.

These can be specified manually by setting the :literal:`CorrectionAlgorithm`,
:literal:`Polynomial`, :literal:`C0`, and :literal:`C1` properties accordingly.

Usage
-----

**Example - Reduce a Run**

.. testcode:: ExReflRedOneAutoSimple

    run = Load(Filename='INTER00013460.nxs')
    # Basic reduction with no transmission run
    IvsQ, IvsLam, thetaOut = ReflectometryReductionOneAuto(InputWorkspace=run, ThetaIn=0.7, Version=1)

    print "The first four IvsLam Y values are: [ %.4e, %.4e, %.4e, %.4e ]" % (IvsLam.readY(0)[0], IvsLam.readY(0)[1], IvsLam.readY(0)[2], IvsLam.readY(0)[3])
    print "The first four IvsQ Y values are: [ %.4e, %.4e, %.4e, %.4e ]" % (IvsQ.readY(0)[0], IvsQ.readY(0)[1], IvsQ.readY(0)[2], IvsQ.readY(0)[3])
    print "Theta out is the same as theta in:",thetaOut

Output:

.. testoutput:: ExReflRedOneAutoSimple

    The first four IvsLam Y values are: [ 5.3860e-06, 9.3330e-06, 6.9796e-06, 6.5687e-06 ]
    The first four IvsQ Y values are: [ 6.1526e-04, 7.7591e-04, 9.1161e-04, 1.0912e-03 ]
    Theta out is the same as theta in: 0.7

**Example - Reduce a Run with a transmission run**

.. testcode:: ExReflRedOneAutoTrans

    run = Load(Filename='INTER00013460.nxs')
    trans = Load(Filename='INTER00013463.nxs')
    # Basic reduction with a transmission run
    IvsQ, IvsLam, thetaOut = ReflectometryReductionOneAuto(InputWorkspace=run, FirstTransmissionRun=trans, ThetaIn=0.7, Version=1)

    print "The first four IvsLam Y values are: [ %.4e, %.4e, %.4e, %.4e ]" % (IvsLam.readY(0)[0], IvsLam.readY(0)[1], IvsLam.readY(0)[2], IvsLam.readY(0)[3])
    print "The first four IvsQ Y values are: [ %.4e, %.4e, %.4e, %.4e ]" % (IvsQ.readY(0)[0], IvsQ.readY(0)[1], IvsQ.readY(0)[2], IvsQ.readY(0)[3])
    print "Theta out is the same as theta in:",thetaOut

Output:

.. testoutput:: ExReflRedOneAutoTrans

    The first four IvsLam Y values are: [ 3.2705e-05, 5.5450e-05, 3.9630e-05, 3.5770e-05 ]
    The first four IvsQ Y values are: [ 8.4835e-01, 1.0204e+00, 1.3771e+00, 1.2833e+00 ]
    Theta out is the same as theta in: 0.7

**Example - Reduce a Run overloading default parameters**

.. testcode:: ExReflRedOneAutoOverload

    run = Load(Filename='INTER00013460.nxs')
    # Reduction overriding the default values for MonitorBackgroundWavelengthMin and MonitorBackgroundWavelengthMax which would otherwise be retirieved from the workspace
    IvsQ, IvsLam, thetaOut = ReflectometryReductionOneAuto(InputWorkspace=run, ThetaIn=0.7, MonitorBackgroundWavelengthMin=0.0, MonitorBackgroundWavelengthMax=1.0, Version=1)

    print "The first four IvsLam Y values are: [ %.4e, %.4e, %.4e, %.4e ]" % (IvsLam.readY(0)[0], IvsLam.readY(0)[1], IvsLam.readY(0)[2], IvsLam.readY(0)[3])
    print "The first four IvsQ Y values are: [ %.4e, %.4e, %.4e, %.4e ]" % (IvsQ.readY(0)[0], IvsQ.readY(0)[1], IvsQ.readY(0)[2], IvsQ.readY(0)[3])
    print "Theta out is the same as theta in:",thetaOut

Output:

.. testoutput:: ExReflRedOneAutoOverload

    The first four IvsLam Y values are: [ 5.3868e-06, 9.3344e-06, 6.9807e-06, 6.5696e-06 ]
    The first four IvsQ Y values are: [ 6.1535e-04, 7.7602e-04, 9.1174e-04, 1.0913e-03 ]
    Theta out is the same as theta in: 0.7

**Example - Polynomial correction**

.. testcode:: ExReflRedOneAutoPoly

    run = Load(Filename='INTER00013460.nxs')
    # Set up some paramters, allowing the algorithm to automatically detect the correction to use
    SetInstrumentParameter(run, "correction", Value="polynomial")
    SetInstrumentParameter(run, "polynomial", Value="0,0.5,1,2,3")

    IvsQ, IvsLam, thetaOut = ReflectometryReductionOneAuto(InputWorkspace=run, ThetaIn=0.7, Version=1)

    def findByName(histories, name):
        return filter(lambda x: x.name() == name, histories)[0]

    # Find the PolynomialCorrection entry in the workspace's history
    algHist = IvsLam.getHistory()
    refRedOneAutoHist = findByName(algHist.getAlgorithmHistories(), "ReflectometryReductionOneAuto")
    refRedOneHist = findByName(refRedOneAutoHist.getChildHistories(), "ReflectometryReductionOne")
    polyCorHist = findByName(refRedOneHist.getChildHistories(), "PolynomialCorrection")

    coefProp = findByName(polyCorHist.getProperties(), "Coefficients")

    print "Coefficients: '" + coefProp.value() + "'"

Output:

.. testoutput:: ExReflRedOneAutoPoly

    Coefficients: '0,0.5,1,2,3'

.. categories::

.. sourcelink::
