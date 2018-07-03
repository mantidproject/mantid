============
SANS Changes
============

.. contents:: Table of Contents
   :local:

* :ref:`LoadILLSANS <algm-LoadILLSANS>` is upgraded to load the modern nexus files from instruments D11, D22 and D33 at the ILL.
* EQSANS is configured for live data

ISIS SANS Interface
----------------------------

New
###
* A string of wavelength ranges can now be specified. A reduction is then done for each wavelength range.
* :ref:`SANSMask <algm-SANSMask>` is extended to have a `MaskedWorkspace` property, to copy the mask from.

Improvements
############
* Transmission workspaces are now output by default from the new GUI.
* The Beam centre finder now takes the default radius limits from the instrument parameter file if specified.
* Updated old backend to mask by detector ID rather than spectrum number, improving reliability. 
* Added EventSlice option to options column in new GUI.
* Added thickness column to table in new GUI.
* Added Radius Cutoff and Wavelength Cutoff boxes to the old and new GUI.
* Improved error messages in the new GUI to be more obvious and clearer.
* Updated the naming of workspace groups in a sliced reduction.
* Updated old backend to mask by detector ID rather than spectrum number, improving reliability.

Bugfixes
########
* The beam stop arm is now masked on LOQ for the new backend.
* Fixed a bug in the old backend where for LOQ the high angle bank was not being centered correctly in some cases.
* Userfiles specified in the batch file are now being loaded into the new GUI.
* The new sans GUI will now save out all the outputs of a time sliced reduction.
* Fixed a bug where save_format was not being specified if a user file was entered for a row.
* Use gravity now defaulting to false. 
* MASK/TIME and TIME/MASK now both work in new backend.

Features Removed
################

* Removed SaveNist as an option from both GUI's

:ref:`Release 3.13.0 <v3.13.0>`
