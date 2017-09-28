========================
Direct Inelastic Changes
========================

.. contents:: Table of Contents
   :local:

New features
------------

Algorithms
##########

- Data reduction workflow algorithms for ILL's TOF spectrometers IN4, IN5 and IN6 have been added to Mantid. A guide to the six new algorithms is provided :ref:`here <DirectILL>`.
- :ref:`algm-DetectorEfficiencyCorUser` now accepts component-specific efficiency correction formulas.

`Full list of changes on GitHub <http://github.com/mantidproject/mantid/pulls?q=is%3Apr+milestone%3A%22Release+3.11%22+is%3Amerged+label%3A%22Component%3A+Direct+Inelastic%22>`_

Improvements
------------

Algorithms
##########

- A bug was fixed in :ref:`DPDFReduction <algm-DPDFReduction>` to comply with the signature of one of the underlying C-functions.
- :ref:`algm-ComputeCalibrationCoefVan` uses :ref:`algm-IntegrateEPP` as its backend instead of manual summation when integrating the vanadium peak.

Crystal Field
#############

- Added new fitting :ref:`function <func-CrystalFieldFunction>` that calculates crystal field spectra and physical properties for a multi-site case.

Other changes
#############

- New :math:`T_0`  formula for CNCS instrument

Instrument definitions
----------------------

- The detector efficiency correction formulas for ILL's IN4, IN5 and IN6 spectrometers have been revised. IN4 has now separate formulas for the low and large angle detectors while the previously incorrect IN5 and IN6 formulas were fixed.
