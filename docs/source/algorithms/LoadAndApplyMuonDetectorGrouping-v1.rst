.. algorithm::

.. summary::

.. relatedalgorithms::

.. properties::

Description
-----------

When interacting with the :ref:`Muon_Analysis-ref` GUI, the contents of the grouping and pairing table can be saved to an XML format *'grouping file'*. This algorithms loads the grouping file and applies all the grouping/asymmetry analysis stored in the file, saving the resulting workspaces with the same naming format as the Muon Analysis GUI.

The analysis is performed via the :ref:`algm-ApplyMuonDetectorGrouping` and :ref:`algm-ApplyMuonDetectorGroupPairing` algorithms, which handle the grouping (counts or group-asymmetry, see :ref:`algm-EstimateMuonAsymmetryFromCounts`) and pairing (pair-asymmetry, see :ref:`algm-CalculateMuonAsymmetry`) respectively.

Analysis
########

All the grouping information (the groupings of detector IDs, and the pairs of groups) is loaded from the XML file specified by the **Filename** property. This information specifies how to perform grouping/pairing analysis on the contents of **InputWorkspace**, which are saved to **WorkspaceGroup**.

If WorkspaceGroup is not specified then the default format is "<INSTRUMENT><RUN NUMBER>" where for example if the data is from the "EMU" instrument for run number "12345" the name would be "EMU00012345", with the run numbers padded by zero up to 8 digits. If the instrument is not recognized (e.g. "LHC") run numbers below 100 are padded to three digits with zeros, so for run number 10 this would be "LHC010".


Rebinning is optional and can be achieved through the **RebinArgs** property (using syntax as in :ref:`algm-Rebin`). Each time a group or pair is analysed, two workspaces are added to WorkspaceGroup; the one ending in "_Raw" has no rebinning applied. If the RebinArgs property is not set then these workspaces will simply be duplicates of each other.


The entire time axis can be shifted by a given amount using the **TimeOffset** parameter.


An option exists to apply dead time correction to the y-values of some or all of the spectra in the InputWorkspace, before carrying out the asymmetry analysis. This requires a **DeadTimeTable** with the format as in :ref:`algm-ApplyDeadTimeCorr`.


Multiple period data is supported, allowing both +/- operations to be applied to periods through the **SummedPeriods** and **SubtractedPeriods** properties, which are comma-separated lists of periods to combine (numbering starts from 1). Note that summation is applied *before* asymmetry operations and subtraction is applied *after* asymmetry operations.

For example, setting SummedPeriods to "1,2" and SubtractedPeriods to "3,4" would result in the period spectra being combined in the form :math:`(1+2)-(3+4)`. To see how this would be done, we will assume two groups "group1" groups detectors "6-10" and "group2" groups detectors "11-15", then "pair1" is the asymmetry between these two groups. For the pair asymmetry calculation this would involve the following steps :

#. The group counts are produced according to the grouping information, so the count spectra for IDs 6-10 (11-15) are summed and become "group1" ("group2"). Each period has its own summation applied.
#. For both groups the period sum "1+2" and "3+4" is done.
#. The asymmetry between group1/group2 is calculated twice (one for the "1+2"/"3+4" period combinations)
#. These asymmetries are subtracted to give the final result.

The workspaces which hold the analysed data are stored in the **WorkspaceGroup**, using a unique name (see :ref:`algm-ApplyMuonDetectorGrouping` and :ref:`algm-ApplyMuonDetectorGroupPairing` for details).

The algorithm will work with XML files as output by the *Save Grouping* button in the *Grouping Options* tab of the current :ref:`Muon_Analysis-ref` GUI.

Usage
-----

.. include:: ../usagedata-note.txt

.. note::

   For examples of applying custom dead times, please refer to :ref:`algm-ApplyDeadTimeCorr`
   documentation.

   For examples of applying custom grouping, please refer to :ref:`algm-MuonGroupDetectors`
   documentation.

   For examples of applying custom rebinning, please refer to :ref:`algm-Rebin` documentation.

**Example - Load and Apply MUSR run:**

.. testcode:: ExLoadAndApplyMUSR

	# Clear the ADS before starting
	AnalysisDataService.clear()

	# Create the workspace group in which the analysed workspaces will be placed
	ws = CreateSampleWorkspace()
	wsGroup = GroupWorkspaces("ws")
	RenameWorkspace(
					  InputWorkspace="wsGroup",
					  OutputWorkspace='MUSR00015189',
					  OverwriteExisting=True)

	# Load the data
	Load(Filename='MUSR00015189.nxs',
		 OutputWorkspace='MuonAnalysis')

	LoadAndApplyMuonDetectorGrouping(
		Filename='MUSRGrouping.xml',
		InputWorkspace='MuonAnalysis',
		WorkspaceGroup='MUSR00015189',
		ApplyAsymmetryToGroups=False)


	MUSR_asym = mtd["MUSR00015189; Pair; long; Asym; #1"]


	print([f'{y:.2f}' for y in MUSR_asym.readY(0)[1:10]])

Output:

.. testoutput:: ExLoadAndApplyMUSR

   ['0.00', '-1.00', '0.00', '0.33', '0.31', '0.27', '0.37', '0.32', '0.21']

.. categories::

.. sourcelink::
