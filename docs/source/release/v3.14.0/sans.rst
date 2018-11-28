============
SANS Changes
============

.. contents:: Table of Contents
   :local:

New
###

- :ref:`SANSILLReduction <algm-SANSILLReduction>` performs SANS data reduction for ILL instruments D11, D22, D33.
- :ref:`SANSILLIntegration <algm-SANSILLIntegration>` performs integration of corrected SANS data to produce I(Q), I(Phi,Q) or I(Qx,Qy).

ISIS SANS Interface
-------------------

Improved
########
* Updated workspace naming scheme for new backend.
* Added shortcut keys to copy/paste/cut rows of data.
* Added shortcut keys to delete or add rows.
* Added tabbing support to table.
* Added error notifications on a row by row basis.
* Updated file adding to prefix the instrument name
* Updated file finding to be able to find added runs without instrument name prefix
* Updated GUI code so calculated merge scale and shift are shown after reduction.
* Removed instrument selection box. Instrument is now determined by user file.
* Automatically remembers last loaded user file
* Added display of current save directory

Bug fixes
#########
* Fixed an issue where the run tab was difficult to select.
* Changed the geometry names from CylinderAxisUP, Cuboid and CylinderAxisAlong to Cylinder, FlatPlate and Disc
* The GUI now correctly resets to default values when a new user file is loaded.
* The GUI no longer hangs whilst searching the archive for files.
* Updated the options and units displayed in wavelength and momentum range combo boxes.
* Fixed a bug which crashed the beam centre finder if a phi mask was set.

Improvements
############

- :ref:`Q1DWeighted <algm-Q1DWeighted>` now supports the option of asymmetric wedges for unisotropic scatterer.

:ref:`Release 3.14.0 <v3.14.0>`
