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

New Algorithms
##############
- New algorithm :ref:`SaveSampleEnvironmentAndShape <algm-SaveSampleEnvironmentAndShape>` to save out a sample shape and environment as a single STL file, this can be used to save out pieces with a set rotation or scale, or to assemble multiple stl files into one for viewing purposes.

Improvements
############

- The Gem scripts can now be used to automatically generate a .cal file, similar to pearl. They can also adjust a parameter file passed in using the argument "calibration_to_adjust".

- The Polaris scripts can now detect the chopper mode if none is provided using the frequency block logs.

- :ref:`SNSPowderReduction <algm-SNSPowderReduction>` has a new property, ``OffsetData``, which adds a constant to the data at the very end of the reduction.

- Added two new options to the HRPD scripts `do_solid_angle_corrections` and `subtract_empty_instrument` allowing setting whether or not these actions should take place.

- :ref:`BASISPowderDiffraction <algm-BASISPowderDiffraction>` has a new property, ``RemoveTemp``, which allows the user to inspect temporary workspaces is left unchecked.

- :ref:`LoadSampleEnvironment <algm-LoadSampleEnvironment>` now correctly takes into account scale for translation. Rotation is now applied before translation to reduce confusion.

- The Pearl scripts now automatically disable attenuation on long-mode.

- The Pearl scripts now set now use a spline coefficient of 5 on long-mode due to the increased amount of noise.

- New IDF have been added for VULCAN.

- The Pearl scripts now crop to a dspacing of 8 on long-mode to avoid negative values caused by noise after this point.

Bug Fixes
#########

- HRPD Absorption corrections now correctly takes into account the thickness of the slab.
- Pearl no longer produces an output of NaN when long-mode is changed after focusing.
- :ref:`LoadILLDiffraction <algm-LoadILLDiffraction>` if fixed to load also single point scans.
- :ref:`AlignAndFocusPowderFromFiles <algm-AlignAndFocusPowderFromFiles>` no longer errors out if the first chunk has no events

Engineering Diffraction
-----------------------

Improvements
############

- Changed focus to save out .his files in the format <run-number><instrument> as opposed to <run-number>_<instrument> to allow for better compatibility with opengenie.
- Added sample environment file for POWGEN that includes many of the standard sample containers

Bug Fixes
#########

- Prevented crash caused by canceling algorithms called by GUI.

- Prevented GUI breaking bug caused by entering files from the wrong instrument to calibration.

- Prevented issue with reading CSV files on python 3

- GUI now correctly loads the file browsed to instead of looking for a run number in every folder along the path to that file.

- :ref:`MDNorm <algm-MDNorm>` will not crash if the detector is masked in the flux workspace, but not in the input workspace.


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
- New TOPAZ instrument geometry for 2019B run cycle
- Maximum order of modulated vectors is now available to python: ws.sample().getOrientedLattice().getMaxOrder()

Bug Fixes
#########

- :ref:`MDNorm <algm-MDNorm>` now checks for consistent binning between the given parameters and the input accumulation workspaces if the latter are given.
- :ref:`StatisticsOfPeaksWorkspace <algm-StatisticsOfPeaksWorkspace>` now only calculates statistics for integer HKL (not satellite peaks) instead of combining. Statistics for satellite peaks will be added later.

Imaging
-------

:ref:`Release 4.1.0 <v4.1.0>`
