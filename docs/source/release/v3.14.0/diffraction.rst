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

Improvements
############

- :ref:`LoadILLDiffraction <algm-LoadILLDiffraction>` will not flip the even-numbered tubes when using the calibrated data, since they are flipped already in the nexus files.
- :ref:`PowderDiffILLDetScanReduction <algm-PowderDiffILLDetScanReduction>` will scale the counts by 1M, when normalisation to monitor is requested, and it will also offer to enable/disable the tube alignment, and offer tube by tube reduction.
- :ref:`PowderDiffILLDetEffCorr <algm-PowderDiffILLDetEffCorr>` now offers to use the raw or calibrated data blocks in the nexus files.
- :ref:`SNAPReduce <algm-SNAPReduce>` now has progress bar and all output workspaces have history
- :ref:`AlignAndFocusPowder <algm-AlignAndFocusPowder>` and :ref:`AlignAndFocusPowderFromFiles <algm-AlignAndFocusPowderFromFiles>` now support outputting the unfocussed data and weighted events (with time). This allows for event filtering **after** processing the data.
- :ref:`LoadWAND <algm-LoadWAND>` has grouping option added and loads faster
- Mask workspace option added to :ref:`WANDPowderReduction <algm-WANDPowderReduction>`

:ref:`Release 3.14.0 <v3.14.0>`


Single Crystal Diffraction
--------------------------

Improvements
############

- :ref:`IntegratePeaksProfileFitting <algm-IntegratePeaksProfileFitting>` now supports MaNDi, TOPAZ, and CORELLI. Other instruments can easily be added as well.
- :ref:`MDNormSCD <algm-MDNormSCD>` now can handle merged MD workspaces.

Bugfixes
########

- :ref:`CentroidPeaksMD <algm-CentroidPeaksMD>` now updates peak bin counts.

- :ref:`FindPeaksMD <algm-FindPeaksMD>` now finds peaks correctly with the crystallography convention setting and reduction with crystallography convention is tested with a system test.

Total Scattering
----------------

Improvements
############

- :ref:`LoadGudrunOutput <algm-LoadGudrunOutput>` is a new algorithm that allows users to load the standard Gudrun output files into Mantid.
