=======================
Direct Geometry Changes
=======================

.. contents:: Table of Contents
   :local:

General
-------

New features
############
- In :ref:`PyChop`, the phase offset for the disk chopper controller on MERLIN can now be modified through the yaml file for MERLIN. Furthermore, the default value has been changed from `1500` to `17000`.

Bugfixes
############
- Fixed a bug that prevented being able to `Show detectors` for LET data.


MSlice
------

Bugfixes
############
- Saving a .nxs file as .nxspe or vice versa no longer causes a crash. Instead, a warning message is now being shown.
- Cut plots with GDOS intensity no longer have empty lines in the MDHisto tab and so no longer cause a crash when deleting these.

:ref:`Release 6.10.0 <v6.10.0>`
