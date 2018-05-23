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

- :ref:`PowderDiffILLDetEffCorr <algm-PowderDiffILLDetEffCorr>` is extended to compute the detector efficiencies also for the 2-dimensional scanning diffractometer D2B at the ILL.


Engineering Diffraction
-----------------------

- Improvements to the GSAS tab:

  - GSASIIRefineFitPeaks is now run asynchronously in the GUI, so the
    tab no longer locks when a refinement is run
  - A **Refine All** button was added to run refinement on every run
    loaded into the tab

- :ref:`GSASIIRefineFitPeaks <algm-GSASIIRefineFitPeaks>` now supports Pawley refinement as well as Rietveld
- HDF5 is now the standard format for saving data from the GUI:

  - Single peak fitting output is now saved as HDF5 instead of CSV,
    using :ref:`EnggSaveSinglePeakFitResultsToHDF5
    <algm-EnggSaveSinglePeakFitResultsToHDF5>`. The algorithm
    previously used for saving to CSV, **SaveDiffFittingAscii**, has
    been deprecated
  - Fit results and parameters are saved to HDF5 from the **GSAS
    Refinement** tab


:ref:`Release 3.13.0 <v3.13.0>`

Single Crystal Diffraction
--------------------------

- New algorithm :ref:`LoadDNSSCD <algm-LoadDNSSCD>` to load multiple single crystal diffraction data files from the DNS instrument into MDEventWorkspace.

- :ref:`SaveLauenorm <algm-SaveLauenorm>` now has input options for crystal system and reflection condition for lscale output instead of trying to determine from lattice parameters.

Improvements
############

- PeaksWorkspace has column added for the unique peak number so peaks can be found after sorting or filtering.

- :ref:`StatisticsOfPeaksWorkspace <algm-StatisticsOfPeaksWorkspace>` has option to use a weighted Z score for determining which peaks are outliers and has a new output workspace for plotting intensities of equivalent peaks.
