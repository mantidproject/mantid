.. algorithm::

.. summary::

.. alias::

.. properties::

Description
-----------

Facade over
:ref:`algm-CreateTransmissionWorkspace`. Pulls numeric parameters out of the instrument parameters where possible. You can override any of these automatically applied defaults by providing your own value for the input.

If ProcessingInstructions is not set its value is inferred from other properties:

* If AnalysisMode = PointDetectorAnalaysis and PointDetectorStart = PointDetectorStop then the spectrum specified by PointDetectorStart is used.
* If AnalysisMode = PointDetectorAnalaysis and PointDetectorStart ≠ PointDetectorStop then the spectra specified by PointDetectorStart and PointDetectorStop are used.
* If AnalysisMode = MultiDetectorAnalaysis then the spectrum specified by MultiDetectorStart and the last spectrum are used.

See :ref:`algm-CreateTransmissionWorkspace` for more information on the wrapped algorithm.

Usage
-----

.. include:: ../usagedata-note.txt

**Example - Create a transmission run**

.. testcode:: ExCreateTransWSAutoSimple

    trans = Load(Filename='INTER00013463.nxs')
    # Reduction overriding the default values for MonitorBackgroundWavelengthMin and MonitorBackgroundWavelengthMax which would otherwise be retirieved from the workspace
    transWS = CreateTransmissionWorkspaceAuto(FirstTransmissionRun=trans, WavelengthStep=0.05)

    print "The first four transWS Y values are:"
    for i in range (4):
        print "%.4f" % transWS.readY(0)[i]


Output:

.. testoutput:: ExCreateTransWSAutoSimple

    The first four transWS Y values are:
    0.0223
    0.0602
    0.1020
    0.1207

    
**Example - Create a transmission run, overloading default parameters**

.. testcode:: ExCreateTransWSAutoOverload

    trans = Load(Filename='INTER00013463.nxs')
    # Reduction overriding the default values for MonitorBackgroundWavelengthMin and MonitorBackgroundWavelengthMax which would otherwise be retirieved from the workspace
    transWS = CreateTransmissionWorkspaceAuto(FirstTransmissionRun=trans, MonitorBackgroundWavelengthMin=0.0, MonitorBackgroundWavelengthMax=1.0, WavelengthStep=0.05)

    print "The first four transWS Y values are:"
    for i in range (4):
        print "%.4f" % transWS.readY(0)[i]

Output:

.. testoutput:: ExCreateTransWSAutoOverload

    The first four transWS Y values are:
    0.0221
    0.0598
    0.1013
    0.1198

    
**Example - Create a transmission run from two runs**

.. testcode:: ExCreateTransWSAutoTwo

    trans1 = Load(Filename='INTER00013463.nxs')
    trans2 = Load(Filename='INTER00013464.nxs')
    # Reduction overriding the default values for MonitorBackgroundWavelengthMin and MonitorBackgroundWavelengthMax which would otherwise be retirieved from the workspace
    transWS = CreateTransmissionWorkspaceAuto(FirstTransmissionRun=trans1, SecondTransmissionRun=trans2, WavelengthStep=0.05, Params=[1.5,0.02,17], StartOverlap=10.0, EndOverlap=12.0)

    print "The first four transWS Y values are:"
    for i in range (4):
        print "%.4f" % transWS.readY(0)[i]

Output:

.. testoutput:: ExCreateTransWSAutoTwo

    The first four transWS Y values are:
    0.0567
    0.0575
    0.0577
    0.0580


.. categories::

.. sourcelink::
