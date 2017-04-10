==========================
Indirect Inelastic Changes
==========================

.. contents:: Table of Contents
   :local:

New features
------------

Algorithms
##########

- A new input property *RebinCanToSample* was added to :ref:`ApplyPaalmanPingsCorrection <algm-ApplyPaalmanPingsCorrection>` which enables or disables the rebinning of the empty container workspace.
- :ref:`FlatPlatePaalmanPingsCorrection <algm-FlatPlatePaalmanPingsCorrection>` and :ref:`CylinderPaalmanPingsCorrection <algm-CylinderPaalmanPingsCorrection>` algorithms are extended for `Efixed` mode, where the correction will be computed for a single wavelength point.
- :ref:`LoadVesuvio <algm-LoadVesuvio>` can now load NeXus files as well as raw files
- :ref:`BASISReduction311 <algm-BASISReduction311>` has been deprecated (2017-03-11). Use :ref:`BASISReduction <algm-BASISReduction>` instead.
- :ref:`BASISReduction <algm-BASISReduction>` includes now an option to compute and save the dynamic susceptibility.
- :ref:`VesuvioDiffractionReduction <algm-VesuvioDiffractionReduction>` has been deprecated, use :ref:`ISISIndirectDiffractionReduction <ISISIndirectDiffractionReduction>`

QuickRuns
~~~~~~~~~

- :ref:`IndirectDiffScan <algm-IndirectDiffScan>` to improve diffraction reduction workflow. Runs a diffraction reduction in diffspec mode with several parameters at fixed values.
- A new algorithm IndirectSampleChanger was added to work with the sample changer for IRIS

Data Reduction
##############

Calibration
~~~~~~~~~~~

- The range selector for resolution files is now dependent on the range of the spectrum, not the limit in the IPF


Data Analysis
#############

ConvFit
~~~~~~~

* All FABADA minimizer options are now accessible from the function browser.

- The Delta Function option can now be used with StretchedExpFT mode


Jump Fit
~~~~~~~~

Improvements
------------

- Bayes interfaces have the functionality to plot the current preview in the miniplot
- OSIRIS diffraction now rebins container workspaces to match the sample workspace
- :ref:`ISISIndirectDiffractionReduction <ISISIndirectDiffractionReduction>` now fully supports VESUVIO data
- Inelastic pixel ID's in BASIS instrument definition file grouped into continuous physical pixels.


Bugfixes
--------

- The *Diffraction* Interface no longer crashes when in OSIRIS diffonly mode
- *Abins*:  fix setting very small off-diagonal elements of b tensors
- Fix errors from calling Rebin from VisionReduction.
- Fixed validation of inputs in *CalculatePaalmanPings*

`Full list of changes on GitHub <http://github.com/mantidproject/mantid/pulls?q=is%3Apr+milestone%3A%22Release+3.10%22+is%3Amerged+label%3A%22Component%3A+Indirect+Inelastic%22>`_
