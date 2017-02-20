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
- :ref:`FlatPlatePaalmanPings <algm-FlatPlatePaalmanPings>` and :ref:`CylinderPaalmanPings <algm-CylinderPaalmanPings>` are extended with `Efixed` option, where the correction is computed for a single wavelength.

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

`Full list of changes on GitHub <http://github.com/mantidproject/mantid/pulls?q=is%3Apr+milestone%3A%22Release+3.10%22+is%3Amerged+label%3A%22Component%3A+Indirect+Inelastic%22>`_
