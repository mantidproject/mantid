=======================
Direct Geometry Changes
=======================

.. contents:: Table of Contents
   :local:

ALF View
--------

Improvements
############
- Internal unit tests have been added to the GUI ensuring greater stability.

Algorithms
----------

Improvements
############

- For ILL time-of-flight instruments, the :ref:`LoadILLTOF <algm-LoadILLTOF-v2>` algorithm now
  allows bin edges determined by channel number by default, but you can still convert to time-of-flight
  using the `ConvertToTOF` property.
- In the incident energy calibration has been fixed for PANTHER and IN6 in the
  :ref:`DirectILLCollectData <algm-DirectILLCollectData-v1>` algorithm.

Bugfixes
########

- A bug introduced in v5.0 causing error values to tend to zero on multiple instances of the
  :ref:`Rebin2D <algm-Rebin2D>` algorithm on the same workspace has been fixed.
- A bug in the validation of temporary workspaces for the :ref:`MDNorm <algm-MDNorm>` algorithm
  has been fixed. Temporary workspaces can now be used in any order when slicing.
- Fixed bugs in the DGSPlanner interface, that cause errors when using negative values.
- SliceViewer can now correctly display non-orthogonal axes for output of the :ref:`MDNorm <algm-MDNorm>` algorithm.

MSlice
------

Bugfixes
--------

- A bug causing an exception when attempting to overplot workspaces with different axes has been fixed.
- A crash when clicking cancel while renaming or adding a workspace has been fixed.

:ref:`Release 5.1.0 <v5.1.0>`
