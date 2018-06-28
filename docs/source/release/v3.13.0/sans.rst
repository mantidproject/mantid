============
SANS Changes
============

.. contents:: Table of Contents
   :local:

* :ref:`LoadILLSANS <algm-LoadILLSANS>` is upgraded to load the modern nexus files from instruments D11, D22 and D33 at the ILL.

ISIS SANS Interface
----------------------------

New features
############
* A string of wavelength ranges can now be specified. A reduction is then done for each wavelength range.
* :ref:`SANSMask <algm-SANSMask>` is extended to have a `MaskedWorkspace` property, to copy the mask from.
* EQSANS is configured for live data

Improvements
############
* Transmission workspaces are now output by default from the new GUI.
* The Beam centre finder now takes the default radius limits from the instrument parameter file if specified.
* Updated old backend to mask by detector ID rather than spectrum number, improving reliability. 
* Added EventSlice option to options column in new GUI.
* Added thickness column to table in new GUI.
* Added EventSlice option to options column in new GUI.
* Added Radius Cutoff and Wavelength Cutoff boxes to the old and new GUI.

Bug fixes
#########
* Fixed a bug where the beam stop arm was not being masked on LOQ for the new backend.
* Fixed a bug in the old backend where for LOQ the high angle bank was not being centered correctly in some cases.
* Fixed a bug where userfiles specified in the batch file were not being loaded into the new GUI.

Features Removed
################

:ref:`Release 3.13.0 <v3.13.0>`
