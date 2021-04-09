============
SANS Changes
============

.. contents:: Table of Contents
   :local:

.. warning:: **Developers:** Sort changes under appropriate heading
    putting new features at the top of the section, followed by
    improvements, followed by bug fixes.

Bugfixes
--------

- Fix a bug that made it impossible to process flux in SANSILLAutoprocess.
- On D16 using :ref:`SANSILLAutoProcess <algm-SANSILLAutoProcess>`, now use the correct monitor for normalization, fix a bug where processing transmission would yield undefined values at 90 degrees when using ThetaDependent correction, and improve the q binning used.

Improvements
############

- With SANSILLAutoProcess, the detector distance, the collimation position and the wavelength are appended to the names of the output workspaces (values are taken from the sample logs).
- :ref:`SANSILLAutoProcess <algm-SANSILLAutoProcess>` has new property: `StitchReferenceIndex` to denote the index of ws that should be a reference
  for scaling during stitching
- The core algorithm for ISIS SANS reductions underwent a significant rewrite to improve processing wavelength slices.
  For 15 slices the reduction time dropped by 55% compared to the previous release.


Algorithms and instruments
--------------------------


:ref:`Release 6.1.0 <v6.1.0>`
