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
                                      OutputType = "PairAsymmetry",
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

    pairing_output = MuonPairingAsymmetry(InputWorkspace1=group_1, InputWorkspace2=group_2, SummedPeriods="1", Alpha=1.0)

    output_int = Integration(muon_process_output)

    output_int_new = Integration(pairing_output)

    print('Integrated asymmetry for the run: {0:.3f}'.format(output_int.readY(0)[0]))
    print('Integrated asymmetry for the run: {0:.3f}'.format(output_int_new.readY(0)[0]))
    

Output
------

.. testoutput:: MuonPreProcess
    
    Integrated asymmetry for the run: 1.701
    Integrated asymmetry for the run: 1.701

.. categories::

.. sourcelink::MuonPairAsymmetry-v1.rst