.. algorithm::

.. summary::

.. relatedalgorithms::

.. properties::

Description
-----------

A brief description here

Usage
-----

**Example - Using Only MuonGroupingAsymmetry on Single Period Data**

.. testcode:: SinglePeriod

    # Create a workspaces with four spectra
    dataX = [0, 1, 2, 3, 4, 5] * 4
    dataY = [10, 20, 30, 20, 10] * 4
    ws = CreateWorkspace(dataX, dataY, NSpec=4)
    ws.mutableRun().addProperty("goodfrm", 10, 'nonDim', True)
    for i in range(4):
        # set detector IDs to be 1,2,3,4
        # these do not have to be the same as the spectrum numbers
        # (the spectrum number are 0,1,2,3 in this case)
        ws.getSpectrum(i).setDetectorID(i + 1)
    # Put the workspace inside a WorkspaceGroup
    input_workspace = GroupWorkspaces(ws)

    output_workspace = MuonGroupingAsymmetry(InputWorkspace=input_workspace,
                                                    GroupName="fwd")
    print("X values are : {}".format(output_workspace.readX(0)))
    print("Y values are : {}".format(output_workspace.readY(0).round(3)))

Output
------

.. testoutput:: SinglePeriod

    X values are : [ 0.  1.  2.  3.  4.  5.]
    Y values are : [-0.864 -0.572  0.011  0.063 -0.162]

**Example - Using Only MuonGroupingAsymmetry on Multi Period Data**

.. testcode:: ExampleMultiPeriod

    # Create two workspaces with four spectra
    dataX = [0, 1, 2, 3, 4, 5] * 4
    dataY = [10, 20, 30, 20, 10] * 4
    ws1 = CreateWorkspace(dataX, dataY, NSpec=4)
    ws1.mutableRun().addProperty("goodfrm", 10, 'nonDim', True)
    ws2 = CreateWorkspace(dataX, dataY, NSpec=4)
    ws2.mutableRun().addProperty("goodfrm", 10, 'nonDim', True)
    for i in range(4):
        # set detector IDs to be 1,2,3,4
        # these do not have to be the same as the spectrum numbers
        # (the spectrum number are 0,1,2,3 in this case)
        ws1.getSpectrum(i).setDetectorID(i + 1)
        ws2.getSpectrum(i).setDetectorID(i + 1)
    
    # Create multi period data
    multi_period_data = GroupWorkspaces(ws1)
    multi_period_data.addWorkspace(ws2)

    # This time we won't run MuonPreProcess, as we don't want to apply any pre-processing
    # and we already have a WorkspaceGroup
    output_workspace = MuonGroupingAsymmetry(InputWorkspace=multi_period_data, SummedPeriods=[1, 2])
    # We have asked for periods 1+2, with each period summing detectors 1,2,3,4
    print("X values are : {}".format(output_workspace.readX(0)))
    print("Y values are : {}".format(output_workspace.readY(0).round(3)))

Output
------

.. testoutput:: ExampleMultiPeriod

    X values are : [ 0.  1.  2.  3.  4.  5.]
    Y values are : [-0.864 -0.572  0.011  0.063 -0.162]

.. categories::

.. sourcelink::roupingAsymmetry-v1.rst