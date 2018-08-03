============
MuSR Changes
============

.. contents:: Table of Contents
   :local:
   
Interface
---------

Improvements
############
- The updated :ref:`EstimateMuonAsymmetryFromCounts <algm-EstimateMuonAsymmetryFromCounts>` and :ref:`CalculateMuonAsymmetry <algm-CalculateMuonAsymmetry>` are used in Muon Analysis.
- TF Asymmetry mode now uses :ref:`CalMuonDetectorPhases <algm-CalMuonDetectorPhases>` and the fitting function is updated to show the normalization.

Bugfixes
########

- Results table can now detect sequential fits.
- Fit options are not disabled after changing tabs.
- The run number is now updated before the periods, preventing irrelevant warnings from being produced.
- In single fit the workspace can be changed.
- In multiple fitting the function can be replaced without causing a crash.
- Loading current run in Muon Analysis and going to previous runs no longer creates an error in the simultaneous fit label.

Algorithms
----------

New
###
- :ref:`ConvertFitFunctionForMuonTFAsymmetry <algm-ConvertFitFunctionForMuonTFAsymmetry>` has been added to help convert fitting functions for TF asymmetry fitting.

Improvements
############
- :ref:`EstimateMuonAsymmetryFromCounts <algm-EstimateMuonAsymmetryFromCounts>` now updates a normalization table and produces unnormalized data.
- :ref:`CalculateMuonAsymmetry <algm-CalculateMuonAsymmetry>` now updates a normalization table and can take multiple workspaces.

Bugfixes
########
- :ref:`EstimateMuonAsymmetryFromCounts <algm-EstimateMuonAsymmetryFromCounts>` had a small numerical error in the denominator of the normalisation calculation.
- :ref:`MuonMaxent <algm-MuonMaxent>` and :ref:`PhaseQuad <algm-PhaseQuad>`  no longer include dead detectors (zero counts) when calculating the frequency spectrum.
- :ref:`RemoveExpDecay <algm-RemoveExpDecay>` will not alter data from a dead detectors (zero counts).
- :ref:`CalMuonDetectorPhases <algm-CalMuonDetectorPhases>` will give an error code for dead detectors (zero counts) in the phase table.
- :ref:`MuonProcess <algm-MuonProcess>` always crops the data from `Xmin`.

:ref:`Release 3.13.0 <v3.13.0>`
