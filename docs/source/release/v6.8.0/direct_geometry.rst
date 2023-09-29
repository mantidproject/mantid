=======================
Direct Geometry Changes
=======================

.. contents:: Table of Contents
   :local:


CrystalField
-------------


Bugfixes
############
- Prevent segmentation fault by only accessing FWHM values when the corresponding vector is not empty.


MSlice
------

New features
############
- Upgraded Python from version 3.8 to 3.10.

BugFixes
########
- Stopped storing GDOS cuts in the ADS.
- Fixed a bug that caused incorrect error propagation in GDOS intensity corrections.


:ref:`Release 6.8.0 <v6.8.0>`