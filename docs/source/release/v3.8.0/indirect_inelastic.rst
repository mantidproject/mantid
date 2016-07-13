==========================
Indirect Inelastic Changes
==========================

.. contents:: Table of Contents
   :local:

New features
------------

Algorithms
##########

* Remove CylinderPaalmanPingsCorrection v1. This algorithm has been replaced by :ref:`CylinderPaalmanPingsCorrection <algm-CylinderPaalmanPingsCorrection>`

Data Analysis
#############

Elwin
~~~~~

- Additional option to ungroup Elwin output

Jump Fit
~~~~~~~~

Improvements
------------

- :ref:`LoadVesuvio <algm-LoadVesuvio>` now uses the whole TOF range for loaded monitor data (0-20000)
- Physical positions were included to the 311 reflection of BASIS instrument for improved instrument view.
- Algorithm :ref:`BASISReduction311 <algm-BASISReduction311>` has been included in algorithm :ref:`BASISReduction <algm-BASISReduction>`.
- Range bars colours in the *ISIS Calibration* interface have been updated to match the convention in the fit wizard.
- Vesuvio sigma_theta value updated for single and double differencing in both forward and back scattering. The new value is 0.016 for all.

Bugfixes
--------


* :ref:`IqtFitMultiple <algm-IqtFitMultiple>` no longer creates an unwanted temporary workspace when executed
* The documentation for :ref:`TransformToIqt <algm-TransformToIqt>` now correctly states that the ParameterWorkspace is a TableWorkspace
* Fix memory leak in :ref:`LoadSassena <algm-LoadSassena>`
* The *ResNorm* interface should no longer crash when using workspaces (rather than files) as input.
* Fix bug showing incorrect doublet peaks in :ref:`ISISIndirectDiffractionReduction <algm-ISISIndirectDiffractionReduction>`

`Full list of changes on GitHub <http://github.com/mantidproject/mantid/pulls?q=is%3Apr+milestone%3A%22Release+3.8%22+is%3Amerged+label%3A%22Component%3A+Indirect+Inelastic%22>`_
