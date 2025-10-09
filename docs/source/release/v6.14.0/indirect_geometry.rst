=========================
Indirect Geometry Changes
=========================

.. contents:: Table of Contents
   :local:

New Features
------------



Bugfixes
--------



Algorithms
----------

New features
############
- *<prefix>-phonopy.yml* and *<prefix>-force_constants.hdf5* files
  from `janus-core <https://stfc.github.io/janus-core>`_ are now
  supported as input to Abins/Abins2D Algorithms. Use the
  FORCE_CONSTANTS "AbInitioProgram" as for regular *phonopy.yaml*
  files.
- Four output OutputSusceptibility file options (OutputSusceptibilityFrequencyNXS, OutputSusceptibilityEnergyNXS, OutputSusceptibilityFrequencyASCII, OutputSusceptibilityEnergyASCII) have been added in BASISReduction, instead of one.

Bugfixes
############
- Corrected the EMU loader to be able to support the new chopper installed July-2025 (:ref:`LoadEMU <algm-LoadEMU>`).
- Euphonic dependency is updated to 1.4.5. Notably, this includes a
  bugfix when importing force constants with dipole-dipole corrections
  from Phonopy: if the supercell matrix was non-symmetric, the long-range
  subtraction was done incorrectly. Calculations with "diagonal"
  supercells (e.g. 2×3×4) are unaffected, and primitive-FCC or
  primitive-BCC transformations should also use a symmetric matrix.

:ref:`Release 6.14.0 <v6.14.0>`
