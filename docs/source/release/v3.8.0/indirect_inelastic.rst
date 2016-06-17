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

Jump Fit
~~~~~~~~

Improvements
------------

- Physical positions were included to the 311 reflection of BASIS instrument for improved instrument view.
- Range bars colours in the *ISIS Calibration* interface have been updated to match the convention in the fit wizard.

Bugfixes
--------


* :ref:`IqtFitMultiple <algm-IqtFitMultiple>` no longer creates an unwanted temporary workspace when executed
* The documentation for :ref:`TransformToIqt <algm-TransformToIqt>` now correctly states that the ParameterWorkspace is a TableWorkspace
* Fix memory leak in :ref:`LoadSassena <algm-LoadSassena>`
* The *ResNorm* interface should no longer crash when using workspaces (rather than files) as input.

`Full list of changes on GitHub <http://github.com/mantidproject/mantid/pulls?q=is%3Apr+milestone%3A%22Release+3.8%22+is%3Amerged+label%3A%22Component%3A+Indirect+Inelastic%22>`_
