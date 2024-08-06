.. algorithm::

.. summary::

.. relatedalgorithms::

.. properties::

Description
-----------

When interacting with the :ref:`Muon_Analysis-ref` interface, operations such as detector grouping, group asymmetry and pair asymmetry are performed on data. This algorithm performs a "grouping counts" operation, in other words it sums the counts associated to a given sequence of detector IDs.

This algorithm is part of a set of four; with :ref:`algm-MuonPreProcess` being run first; and the output being fed into this one. This allows the replication of the workflow used by the muon analysis interface to produce group data.

Analysis
########

A workspace has one or more *spectra* contained within it; for muon data each spectra has a unique detector ID. Assuming the y-values represent counts; a *detector grouping* operation causes the counts to be summed across the given set of detector IDs which are supplied to the **Grouping** argument (for example `1,2,3,4,5` and `1-5`).

The **InputWorkspace** must be a *WorkspaceGroup*, where each workspace within the workspace-group represents a single period. Thus, single period data is just a *workspaceGroup* with a single workspace within it.

The detector-group must be given a name via **GroupName** which can consist of letters, numbers and underscores.

#. Valid names : "fwd", "fwd2", "fwd_2", "1234"
#. Invalid names : "", "fwd!", "fwd "

The detector-group name does not affect the data; however the name is used in the muon interface when automatically generating workspace names from detector-group data.

Multi period data
#################

Both single and multi period data are supported by the algorithm.

The **SummedPeriods** and **SubtractedPeriods** inputs are used to control the way that periods are combined. so for example;

#. SummedPeriods = 1,2
#. SubtractedPeriods = 3,4

would combine periods in the combination $(1+2)-(3+4)$.

Usage
-----

**Example - Using MuonPreProcess followed by MuonGroupingCounts on Single Period Data**

.. testcode:: ConvertToGroup

    # Single period data with four spectra
    dataX = [0, 1, 2, 3, 4, 5] * 4
    dataY = [10, 20, 30, 20, 10] * 4
    input_workspace = CreateWorkspace(dataX, dataY, NSpec=4)
    for i in range(4):
        # set detector IDs to be 1,2,3,4
        # these do not have to be the same as the spectrum numbers
        # (the spectrum number are 0,1,2,3 in this case)
        input_workspace.getSpectrum(i).setDetectorID(i + 1)

    # We are not actually applying any processing to the data
    # but the algorithm will convert our single period data into
    # the required form (a WorkspaceGroup)
    pre_processed_workspace = MuonPreProcess(InputWorkspace=input_workspace)

    output_workspace = MuonGroupingCounts(InputWorkspace=pre_processed_workspace,
                                                    GroupName="fwd",
                                                    Grouping=[1, 2, 3, 4])

    print("X values are : {}".format(output_workspace.readX(0)))
    print("Y values are : {}".format(output_workspace.readY(0)))


Output:

.. testoutput:: ConvertToGroup

    X values are : [0. 1. 2. 3. 4. 5.]
    Y values are : [ 40.  80. 120.  80.  40.]

**Example - Using Only MuonGroupingCounts on Single Period Data**

.. testcode:: ConvertToGroup

    # Create a workspaces with four spectra
    dataX = [0, 1, 2, 3, 4, 5] * 4
    dataY = [10, 20, 30, 20, 10] * 4
    ws = CreateWorkspace(dataX, dataY, NSpec=4)
    for i in range(4):
        # set detector IDs to be 1,2,3,4
        # these do not have to be the same as the spectrum numbers
        # (the spectrum number are 0,1,2,3 in this case)
        ws.getSpectrum(i).setDetectorID(i + 1)

    # Put the workspace inside a WorkspaceGroup
    input_workspace = GroupWorkspaces(ws)

    output_workspace = MuonGroupingCounts(InputWorkspace=input_workspace,
                                                    GroupName="fwd",
                                                    Grouping=[1, 2, 3, 4])

    print("X values are : {}".format(output_workspace.readX(0)))
    print("Y values are : {}".format(output_workspace.readY(0)))


Output:

.. testoutput:: ConvertToGroup

    X values are : [0. 1. 2. 3. 4. 5.]
    Y values are : [ 40.  80. 120.  80.  40.]

**Example - Multi Period Data**

.. testcode:: ExampleTimeOffset

    # Create two workspaces with four spectra
    dataX = [0, 1, 2, 3, 4, 5] * 4
    dataY = [10, 20, 30, 20, 10] * 4
    ws1 = CreateWorkspace(dataX, dataY, NSpec=4)
    ws2 = CreateWorkspace(dataX, dataY, NSpec=4)
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

    output_workspace = MuonGroupingCounts(InputWorkspace=multi_period_data,
                                                    GroupName="fwd",
                                                    Grouping=[1, 2, 3, 4],
                                                    SummedPeriods=[1, 2])

    # We have asked for periods 1+2, with each period summing detectors 1,2,3,4
    print("X values are : {}".format(output_workspace.readX(0)))
    print("Y values are : {}".format(output_workspace.readY(0)))


Output:

.. testoutput:: ExampleTimeOffset

    X values are : [0. 1. 2. 3. 4. 5.]
    Y values are : [ 80. 160. 240. 160.  80.]

.. categories::

.. sourcelink::