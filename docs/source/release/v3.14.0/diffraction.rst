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
- Removed save_angles flag for Gem , as it was set by the texture mode
- Added save_all flag to Gem that is set to true by default, setting it to false disables the saving of .NXS files

Improvements
############

- :ref:`SNAPReduce <algm-SNAPReduce>` now has progress bar and all output workspaces have history
- :ref:`LoadWAND <algm-LoadWAND>` has grouping option added and loads faster
- Mask workspace option added to :ref:`WANDPowderReduction <algm-WANDPowderReduction>`

:ref:`Release 3.14.0 <v3.14.0>`


Single Crystal Diffraction
--------------------------

Improvements
############

- :ref:`IntegratePeaksProfileFitting <algm-IntegratePeaksProfileFitting>` now supports MaNDi, TOPAZ, and CORELLI. Other instruments can easily be added as well.

Bugfixes
########

- :ref:`CentroidPeaksMD <algm-CentroidPeaksMD>` now updates peak bin counts.

