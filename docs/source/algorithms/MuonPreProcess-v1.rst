.. algorithm::

.. summary::

.. relatedalgorithms::

.. properties::

Description
-----------

When interacting with the :ref:`Muon_Analysis-ref` interface, operations such as detector grouping, group and pair asymmetry are performed on data. As part of the workflow, several "pre-processing" steps are also necessary; such as rebinning and cropping the data, applying dead time corrections and applying time zero corrections/shifting the time axis by a fixed amount (sometimes referred to as "first good data").

This algorithm intends to capture the common pre-processing steps, which are not muon physics specific concepts, and apply them all at once to produce data which is ready for the muon-specific operations. This way, scripting the workflows of the :ref:`Muon_Analysis-ref` interface is much more intuitive and simple.

Analysis
########

As indicated in the input variable names to this algorithm; there are five distinct operations applied to the input data. In order of application these are;

#. Apply dead time correction through the **DeadTimeTable** input.
#. Apply time zero correction through the **TimeZeroTable** input.
#. Apply a time offset to the time axis through the **TimeOffset** input, which may be positive or negative. This time offset is directly added to all times within the workspace.
#. Apply a cropping to the upper (lower) ends of the time axis via the **TimeMax** (**TimeMin**) input.
#. Finally, apply a rebinning via **RebinArgs**, using the same syntax as in :ref:`algm-Rebin`.

The **InputWorkspace** can be a single workspace object, and multi period data is supported by supplying a *WorkspaceGroup* as the input workspace, where each workspace within the group represents a single period.

The **OutputWorkspace** is always a *Workspace Group*; for single period data the group only contains a single workspace. The reason for this is so that the muon algorithms MuonGroupingCounts, MuonGroupingAsymmetry and MuonPairingAsymmetry can expect a consistent input between single and multi period data, and where single period data is just a limiting case of multi period data.

Four of the operations listed above correspond to a run of :ref:`algm-ApplyDeadTimeCorr`, :ref:`algm-ChangeBinOffset`, :ref:`algm-CropWorkspace` and :ref:`algm-Rebin` respectively; and so the documentation of these algorithms can be consulted for more detailed discussion of precisely how the operations are applied. Time zero correction does not correspond to it's own algorithm.

As already mentioned; the output of this algorithm can (and is intended to be) fed into one of the following algorithms;

#. MuonGroupingCounts
#. MuonGroupingAsymmetry
#. MuonPairingAsymmetry


Usage
-----

.. include:: ../usagedata-note.txt

.. note::

   For examples of applying custom dead times, please refer to :ref:`algm-ApplyDeadTimeCorr`
   documentation.

   For examples of applying custom rebinning, please refer to :ref:`algm-Rebin` documentation.

**Example - The output is always a WorkspaceGroup**

.. testcode:: ConvertToGroup

    # Single period data
    dataX = [0, 1, 2, 3, 4, 5]
    dataY = [10, 20, 30, 20, 10]
    input_workspace = CreateWorkspace(dataX, dataY)

    output_workspace = MuonPreProcess(InputWorkspace=input_workspace)

    print("Input workspace is a {}".format(input_workspace.id()))
    print("Output workspace is a {}".format(output_workspace.id()))
    print("X values are : {}".format(output_workspace[0].readX(0)))
    print("Y values are : {}".format(output_workspace[0].readY(0)))


Output:

.. testoutput:: ConvertToGroup

    Input workspace is a Workspace2D
    Output workspace is a WorkspaceGroup
    X values are : [0. 1. 2. 3. 4. 5.]
    Y values are : [10. 20. 30. 20. 10.]

**Example - Applying only a time offset**

.. testcode:: ExampleTimeOffset

    dataX = [0, 1, 2, 3, 4, 5]
    dataY = [10, 20, 30, 20, 10]
    input_workspace = CreateWorkspace(dataX, dataY)

    output_workspace = MuonPreProcess(InputWorkspace=input_workspace,
                                                TimeOffset=0.5)

    print("X values are : {}".format(output_workspace[0].readX(0)))
    print("Y values are : {}".format(output_workspace[0].readY(0)))


Output:

.. testoutput:: ExampleTimeOffset

    X values are : [0.5 1.5 2.5 3.5 4.5 5.5]
    Y values are : [10. 20. 30. 20. 10.]

**Example - Applying only a rebin**

.. testcode:: ExampleRebin

    dataX = [0, 1, 2, 3, 4, 5]
    dataY = [10, 20, 30, 20, 10]
    input_workspace = CreateWorkspace(dataX, dataY)

    output_workspace = MuonPreProcess(InputWorkspace=input_workspace,
                                                RebinArgs=2)

    print("X values are : {}".format(output_workspace[0].readX(0)))
    print("Y values are : {}".format(output_workspace[0].readY(0)))


Output:

.. testoutput:: ExampleRebin

    X values are : [0. 2. 4. 5.]
    Y values are : [30. 50. 10.]

**Example - Applying only a crop**

.. testcode:: ExampleCrop

    dataX = [0, 1, 2, 3, 4, 5]
    dataY = [10, 20, 30, 20, 10]
    input_workspace = CreateWorkspace(dataX, dataY)

    output_workspace = MuonPreProcess(InputWorkspace=input_workspace,
                                                TimeMin=2,
                                                TimeMax=4)

    print("X values are : {}".format(output_workspace[0].readX(0)))
    print("Y values are : {}".format(output_workspace[0].readY(0)))


Output:

.. testoutput:: ExampleCrop

    X values are : [2. 3. 4.]
    Y values are : [30. 20.]

**Example - Applying only a dead time correction**

.. testcode:: ExampleDeadTime

    dataX = [0, 1, 2, 3, 4, 5] * 4
    dataY = [100, 200, 300, 200, 100] * 4
    input_workspace = CreateWorkspace(dataX, dataY, NSpec=4)
    # dead time correction requires the number of good frames to be set
    AddSampleLog(Workspace=input_workspace, LogName="goodfrm", LogText="1")

    # create the dead time table
    dead_times = CreateEmptyTableWorkspace()
    dead_times.addColumn("int", "spectrum", 0)
    dead_times.addColumn("float", "dead-time", 0)
    [dead_times.addRow([i + 1, 0.5]) for i in range(4)]

    output_workspace = MuonPreProcess(InputWorkspace=input_workspace,
                                                DeadTimeTable=dead_times)
    print("X values are : {}".format(output_workspace[0].readX(0)))
    print("Y values are : {}".format(output_workspace[0].readY(0).round(1)))

Output:

.. testoutput:: ExampleDeadTime

    X values are : [0. 1. 2. 3. 4. 5.]
    Y values are : [100.3 201.2 302.8 201.2 100.3]

**Example - Applying only a time zero correction**

.. testcode:: ExampleTimeZero

    dataX = [0, 1, 2, 3, 4, 5] * 4
    dataY = [100, 200, 300, 200, 100] * 4
    input_workspace = CreateWorkspace(dataX, dataY, NSpec=4)

    # Create a time zero table
    time_zero_table = CreateEmptyTableWorkspace()
    time_zero_table.addColumn("double", "time zero")
    [time_zero_table.addRow([i+2]) for i in range(4)]

    output_workspace = MuonPreProcess(InputWorkspace=input_workspace,
                                      TimeZeroTable=time_zero_table)

    print("X values are : [{:.0f}, {:.0f}, {:.0f}, {:.0f}, {:.0f}, {:.0f}]".format(
        output_workspace[0].readX(0)[0],output_workspace[0].readX(0)[1],
        output_workspace[0].readX(0)[2],output_workspace[0].readX(0)[3],
        output_workspace[0].readX(0)[4],output_workspace[0].readX(0)[5]))
    print("Y values are : [{:.0f}, {:.0f}, {:.0f}, {:.0f}, {:.0f}]".format(
        output_workspace[0].readY(0)[0],output_workspace[0].readY(0)[1],
        output_workspace[0].readY(0)[2],output_workspace[0].readY(0)[3],
        output_workspace[0].readY(0)[4]))

Output:

.. testoutput:: ExampleTimeZero

    X values are : [-2, -1, 0, 1, 2, 3]
    Y values are : [100, 200, 300, 200, 100]

.. categories::

.. sourcelink::
