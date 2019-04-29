.. algorithm::

.. summary::

.. relatedalgorithms::

.. properties::

Description
-----------

When interacting with the :ref:`Muon_Analysis-ref` interface, operations such as detector grouping, group and pair asymmetry are performed on data. This algorithm performs a "grouping asymmetry" operation, in other words it provides a numerical estimation (without fitting) of the asymmetry. More details can be found at :ref:`algm-EstimateMuonAsymmetryFromCounts`.

This algorithm is part of a set of four; with :ref:`algm-MuonPreProcess` being run first; and the output being fed into this one. This allows the replication of the workflow used by the muon analysis interface to produce group data. 

Analysis
########

A workspace has one or more *spectra* contained within it; each spectra has a unique detector ID. Assuming the y-values represent counts; a *detector grouping* operation causes the counts to be summed across the given set of detector IDs which are supplied to the **Grouping** argument (for example `1,2,3,4,5` and `1-5`).

The **InputWorkspace** must be a :ref:`WorkspaceGroup <WorkspaceGroup>`, where each workspace within the group represents a single period. Thus, single period data is just a *WorkspaceGroup* with a single workspace within it.

The group must be given a name via **GroupName** which can consist of letters, numbers and underscores. 

#. Valid names : "fwd", "fwd2", "fwd_2", "1234"
#. Invalid names : "", "fwd!", "fwd "

The group name does not affect the data; however the name is used in the muon interface when automatically generating workspace names from group data.

After the grouping is performed, the analysis described in :ref:`algm-EstimateMuonAsymmetryFromCounts` will be run; effectively estimating the muon decay curve and subtracting it from the grouped data and then estimating the asymmetry on the grouped data. The range over which the estimate is performed is controlled by the **AsymmetryTimeMin** and **AsymmetryTimeMax** inputs.

**Note** : The workspaces supplied to the algorithm must have a number of good frames set in their sample logs. The sample log is called "goodfrm" and can be set using;

.. code:: 

    AddSampleLog(workspace=workspace, LogName="goodfram", LogText ="10")

Multi period data 
#################

Both single and multi period data are supported by the algorithm.

The **SummedPeriods** and **SubtractedPeriods** inputs are used to control the way that periods are combined. so for example;

#. SummedPeriods = 1,2
#. SubtractedPeriods = 3,4 

would combine periods in the combination :math:`(1+2)-(3+4)`.

Usage
-----

**Example - Using MuonPreProcess followed by MuonGroupingAsymmetry on Single Period Data**

.. testcode:: SinglePeriod

    # Create a workspaces with one spectrum
    dataX = [0, 1, 2, 3, 4, 5]
    dataY = [10, 20, 30, 20, 10]
    input_workspace = CreateWorkspace(dataX, dataY, NSpec=1)
    input_workspace.getSpectrum(0).setDetectorID(1)
    # Workspace must have the number of good frames set
    AddSampleLog(Workspace=input_workspace, LogName='goodfrm', LogText="10")

    pre_processed_workspace = MuonPreProcess(InputWorkspace=input_workspace)

    output_workspace = MuonGroupingAsymmetry(InputWorkspace=pre_processed_workspace,
                                                       GroupName="fwd",
                                                       Grouping=[1])

    print("X values are : {}".format([round(float(i), 3) for i in output_workspace.readX(0)]))
    print("Y values are : {}".format([round(float(i), 3) for i in output_workspace.readY(0)]))


Output:

.. testoutput:: SinglePeriod

    X values are : [0.0, 1.0, 2.0, 3.0, 4.0, 5.0]
    Y values are : [-0.789, -0.334, 0.576, 0.656, 0.306]

**Example - Using MuonPreProcess followed by MuonGroupingAsymmetry on Multi Period Data**

.. testcode:: MultiPeriod

    # Create two workspaces each with two spectra
    dataX = [0, 1, 2, 3, 4, 5] * 2
    dataY_period1 = [10, 20, 30, 20, 10] + \
                    [20, 30, 40, 30, 20]
    dataY_period2 = [30, 40, 50, 40, 30] + \
                    [40, 50, 60, 50, 40]
    ws1 = CreateWorkspace(dataX, dataY_period1, NSpec=2)
    ws2 = CreateWorkspace(dataX, dataY_period2, NSpec=2)
    AddSampleLog(Workspace=ws1, LogName='goodfrm', LogText="1")
    AddSampleLog(Workspace=ws2, LogName='goodfrm', LogText="1")
    for i in range(2):
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

    output_workspace = MuonGroupingAsymmetry(InputWorkspace=multi_period_data,
                                                       GroupName="fwd",
                                                       Grouping=[1, 2],
                                                       SummedPeriods=[1, 2])

    # We have asked for periods 1+2, with each period summing detectors 1,2
    print("X values are : {}".format([round(float(i), 3) for i in output_workspace.readX(0)]))
    print("Y values are : {}".format([round(float(i), 3) for i in output_workspace.readY(0)]))
    

Output:

.. testoutput:: MultiPeriod

    X values are : [0.0, 1.0, 2.0, 3.0, 4.0, 5.0]
    Y values are : [-0.712, -0.364, 0.289, 0.581, 0.78]

.. categories::

.. sourcelink::