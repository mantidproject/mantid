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
- OSIRIS diffraction now rebins container workspaces to match the sample workspace

Bugfixes
--------

- The *Diffraction* Interface no longer crashes when in OSIRIS diffonly mode
- *Abins*:  fix setting very small off-diagonal elements of b tensors
- Fix errors from calling Rebin from VisionReduction.

`Full list of changes on GitHub <http://github.com/mantidproject/mantid/pulls?q=is%3Apr+milestone%3A%22Release+3.10%22+is%3Amerged+label%3A%22Component%3A+Indirect+Inelastic%22>`_
