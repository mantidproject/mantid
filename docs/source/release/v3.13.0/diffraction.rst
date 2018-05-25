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

- Changing settings while running methods on the PEARL object no
  longer updates the default settings. Instead, initial settings are
  taken as the default, and any changes are reverted back to the
  default once the line they were made on has finished executing
- :ref:`PDCalibration <algm-PDCalibration>` has major upgrades including making use of :ref:`FitPeaks <algm-FitPeaks>` for the individual peak fitting

- New SNAP instrument geometry for 2018 run cycle

- New POWGEN instrument geometry for 2018 run cycle

New Features
------------

- :ref:`LoadILLDiffraction <algm-LoadILLDiffraction>` is extended to apply vertical and horizontal tube alignment for D2B, based on the IPF.
- :ref:`PowderDiffILLDetEffCorr <algm-PowderDiffILLDetEffCorr>` is extended to compute the detector efficiencies also for the 2-dimensional scanning diffractometer D2B at the ILL.
- :ref:`PowderDiffILLDetEffCorr <algm-PowderDiffILLDetEffCorr>` is extended to provide automatic masking of the pixels with spurious calibration constants.
- :ref:`PowderDiffILLDetScanReduction <algm-PowderDiffILLDetScanReduction>` is extended to provide initial masking of the top and bottom parts of the tubes, and final masking of the 2D outputs.
- :ref:`WANDPowderReduction <algm-WANDPowderReduction>` performs powder diffraction data reduction for WAND² with calibration, monitor normalisation and background subtraction.

Engineering Diffraction
-----------------------

- Improvements to the GSAS tab:

  - GSASIIRefineFitPeaks is now run asynchronously in the GUI, so the
    tab no longer locks when a refinement is run
  - A **Refine All** button was added to run refinement on every run
    loaded into the tab

- :ref:`GSASIIRefineFitPeaks <algm-GSASIIRefineFitPeaks>` now supports Pawley refinement as well as Rietveld


:ref:`Release 3.13.0 <v3.13.0>`

Powder Diffraction
------------------

- :ref:`GetDetectorOffsets <algm-GetDetectorOffsets>` is improved on peak fitting for more precise result on peak center calculation and masking.



Single Crystal Diffraction
--------------------------


- New algorithm :ref:`LoadDNSSCD <algm-LoadDNSSCD>` to load multiple single crystal diffraction data files from the DNS instrument into MDEventWorkspace.

- :ref:`SaveLauenorm <algm-SaveLauenorm>` now has input options for crystal system and reflection condition for lscale output instead of trying to determine from lattice parameters.

- :ref:`CreatePeaksWorkspace <algm-CreatePeaksWorkspace>` now accepts MD workspaces as input.

Improvements
############

- PeaksWorkspace has column added for the unique peak number so peaks can be found after sorting or filtering.

- :ref:`StatisticsOfPeaksWorkspace <algm-StatisticsOfPeaksWorkspace>` has option to use a weighted Z score for determining which peaks are outliers and has a new output workspace for plotting intensities of equivalent peaks.
