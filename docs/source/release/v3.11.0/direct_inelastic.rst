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

Algorithms
##########

- A bug was fixed in :ref:`DPDFReduction <algm-DPDFReduction>` to comply with the signature of one of the underlying C-functions.

Other changes
#############

- New T0 formula for CNCS instrument
