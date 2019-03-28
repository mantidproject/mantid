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

Engineering Diffraction
-----------------------

Single Crystal Diffraction
--------------------------

New Algorithms
##############

- New algorithm :ref:`PredictSatellitePeaks <algm-PredictSatellitePeaks>` to predict satellite peaks using modulation vectors and maximum order of satellite from PeaksWorkspace or range of wavelength and DSpacing.

- New algorithm :ref:`IndexPeaksWithSatellites <algm-IndexPeaksWithSatellites>` to index peaks with satellites and set modulation vectors and maximum order of satellite from input values.


Improvements
############
- :ref:`LoadIsawPeaks <algm-LoadIsawPeaks>` will load satellite peaks using the order of each satellite by the hkl of the nuclear peak and the mnp of the satellite peak.

- :ref:`SaveIsawPeaks <algm-SaveIsawPeaks>` will save satellite peaks using the order of each satellite by the hkl of the nuclear peak and the mnp of the satellite peak.

- :ref:`FindUBUsingIndexedPeaks <algm-FindUBUsingIndexedPeaks>` finds UB matrix using the indexed peaks, modulation vectors and maximum order of satellite from PeaksWorkspace.

- :ref:`IndexPeaks <algm-IndexPeaks>` now will also index satellite peaks using modulation vectors and maximum order of satellite from PeaksWorkspace.

- :ref:`IntegrateEllipsoids <algm-IntegrateEllipsoids>` will integrate peaks using the indexed peaks, modulation vectors and maximum order of satellite from PeaksWorkspace

- :ref:`DeltaPDF3D <algm-DeltaPDF3D>` has a new method for peak removal, KAREN (K-space Algorithmic REconstructioN)


Imaging
-------

:ref:`Release 4.1.0 <v4.1.0>`
