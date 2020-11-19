===================
Diffraction Changes
===================

.. contents:: Table of Contents
   :local:

.. warning:: **Developers:** Sort changes under appropriate heading
    putting new features at the top of the section, followed by
    improvements, followed by bug fixes.

New features
------------

- New algorithm :ref:`D7YIGPositionCalibration <algm-D7YIGPositionCalibration>` to perform wavelength and detector position calibration for the ILL D7 instrument.

Powder Diffraction
------------------
New features
############

- Add ability to store multiple alternative attenuation file paths in the Pearl yaml configuration file
- Modify filenames of xye outputs from running a focus in the Pearl power diffraction scripts
- Remove _noatten workspace that was produced by the Pearl powder diffraction scripts when run with perform_attenuation=True
- Speed up focus action in ISIS powder diffraction scripts by saving pre-summed empty instrument workspace during calibration step
- Add sample_empty and sample_empty_scale into Pearl configuration for ISIS powder diffraction scripts
- New algorithm to clip peaks, providing a background estimation :ref:`ClipPeaks <algm-ClipPeaks>`.
- Scripts for pixel calibration of CORELLI 16-packs. Produce a calibration table, a masking table, and a goodness of fit workspace.
- New algorithm :ref:`PaalmanPingsAbsorptionCorrection <algm-PaalmanPingsAbsorptionCorrection>` uses a numerical integration method to calculate attenuation factors for all Paalmin Pings terms
- Added a new :ref:`Transfit <algm-PEARLTransfit>` Algorithm for PEARL that uses a TransVoigt function to determine the temperature of a given sample
- Modified :ref:`SNSPowderReduction <algm-SNSPowderReduction>` to include several different absorption correction methods.
- Added new absorption options from :ref:`SNSPowderReduction <algm-SNSPowderReduction>` to Powder Diffraction Reduction GUI
- New algorithm :ref:`HB3AAdjustSampleNorm <algm-HB3AAdjustSampleNorm>` to convert DEMAND data to Q-space and allow the detector position to be adjusted by offsets.


Improvements
############
- :ref:`WANDPowderReduction <algm-WANDPowderReduction>` now accepts a sequence of input workspaces, combining them to reduce to a single spectrum.
- The plot pane in the Engineering Diffraction interface can now be undocked from the main window, making this more readable when the fit property view is open.
- The height of the :ref:`func-BackToBackExponential` peak is now preserved when changing the FWHM sliders when fitting.
- :ref:`PowderILLDetectorScan <algm-PowderILLDetectorScan>` is corrected when treating multiple scans merged.
- The default loadpath in the fitting tab of the Engineering Diffraction UI is now set to the most recently focused files.
- The :ref:`HB2AReduce <algm-HB2AReduce>` now can save reduced data to GSAS or XYE file.


Bugfixes
########

- Dummy detectors in polaris workspaces no longer prevent unit conversion.
- Focus in PEARL powder diffraction scripts no longer fails if previous run has left Van splines workspace group in ADS


Engineering Diffraction
-----------------------
- PaalmanPingsMonteCarloAbsorption can now use tabulated density values, and allows for overridden X Sections

Bugfixes
############
- Settings are now saved only when the Apply or OK button are clicked (i.e. clicking cancel will not update the settings).

Improvements
############
- The user is no longer asked to overwrite an automatically generated model that is saved in as a Custom Setup in the fit browser (it is overwritten).

New features
############
- When a fit is successful the model will be stored as a Custom Setup in the fit property browser under the name of the workspace fitted. 
- The fitting tab now creates a group of workspaces that store the model string and the fit value and error of parameters of the model for each loaded workspace.
- Sequential fitting of workspaces now provided in fitting tab by average value of a log set in settings.

Single Crystal Diffraction
--------------------------
Bugfixes
########
- Fix bug in :ref:`SaveHKL <algm-SaveHKL>` where the direction cosines were calculated incorrectly


New features
############
- Scripts for pixel calibration of CORELLI 16-packs. Produce a calibration table, a masking table, and a goodness of fit workspace.
- Fix problem that was causing matrix diagonalization to return NaNs in certain cases. The diagonalization is used in :ref:`CalculateUMatrix <algm-CalculateUMatrix>` and :ref:`IntegratePeaksMD <algm-IntegratePeaksMD>`


Imaging
-------

:ref:`Release 6.0.0 <v6.0.0>`
