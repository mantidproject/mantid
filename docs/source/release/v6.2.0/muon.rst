============
MuSR Changes
============

.. contents:: Table of Contents
   :local:

Muon Analysis 2
---------------------------------------------

BugFixes
############
- A bug has been fixed in the BinWidth for the Dynamic Kobu Toyabe Fitting Function which caused a crash and did not provide any information about why the value was invalid. Will now revert to last viable BinWidth used and explain why.


Muon Analysis 2 and Frequency Domain Analysis
---------------------------------------------

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

Elemental Analysis
------------------

Improvements
############
- Updated :ref:`LoadElementalAnalysisData <algm-LoadElementalAnalysisData>` algorithm to include Poisson errors for the counts data.

Algorithms
##########

- Updated :ref:`LoadMuonLog <algm-LoadMuonLog>` to read units for most log values.

:ref:`Release 6.2.0 <v6.2.0>`
