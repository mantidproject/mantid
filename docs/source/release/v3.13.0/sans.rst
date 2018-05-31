============
SANS Changes
============

.. contents:: Table of Contents
   :local:

.. warning:: **Developers:** Sort changes under appropriate heading
    putting new features at the top of the section, followed by
    improvements, followed by bug fixes.

ISIS SANS Interface
----------------------------

New features
############
* A string of wavelength ranges can now be specified. A reduction is then done for each wavelength range.
* :ref:`SANSMask <algm-SANSMask>` is extended to have a `MaskedWorkspace` property, to copy the mask from.

Improvements
############
* Updated old backend to mask by detector ID rather than spectrum number, improving reliability. 

* Added thickness column to table in new GUI.
* Added EventSlice option to options column in new GUI.
* Added Radius Cutoff and Wavelength Cutoff boxes to the old and new GUI.

Bug fixes
#########
* Fixed a bug where the beam stop arm was not being masked on LOQ.

Features Removed
################


:ref:`Release 3.13.0 <v3.13.0>`
