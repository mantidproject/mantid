=======================
Direct Geometry Changes
=======================

.. contents:: Table of Contents
   :local:


ALFView
-------


Bugfixes
############
- An unreliable crash when loading a Sample run number has been fixed.


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
- Fixed several bugs with script generation regarding cut plots.
- Fixed a bug that was causing underlying workspaces to be saved instead of the intended workspace.
- Fixed an bug that was causing an exception to be thrown when plotting cuts from the command line.
- Recoil and Bragg line colours are now propagated to generated scripts.
- Fixed a bug that was causing Windows file paths to be interpreted as unicode in generated scripts.
- Fixed a bug that was causing an exception when renaming workspaces containing special characters.

:ref:`Release 6.8.0 <v6.8.0>`
