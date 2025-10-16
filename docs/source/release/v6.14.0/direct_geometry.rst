=======================
Direct Geometry Changes
=======================

.. contents:: Table of Contents
   :local:

General
-------

New features
############
- Extended PyChop calculations by adding a chopper for the SEQUOIA spectrometer.
- Added a new instrument geometry file for the CNCS spectrometer.

Bugfixes
############
- Minor bug fix to increase the speed of masking in :ref:`DGSPlanner <dgsplanner-ref>`.
- :ref:`DGSPlanner <dgsplanner-ref>` will now warn if the crystallographic convention is used.


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
- A temperature log cache has been implemented, which will be used by default for subsequent slices to set the temperature. Also added is a menu item, which is available when a relevant intensity is chosen, permitting the manual setting of this option.
- From this point forward, pre-release versions of MSlice will be identified by the upcoming release version number, the “dev” suffix, and the GitHub commit number.

Bugfixes
############
- Setting the width of a cut to ``None`` will no longer prevent scripts from being generated.
- The correct axes will now be maintained when exporting cuts from MSlice to the Mantid workbench.
- The error bars for cuts with GDOS corrected intensity, with energy units of cm-1, now display the proper dimensions.

:ref:`Release 6.14.0 <v6.14.0>`
