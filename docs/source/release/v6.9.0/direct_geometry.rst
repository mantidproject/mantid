=======================
Direct Geometry Changes
=======================

.. contents:: Table of Contents
   :local:

General
-------

New features
############
- A new feature in PyChop which checks that the user input incident energy is within range for a particular instrument and chopper was added.

Bugfixes
############
- A bug in PyChop where opening the "Ascii Data Window" resets the internal state so that subsequent calculations become incorrect has been fixed.
- In the :ref:`QECoverage <QE Coverage>` tool, fixed crash and replaced multiple pop-up windows by warnings when invalid input is passed to Ei and Emin (and S2 in the case of HYSPEC).
- A crash when closing the ALFView interface while it is processing data has been fixed.
- Allow loading of buggy .nxspe files with incorrect fields or detector sizes


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
- When closing the MSlice interface in Mantid the Jupyter QtConsole is now cleaned up
- Upgrade from matplotlib 3.6 to 3.7

Bugfixes
############
- Fix for error thrown when trying to remove Bragg peaks.
- Fixed bug where data gets distorted from setting log scale to x axis and then setting it to linear again
- Deleted hidden workspaces are now also removed from the ADS to avoid memory issues
- Added import for Mantid algorithm wrappers back into the command line interface


DNS_Reduction
-------------

New features
############


Bugfixes
############
-  The legacy file format is now supported by the :ref:`DNS Reduction GUI<dns_reduction-ref>`.


:ref:`Release 6.9.0 <v6.9.0>`
