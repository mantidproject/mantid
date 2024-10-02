=======================
Direct Geometry Changes
=======================

.. contents:: Table of Contents
   :local:

General
-------

New features
############
- SaveNXSPE now stores the run number of the input workspace in the instrument section.

  For Indirect instrument it also saves efixed taken from detector's crystal analyzer properties.
  If all crystal analyzers have the same energy, single efixed is written
  into NXSPE_info->fixed_energy as before, while if detectors have different analyzer's energies,
  array of the energies is written into NXSPE_info->fixed_energy field of nxspe file.
  Also for Indirect instrument, no "fermi" field is added to the "instrument" folder.

  The algorithm have been also modified so if Ei is provided as the propery value,
  it always overrides the values, retrieved from the workspace.

Bugfixes
############
- Fixed a crash on ALFView when moving the cursor over the Instrument View while loading data.
- Random phase values are no longer used on Merlin when using :ref:`PyChop` with instrument scientist mode disabled.


CrystalField
-------------

New features
############


Bugfixes
############
- The method `cf.getMagneticMoment()` no longer ignores the `Hmag` field, which is now
  correctly considered in the calculations of the magnetic moment.


MSlice
------

New features
############


Bugfixes
############



:ref:`Release 6.11.0 <v6.11.0>`