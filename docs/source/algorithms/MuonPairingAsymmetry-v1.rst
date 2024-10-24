.. algorithm::

.. summary::

.. relatedalgorithms::

.. properties::

Description
-----------

When interacting with the :ref:`Muon_Analysis-ref` interface, operations such as detector grouping, group and pair asymmetry are performed on data. This algorithm performs a "pair asymmetry" operation, in other words it takes two groups (from :ref:`algm-MuonGroupingCounts` for example) and calculates the asymmetry between them using the common formula from :ref:`algm-AsymmetryCalc`.

This algorithm is part of a set of four; with :ref:`algm-MuonPreProcess` being run first; and the output being fed into this one. Alternatively, the groups may be calculated by using :ref:`algm-MuonPreProcess` followed by :ref:`algm-MuonGroupingCounts` and fed into this algorithm by switching off **SpecifyGroupsManually**. This allows the replication of the workflow used by the muon analysis interface to produce group data.

Analysis
########

A *Pair* in this context refers to a pair of groups. A workspace has one or more *spectra* contained within it; each spectra has a unique detector ID. Assuming the y-values represent counts; a *detector group* is a sum over a given set of detectors.

With a pair, one may define an asymmetry operation as in :ref:`algm-AsymmetryCalc`;

.. math:: A = \frac{F-\alpha B}{F+\alpha B},

where :math:`F` and :math:`B` are the forward and backwards groups and alpha is the balance parameter.

The pair must be given a name via **PairName** which can consist of letters, numbers and underscores.

#. Valid names : "pair", "pair2", "pair_2", "1234"
#. Invalid names : "", "pair!", "pair "

The pair name does not affect the data; however the name is used in the muon interface when automatically generating workspace names from group data.

Additionally, a value for **alpha** must be supplied, and which must be non-negative.

There are two options for supplying the group data :

#. If **SpecifyGroupsManually** is checked, then the detector ID's that define the two groups must be given (they must be unique); and an **InputWorkspace** supplied. This workspace must be a :ref:`WorkspaceGroup <WorkspaceGroup>` to match the output of :ref:`algm-MuonPreProcess`, where the group contains one *MatrixWorkspace* for each period.

#. If **SpecifyGroupsManually** is not checked, then two workspaces must be supplied which represent the two groups via **InputWorkspace1** and **InputWorkspace2**. These may be *MatrixWorkspace*s (in the case of single period data); or *WorkspaceGroup*s (in the case of multi period data, the two groups must contain the same number of workspaces). Any *MatrixWorkspace* must only contain one spectra (the group counts).


Multi period data
#################

Both single and multi period data are supported by the algorithm.

The **SummedPeriods** and **SubtractedPeriods** inputs are used to control the way that periods are combined. so for example;

#. SummedPeriods = 1,2
#. SubtractedPeriods = 3,4

would combine periods in the combination :math:`(1+2)-(3+4)`.

Usage
-----

**Example - Using MuonPreProcess and Specifying Groups Manually for Single Period Data**

.. testcode:: SpecifyGroupsManuallySinglePeriod

    # Create a workspaces with four spectra
    dataX = [0, 1, 2, 3, 4, 5] * 4
    dataY = [10, 20, 30, 20, 10] + \
            [20, 30, 40, 30, 20] + \
            [30, 40, 50, 40, 30] + \
            [40, 50, 60, 50, 40]
    input_workspace = CreateWorkspace(dataX, dataY, NSpec=4)
    for i in range(4):
        # set detector IDs to be 1,2,3,4
        # these do not have to be the same as the spectrum numbers
        # (the spectrum number are 0,1,2,3 in this case)
        input_workspace.getSpectrum(i).setDetectorID(i + 1)

    pre_processed_workspace = MuonPreProcess(InputWorkspace=input_workspace)

    output_workspace = MuonPairingAsymmetry(InputWorkspace=pre_processed_workspace,
                                                      PairName="myPair",
                                                      Alpha=1.0,
                                                      SpecifyGroupsManually=True,
                                                      Group1=[1, 2],
                                                      Group2=[3, 4])

    print("X values are : {}".format([round(float(i), 3) for i in output_workspace.readX(0)]))
    print("Y values are : {}".format([round(float(i), 3) for i in output_workspace.readY(0)]))


Output:

.. testoutput:: SpecifyGroupsManuallySinglePeriod

    X values are : [0.5, 1.5, 2.5, 3.5, 4.5]
    Y values are : [-0.4, -0.286, -0.222, -0.286, -0.4]

**Example - Using MuonPreProcess and Specifying Groups Manually for Multi Period Data**

.. testcode:: SpecifyGroupsManuallyMultiPeriod

    # Create a workspaces with four spectra
    dataX = [0, 1, 2, 3, 4, 5] * 4
    dataY = [10, 20, 30, 20, 10] + \
            [20, 30, 40, 30, 20] + \
            [30, 40, 50, 40, 30] + \
            [40, 50, 60, 50, 40]

    input_workspace = CreateWorkspace(dataX, dataY, NSpec=4)
    input_workspace_1 = CreateWorkspace(dataX, dataY, NSpec=4)
    for i in range(4):
        # set detector IDs to be 1,2,3,4
        # these do not have to be the same as the spectrum numbers
        # (the spectrum number are 0,1,2,3 in this case)
        input_workspace.getSpectrum(i).setDetectorID(i + 1)
        input_workspace_1.getSpectrum(i).setDetectorID(i + 1)

    # Create multi period data
    multi_period_data = GroupWorkspaces(input_workspace)
    multi_period_data.addWorkspace(input_workspace_1)

    pre_processed_workspace = MuonPreProcess(InputWorkspace=multi_period_data)

    output_workspace = MuonPairingAsymmetry(InputWorkspace=pre_processed_workspace,
                                                      PairName="myPair",
                                                      Alpha=1.0,
                                                      SpecifyGroupsManually=True,
                                                      Group1=[1, 2],
                                                      Group2=[3, 4],
                                                      SummedPeriods=[1, 2])

    print("X values are : {}".format([round(float(i), 3) for i in output_workspace.readX(0)]))
    print("Y values are : {}".format([round(float(i), 3) for i in output_workspace.readY(0)]))


Output:

.. testoutput:: SpecifyGroupsManuallyMultiPeriod

    X values are : [0.5, 1.5, 2.5, 3.5, 4.5]
    Y values are : [-0.4, -0.286, -0.222, -0.286, -0.4]

**Example - Using MuonPreProcess, MuonGroupingCounts for Single Period Data**

.. testcode:: SinglePeriod

    # Create a workspaces with four spectra
    dataX = [0, 1, 2, 3, 4, 5] * 4
    dataY = [10, 20, 30, 20, 10] + \
            [20, 30, 40, 30, 20] + \
            [30, 40, 50, 40, 30] + \
            [40, 50, 60, 50, 40]
    input_workspace = CreateWorkspace(dataX, dataY, NSpec=4)
    for i in range(4):
        # set detector IDs to be 1,2,3,4
        # these do not have to be the same as the spectrum numbers
        # (the spectrum number are 0,1,2,3 in this case)
        input_workspace.getSpectrum(i).setDetectorID(i + 1)

    pre_processed_workspace = MuonPreProcess(InputWorkspace=input_workspace)
    group_workspace_1 = MuonGroupingCounts(InputWorkspace=pre_processed_workspace,
                                                     GroupName="fwd",
                                                     Grouping=[1, 2])
    group_workspace_2 = MuonGroupingCounts(InputWorkspace=pre_processed_workspace,
                                                     GroupName="bwd",
                                                     Grouping=[3, 4])

    output_workspace = MuonPairingAsymmetry(InputWorkspace=pre_processed_workspace,
                                                      PairName="myPair",
                                                      Alpha=1.0,
                                                      SpecifyGroupsManually=False,
                                                      InputWorkspace1=group_workspace_1,
                                                      InputWorkspace2=group_workspace_2)

    print("X values are : {}".format([round(float(i), 3) for i in output_workspace.readX(0)]))
    print("Y values are : {}".format([round(float(i), 3) for i in output_workspace.readY(0)]))


Output:

.. testoutput:: SinglePeriod

    X values are : [0.5, 1.5, 2.5, 3.5, 4.5]
    Y values are : [-0.4, -0.286, -0.222, -0.286, -0.4]

.. categories::

.. sourcelink::
