.. algorithm::

.. summary::

.. alias::

.. properties::

Description
-----------

Facade over :ref:`algm-ReflectometryReductionOne`.

Pulls numeric parameters out of the instrument parameters where possible. You can override any of these automatically applied defaults by providing your own value for the input.

If ProcessingInstructions is not set its value is inferred from other properties:

* If AnalysisMode = PointDetectorAnalaysis and PointDetectorStart = PointDetectorStop then the spectrum specified by PointDetectorStart is used.
* If AnalysisMode = PointDetectorAnalaysis and PointDetectorStart ≠ PointDetectorStop then the sum of the spectra from PointDetectorStart to PointDetectorStop is used.
* If AnalysisMode = MultiDetectorAnalaysis then all of the spectra from MultiDetectorStart onwards are used.

See :ref:`algm-ReflectometryReductionOne` for more information on the wrapped algorithm.

Workflow for WorkspaceGroups
############################

If a WorkspaceGroup is provided to ReflectometryReductionOneAuto, it will follow the steps shown in the diagram below to produce its output.

.. diagram:: ReflectometryReductionOneAuto-v1-Groups_wkflw.dot

Workflow for Polarization Correction
####################################

If polarization correction is enabled, it is performed as an additional step once the main processing has completed.
The following diagram shows how the :ref:`algm-PolarizationCorrection` algorithm is used.

.. diagram:: ReflectometryReductionOneAuto-v1-PolarizationCorrection_wkflw.dot

Usage
-----

**Example - Reduce a Run**

.. testcode:: ExReflRedOneAutoSimple

    run = Load(Filename='INTER00013460.nxs')
    # Basic reduction with no transmission run
    IvsQ, IvsLam, thetaOut = ReflectometryReductionOneAuto(InputWorkspace=run, ThetaIn=0.7)

    print "The first four IvsLam Y values are: [", str(IvsLam.readY(0)[0]),",", str(IvsLam.readY(0)[1]),",", str(IvsLam.readY(0)[2]),",", str(IvsLam.readY(0)[3]),"]"
    print "The first four IvsQ Y values are: [", str(IvsQ.readY(0)[0]),",", str(IvsQ.readY(0)[1]),",", str(IvsQ.readY(0)[2]),",", str(IvsQ.readY(0)[3]),"]"
    print "Theta out is the same as theta in:",thetaOut

Output:

.. testoutput:: ExReflRedOneAutoSimple

    The first four IvsLam Y values are: [ 0.0 , 0.0 , 0.0 , 1.19084981351e-06 ]
    The first four IvsQ Y values are: [ 3.26638386884e-05 , 5.41802219385e-05 , 4.89364938612e-05 , 5.50890537024e-05 ]
    Theta out is the same as theta in: 0.7

**Example - Reduce a Run with a transmission run**

.. testcode:: ExReflRedOneAutoTrans

    run = Load(Filename='INTER00013460.nxs')
    trans = Load(Filename='INTER00013463.nxs')
    # Basic reduction with a transmission run
    IvsQ, IvsLam, thetaOut = ReflectometryReductionOneAuto(InputWorkspace=run, FirstTransmissionRun=trans, ThetaIn=0.7)

    print "The first four IvsLam Y values are: [", str(IvsLam.readY(0)[0]),",", str(IvsLam.readY(0)[1]),",", str(IvsLam.readY(0)[2]),",", str(IvsLam.readY(0)[3]),"]"
    print "The first four IvsQ Y values are: [", str(IvsQ.readY(0)[0]),",", str(IvsQ.readY(0)[1]),",", str(IvsQ.readY(0)[2]),",", str(IvsQ.readY(0)[3]),"]"
    print "Theta out is the same as theta in:",thetaOut

Output:

.. testoutput:: ExReflRedOneAutoTrans

    The first four IvsLam Y values are: [ 0.0 , 0.0 , 0.0 , 1.00380795752e-05 ]
    The first four IvsQ Y values are: [ 0.801217183288 , 0.93447842459 , 0.541409172289 , 0.920895160905 ]
    Theta out is the same as theta in: 0.7

**Example - Reduce a Run overloading default parameters**

.. testcode:: ExReflRedOneAutoOverload

    run = Load(Filename='INTER00013460.nxs')
    # Reduction overriding the default values for MonitorBackgroundWavelengthMin and MonitorBackgroundWavelengthMax which would otherwise be retirieved from the workspace
    IvsQ, IvsLam, thetaOut = ReflectometryReductionOneAuto(InputWorkspace=run, ThetaIn=0.7, MonitorBackgroundWavelengthMin=0.0, MonitorBackgroundWavelengthMax=1.0)

    print "The first four IvsLam Y values are: [", str(IvsLam.readY(0)[0]),",", str(IvsLam.readY(0)[1]),",", str(IvsLam.readY(0)[2]),",", str(IvsLam.readY(0)[3]),"]"
    print "The first four IvsQ Y values are: [", str(IvsQ.readY(0)[0]),",", str(IvsQ.readY(0)[1]),",", str(IvsQ.readY(0)[2]),",", str(IvsQ.readY(0)[3]),"]"
    print "Theta out is the same as theta in:",thetaOut

Output:

.. testoutput:: ExReflRedOneAutoOverload

    The first four IvsLam Y values are: [ 0.0 , 0.0 , 0.0 , 1.17970288209e-06 ]
    The first four IvsQ Y values are: [ 3.2358089327e-05 , 5.36730688015e-05 , 4.84784245605e-05 , 5.45733934596e-05 ]
    Theta out is the same as theta in: 0.7

.. categories::
