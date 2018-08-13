============
MuSR Changes
============

.. contents:: Table of Contents
   :local:
   
Interface
---------

Improvements
############
- Elemental Analysis added to Muon Interfaces: Includes a selectable Periodic Table.
- TF Asymmetry mode now displays the chi squared value at the top of the browser.

Bugfixes
########
- Results table now includes all logs that are common to all of the loaded files.
- When turning TF Asymmetry mode off it no longer resets the global options.

Algorithms
----------

New
###

- :ref:`ApplyMuonDetectorGrouping <algm-ApplyMuonDetectorGrouping>` added to allow scripting of the Muon Analysis GUI workflow. Applies grouping counts/asymmetry to muon data and stores the result in the ADS.
- :ref:`ApplyMuonDetectorGroupPairing <algm-ApplyMuonDetectorGroupPairing>` added to allow scripting of the Muon Analysis GUI workflow. Applies a group pairing asymmetry calculation to muon data and stores the result in the ADS.
- :ref:`LoadAndApplyMuonDetectorGrouping <algm-LoadAndApplyMuonDetectorGrouping>` added to allow scripting of the Muon Analysis GUI workflow. The grouping/pairing information is loaded from an XML format file, which can be produced through the muon analysis GUI via the 'Save Grouping' button. Replicates the `Load Grouping` button of the grouping tab, adds workspaces to the ADS.

Improvements
############

Bugfixes
########

:ref:`Release 3.14.0 <v3.14.0>`
