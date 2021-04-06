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

Fit Functions
-------------

:ref:`Release 6.1.0 <v6.1.0>`