=======================
Direct Geometry Changes
=======================

.. contents:: Table of Contents
   :local:

General
-------

New features
############
- :ref:`SaveNXSPE <algm-SaveNXSPE-v1` now stores the run number of the input workspace in the instrument section.

  For Indirect instruments it also saves ``efixed`` taken from detector's crystal analyzer properties.
  If all crystal analyzers have the same energy, a single ``efixed`` is written into NXSPE_info->fixed_energy as before,
  while if detectors have different analyzer's energies an array of the energies is written into NXSPE_info->fixed_energy field of the ``.nxspe`` file.
  Also for Indirect instruments, no ``fermi`` field is added to the ``instrument`` folder.
  The algorithm has also been modified so if ``Ei`` is provided as the propery value it always overrides the values retrieved from the workspace.

Bugfixes
############
- :ref:`ALFView <ALFView-ref>` not longer crashs when moving the cursor over the Instrument View while loading data.
- :ref:`PyChop` no longer uses random phase values on Merling when instrument scientist mode disabled.


CrystalField
-------------

Bugfixes
############
- The method `cf.getMagneticMoment()` no longer ignores the `Hmag` field, which is now
  correctly considered in the calculations of the magnetic moment.


:ref:`Release 6.11.0 <v6.11.0>`
