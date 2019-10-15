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

- The HRPD scripts now mask out the Bragg peaks from the Vanadium.
- The file-naming scheme for ISIS powder is now controlled by a string template
- The file-naming of output on HRPD as been updated to closely match old script outputs
- Geometry definition for LLB 5C1
- :ref:`SNAPReduce <algm-SNAPReduce-v1>` has an additional parameter ``MaxChunkSize`` for customizing the chunking behavior
- :ref:`LorentzCorrection <algm-LorentzCorrection-v1>` has an additional option for single crystal (default) or powder operation
- The create_total_scattering_pdf method in Polaris scripts now supports merging banks with a weighted mean.

Bug Fixes
#########

- The values used to mask the prompt pulse on HRPD have been fixed.
- :ref:`AlignAndFocusPowderFromFiles <algm-AlignAndFocusPowderFromFiles-v1>` will reload the instrument if logs are skipped
- Fixed issues with OptimizeLatticeForCellType, SelectCellOfType, SelectCellWithForm and TransformHKL when using modulated structures.

Engineering Diffraction
-----------------------

Single Crystal Diffraction
--------------------------

Improvements
############

- :ref:`SaveHKL <algm-SaveHKL>` now saves the tbar and transmission values for shapes and materials provided by :ref:`SetSample <algm-SetSample>`.


Bug Fixes
#########

- :ref:`IndexPeaksWithSatellites <algm-IndexPeaksWithSatellites>` & :ref:`IndexPeaks <algm-IndexPeaks>` have been fixed
  so that they correctly report the number of main & satellite reflections that have been indexed. Also, if a satellite
  peak cannot be indexed its HKL is reset to 000 rather than accidentally setting it to the HKL of the main reflection.

Imaging
-------

:ref:`Release 4.2.0 <v4.2.0>`
