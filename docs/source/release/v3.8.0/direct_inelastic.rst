========================
Direct Inelastic Changes
========================

.. contents:: Table of Contents
   :local:

Improvements
------------

- :ref:`MDNormDirectSC <algm-MDNormDirectSC>` has an option to skip safety checks. This improves the speed when acting on workspace groups.

`Full list of changes on GitHub <http://github.com/mantidproject/mantid/pulls?q=is%3Apr+milestone%3A%22Release+3.8%22+is%3Amerged+label%3A%22Component%3A+Direct+Inelastic%22>`_

Crystal Field
-------------

- A fitting function was added (:ref:`CrystalFieldMultiSpectrum <func-CrystalFieldMultiSpectrum>`) that fits crystal field parameters to multiple spectra simultaneously.
- A preliminary python interface to the Crystal Field functionality was added. It includes classes for defining a problem, performing a fit and basic plotting facilities.
