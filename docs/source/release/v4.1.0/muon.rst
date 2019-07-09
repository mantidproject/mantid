============
MuSR Changes
============

.. contents:: Table of Contents
   :local:

.. warning:: **Developers:** Sort changes under appropriate heading
    putting new features at the top of the section, followed by
    improvements, followed by bug fixes.


:ref:`Release 4.1.0 <v4.1.0>`

New
###

* Frequency Domain Analysis GUI added to workbench.
* Muon ALC GUI added to workbench.
* Added phase tab for calculating :ref:`phase tables <algm-CalMuonDetectorPhases>` and :ref:`PhaseQuad <algm-PhaseQuad>` workspaces to Frequency Domain Analysis GUI.
* :ref:`Muon Analysis v2 <https://docs.mantidproject.org/nightly/interfaces/Muon%20Analysis%202.html>` added to MantidPlot and workbench.
* Elemental analysis interface added to workbench.

Improvements
############

* Phase table and phase Quad options from frequency domain transform tab moved to phase calculations tab.
* The new interface significantly improves stability and is designed to better handle multiple runs while being
  more intuitive to use.


Bug Fixes
#########

* Muon Analysis (original) no longer crashes when `TF Asymmetry` mode is activated.
* Muon Analysis (original) can now produce results tables when columns contain both ranges and single values.
* Frequency Domain Analysis GUI added to workbench.
