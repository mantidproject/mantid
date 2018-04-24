===================
Diffraction Changes
===================

.. contents:: Table of Contents
   :local:

.. warning:: **Developers:** Sort changes under appropriate heading
    putting new features at the top of the section, followed by
    improvements, followed by bug fixes.

New Features
------------

- :ref:`LoadILLDiffraction <algm-LoadILLDiffraction>` is extended to apply vertical and horizontal tube alignment for D2B, based on the IPF.
- :ref:`PowderDiffILLDetEffCorr <algm-PowderDiffILLDetEffCorr>` is extended to compute the detector efficiencies also for the 2-dimensional scanning diffractometer D2B at the ILL.
- :ref:`PowderDiffILLDetEffCorr <algm-PowderDiffILLDetEffCorr>` is extended to provide automatic masking of the pixels with spurious calibration constants.
- :ref:`PowderDiffILLDetScanReduction <algm-PowderDiffILLDetScanReduction>` is extended to provide initial masking of the top and bottom parts of the tubes, and final masking of the 2D outputs.

Engineering Diffraction
-----------------------

- GSASIIRefineFitPeaks is now run asynchronously in the GUI, so the GSAS tab no longer locks when a refinement is run

:ref:`Release 3.13.0 <v3.13.0>`

Single Crystal Diffraction
--------------------------

Improvements
############

- PeaksWorkspace has column added for the unique peak number so peaks can be found after sorting or filtering.

- :ref:`StatisticsOfPeaksWorkspace <algm-StatisticsOfPeaksWorkspace>` has option to use a weighted Z score for determining which peaks are outliers and has a new output workspace for plotting intensities of equivalent peaks.
