===================
Diffraction Changes
===================

.. contents:: Table of Contents
   :local:

.. warning:: **Developers:** Sort changes under appropriate heading
    putting new features at the top of the section, followed by
    improvements, followed by bug fixes.

:ref:`Release 3.14.0 <v3.14.0>`

Single Crystal Diffraction
--------------------------

New
###

- New algorithm :ref:`PredictSatellitePeaks <algm-PredictSatellitePeaks>` to predict satellite peaks using offset vectors and maximum order of satellite from PeaksWorkspace or range of wavelength and DSpacing.

- New algorithm :ref:`IndexPeakswithSatellites <algm-IndexPeakswithSatellites>` to index satellite peaks using offset vectors and maximum order of satellite from PeaksWorkspace.

- New algorithm :ref:`FindUBUsingIndexedPeakswithSatellites <algm-FindUBUsingIndexedPeakswithSatellites>` to find UB matrix using the indexed peaks, offset vectors and maximum order of satellite from PeaksWorkspace.

Improvements
############

- :ref:`LoadIsawPeaks <algm-LoadIsawPeaks>` will load satellite peaks using the offset vectors in the header and the order of each satellite by the hkl of the nuclear peak.

- :ref:`SaveIsawPeaks <algm-SaveIsawPeaks>` will save satellite peaks with the offset vectors in the header and the order of each satellite by the hkl of the nuclear peak.

- :ref:`IntegratePeaksProfileFitting <algm-IntegratePeaksProfileFitting>` now supports MaNDi, TOPAZ, and CORELLI. Other instruments can easily be added as well.
