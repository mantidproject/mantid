=========================
Indirect Geometry Changes
=========================

.. contents:: Table of Contents
   :local:

New Features
------------
- Added sample logs for multiple run reductions in :ref:`ISISIndirectEnergyTransfer <algm-ISISIndirectEnergyTransfer>`. `run_number` log stores the run numbers used in the reduction, separated by commas. `multi_run_reduction` log informs about the reduction done on chopped runs or regular runs.
- Added string with first and last run numbers in the reduced output workspace used with multiple runs in :ref:`ISISIndirectEnergyTransfer <algm-ISISIndirectEnergyTransfer>`.
- Added the ability to use `File` detector grouping from the :ref:`Indirect Diffraction interface <interface-indirect-diffraction>` for all instruments.
- Added the ability to use `Custom` detector grouping from the :ref:`Indirect Diffraction interface <interface-indirect-diffraction>` for all instruments.
- Added the ability to use `Groups` detector grouping from the :ref:`Indirect Diffraction interface <interface-indirect-diffraction>` for all instruments.


Bugfixes
--------
- Fixed an out-of-range error when running a :ref:`Indirect Data Reduction <interface-indirect-data-reduction>` on old TOSCA data.
- Fixed a bug where the wrong spectrum would be masked out for old TOSCA data. Masking detectors in the :ref:`ISISIndirectEnergyTransfer <algm-ISISIndirectEnergyTransfer>` algorithm was using workspace indices, which assume spectra do not shift in position after the mask is calculated, however this was not the case. Spectra indices are now used for masking detectors.
- Fixed an error on the :ref:`ISIS Energy Transfer tab <ISISEnergyTransfer>` caused by providing a number of detector groups which is larger than the number of spectra in a reduction.
- Fixed a bug where the EFixed when switching to the 'graphite' analyser would not be updated on the :ref:`Indirect Data Reduction <interface-indirect-data-reduction>` interface.


Algorithms
----------

New features
############
- Added the ability to specify a custom 'GroupingString' property to the :ref:`ISISIndirectDiffractionReduction <algm-ISISIndirectDiffractionReduction>` and :ref:`OSIRISDiffractionReduction <algm-OSIRISDiffractionReduction>` algorithms to be used for the grouping of detectors.
- Added the ability to specify a 'GroupingFile' property to the :ref:`ISISIndirectDiffractionReduction <algm-ISISIndirectDiffractionReduction>` and :ref:`OSIRISDiffractionReduction <algm-OSIRISDiffractionReduction>` algorithms to be used for the grouping of detectors.
- Added the ability to specify a 'NGroups' property to the :ref:`ISISIndirectDiffractionReduction <algm-ISISIndirectDiffractionReduction>` and :ref:`OSIRISDiffractionReduction <algm-OSIRISDiffractionReduction>` algorithms to be used for the grouping of detectors.
- The :ref:`algm-SaveAscii-v1` algorithm can now be found in the algorithm list using the ``SaveAsciiTOSCA`` alias.
- Added the ability to specify a 'NGroups' property to the :ref:`ISISIndirectEnergyTransfer <algm-ISISIndirectEnergyTransfer>` algorithm to be used for grouping detectors into a specified number of groups.

Bugfixes
############
- Fixed a bug in :ref:`algm-ISISIndirectDiffractionReduction` where the vanadium files were not being calibrated when a calibration file was provided for OSIRIS diffspec mode.

:ref:`Release 6.10.0 <v6.10.0>`