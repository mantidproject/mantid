============
MuSR Changes
============

.. contents:: Table of Contents
   :local:

Muon Analysis
-------------

New Features
############

- The Model Fitting tab allows you to perform fits across the sample logs and fit parameters stored in your results table.

Improvements
############

- When running the Dynamic Kubo Toyabe fit function you should now be able to see the BinWidth to 3 decimal places.

BugFixes
############
- A bug has been fixed in the BinWidth for the Dynamic Kobu Toyabe Fitting Function which caused a crash and did not provide
  any information about why the value was invalid. Will now revert to last viable BinWidth used and explain why.
- The autoscale option when `All` is selected will now show the largest and smallest y value from the all of the plots.

Muon Analysis and Frequency Domain Analysis
-------------------------------------------

New Features
############

- It is now possible to Exclude a range from a fit range when doing a fit on the Fitting tab.

Improvements
############

- It is now possible to do a vertical resize of the plot in Muon Analysis and Frequency Domain Analysis.
- The plotting has been updated for better stability.
- The plotting now has autoscale active by default.
- Added a table to store phasequads in the phase tab, phasequads also no longer automatically delete themselves
  when new data is loaded

ALC
---

New Features
############

- Added an external plot button to the ALC interface which will plot in workbench the current tab's plot
- Added a period info button to the ALC interface which displays a table of period information from the loaded runs
  (this is equivalent to the periods button in the Muon Analysis and Frequency Domain Analysis Interfaces)

Elemental Analysis
------------------

Improvements
############
- Updated :ref:`LoadElementalAnalysisData <algm-LoadElementalAnalysisData>` algorithm to include Poisson errors for the counts data.

Algorithms
##########

- Updated :ref:`LoadMuonLog <algm-LoadMuonLog>` to read units for most log values.
- It is now possible to exclude a fit range when executing the :ref:`CalculateMuonAsymmetry <algm-CalculateMuonAsymmetry>` algorithm.

:ref:`Release 6.2.0 <v6.2.0>`
