============
SANS Changes
============

.. contents:: Table of Contents
   :local:

Bug Fixes
---------
- Fixed wrong first spectrum number for LARMOR. The first non-monitor spectrum number is 11, but it had been set to 10.
- Fixed inconsistent detector selection for high-angle-bank-type detectors.
- Fixed LOQ Batch mode bug where geometry information was not saved out when using SaveCanSAS1D.
- Fixed LOQ Batch mode bug where custom user file without a .txt ending was not being picked up.
- Fixed Batch mode bug where the output name suffix was hardcoded to SANS2D. It now takes the individual instruments into account.
- Fixed LOQ bug where prompt peak was not set correctly for monitor normalisation.

`Full list of changes on github <http://github.com/mantidproject/mantid/pulls?q=is%3Apr+milestone%3A%22Release+3.10%22+is%3Amerged+label%3A%22Component%3A+SANS%22>`__
