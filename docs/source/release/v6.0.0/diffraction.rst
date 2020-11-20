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
- New algorithm :ref:`PaalmanPingsAbsorptionCorrection <algm-PaalmanPingsAbsorptionCorrection>` uses a numerical integration method to calculate attenuation factors for all Paalmin Pings terms
- Modified :ref:`SNSPowderReduction <algm-SNSPowderReduction>` to include several different absorption correction methods.
- Added new absorption options from :ref:`SNSPowderReduction <algm-SNSPowderReduction>` to Powder Diffraction Reduction GUI
- New algorithm :ref:`HB3AAdjustSampleNorm <algm-HB3AAdjustSampleNorm>` to convert DEMAND data to Q-space and allow the detector position to be adjusted by offsets.

Improvements
############
- :ref:`WANDPowderReduction <algm-WANDPowderReduction>` now accepts a sequence of input workspaces, combining them to reduce to a single spectrum.

Bugfixes
########
- :ref:`WANDPowderReduction <algm-WANDPowderReduction>` once again accepts multiple input workspaces and outputs a group workspace when specified by user.

Engineering Diffraction
-----------------------

Single Crystal Diffraction
--------------------------
Bugfixes
########
- Fix bug in :ref:`SaveHKL <algm-SaveHKL>` where the direction cosines were calculated incorrectly


Imaging
-------

:ref:`Release 6.0.0 <v6.0.0>`

