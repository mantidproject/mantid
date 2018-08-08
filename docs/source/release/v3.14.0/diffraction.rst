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


- :ref:`SaveFocusedXYE <algm-SaveFocusedXYE>` will always set the value of
  Phi to 0 when writing MAUD headers. This resolves an issue where data was not
  displayed with a value of -90.0

- :ref:`SNAPReduce <algm-SNAPReduce>` now has progress bar and all output workspaces have history

:ref:`Release 3.14.0 <v3.14.0>`


Single Crystal Diffraction
--------------------------

Improvements
############

- :ref:`IntegratePeaksProfileFitting <algm-IntegratePeaksProfileFitting>` now supports MaNDi, TOPAZ, and CORELLI. Other instruments can easily be added as well.

Bugfixes
########

- :ref:`CentroidPeaksMD <algm-CentroidPeaksMD>` now updates peak bin counts.

