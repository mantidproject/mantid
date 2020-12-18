============
MuSR Changes
============

.. contents:: Table of Contents
   :local:

.. warning:: **Developers:** Sort changes under appropriate heading
    putting new features at the top of the section, followed by
    improvements, followed by bug fixes.

Muon Analysis 2 and Frequency Domain Analysis
---------------------------------------------

New Features
############

Improvements
############
- Phasequads are now available for analysis in both GUI's.
- Ticking co-add now works in the GUI's so you can analyse multiple runs as one run

Bug fixes
#########
- Fixed a bug where ties and constraints were not being respected.
- Fixed a bug to swap start and end time fit properties on the interface if start > end
- Fixed a bug where editing constraints would result in a crash.
- Fixed a bug where global parameter values would reset when changing the displayed dataset.
- Fixed a crash when adding the DynamicKuboToyabe function on the fitting tab of Muon Analysis.
- Fixed an error caused by switching to Simultaneous fitting when TF Asymmetry mode is ticked.
- Fixed an error caused by switching between the Run and Group/Pair selection when TF Asymmetry mode is ticked.
- Fixed a crash when trying to do a simultaneous fit with no data loaded after pressing clear all.

ALC
---

New Features
############
- The data loading section has been updated for this release. See :ref:`Muon ALC <MuonALC-ref>` for more.
- Can now set alpha in the ALC interface for single period data

Improvements
############
- Added an x label to the plot in data loading

Bug fixes
##########
- Stopped scientific notation when plotting run numbers on x axis

Elemental Analysis
------------------

New Features
############

Bug fixes
#########

Algorithms
----------
- :ref:`algm-LoadElementalAnalysisData` algorithm was introduced for loading runs for the new Elemental Analysis GUI, enabling it to be registered by WorkspaceHistory.
- :ref:`algm-PlotAsymmetryByLogValue` has a new property Alpha to set the balance parameter, default to 1.0  
- The functions RemoveExpDecay and EstimateMuonAsymmetryFromCounts were modified to use point data instead of bin edges for removing the exponential.
- LoadPSIMuonBin and LoadMuonNexusV2 can now return a table of time zeros
- MuonPreProcess has a new input 'TimeZeroTable' which requires a TableWorkspace of time zero values

:ref:`Release 6.0.0 <v6.0.0>`
