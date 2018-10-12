===================
Diffraction Changes
===================

.. contents:: Table of Contents
   :local:

.. warning:: **Developers:** Sort changes under appropriate heading
    putting new features at the top of the section, followed by
    improvements, followed by bug fixes.


Improvements
############

- :ref:`LoadILLDiffraction <algm-LoadILLDiffraction>` will not flip the even-numbered tubes when using the calibrated data, since they are flipped already in the nexus files.
- :ref:`PowderDiffILLDetScanReduction <algm-PowderDiffILLDetScanReduction>` will scale the counts by 1M, when normalisation to monitor is requested, and it will also offer to enable/disable the tube alignment, and offer tube by tube reduction.
- :ref:`PowderDiffILLDetEffCorr <algm-PowderDiffILLDetEffCorr>` now offers to use the raw or calibrated data blocks in the nexus files.
- :ref:`SNAPReduce <algm-SNAPReduce>` now has progress bar and all output workspaces have history
- :ref:`SNAPReduce <algm-SNAPReduce>` has been completely refactored. It now uses :ref:`AlignAndFocusPowderFromFiles <algm-AlignAndFocusPowderFromFiles>` for a large part of its functionality. It has progress bar and all output workspaces have history. It is also more memory efficient by reducing the number of temporary workspaces created.
- :ref:`AlignAndFocusPowder <algm-AlignAndFocusPowder>` and :ref:`AlignAndFocusPowderFromFiles <algm-AlignAndFocusPowderFromFiles>` now support outputting the unfocussed data and weighted events (with time). This allows for event filtering **after** processing the data.
- :ref:`LoadWAND <algm-LoadWAND>` has grouping option added and loads faster
- Mask workspace option added to :ref:`WANDPowderReduction <algm-WANDPowderReduction>`
- :ref:`Le Bail concept page <Le Bail Fit>` moved from mediawiki

Single Crystal Diffraction
--------------------------

Improvements
############

- :ref:`IntegratePeaksProfileFitting <algm-IntegratePeaksProfileFitting>` now supports MaNDi, TOPAZ, and CORELLI. Other instruments can easily be added as well.  In addition, the algorithm can now automatically generate a strong peaks library is one is not provided.
- :ref:`MDNormSCD <algm-MDNormSCD>` now can handle merged MD workspaces.
- :ref:`StartLiveData <algm-StartLiveData>` will load "live"
  data streaming from TOPAZ new Adara data server.
- :ref:`IntegratePeaksMD <algm-IntegratePeaksMD>` with Cylinder=True now has improved fits using BackToBackExponential and IkedaCarpenterPV functions.
- :ref:`SCDCalibratePanels <algm-SCDCalibratePanels>` now rotates the panels with YZY rotations and clones the workspace instead of removing rotations.

Bugfixes
########

- :ref:`CentroidPeaksMD <algm-CentroidPeaksMD>` now updates peak bin counts.

- :ref:`FindPeaksMD <algm-FindPeaksMD>` now finds peaks correctly with the crystallography convention setting and reduction with crystallography convention is tested with a system test.
- :ref:`SaveIsawPeaks <algm-SaveIsawPeaks>` does not have duplicate peak numbers when saving PeaksWorkspaces with more than one RunNumber.

Powder Diffraction
------------------

Improvements
############

- Focusing in texture mode for Gem now properly saves .gda files.
- Focusing on Gem now crops values that would be divided by very small or zero vanadium values
- Removed save_angles flag for Gem , as it was set by the texture mode.
- Added save_all flag to Gem that is set to true by default, setting it to false disables the saving of .NXS files.
- Changed spline coefficient so that the default for long_mode on and long_mode off can be set separately.

Bugfixes
########

- multiple_scattering flag is now optional for Polaris focus when absorb_correction is true.
- Normalisation is fixed in :ref:`SumOverlappingTubes <algm-SumOverlappingTubes>`, which was causing very low peak to background ratio for reduced D2B data.

New
###

- :ref:`HB2AReduce <algm-HB2AReduce>` algorithm reduces HFIR POWDER (HB-2A) data
- :ref:`LoadGudrunOutput <algm-LoadGudrunOutput>` is a new algorithm that allows users to load the standard Gudrun output files into Mantid.

:ref:`Release 3.14.0 <v3.14.0>`
