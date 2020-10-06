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

- Add ability to store multiple alternative attenuation file paths in the Pearl yaml configuration file

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
