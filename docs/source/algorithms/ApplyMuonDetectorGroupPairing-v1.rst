.. algorithm::

.. summary::

.. relatedalgorithms::

.. properties::

Description
-----------

The algorithm is essentially a wrapper around the :ref:`algm-MuonProcess` algorithm, performing an asymmetry calculation on a pair of groups of detectors. These groups may be specified manually, allowing various processing options such as rebinning and dead time correction, or alternatively the algorithm accepts two workspaces which are assumed to contain the processed groups as a single spectra.

The result of the calculation are stored in the provided **WorkspaceGroup** using a unique name.

Analysis
########

This algorithm performs the *PairAsymmetry* option of the :ref:`algm-MuonProcess` algorithm, requiring two items of data; spectra corresponding to the groups of detector IDs. This can be given directly to the algorithm via the **InputWorkspace1** and **InputWorkspace2** properties, which accept MatrixWorkspaces with single spectra and identical binning. The pair asymmetry is calculated between the times **TimeMin** and **TimeMax**, and using the **Alpha** parameter (see :ref:`algm-CalculateMuonAsymmetry` for details).

Alternatively by checking setting **SpecifyGroupsManually** to true the groupings can be performed as part of the algorithm (requiring the full, ungrouped, data in **InputWorkspace**).

In this case **Group1** and **Group2** are separator or range based lists (e.g. "1,2,3-6,7") of detector IDs.

Rebbing is optional and can be achieved through the **Rebin** property (using syntax as in :ref:`algm-Rebin`).

The way in which multi period data is combined before the analysis is determined by
**SummedPeriods** and **SubtractedPeriods**.
For example, setting SummedPeriods to "1,2" and SubtractedPeriods to "3,4" would result in
the period spectra being combined in the form :math:`(1+2)-(3+4)`.

Note that the action of SummedPeriods and SubtractedPeriods is performed **before** the asymmetry analysis.

The workspaces which hold the analysed data are stored in the **WorkspaceGroup**, using a unique name. As an example, from the following variables :

#. The **InputWorkspaceGroup** name is "inputGroup"
#. **PairName** is "test"
#. **SummedPeriods** is "1,2"
#. **SubtractedPeriods** is "3,4"

then the name would be *"inputGroup; Pair; test; Asym; 1+2-3+4; #1"*. A second workspace is also created with the same name, but ending in "#1_Raw", the only difference being that the **RebinArgs** are not applied to the latter workspace. For single period data the name would instead be *"inputGroup; Pair; test; Asym; #1"*.

An option exists to apply dead time correction to the counts if **ApplyDeadTimeCorrection** is true, requiring a **DeadTimeTable** with the format as in :ref:`algm-ApplyDeadTimeCorr`.

The **TimeOffset** parameter shifts the time axis to start at the given time (all bins are offset by the value). In :ref:`algm-MuonProcess` this is equivalent to *LoadedTimeZero* - *TimeZero*. Note that the **TimeMin** and **TimeMax** refer to the values before the offset is applied.

Usage
-----

.. include:: ../usagedata-note.txt

.. note::

   For examples of applying custom dead times, please refer to :ref:`algm-ApplyDeadTimeCorr`
   documentation.

   For examples of applying custom grouping, please refer to :ref:`algm-MuonGroupDetectors`
   documentation.

   For examples of applying custom rebinning, please refer to :ref:`algm-Rebin` documentation.

**Example - Pair asymmetry for MUSR run:**

.. testcode:: ExMUSRPairAsymmetry

    # Clear the ADS before starting
    AnalysisDataService.clear()

    # Create the workspace group in which the analysed workspaces will be placed
    ws = CreateSampleWorkspace()
    wsGroup = GroupWorkspaces("ws")
    RenameWorkspace(
                      InputWorkspace="wsGroup",
                      OutputWorkspace='MUSR00015193',
                      OverwriteExisting=True)

    # Load the data
    LoadMuonNexus(  Filename='MUSR00015193.nxs',
                    OutputWorkspace='MuonAnalysis')

    # Create two detector groupings
    ApplyMuonDetectorGrouping(
        InputWorkspace='MuonAnalysis',
        InputWorkspaceGroup='MUSR00015193',
        GroupName='fwd',
        Grouping='1-32'
        )
    ApplyMuonDetectorGrouping(
        InputWorkspace='MuonAnalysis',
        InputWorkspaceGroup='MUSR00015193',
        GroupName='bwd',
        Grouping='33-64'
        )
    # Add the groupings to the group workspace
    wsGroup.add('MUSR00015193; Group; fwd; Counts; #1_Raw')
    wsGroup.add('MUSR00015193; Group; bwd; Counts; #1_Raw')

    # Apply the pairing algorithm to the two groups
    ApplyMuonDetectorGroupPairing(
           InputWorkspaceGroup='MUSR00015193',
           PairName='pairTest',
           Alpha=1.0,
           SpecifyGroupsManually=False,
           InputWorkspace1='MUSR00015193; Group; fwd; Counts; #1_Raw',
           InputWorkspace2='MUSR00015193; Group; bwd; Counts; #1_Raw'
           )

    output = mtd['MUSR00015193; Pair; pairTest; Asym; #1_Raw']


    print('{:.8f}'.format(output.readY(0).mean()))

Output:

.. testoutput:: ExMUSRPairAsymmetry

   -0.01419801

.. categories::

.. sourcelink::
