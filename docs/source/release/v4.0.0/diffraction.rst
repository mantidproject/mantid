===================
Diffraction Changes
===================

.. contents:: Table of Contents
   :local:

Single Crystal Diffraction
--------------------------

New Algorithms
##############

- The new algorithm :ref:`MDNorm <algm-MDNorm>` can be used to calculate cross section for single crystal diffraction measurements.

Improvements
############

- SNAP instrument geometry updated to include downstream monitor.
- :ref:`SNAPReduce <algm-SNAPReduce>` now has progress bar and all output workspaces have history
- :ref:`SNAPReduce <algm-SNAPReduce>` has been completely refactored. It now uses :ref:`AlignAndFocusPowderFromFiles <algm-AlignAndFocusPowderFromFiles>` for a large part of its functionality. It has progress bar and all output workspaces have history. It is also more memory efficient by reducing the number of temporary workspaces created.
- :ref:`LoadWAND <algm-LoadWAND>` has grouping option added and loads faster.
- Mask workspace option added to :ref:`WANDPowderReduction <algm-WANDPowderReduction>`.
- :ref:`Le Bail concept page <Le Bail Fit>` moved from mediawiki.
- :ref:`ConvertToMD <algm-ConvertToMD>` now has `ConverterType = {Default, Indexed}` setting: `Default` keeps the old
  version of the algorithm, `Indexed` provide the new one with better performance and some restrictions
  (see :ref:`ConvertToMD <algm-ConvertToMD>` Notes).
- New TOPAZ instrument geometry for 2019 run cycle
- :ref:`LoadDiffCal <algm-LoadDiffCal>` has an additional parameter to allow for a second file specifying a grouping to override the one in the calibration file
- :ref:`IntegratePeaksProfileFitting <algm-IntegratePeaksProfileFitting>` now supports MaNDi, TOPAZ, and CORELLI. Other instruments can easily be added as well.  In addition, the algorithm can now automatically generate a strong peaks library is one is not provided.  Peakshapes will be learned to improve initial guesses as the strong peak library is generated.
- :ref:`MDNormSCD <algm-MDNormSCD>` now can handle merged MD workspaces.
- :ref:`StartLiveData <algm-StartLiveData>` will load "live"
  data streaming from TOPAZ new Adara data server.
- :ref:`IntegratePeaksMD <algm-IntegratePeaksMD>` with Cylinder=True now has improved fits using BackToBackExponential and IkedaCarpenterPV functions.
- :ref:`SCDCalibratePanels <algm-SCDCalibratePanels>` now attempts to index all the peaks at each iteration instead of only using initially indexed peaks.
- :ref:`SaveIsawPeaks <algm-SaveIsawPeaks>` now has option to renumber peaks sequentially.
- SCD Event Data Reduction Diffraction Interface now has option to create MD HKL workspace.
- :ref:`IntegratePeaksUsingClusters <algm-IntegratePeaksUsingClusters>` will now treat NaN's as background.
- SCD Event Data Reduction Diffraction Interface now adds goniometer for CORELLI and used proton charge as monitor count if no monitors are in input file.
- :ref:`SetCrystalLocation <algm-SetCrystalLocation>` is a new algorithm to set the sample location in events workspaces.
- :ref:`OptimizeCrystalPlacementByRun <algm-OptimizeCrystalPlacementByRun>` is new algorithm to update the sample position for each run in a peaks workspace.
- :ref:`SingleCrystalDiffuseReduction <algm-SingleCrystalDiffuseReduction>` has been update to use :ref:`MDNorm <algm-MDNorm>` instead of :ref:`MDNormSCD <algm-MDNormSCD>` internally. Additionally more options have been added to apply either a calibration with :ref:`ApplyCalibration <algm-ApplyCalibration>` or to copy an Instrument with :ref:`CopyInstrumentParameters <algm-CopyInstrumentParameters>` (these were also added to :ref:`ConvertMultipleRunsToSingleCrystalMD <algm-ConvertMultipleRunsToSingleCrystalMD>`); options have been added that allow you to specify either a UB matrix file or omega offset separately for each run; by default the SolidAngle and Flux workspaces will not be deleted and will be reused the next time the algorithm is used. Incompatible changes include changing of parameters names for projection, binning and symmetry operations to match :ref:`MDNorm <algm-MDNorm>`; symmetry operations will now use the symmetry of the point group instead of space group and will no longer accept space group number to avoid ambiguity of which point group to use; binning parameter has been changed match :ref:`MDNorm <algm-MDNorm>` where the bin width is specified instead of the number of bins.

Bugfixes
########

- :ref:`CentroidPeaksMD <algm-CentroidPeaksMD>` now updates peak bin counts.
- :ref:`FindPeaksMD <algm-FindPeaksMD>` now finds peaks correctly with the crystallography convention setting and reduction with crystallography convention is tested with a system test.
- :ref:`SaveIsawPeaks <algm-SaveIsawPeaks>` does not have duplicate peak numbers when saving PeaksWorkspaces with more than one RunNumber.
- :ref:`LoadIsawPeaks <algm-LoadIsawPeaks>` now loads the calibration from the peaks file correctly.
- :ref:`OptimizeCrystalPlacement <algm-OptimizeCrystalPlacement>` now updates the sample location used by peaks.  Previously, the sample was effectively left unmoved. Default for indexing tolerance was lowered to 0.15 and can now be called more than once without error.

Powder Diffraction
------------------

New Algorithms
##############

- :ref:`HB2AReduce <algm-HB2AReduce>` algorithm reduces HFIR POWDER (HB-2A) data.
- :ref:`LoadGudrunOutput <algm-LoadGudrunOutput>` is a new algorithm that allows users to load the standard Gudrun output files into Mantid.
- :ref:`PDConvertReciprocalSpace <algm-PDConvertReciprocalSpace>` new algorithm to convert between reciprocal space units.
- :ref:`PDConvertRealSpace <algm-PDConvertRealSpace>` new algorithm to convert between real space units.

Improvements
############

- Focusing in texture mode for Gem now properly saves .gda files.
- Focusing on Gem now crops values that would be divided by very small or zero vanadium values.
- Removed save_angles flag for Gem , as it was set by the texture mode.
- Added save_all flag to Gem that is set to true by default, setting it to false disables the saving of .NXS files.
- Added subtract_empty_instrument flag to Gem that is true by default, setting it to false disables subtracting the empty.
- Changed spline coefficient so that the default for long_mode on and long_mode off can be set separately.
- Focus on Pearl now has a focused_bin_widths parameter in pearl_advanced_config.py to allow setting default rebin values.
- Focus on Pearl now saves out xye_tof files.
- :ref:`LoadILLDiffraction <algm-LoadILLDiffraction>` will not flip the even-numbered tubes when using the calibrated data, since they are flipped already in the nexus files.
- :ref:`PowderILLDetectorScan <algm-PowderILLDetectorScan>` will scale the counts by 1M, when normalisation to monitor is requested, and it will also offer to enable/disable the tube alignment, and offer tube by tube reduction.
- :ref:`PowderILLEfficiency <algm-PowderILLEfficiency>` now offers to use the raw or calibrated data blocks in the nexus files.
- :ref:`AlignAndFocusPowder <algm-AlignAndFocusPowder>` and :ref:`AlignAndFocusPowderFromFiles <algm-AlignAndFocusPowderFromFiles>` now support outputting the unfocussed data and weighted events (with time). This allows for event filtering **after** processing the data.
- :ref:`AlignAndFocusPowderFromFiles <algm-AlignAndFocusPowderFromFiles>` has significant performance improvements when used with chunking and can now use summed cache files. Failing to load a cache file will now produce a warning, rather than exception, and the algorithm will continue without the file.
- Rework of :ref:`powder diffraction calibration <Powder Diffraction Calibration>` documentation.
- :ref:`LoadILLDiffraction <algm-LoadILLDiffraction>` will now correctly resolve for the scan type and drive the detector to the offset corrected :math:`2theta_0` for D20 detector scans.
- :ref:`PowderILLDetectorScan <algm-PowderILLDetectorScan>` will never merge the detector scans at the raw level even if they are supplied with + operator; it will process them separately and merge at the end.
- :ref:`PDLoadCharacterizations <algm-PDLoadCharacterizations>` now sets the same run numbers for all rows when using an ``exp.ini`` file.
- :ref:`PDDetermineCharacterizations <algm-PDDetermineCharacterizations>` will generate an exception if it cannot determine the frequency or wavelength
- Focus now checks if the vanadium for a run is already loaded before loading it in to prevent reloading the same vanadium multiple times.
- :ref:`SaveReflections <algm-SaveReflections>` now supports saving indexed modulated peaks in the Jana format.
- `PyStoG <https://pystog.readthedocs.io/en/latest/>`_ has been added as an external project.
- :ref:`SNSPowderReduction <algm-SNSPowderReduction>` has been refactored to process the vanadium data in chunks. It has also been modified to calculate the vanadium correction for multiple wavelengths

Bugfixes
########

- multiple_scattering flag is now optional for Polaris focus when absorb_correction is true.
- Normalisation is fixed in :ref:`SumOverlappingTubes <algm-SumOverlappingTubes>`, which was causing very low peak to background ratio for reduced D2B data.
- Sudden drops at either end of spectra in Pearl caused by partial bins are now cropped.
- The Powder Diffraction GUI now remembers whether linear or logarithmic binning was selected between uses.
- Fixed a bug in :ref:`GenerateGroupingPowder <algm-GenerateGroupingPowder>` which caused detectors without corresponding spectrum to get included in grouping.
- :ref:`AlignAndFocusPowderFromFiles <algm-AlignAndFocusPowderFromFiles>` now does not use cache file when the grouping has changed.
- :ref:`PowderILLParameterScan <algm-PowderILLParameterScan>` will now accept also single run, when the time normalisation is enabled.

New Algorithms
##############

- :ref:`HB2AReduce <algm-HB2AReduce>` algorithm reduces HFIR POWDER (HB-2A) data
- :ref:`LoadGudrunOutput <algm-LoadGudrunOutput>` is a new algorithm that allows users to load the standard Gudrun output files into Mantid.
- New algorithm :ref:`PDConvertReciprocalSpace <algm-PDConvertReciprocalSpace>` to convert between reciprocal space units.
- New algorithm :ref:`PDConvertRealSpace <algm-PDConvertRealSpace>` to convert between real space units.

Renamed Algorithms
##################

- **PowderDiffILLReduction** is renamed to :ref:`PowderILLParameterScan <algm-PowderILLParameterScan>`
- **PowderDiffILLDetEffCorr** is renamed to :ref:`PowderILLEfficiency <algm-PowderILLEfficiency>`
- **PowderDiffILLDetScanReduction** is renamed to :ref:`PowderILLDetectorScan <algm-PowderILLDetectorScan>`

Engineering Diffraction
-----------------------

New
###
- Scripts added that produce the same results as the ISIS engineering gui (supports ENGINX and IMAT), this is to allow use with ISIS autoreduction. The script plots calibration automatically, like the GUI.
- Changed 'Add Peak' button on fitting tab of gui to read 'Add Peak to List' to clarify use. 

Bugfixes
########

- Fixed a crash in the gui caused by running a calibration without setting a calibration directory.
- Fixed a crash in the gui caused by having the EnggFitPeaks algorithm fail in the fitting tab.

:ref:`Release 4.0.0 <v4.0.0>`
