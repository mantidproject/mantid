============
MuSR Changes
============

.. contents:: Table of Contents
   :local:

.. figure:: ../../images/muon_release_3_12.png
   :class: screenshot
   :align: right
   :width: 500 px


Bug Fixes
---------
- :ref:`CalMuonDetectorPhases <algm-CalMuonDetectorPhases>` has had the sign of the phase shift changed, this produces data with a positive frequency spike as expected.

Interface
---------
- Added a cancel button to the MaxEnt widget in Frequency Domain Analysis.
- Added checkboxes for "add all pairs" and "add all groups" to the settings tab. 
- The data plot style in the settings tab of Muon Analysis, only alters the plot range. It no longer crops the data.  
- Results table in Muon Analysis now sets relevant columns to numeric. 
- The Frequency Domain Analysis GUI now uses :ref:`CalMuonDetectorPhases <algm-CalMuonDetectorPhases>` to create the phase table for PhaseQuad FFTs. 
- The Frequency Domain Analysis GUI now uses :ref:`MuonMaxent <algm-MuonMaxent>` to calculate the frequency spectrum in MaxEnt mode.  
- The period display no longer becomes blank when changing the period selection in multiple fitting mode. 
- The group/pair selection in Muon Analysis no longer resets when changing tabs or loading data.

Algorithms
----------
- :ref:`MuonProcess <algm-MuonProcess>` now has a flag to determine if to crop the input workspace (default is true). In the Muon Analysis interface this flag has been set to false.
- :ref:`MuonMaxent <algm-MuonMaxent>` calculates a single frequency spectrum from multiple time domain spectra. 

:ref:`Release 3.12.0 <v3.12.0>`
