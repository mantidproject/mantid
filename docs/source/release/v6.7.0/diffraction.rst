===================
Diffraction Changes
===================

.. contents:: Table of Contents
   :local:

Powder Diffraction
------------------

New features
############
- For ILL/D2B instrument, only calibrated data are loaded from the nexus file.
- the ISIS powder diffraction scripts are now able to support per detector Vanadium normalisation. This has been enabled on the POLARIS instrument only so far
- It is now possible to provide more than one detector height range in :ref:`PowderILLDetectorScan <algm-PowderILLDetectorScan>`
- InitialMask and FinalMask properties of :ref:`PowderILLDetectorScan <algm-PowderILLDetectorScan>` have been removed
- Improvements to PEARL powder reduction output file and directory naming (.gss files and TOF and d-spacing .xye files are now in separate directories)
- OSIRIS scripts now uses focus function instead of run_diffraction_focusing function to perform the focusing
- create_vanadium function is added to the OSIRIS object which should be called before the focus function to create the vanadium runs
- It is now possible to mask out detectors in the absorption and multiple scattering calculation :ref:`AbsorptionCorrection <algm-AbsorptionCorrection>`, :ref:`PaalmanPingsAbsorptionCorrection <algm-PaalmanPingsAbsorptionCorrection>`, :ref:`MultipleScatteringCorrection <algm-MultipleScatteringCorrection>`
- Additional, optional, source terms added to the :ref:`EstimateResolutionDiffraction <algm-EstimateResolutionDiffraction>`. For instruments with small detector pixels (e.g. SNAP), the size of the source is the overwhelming contributor to the resolution calculation.
- :ref:`EstimateResolutionDiffraction <algm-EstimateResolutionDiffraction>` will average the resolution from summed pixels

Bugfixes
############
- The Osiris script and the Indirect Diffraction UI now produces the same results


Engineering Diffraction
-----------------------

New features
############
- The GSASII tab outputs the sample logs attached to the relevant focused data, as specified in the Engineering Diffraction settings. This behaves in the same way as before for the fitting tab.
- Check peak-window signal-to-noise ratio before attempting to fit a diffraction peak in :ref:`FitPeaks <algm-FitPeaks>`
- It is now possible to load and plot multiple phases in the GSASII tab.

Bugfixes
############
- extra validation has been added to the "Path to GSASII" setting so that Workbench no longer crashes if a GSAS refinement is run with an invalid GSASII path in settings
- Loading or creating a calibration now fills the GSAS II tab with the currently loaded calibration's ``.prm`` file.


Single Crystal Diffraction
--------------------------

New features
############
- Added new methods to calculate the directions of the reverse incident and scattered beam within `Peak`
- Added new method to calculate the direction cosine from a provided direction within `OrientedLattice`
- Support workspaces with d-spacing xunit in :ref:`IntegratePeaksSkew <algm-IntegratePeaksSkew>`
- Add option ``OptimiseXWindowSize`` to fix TOF (or d-sapcing) window extent below a threshold intensity/sigma (``ThresholdIoverSigma``) in :ref:`IntegratePeaksSkew <algm-IntegratePeaksSkew>`
- Improved formatting of plots in pdf output of :ref:`IntegratePeaksSkew <algm-IntegratePeaksSkew>`
- Parameter ``NTOFBinsMin`` in :ref:`IntegratePeaksSkew <algm-IntegratePeaksSkew>` now refers to the number of non-zero bins in the integration window with y/error > 1
- :ref:`OptimizeLatticeForCellType <algm-OptimizeLatticeForCellType>` now properly supports mnp-modulation vectors
- :ref:`SaveHKL <algm-SaveHKL>` supports 6-column `hklmnp` format
- :ref:`LoadHKL <algm-LoadHKL>` supports 6-column `hklmnp` format
- :ref:`FilterPeaks <algm-FilterPeaks>` can now filter out modulation vectors by `m^2+n^2+p^2`
- :ref:`SortHKL <algm-SortHKL>` skips modulation vectors
- :ref:`StatisticsOfPeaksWorkspace <algm-StatisticsOfPeaksWorkspace>` skips modulation vectors
- :ref:`SaveNexus <algm-SaveNexus>` now supports modulation vectors
- :ref:`LoadNexus <algm-LoadNexus>` now supports modulation vectors
- :ref:`CompareWorkspaces <algm-CompareWorkspaces>` now supports modulation vectors
- Peaks workspaces now display `IntHKL` and `IntMNP` columns
- Add new instrument definition for SXD post bank 1 detector upgrade

Bugfixes
############
- Increased number of nearest neighbor detector pixels to search for :ref:`PredictPeaks <algm-PredictPeaks>` that caused some obvious peaks  on detector to be missed.
- Fixed the calcuation of direction cosines in :ref:`SaveHKL <algm-SaveHKL>` and :ref:`SaveHKLCW <algm-SaveHKLCW>`
- Added back in `Peak` method `getDetectorID()` that was removed in previous release associated with `LeanElasticPeak`
- Added a system test with new test data for :ref:`IntegratePeaksProfileFittinng <algm-IntegratePeaksProfileFitting>` to help resolve a bug related to BivariateGuassian module. The test compares to Mantid v6.4
- Fix bug in estimating TOF window parameters in :ref:`IntegratePeaksSkew <algm-IntegratePeaksSkew>`
- :ref:`IndexPeaks <algm-IndexPeaks>` now properly accountes for fractional offsets when `RoundHKLs=True`

:ref:`Release 6.7.0 <v6.7.0>`