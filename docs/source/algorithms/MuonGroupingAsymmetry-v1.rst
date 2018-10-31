.. algorithm::

.. summary::

.. relatedalgorithms::

.. properties::

Description
-----------

A brief description here

Usage
-----
**Example**

.. testcode:: MuonPreProcess

    # Begin by loading data into a workspace to process
    load_result = LoadMuonNexus(Filename = "MUSR0015189.nxs",
                                DetectorGroupingTable = "grouping",
                                OutputWorkspace = "loadedWS")
    loaded_time_zero = load_result[2] # time zero loaded from file
    grouping = mtd['grouping']        # detector grouping loaded from file
    grouping = grouping.getItem(0)    # grouping here is a WorkspaceGroup - use table for first period

    muon_process_output = MuonProcess(InputWorkspace = "loadedWS",
                                      Mode = "Combined",
                                      DetectorGroupingTable = grouping,
                                      SummedPeriodSet = "1",
                                      LoadedTimeZero = loaded_time_zero,
                                      TimeZero = 0.55,
                                      Xmin = 0.11,
                                      Xmax = 12,
                                      OutputType = "GroupAsymmetry",
                                      PairFirstIndex = 0,
                                      PairSecondIndex = 1,
                                      Alpha = 1.0,
                                      OutputWorkspace = "MuonProcess_output",
                                      groupIndex=1)
            
    pre_processed_workspace = MuonPreProcess(InputWorkspace="loadedWS",
                                             TimeMin=0.11, TimeMax=12, 
                                             TimeOffSet=loaded_time_zero-0.55)

    group_1 = MuonGroupingCounts(InputWorkspace=pre_processed_workspace, GroupName="group_1", Grouping="33-64")
    group_2 = MuonGroupingCounts(InputWorkspace=pre_processed_workspace, GroupName="group_2", Grouping="1-32")

    apended_workspace = AppendSpectra(group_1, group_2)
    workspace_group = GroupWorkspaces(apended_workspace)

    muon_grouping_asymmetry_output = MuonGroupingAsymmetry(InputWorkspace=workspace_group,
                                                           DetectorIndex=1,
                                                           AsymmetryTimeMin=0.11,
                                                           AsymmetryTimeMax=12)

    output_int = Integration(muon_process_output)

    output_int_new = Integration(muon_grouping_asymmetry_output)

    print('Integrated asymmetry for the run: {0:.3f}'.format(output_int.readY(0)[0]))
    print('Integrated asymmetry for the run: {0:.3f}'.format(output_int_new.readY(0)[0]))

Output
------

.. testoutput:: MuonPreProcess
    
    Integrated asymmetry for the run: 5.048
    Integrated asymmetry for the run: 5.048

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

.. sourcelink::MuonGroupingAsymmetry-v1.rst