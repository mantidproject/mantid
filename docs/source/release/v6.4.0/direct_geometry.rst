=======================
Direct Geometry Changes
=======================

.. contents:: Table of Contents
   :local:

General
-------

New Features
############

- :ref:`CorrectTOFAxis <algm-CorrectTOFAxis>` can now accept fractional bin indices for more precise calculation of the elastic peak position calibration
- :ref:`DirectILLCollectData <algm-DirectILLCollectData>` will now accept fractional elastic peak reference bin as well as forward fractional elastic
  peak index to the :ref:`CorrectTOFAxis <algm-CorrectTOFAxis>`
- :ref:`DirectILLApplySelfShielding <algm-DirectILLApplySelfShielding>` now ensures that the subtracted container and self-attenuation correction workspaces
  have consistent binning with the provided sample to be corrected by rebinning to the sample
- :ref:`LoadPLN <algm-LoadPLN>` now supports loading ANSTO PELICAN data during event capture mid experiment as a cross check for long experiments.

Bugfixes
########

- :ref:`LoadILLTOF <algm-LoadILLTOF>` the sign of the half-channel width has been changed from negative to positive to ensure TOF axis is always positive.
- Minor bugfixes in the :ref:`PyChop <PyChop>` interface, to make sure the command line version works according to documentation.

Algorithms
----------

New Features
############

- :ref:`DirectILLAutoProcess <algm-DirectILLAutoProcess>` performs full data reduction treatment for ILL direct geometry instruments for empty container, vanadium, and sample, both single crystal and powder.
- :ref:`DirectILLCollectData <algm-DirectILLCollectData>` has two new properties: `GroupDetHorizontallyBy` and `GroupDetVerticallyBy` which allow for averaging pixel counts between the tubes and inside them, respectively, of for flat background calculation
- :ref:`PelicanReduction <algm-PelicanReduction>` add autoscaling the Q range to match the energy transfer as default in the UI.

Bugfixes
########



CrystalField
------------

New Features
############

- The :ref:`Crystal Field Python interface <Crystal Field Python Interface>` has been extended to include functions to calculate the x y and z components of
  the dipole transition maxtrix, ``getXDipoleMatrixComponent()``, ``getYDipoleMatrixComponent()`` and ``getZDipoleMatrixComponent()``.

Bugfixes
########



MSlice
------

New Features
############

- Various documentation updates
- MSlice now available as a noarch conda package
- Update to matplotlib 3.5
- Add ability to change slice plot font sizes using quick options
- Changed default cut algorithm to Integration

Bugfixes
########

- Fix for bug that left the menu for Default Energy Unit empty
- Fix for permanently enabled 'Show Legend' check boxes



:ref:`Release 6.4.0 <v6.4.0>`
