=======================
Direct Geometry Changes
=======================

.. contents:: Table of Contents
   :local:

Algorithms
----------
New Features
############
- :ref:`LoadDNSEvent <algm-LoadDNSEvent>` loads data from DNS PSD detector into an ``EventWorkspace``.

Improvements
############
- The default value of monitor peak width multiplier (``MonitorPeakWidthInSigmas``) has been changed from 3 to 7 in :ref:`DirectILLCollectData <algm-DirectILLCollectData>` .

Bugfixes
########
- Updated instrument created by :ref:`LoadNXSPE <algm-LoadNXSPE>` is viewable in instrument 3D view.

Interfaces
----------
Improvements
############
- The :ref:`PyChop <PyChop>` GUI now has the ability to handle multiple independently phased choppers.

CrystalField
------------
Improvements
############
- The :ref:`Crystal Field Python interface <Crystal Field Python Interface>` has two new fitting functions, ``two_step_fit`` and ``two_step_fit_sc``, alternating optimization over field parameters and peak parameters. One function is based on the Mantid fitting for both parts, the other uses ``scipy.optimize.minimize`` for the field parameters.

MSlice
------
BugFixes
########
- Fixed a bug that caused empty plot windows and crashes when running scripts generated from plot windows.
- Fixed bug that prevented the print button from opening the print dialog.
- Fixed bug that caused duplicated colour bars for slice plots and exceptions for cut plots when running a generated script while the original plot is still open.
- Fixed bug in script generation for cut plots.
- Cleaned up File menu for interactive cut plots.
- Enabled editing for Bragg peaks on cut plots.
- Prevented an exception when generating script from plot created via script.
- Added legends for recoil lines and Bragg peaks on slice plots.
- Fixed a bug that caused exceptions when scaling a workspace.
- Added an error message when attempting to load a file by path on the data loading tab.
- Fixed a bug that caused infinitely repeating energy unit conversions when changing the default energy unit.
- When closing the dialog for adding a Bragg peak from a ``CIF`` file without selecting a ``CIF`` file, the corresponding menu entry now remains unselected.


:ref:`Release 6.3.0 <v6.3.0>`
