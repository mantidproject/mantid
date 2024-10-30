.. algorithm::

.. summary::

.. relatedalgorithms::

.. properties::

Description
-----------

The algorithm is essentially a wrapper around the :ref:`algm-MuonProcess` algorithm, summing a group of detector IDs, and optionally performing an asymmetry calculation. This algorithm only applies to a grouping of detectors (see ... for the equivalent algorithm on pairs of groups), it does not crop the data.

*ApplyMuonDetectorGrouping* takes an input workspace containing the data (with multiple detector IDs), groups it according to the optional user inputs, creates a unique name for the grouped data, and finally adds it to (or replaces it in) the supplied WorkspaceGroup. Multiple period data is supported through SummedPeriods and SubtractedPeriods. Various other optional behaviours are possible as detailed below.

Analysis
########

The algorithm supports two analysis types:

- *Counts* - The spectra associated to the given detector IDs via **Grouping** are summed (with errors added in quadrature), see :ref:`algm-MuonGroupDetectors`. Note that if several detector IDs are associated to the same spectra, the spectra will be counted **once for each ID**.
- *Asymmetry* - The summation from Counts is performed. Then the asymmetry between the given group and muon exponential decay is calculated (see :ref:`algm-EstimateMuonAsymmetryFromCounts`), between the times **TimeMin** and **TimeMax**.

The way in which period data is combined before the analysis is determined by
**SummedPeriods** and **SubtractedPeriods**.
For example, setting SummedPeriods to "1,2" and SubtractedPeriods to "3,4" would result in
the period spectra being combined in the form :math:`(1+2)-(3+4)`.

Note that the action of SummedPeriods and SubtractedPeriods is performed **before** the Counts/Asymmetry analysis.

The workspaces which hold the analysed data are stored in the **WorkspaceGroup**, using a unique name. From the following variables

#. The **InputWorkspaceGroup** name is "inputGroup"
#. **AnalysisType** is "Counts"
#. **GroupName** is "test"

then the name would be *"inputGroup; Group; test; Counts; #1"*. A second workspace is also created with the same name, but ending in "#1_Raw", the only difference being that the **RebinArgs** are not applied to the latter workspace. If period arithmetic is required, for example SummedPeriods is "1,2" then the name would be *"inputGroup; Group; test; Counts; 1+2; #1"*.

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
    Load(Filename='MUSR00015193.nxs',
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

   Total counts (rebinned) : 79970
   Total counts (no rebin) : 79970

   Time range (original) : -0.550 - 31.450 mus
   Time range (no rebin) : 0.106 - 31.450 mus
   Time range (rebinned) : 0.106 - 31.306 mus

   Time step (original)  : 0.016 mus
   Time step (no rebin)  : 0.016 mus
   Time step (rebinned)  : 0.200 mus


.. categories::

.. sourcelink::
