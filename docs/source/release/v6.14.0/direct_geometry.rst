=======================
Direct Geometry Changes
=======================

.. contents:: Table of Contents
   :local:

General
-------

New features
############
- Extended PyChop calculations by adding a chopper for SEQUOIA spectrometer.
- Added a new instrument geometry file for the CNCS spectrometer

Bugfixes
############
- Previously, Mantid could crash if the material set on a sample already existed rather than being created from scratch, improvements have now been made in an attempt to rectify the issues.
- Minor bug fix to increase the speed of masking in DGSPlanner
- DGSPlanner will now warn if the crystallographic convention is used


CrystalField
-------------

New features
############


Bugfixes
############



MSlice
------

New features
############
- ``mslice`` is now an optional dependancy of the ``mantidworkbench`` conda package and must be explicitly installed when required in a conda environment. Full/standalone installers and IDAaaS installations remain unchanged; mslice will be automatically provided there.

Bugfixes
############
- Setting the width of a cut to ``None`` will no longer prevent scripts from being generated.

:ref:`Release 6.14.0 <v6.14.0>`
