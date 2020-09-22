=======================
Direct Geometry Changes
=======================

.. contents:: Table of Contents
   :local:

ALF View
########

Improvements
------------
- Internal unit tests have been added to the GUI ensuring greater stability.

Algorithms
##########

Improvements
------------

- For ILL time-of-flight instruments, :ref:`LoadILLTOF <algm-LoadILLTOF-v2>` now allows bin edges determined by channel number by default, but you can still convert to time-of-flight using the `ConvertToTOF` property.
- In the The incident energy calibration has been fixed for PANTHER and IN6 in :ref:`DirectILLCollectData <algm-DirectILLCollectData-v1>`.

Bugfixes
--------

- A bug introduced in v5.0 causing error values to tend to zero on multiple instances of :ref:`Rebin2D <algm-Rebin2D>` on the same workspace has been fixed.
- A bug in the validation of temporary workspaces for :ref:`MDNorm <algm-MDNorm>` has been fixed.Temporary workspaces can now be used in any order when slicing.
- Fixed bugs in the DGSPlanner interface, that cause errors when using negative values.
- SliceViewer can now correctly display non-orthogonal axes for output of :ref:`MDNorm <algm-MDNorm>`.

MSlice
######

Bugfixes
--------

- A bug causing an exception when attempting to overplot workspaces with different axes has been fixed.
- A crash when clicking cancel while renaming or adding a workspace has been fixed.

:ref:`Release 5.1.0 <v5.1.0>`
