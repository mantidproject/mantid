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
- Modified :ref:`AlignAndFocusPowderFromFiles <algm-AlignAndFocusPowderFromFiles>` Copies the sample from the absorption workspace to the output workspace
- Modified :ref:`SNSPowderReduction <algm-SNSPowderReduction>` to include several different absorption correction methods.
- Modified the vanadium absorption correction in :ref:`SNSPowderReduction <algm-SNSPowderReduction>` to be calculated using numerical integration rather than Carpenter method.
- Added new absorption options from :ref:`SNSPowderReduction <algm-SNSPowderReduction>` to Powder Diffraction Reduction GUI
- New algorithm :ref:`HB3AAdjustSampleNorm <algm-HB3AAdjustSampleNorm>` to convert DEMAND data to Q-space and allow the detector position to be adjusted by offsets.
- Files from ILL's instrument D1B can now be loaded.
- New algorithm :ref:`CorelliCalibrationDatabase <algm-CorelliCalibrationDatabase>` to save Corelli geometry calibration to database
- New algorithm :ref:`CorelliCalibrationApply <algm-CorelliCalibrationApply>` to apply a CORELLI calibration table to CORELLI EventWorkspace.
- New algorithm :ref:`CorelliCalibrationCreate <algm-CorelliPowderCalibrationCreate>` adjusts the position and orientation of Corelli banks in order to optimize the comparison of observed peaks to reference data
- Modified creation of absorption input in :ref:`SNSPowderReduction <algm-SNSPowderReduction>` to automatically get information from sample logs.
- :ref:`PDCalibration <algm-PDCalibration>` now supports workspaces with grouped detectors (i.e. more than one detector per spectrum)
- Added refined back to back exponential coeficients (from standard sample) to WISH Parameters.xml so A, B and S are guessed automatically.
- New algorithm :ref:`PolDiffILLReduction <algm-PolDiffILLReduction>` to perform polarised diffraction data reduction for the ILL D7 instrument.
- New algorithm :ref:`D7AbsoluteCrossSections <algm-D7AbsoluteCrossSections>` to separate magnetic, nuclear coherent, and incoherent cross-sections using spin-flip and non-spin-flip cross-sections, and to normalise D7 data to a given standard.

Improvements
############
- :ref:`WANDPowderReduction <algm-WANDPowderReduction>` now accepts a sequence of input workspaces, combining them to reduce to a single spectrum.
- The plot pane in the Engineering Diffraction interface can now be undocked from the main window, making this more readable when the fit property view is open.
- The height of the :ref:`func-BackToBackExponential` peak is now preserved when changing the FWHM sliders when fitting.
- :ref:`PowderILLDetectorScan <algm-PowderILLDetectorScan>` is corrected when treating multiple scans merged.
- The default loadpath in the fitting tab of the Engineering Diffraction UI is now set to the most recently focused files.
- The :ref:`HB2AReduce <algm-HB2AReduce>` now can save reduced data to GSAS or XYE file.
- The :ref:`D7YIGPositionCalibration <algm-D7YIGPositionCalibration>` now can do the YIG Bragg peak fitting individually or simultaneously, or not at all and provide feedback on the initial guess quality
- :ref:`PDCalibration <algm-PDCalibration>` now intitialises A,B and S of BackToBackExponential if correpsonding coeficients are in the instrument parameter.xml file.
- PaalmanPingsMonteCarloAbsorption can now make use of predefined sample and container geometries

Bugfixes
########

- Dummy detectors in polaris workspaces no longer prevent unit conversion.
- Focus in PEARL powder diffraction scripts no longer fails if previous run has left Van splines workspace group in ADS
- Fix out-of-range bug in :ref:`FitPeaks <algm-FitPeaks>` for histogram data.
- Fix bug to actually implement intended sequential fit of DIFC, DIFA, TZERO in :ref:`PDCalibration <algm-PDCalibration>`.


Bugfixes
########
- :ref:`WANDPowderReduction <algm-WANDPowderReduction>` once again accepts multiple input workspaces and outputs a group workspace when specified by user.

Engineering Diffraction
-----------------------
- PaalmanPingsMonteCarloAbsorption can now use tabulated density values, and allows for overridden X Sections

Bugfixes
############
- Settings are now saved only when the Apply or OK button are clicked (i.e. clicking cancel will not update the settings).

Improvements
############
- The user is no longer asked to overwrite an automatically generated model that is saved in as a Custom Setup in the fit browser (it is overwritten).
- Generic Sequential Fit button removed from fit menu (users should use sequential fit button below the table in the fitting tab of the UI).
- Status of fit updated in fit browser when Sequential Fit performed in the fittinng tab of the UI.

New features
############
- Added refined back to back exponential coeficients (from standard ceria run)to ENGIN-X Parameters.xml so A, B and S are guessed automatically.
- When a fit is successful the model will be stored as a Custom Setup in the fit property browser under the name of the workspace fitted.
- The fitting tab now creates a group of workspaces that store the model string and the fit value and error of parameters of the model for each loaded workspace.
- Sequential fitting of workspaces now provided in fitting tab by average value of a log set in settings.

Single Crystal Diffraction
--------------------------
New features
############
- New algorithm :ref:`ConvertQtoHKLMDHisto <algm-ConvertQtoHKLMDHisto>` to convert from a QSample MDEventWorkspace to HKL MDHistoWorkspace with correct peak overlaying.
- New algorithm :ref:`SaveHKLCW <algm-SaveHKLCW>` for SHELX76 constant wavelength format.
- Scripts for pixel calibration of CORELLI 16-packs. Produce a calibration table, a masking table, and a goodness of fit workspace.
- Fix problem that was causing matrix diagonalization to return NaNs in certain cases. The diagonalization is used in :ref:`CalculateUMatrix <algm-CalculateUMatrix>` and :ref:`IntegratePeaksMD <algm-IntegratePeaksMD>`
- New algorithm :ref:`HB3AFindPeaks <algm-HB3AFindPeaks>` to find peaks and set the UB matrix for DEMAND data.
- New algorithm :ref:`HB3APredictPeaks <algm-HB3APredictPeaks>` to predict peaks for DEMAND data.
- New algorithm :ref:`HB3AIntegratePeaks <algm-HB3AIntegratePeaks>` used to integrate peaks from an MDEventWorkspace and apply Lorentz correction on DEMAND data.

Improvements
############
- Support added for DEMAND (HB3A) to :ref:`PredictPeaks <algm-PredictPeaks-v1>` in order to handle additional goniometers.

Bugfixes
########
- Fix bug in :ref:`SaveHKL <algm-SaveHKL>` where the direction cosines were calculated incorrectly
- Updated ref:`SaveHKL <algm-SaveHKL>` to only recalculate tbar if it's not already populated in the input peaks workspace

New features
############
- Scripts for pixel calibration of CORELLI 16-packs. Produce a calibration table, a masking table, and a goodness of fit workspace.
- Fix problem that was causing matrix diagonalization to return NaNs in certain cases. The diagonalization is used in :ref:`CalculateUMatrix <algm-CalculateUMatrix>` and :ref:`IntegratePeaksMD <algm-IntegratePeaksMD>`

Improvements
############
- Support added for DEMAND (HB3A) to :ref:`PredictPeaks <algm-PredictPeaks-v1>` in order to handle additional goniometers.


Imaging
-------

:ref:`Release 6.0.0 <v6.0.0>`
