===================
Diffraction Changes
===================

.. contents:: Table of Contents
   :local:

.. warning:: **Developers:** Sort changes under appropriate heading
    putting new features at the top of the section, followed by
    improvements, followed by bug fixes.

Powder Diffraction
------------------
New features
############
- New algorithm to clip peaks, providing a background estimation :ref:`ClipPeaks <algm-ClipPeaks>`.
- Scripts for pixel calibration of CORELLI 16-packs. Produce a calibration table, a masking table, and a goodness of fit workspace.

Improvements
############
- :ref:`WANDPowderReduction <algm-WANDPowderReduction>` now accepts a sequence of input workspaces, combining them to reduce to a single spectrum.

New features
############

- Modify filenames of xye outputs from running a focus in the Pearl power diffraction scripts
- Remove _noatten workspace that was produced by the Pearl powder diffraction scripts when run with perform_attenuation=True

Bugfixes
########

- Dummy detectors in polaris workspaces no longer prevent unit conversion.

Engineering Diffraction
-----------------------

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

New features
############
- Scripts for pixel calibration of CORELLI 16-packs. Produce a calibration table, a masking table, and a goodness of fit workspace.
- Fix problem that was causing matrix diagonalization to return NaNs in certain cases. The diagonalization is used in :ref:`CalculateUMatrix <algm-CalculateUMatrix>` and :ref:`IntegratePeaksMD <algm-IntegratePeaksMD>`


Imaging
-------

:ref:`Release 6.0.0 <v6.0.0>`
