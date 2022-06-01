=======================
Direct Geometry Changes
=======================

.. contents:: Table of Contents
   :local:

General
-------

New Features
############

- :ref:`CorrectTOFAxis <algm-CorrectTOFAxis>` can now accept fractional bin indices for more precise calculation of the elastic peak position calibration.
- :ref:`DirectILLCollectData <algm-DirectILLCollectData>` will now accept fractional elastic peak reference bins as well as forward fractional elastic
  peak indexes in the :ref:`CorrectTOFAxis <algm-CorrectTOFAxis>` algorithm.
- :ref:`DirectILLApplySelfShielding <algm-DirectILLApplySelfShielding>` now ensures that the subtracted container and self-attenuation correction workspaces
  have consistent binning by rebinning to the sample that will be corrected.
- :ref:`LoadPLN <algm-LoadPLN>` now supports the loading of ANSTO PELICAN data during event capture mid-experiment, as a cross check for long experiments.

Bugfixes
########

- The sign of the half-channel width in the :ref:`LoadILLTOF <algm-LoadILLTOF>` algorithm has been changed from negative to positive to ensure the TOF axis is always positive.
- The :ref:`PyChop <PyChop>` interface has been adjusted to ensure that the command-line version functions as described in the documentation.

Algorithms
----------

New Features
############

- :ref:`DirectILLAutoProcess <algm-DirectILLAutoProcess>` now performs the full data reduction treatment for ILL direct geometry instruments for an empty container, vanadium, and sample, both for single crystal and powder.
- :ref:`DirectILLCollectData <algm-DirectILLCollectData>` has two new properties: ``GroupDetHorizontallyBy`` and ``GroupDetVerticallyBy`` which allow for averaging pixel counts between the tubes and inside them, respectively, or for flat background calculations.
- Autoscaling has been added to the :ref:`PelicanReduction <algm-PelicanReduction>` algorithm to ensure that the Q range matches the energy transfer, as is default in the UI.

Bugfixes
########



CrystalField
------------

New Features
############

- The :ref:`Crystal Field Python interface <Crystal Field Python Interface>` has been extended to include functions to calculate the x, y, and z components of
  the dipole transition matrix: ``getXDipoleMatrixComponent()``, ``getYDipoleMatrixComponent()`` and ``getZDipoleMatrixComponent()``.

Bugfixes
########



MSlice
------

New Features
############

- The documentation has been improved to include more up-to-date screenshots and more information about cutting methods.
- The programme is now available as a noarch conda package.
- MSlice is now compatible with ``matplotlib 3.5.0``.
- Slice plot font sizes can now be changed using the quick options.
- The default cut algorithm has been changed to ``Integration``.

Bugfixes
########

- The Default Energy Unit menu is no longer empty.
- The ``Show Legend`` check boxes now appear with the correct state.


:ref:`Release 6.4.0 <v6.4.0>`
