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
- ``<prefix>-phonopy.yml`` and ``<prefix>-force_constants.hdf5`` files from `janus-core <https://stfc.github.io/janus-core>`_ are now supported as input to :ref:`Abins <algm-Abins>`/:ref:`Abins2D <algm-Abins2D>` Algorithms. Use the FORCE_CONSTANTS "AbInitioProgram" for regular ``phonopy.yaml`` files.
- Four output "OutputSusceptibility" file options (``OutputSusceptibilityFrequencyNXS``, ``OutputSusceptibilityEnergyNXS``, ``OutputSusceptibilityFrequencyASCII``, ``OutputSusceptibilityEnergyASCII``) have been added in :ref:`algm-BASISReduction`, instead of one.
- Support is added to :ref:`Abins <algm-Abins>`/:ref:`Abins2D <algm-Abins2D>` to import .mol data files for Molden. These can be output by CP2K calculations, but do not include mass/isotope information.

Bugfixes
############
- Corrected :ref:`LoadEMU <algm-LoadEMU>` to be able to support the new chopper installed July-2025.
- Euphonic dependency is updated to 1.4.5. Notably, this includes a bugfix when importing force constants with dipole-dipole corrections from Phonopy: if the supercell matrix was non-symmetric, the long-range subtraction was done incorrectly. Calculations with "diagonal" supercells (e.g. 2×3×4) are unaffected, and primitive-FCC or primitive-BCC transformations should also use a symmetric matrix.

:ref:`Release 6.14.0 <v6.14.0>`
