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
- Fixed a bug in PyChop where opening the "Ascii Data Window" caused subsequent calculations to be incorrect.
- Fixed a crash in the :ref:`QECoverage <QE Coverage>` tool.
- Replaced multiple pop-up windows with logger warnings in the :ref:`QECoverage <QE Coverage>` tool when invalid input is passed to Ei and Emin (and S2 in the case of HYSPEC).
- Fixed a crash when closing the ALFView interface while it is still processing data.
- The loading of .nxspe files with incorrect fields or detector sizes is now allowed.

MSlice
------

New features
############
- When closing the MSlice interface in Mantid the Jupyter QtConsole is now cleaned up.
- Upgraded the matplotlib version from 3.6 to 3.7.

Bugfixes
############
- Fixed an error when trying to remove Bragg peaks.
- Fixed a bug where data gets distorted from setting log scale to x axis and then setting it to linear again.
- Deleted hidden workspaces are now also removed from the ADS to avoid memory issues.
- Added import for Mantid algorithm wrappers back into the command line interface.
- Fixed a bug that caused crashes when saving a cut workspace with a changed intensity correction to Mantid Workbench.

DNS_Reduction
-------------

Bugfixes
############
-  The legacy file format is now supported by the :ref:`DNS Reduction GUI<dns_reduction-ref>`.


:ref:`Release 6.9.0 <v6.9.0>`
