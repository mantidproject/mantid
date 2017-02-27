============
SANS Changes
============

.. contents:: Table of Contents
   :local:

Features
---------

- Make added multi-period event files available for the ISIS SANS reduction.
- Added NXcanSAS as an output format of the ISIS SANS Gui.
- Added MASK/LINE for LOQ. This allows for masking of the beam stop arm.

Bug Fixes
---------

|

- Fix for LARMOR multi-period loading. The initial rotation was not correctly applied to all child workspaces.
- IDF bug when using Larmor in batch mode was resolved.
- Issue where Gui changes were not picked up for batch reductions was resolved.
- Remove SaveNexusProcessed and SaveCSV as an option. Reorder options by dimensionality.
- Fix for merged reduction with phi masking.
- Fix SAScollimation issue in SaveCanSAS1D and LoadCanSAS1D.
- Improved CropToComponent and removed detector ordering.
- Fix for faulty beam stop arm masking for certain angles.

X uncertainties (delta-Q)
-------------------------

- X uncertainties (Dx) are now always stored as one value per *bin*, i.e., not as one value per *bin edge*.
- When loading legacy Nexus files (files with Dx data that were stored with previous Mantid versions) the last Dx value is ignored.


`Full list of changes on github <http://github.com/mantidproject/mantid/pulls?q=is%3Apr+milestone%3A%22Release+3.9%22+is%3Amerged+label%3A%22Component%3A+SANS%22>`__
