=========================
Indirect Geometry Changes
=========================

.. contents:: Table of Contents
   :local:

New Features
############
- A new algorithm :ref:`IndirectILLReductionDIFF <algm-IndirectILLReductionDIFF>` added for treatment of diffraction detector data from the indirect geometry beamline IN16B at the ILL.
- Three fitting functions `IsoRotDiff`, `DiffSphere` and `DiffRotDiscreteCircle` have been made available in the fitting browser
- `IsoRotDiff`, `DiffSphere` and `DiffRotDiscreteCircle` have been added to the function options in Indirect Data Analysis ConvFit.
- The Abins Algorithm has an additional "setting" option which may be used to select between configurations of a given instrument. (For TOSCA this is a choice of forward/back detector banks, for Lagrange this is a choice between monochromator crystals.)
- Support has been added to :ref:`Abins <algm-Abins>` for the ILL-Lagrange instrument. As Lagrange collects inelastically scattered neutrons over a wide solid angle, the spectrum is computed at several angles and averaged. Resolution functions are applied depending on the monochromator setting.

Improvements
############

- In Indirect Data Analysis F(Q) fit the default fitting function remains None when switching to EISF.
- Added a scroll bar to the Bayes interface tabs and Elwin and I(Q, t) in data analysis for users on small screens.
- IN16B's single detectors are now correctly taken into account when computing the energy transfer in :ref:`IndirectILLEnergyTransfer <algm-IndirectILLEnergyTransfer>`.
- Detector tables produced from `_red` and `_sqw` workspaces now use `Q elastic` as the label for its column instead of `Q`.

Bug Fixes
#########
- The x range markers on the Symmetrise plot of Data Reduction are no longer restricted in movement.
- The x range markers on the ISISDiagnostics plot of Data Reduction remain present.
- Fixed a crash on the Data Analysis interface when attempting to drag the Start and End X sliders on the preview plot.
- In IsoRotDiff, DiffSphere, and DiffRotDiscreteCircle Aliases have been removed to avoid clashes with interfaces.
- Stopped error warning thrown when adding EISF data to F(q) fit if Width has already been added.
- Fixed a bug that caused the spectra list in F(q) fit to be blank when reopening the add workspace dialog.
- Previous fits in the Indirect Data Analysis fit tabs no longer erased when changing to the full function view.
- Fit ranges in Indirect Data Analysis can now be negative.

:ref:`Release 6.1.0 <v6.1.0>`
