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
    transWS = CreateTransmissionWorkspaceAuto(FirstTransmissionRun=trans, Version=1)

    print("The first four transWS Y values are:")
    for i in range (4):
        print("{:.4f}".format(transWS.readY(0)[i]))


Output:

.. testoutput:: ExCreateTransWSAutoSimple

    The first four transWS Y values are:
    0.1647
    0.1683
    0.1761
    0.1836

    
**Example - Create a transmission run, overloading default parameters**

.. testcode:: ExCreateTransWSAutoOverload

    trans = Load(Filename='INTER00013463.nxs')
    # Reduction overriding the default values for MonitorBackgroundWavelengthMin and MonitorBackgroundWavelengthMax which would otherwise be retirieved from the workspace
    transWS = CreateTransmissionWorkspaceAuto(FirstTransmissionRun=trans, MonitorBackgroundWavelengthMin=0.0, MonitorBackgroundWavelengthMax=1.0, Version=1)

    print("The first four transWS Y values are:")
    for i in range (4):
        print("{:.4f}".format(transWS.readY(0)[i]))

Output:

.. testoutput:: ExCreateTransWSAutoOverload

    The first four transWS Y values are:
    0.1647
    0.1683
    0.1761
    0.1836

    
**Example - Create a transmission run from two runs**

.. testcode:: ExCreateTransWSAutoTwo

    trans1 = Load(Filename='INTER00013463.nxs')
    trans2 = Load(Filename='INTER00013464.nxs')
    # Reduction overriding the default values for MonitorBackgroundWavelengthMin and MonitorBackgroundWavelengthMax which would otherwise be retirieved from the workspace
    transWS = CreateTransmissionWorkspaceAuto(FirstTransmissionRun=trans1, SecondTransmissionRun=trans2, Params=[1.5,0.02,17], StartOverlap=10.0, EndOverlap=12.0, Version=1)

    print("The first four transWS Y values are:")
    for i in range (4):
        print("{:.4f}".format(transWS.readY(0)[i]))

Output:

.. testoutput:: ExCreateTransWSAutoTwo

    The first four transWS Y values are:
    0.0000
    0.0126
    0.0579
    0.0579


.. categories::

.. sourcelink::
