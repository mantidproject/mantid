============
Muon Changes
============

.. contents:: Table of Contents
   :local:
   
Interface
---------

New
###
- Elemental Analysis added to Muon Interfaces: Includes a selectable Periodic Table.
- Frequency Domain Analysis 2 added. See the documentation here :ref:`Frequency Domain Analysis <Frequency_Domain_Analysis_2-ref>`

Improvements
############
- Elemental Analysis added to Muon Interfaces: Includes a selectable Periodic Table. There are two versions (Elemental Analysis and Elemental Analysis Old) and the difference is the plotting used.
- TF Asymmetry mode now displays the chi squared value at the top of the browser.
- ALC interface now sorts the data into ascending order.
- Muon Analysis now includes number of event per frame and number of events per frame per detector in the run info box on the home tab.
- Frequency Domain Analysis now lets the user select the phase table in MaxEnt mode.
- CHRONUS now has a transverse and longitudanal default grouping table, the main field direction is read from the file to determine which to use.

Bugfixes
########
- Results table now includes all logs that are common to all of the loaded files.
- When turning TF Asymmetry mode off it no longer resets the global options.
- Results table will produce correct values for co-added runs.
- The x limits on the settings tab will now correct themselves if bad values are entered. 
- The `load current run` button now works for CHRONUS in muon analysis.

Algorithms
----------

New
###

- :ref:`ApplyMuonDetectorGrouping <algm-ApplyMuonDetectorGrouping>` added to allow scripting of the Muon Analysis GUI workflow. Applies grouping counts/asymmetry to muon data and stores the result in the ADS.
- :ref:`ApplyMuonDetectorGroupPairing <algm-ApplyMuonDetectorGroupPairing>` added to allow scripting of the Muon Analysis GUI workflow. Applies a group pairing asymmetry calculation to muon data and stores the result in the ADS.
- :ref:`LoadAndApplyMuonDetectorGrouping <algm-LoadAndApplyMuonDetectorGrouping>` added to allow scripting of the Muon Analysis GUI workflow. The grouping/pairing information is loaded from an XML format file, which can be produced through the muon analysis GUI via the 'Save Grouping' button. Replicates the `Load Grouping` button of the grouping tab, adds workspaces to the ADS.
- :ref:`LoadPSIMuonBin <algm-LoadPSIMuonBin>` added the ability to load a .bin file from the PSI facility in switzerland, as a workspace.

Improvements
############

Bugfixes
########

- :ref:`EstimateMuonAsymmetryFromCounts <algm-EstimateMuonAsymmetryFromCounts>` had a bug that meant the first good bin was excluded from calculating the normalization.


:ref:`Release 4.0.0 <v4.0.0>`
