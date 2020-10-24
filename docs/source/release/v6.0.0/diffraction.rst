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
- Modified :ref:`SNSPowderReduction <algm-SNSPowderReduction>' to include several different absorption correction methods.

Improvements
############
- :ref:`WANDPowderReduction <algm-WANDPowderReduction>` now accepts a sequence of input workspaces, combining them to reduce to a single spectrum.

Engineering Diffraction
-----------------------

Single Crystal Diffraction
--------------------------

Imaging
-------

:ref:`Release 6.0.0 <v6.0.0>`
