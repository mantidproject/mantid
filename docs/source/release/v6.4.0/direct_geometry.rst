=======================
Direct Geometry Changes
=======================

.. contents:: Table of Contents
   :local:

Direct Geometry
---------------

Improvements
############

- :ref:`CorrectTOFAxis <algm-CorrectTOFAxis>` can now accept fractional bin indices for more precise calculation of the elastic peak position calibration
- :ref:`DirectILLCollectData <algm-DirectILLCollectData>` will now accept fractional elastic peak reference bin as well as forward fractional elastic
  peak index to the :ref:`CorrectTOFAxis <algm-CorrectTOFAxis>`
- :ref:`DirectILLApplySelfShielding <algm-DirectILLApplySelfShielding>` now ensures that the subtracted container and self-attenuation correction workspaces
  have consistent binning with the provided sample to be corrected by rebinning to the sample

.. warning:: **Developers:** Sort changes under appropriate heading
    putting new features at the top of the section, followed by
    improvements, followed by bug fixes.

:ref:`Release 6.4.0 <v6.4.0>`
