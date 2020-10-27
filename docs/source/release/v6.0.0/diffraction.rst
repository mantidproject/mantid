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

Improvements
############
- :ref:`WANDPowderReduction <algm-WANDPowderReduction>` now accepts a sequence of input workspaces, combining them to reduce to a single spectrum.

Bugfixes
########

- Dummy detectors in polaris workspaces no longer prevent unit conversion.
- Focus in PEARL powder diffraction scripts no longer fails if previous run has left Van splines workspace group in ADS


Engineering Diffraction
-----------------------

Single Crystal Diffraction
--------------------------

New features
############
- Scripts for pixel calibration of CORELLI 16-packs. Produce a calibration table, a masking table, and a goodness of fit workspace.
- Fix problem that was causing matrix diagonalization to return NaNs in certain cases. The diagonalization is used in :ref:`CalculateUMatrix <algm-CalculateUMatrix>` and :ref:`IntegratePeaksMD <algm-IntegratePeaksMD>`


Imaging
-------

:ref:`Release 6.0.0 <v6.0.0>`
