==========================
Indirect Inelastic Changes
==========================

.. contents:: Table of Contents
   :local:

New features
------------

Algorithms
##########

- *EnergyWindowScan* has been renamed to :ref:`IndirectReductionAndAnalysis <algm-IndirectReductionAndAnalysis>`
- An option for Diffraction Reduction has been added to :ref:`IndirectReductionAndAnalysis <algm-IndirectReductionAndAnalysis>`
- A new input property *RebinCanToSample* was added to :ref:`ApplyPaalmanPingsCorrection <algm-ApplyPaalmanPingsCorrection>` which enables or disables the rebinning of the empty container workspace.
- :ref:`LoadVesuvio <algm-LoadVesuvio> can now load NeXus files as well as raw files

Data Analysis
#############

Conv Fit
~~~~~~~~

* All FABADA minimizer options are now accessible from the function browser.

Jump Fit
~~~~~~~~

Improvements
------------


Bugfixes
--------

- The *Diffraction* Interface no longer crashes when in OSIRIS diffonly mode
- *Abins*:  fix setting very small off-diagonal elements of b tensors

`Full list of changes on GitHub <http://github.com/mantidproject/mantid/pulls?q=is%3Apr+milestone%3A%22Release+3.10%22+is%3Amerged+label%3A%22Component%3A+Indirect+Inelastic%22>`_
