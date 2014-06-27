.. algorithm::

.. summary::

.. alias::

.. properties::

Description
-----------

Facade over
:ref:`algm-CreateTransmissionWorkspace`. Pulls numeric parameters out of the instrument parameters where possible. You can override any of these automatically applied defaults by providing your own value for the input.

See :ref:`algm-CreateTransmissionWorkspace` for more information on the wrapped algorithm.

Usage
-----

**Example - Create a transmission run**

.. testcode:: ExCreateTransWSAutoSimple

    trans = Load(Filename='INTER00013463.nxs')
    # Reduction overriding the default values for MonitorBackgroundWavelengthMin and MonitorBackgroundWavelengthMax which would otherwise be retirieved from the workspace
    transWS = CreateTransmissionWorkspaceAuto(FirstTransmissionRun=trans)

    print "The first four transWS Y values are: [", str(transWS.readY(0)[0]),",", str(transWS.readY(0)[1]),",", str(transWS.readY(0)[2]),",", str(transWS.readY(0)[3]),"]"

Output:

.. testoutput:: ExCreateTransWSAutoSimple

    The first four transWS Y values are: [ 0.0159818857034 , 0.0348091167462 , 0.09083218605 , 0.118656501304 ]
    
**Example - Create a transmission run, overloading default parameters**

.. testcode:: ExCreateTransWSAutoOverload

    trans = Load(Filename='INTER00013463.nxs')
    # Reduction overriding the default values for MonitorBackgroundWavelengthMin and MonitorBackgroundWavelengthMax which would otherwise be retirieved from the workspace
    transWS = CreateTransmissionWorkspaceAuto(FirstTransmissionRun=trans, MonitorBackgroundWavelengthMin=0.0, MonitorBackgroundWavelengthMax=1.0)

    print "The first four transWS Y values are: [", str(transWS.readY(0)[0]),",", str(transWS.readY(0)[1]),",", str(transWS.readY(0)[2]),",", str(transWS.readY(0)[3]),"]"

Output:

.. testoutput:: ExCreateTransWSAutoOverload

    The first four transWS Y values are: [ 0.0158418047689 , 0.0345040154775 , 0.0900360436082 , 0.11761647925 ]
    
**Example - Create a transmission run from two runs**

.. testcode:: ExCreateTransWSAutoTwo

    trans1 = Load(Filename='INTER00013463.nxs')
    trans2 = Load(Filename='INTER00013464.nxs')
    # Reduction overriding the default values for MonitorBackgroundWavelengthMin and MonitorBackgroundWavelengthMax which would otherwise be retirieved from the workspace
    transWS = CreateTransmissionWorkspaceAuto(FirstTransmissionRun=trans1, SecondTransmissionRun=trans2, Params=[1.5,0.02,17], StartOverlap=10.0, EndOverlap=12.0)

    print "The first four transWS Y values are: [", str(transWS.readY(0)[0]),",", str(transWS.readY(0)[1]),",", str(transWS.readY(0)[2]),",", str(transWS.readY(0)[3]),"]"

Output:

.. testoutput:: ExCreateTransWSAutoTwo

    The first four transWS Y values are: [ 0.0563401903863 , 0.0564168216735 , 0.0568585531934 , 0.0581920248745 ]

.. categories::
