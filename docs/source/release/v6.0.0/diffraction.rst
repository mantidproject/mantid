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

- New algorithm :ref:`PolDiffILLReduction <algm-PolDiffILLReduction>` to perform polarised diffraction data reduction for the ILL D7 instrument.
- New algorithm :ref:`D7AbsoluteCrossSections <algm-D7AbsoluteCrossSections>` to separate magnetic, nuclear coherent, and incoherent cross-sections using spin-flip and non-spin-flip cross-sections, and to normalise D7 data to a given standard.
- New algorithm :ref:`D7YIGPositionCalibration <algm-D7YIGPositionCalibration>` to perform wavelength and detector position calibration for the ILL D7 instrument.
    
Powder Diffraction
------------------
New features
############

- Add ability to store multiple alternative attenuation file paths in the Pearl yaml configuration file
- Modify filenames of xye outputs from running a focus in the Pearl power diffraction scripts
- Remove _noatten workspace that was produced by the Pearl powder diffraction scripts when run with perform_attenuation=True
- New algorithm to clip peaks, providing a background estimation :ref:`ClipPeaks <algm-ClipPeaks>`.
- Scripts for pixel calibration of CORELLI 16-packs. Produce a calibration table, a masking table, and a goodness of fit workspace.
- New algorithm :ref:`AbsorptionCorrectionPaalmanPings <algm-AbsorptionCorrectionPaalmanPings>` uses a numerical integration method to calculate attenuation factors for all Paalmin Pings terms

Improvements
############
- :ref:`WANDPowderReduction <algm-WANDPowderReduction>` now accepts a sequence of input workspaces, combining them to reduce to a single spectrum.

Bugfixes
########

- Dummy detectors in polaris workspaces no longer prevent unit conversion.


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
