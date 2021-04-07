============
MuSR Changes
============

.. contents:: Table of Contents
   :local:

Muon Analysis 2 and Frequency Domain Analysis
---------------------------------------------

New Features
############
- Differences of groups/pairs can be created on the grouping tab.


Improvements
############
- Both interfaces are now able to analyse Phasequads.
- **Both interfaces are now able to analyse multiple runs as one run by ticking co-add.**

.. figure:: /images/MA_co_add.png
   :class: screenshot
   :width: 750px
   :align: center

- Input validation has been added to First and Last good Data in the Phase Table tab.
- When using co-add and creating a phasequad, the deadtime table will now be taken from the first file only.
- If the start time of a fit is greater than the end time the interface will swap the two.
- Number of Periods has now been added to the end of Run Information.
- The tool tip for periods in the grouping table has been updated for more clarification.

Bug fixes
#########
- A bug has been fixed where ties and constraints were not being respected.
- A bug has been fixed where editing constraints resulted in a crash.
- A bug has been fixed where global parameter values would reset when changing the displayed dataset.
- A bug has been fixed where adding the DynamicKuboToyabe function on the fitting tab of Muon Analysis resulted in a
  crash.
- A bug has been fixed where switching to Simultaneous fitting when TF Asymmetry mode is ticked caused an error.
- A bug has been fixed where switching between the Run and Group/Pair selection when TF Asymmetry mode is ticked caused
  an error.
- A bug has been fixed that can sometimes cause the MaxEnt calculation to fail in frequency domain analysis.
- A bug has been fixed when trying to do a simultaneous fit with no data loaded after pressing clear all would cause a
  crash.
- A bug has been fixed where changing rebin wouldn't update the plot in the GUI.
- A bug has been fixed where the plot would update incorrectly when changing plot raw and plot difference.
- A bug has been fixed where pressing autoscale y without any data loaded would cause a crash.
- A bug has been fixed where autoscale y would not calculate the y limits correctly.

ALC
---

New Features
############
- The data loading section has been updated for this release. See :ref:`Muon ALC <MuonALC-ref>` for more.
- **The alpha in the ALC interface can now be set for single period data.**

.. figure:: /images/ALC_alpha.png
   :class: screenshot
   :width: 750px
   :align: center

Improvements
############
- An x label has been added to the plot in data loading.
- Imported data will now be loaded into all three pages on the ALC interface.

Bug fixes
##########
- Stopped scientific notation when plotting run numbers on x axis.
- A bug has been fixed where exported results were unintentionally being mixed together in their group workspaces.
- A bug has been fixed where the Load button had to be pressed twice after an initial batch of runs had already been loaded.
- A bug has been fixed where the log was being reset when loading runs after an initial batch of runs.
- A bug has been fixed where you could move startx and endx markers past the limits of the data in baseline modelling.

Elemental Analysis
------------------

New Features
############

- A new :ref:`XrayAbsorptionCorrection <algm-XrayAbsorptionCorrection>` algorithm has been added.

Bug fixes
#########

- A bug has been fixed in the Elemental Analysis GUI where minor peaks wouldn't be added to a new detector subplot.

Algorithms
----------
- A new :ref:`LoadElementalAnalysisData <algm-LoadElementalAnalysisData>` algorithm has been added for loading runs for
  the new Elemental Analysis GUI, enabling it to be registered by WorkspaceHistory.
- A new Property ``Alpha`` has been added to :ref:`PlotAsymmetryByLogValue <algm-PlotAsymmetryByLogValue>` to set the
  balance parameter, default to 1.0.
- The algorithms :ref:`RemoveExpDecay <algm-RemoveExpDecay>` and
  :ref:`EstimateMuonAsymmetryFromCounts <algm-EstimateMuonAsymmetryFromCounts>` have been modified to use point data
  instead of bin edges for removing the exponential.
- :ref:`LoadPSIMuonBin <algm-LoadPSIMuonBin>` and :ref:`LoadMuonNexusV2 <algm-LoadMuonNexusV2>` can now return a table
  of time zeros.
- :ref:`MuonPreProcess <algm-MuonPreProcess>` has a new input ``TimeZeroTable`` which requires a TableWorkspace of time
  zero values.

Fit Functions
-------------

- The conversion factor for field in :ref:`StandardSC <func-StandardSC>` has been fixed.

:ref:`Release 6.0.0 <v6.0.0>`
