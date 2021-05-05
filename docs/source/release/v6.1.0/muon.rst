============
MuSR Changes
============

.. contents:: Table of Contents
   :local:

.. warning:: **Developers:** Sort changes under appropriate heading
    putting new features at the top of the section, followed by
    improvements, followed by bug fixes.


Muon Analysis 2 and Frequency Domain Analysis
---------------------------------------------

New Features
############
- Added a new "periods" button to the grouping tab which displays a table of information on periods (currently only supporting nexus V1 files).

Improvements
############

Bug fixes
#########
- Fixed a bug where removing a pair in use would cause a crash.
- Fixed a bug where an error message would appear in workbench after loading a run in both MA and FDA.
- Fixed a bug where rows in the difference table were not being highlighted correctly.
- Fixed a bug in the Grouping tab where an error message would appear when changing the source of
  Group Asymmetry Range with no data loaded.

ALC
---

New Features
############

Improvements
############
- Exported workspaces now have history.
- The interface saves previous settings if possible instead of resetting.
- The interface can now load runs from different directories/cycles

Bug fixes
##########

Elemental Analysis
------------------

New Features
############

Bug fixes
#########
- Updated :ref:`LoadElementalAnalysisData <algm-LoadElementalAnalysisData>` algorithm to crop workspace.

Algorithms
----------
- Added the ability to specify the spectrum number in :ref:`FindPeaksAutomatic <algm-FindPeaksAutomatic>`.
- Added :ref:`PeakMatching <algm-PeakMatching>` algorithm.
- Added the ability to specify a Start and End X in :ref:`PSIBackgroundSubtraction <algm-PSIBackgroundSubtraction>`.
- Added the ability to specify an optional Function to add onto the end of the default function in :ref:`PSIBackgroundSubtraction <algm-PSIBackgroundSubtraction>`.

Fit Functions
-------------

:ref:`Release 6.1.0 <v6.1.0>`