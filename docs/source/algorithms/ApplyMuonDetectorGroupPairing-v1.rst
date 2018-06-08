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

This algorithm performs the *PairAsymmetry* option of the :ref:`algm-MuonProcess` algorithm, requiring two items of data; spectra corresponding to the groups of detector IDs. This can be given directly to the algorithm via the **InputWorkspace1** and **InputWorkspace2** properties, which accept MatrixWorkspaces with single spectra and identical binning. The pair asymmetry is calculated between the times **TimeMin** and **TimeMax**, and using the **Alpha** parameter (see :ref:`alg-MuonAsymmetry` for details).

Alternatively by checking setting **SpecifyGroupsManually** to true the groupings can be performed as part of the algorithm (requiring the full, ungrouped, data in **InputWorkspace**). 

In this case **Group1** and **Group2** are separator or range based lists (e.g. "1,2,3-6,7") of detector IDs. 

Rebbing is optional and can be achieved through the **Rebin** property (using syntax as in :ref:`alg-Rebin`).

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

**Example - Counts for EMU run with time offset and rebinning:**

.. testcode:: ExCountsOffsetAndRebin

    # Clear the ADS before starting
    AnalysisDataService.clear()

    # Create the workspace group in which the analysed workspaces will be placed
    ws = CreateSampleWorkspace()
    wsGroup = GroupWorkspaces("ws")
    RenameWorkspace(  InputWorkspace="wsGroup", 
                      OutputWorkspace='MUSR00015193', 
                      OverwriteExisting=True)

    # Load the data
    LoadMuonNexus(  Filename='MUSR00015193.nxs', 
                    OutputWorkspace='MuonAnalysis')

    ApplyMuonDetectorGrouping(
                    InputWorkspace='MuonAnalysis', 
                    InputWorkspaceGroup='MUSR00015193', 
                    GroupName='Test', 
                    Grouping='1,2,3,5-10', 
                    TimeOffset=0.0,
                    RebinArgs = "0.2")


    output_rebin = mtd['MUSR00015193; Group; Test; Counts; #1']
    print("Total counts (rebinned) : {0:.0f}".format( sum(output_rebin.readY(0))) )

    output_noRebin = mtd['MUSR00015193; Group; Test; Counts; #1_Raw']
    print("Total counts (no rebin) : {0:.0f}\n".format(sum(output_noRebin.readY(0))) )


    print("Time range (original) : {0:.3f} - {1:.3f} mus".format(mtd['MuonAnalysis_1'].readX(0)[0],mtd['MuonAnalysis_1'].readX(0)[-1]))
    print("Time range (no rebin) : {0:.3f} - {1:.3f} mus".format(output_noRebin.readX(0)[0],output_noRebin.readX(0)[-1]))
    print("Time range (rebinned) : {0:.3f} - {1:.3f} mus\n".format(output_rebin.readX(0)[0],output_rebin.readX(0)[-1]))

    print("Time step (original)  : {0:.3f} mus".format(mtd['MuonAnalysis_1'].readX(0)[1]-mtd['MuonAnalysis_1'].readX(0)[0]))
    print("Time step (no rebin)  : {0:.3f} mus".format(output_noRebin.readX(0)[1]-output_noRebin.readX(0)[0]))
    print("Time step (rebinned)  : {0:.3f} mus".format(output_rebin.readX(0)[1]-output_rebin.readX(0)[0]))

Output:

.. testoutput:: ExCountsOffsetAndRebin

   Total counts (rebinned) : 84438
   Total counts (no rebin) : 84438

   Time range (original) : -0.550 - 31.450 mus
   Time range (no rebin) : -0.550 - 31.450 mus
   Time range (rebinned) : -0.550 - 31.450 mus

   Time step (original)  : 0.016 mus
   Time step (no rebin)  : 0.016 mus
   Time step (rebinned)  : 0.200 mus


.. categories::

.. sourcelink::