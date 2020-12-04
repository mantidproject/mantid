============
MuSR Changes
============

.. contents:: Table of Contents
   :local:

=======

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
- Fixed a bug where ties and constraints were not being respected.
- Fixed a bug to swap start and end time fit properties on the interface if start > end
- Fixed a bug where editing constraints would result in a crash

ALC
---

New Features
############
- The data loading section has been updated for this release. See :ref:`Muon ALC <MuonALC-ref>` for more.

Improvements
############
- Added an x label to the plot in data loading

Bug fixes
##########
- Stopped scientific notation when plotting run numbers on x axis

Elemental Analysis
------------------

New Features
############

Bug fixes
#########

Algorithms
----------
- :ref:`algm-LoadElementalAnalysisData` algorithm was introduced for loading runs for the new Elemental Analysis GUI, enabling it to be registered by WorkspaceHistory.
- The functions RemoveExpDecay and EstimateMuonAsymmetryFromCounts were modified to use point data instead of bin edges for removing the exponential.

:ref:`Release 6.0.0 <v6.0.0>`
