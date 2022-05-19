============
SANS Changes
============

.. contents:: Table of Contents
   :local:

New Features
------------

- ILL SANS reduction suite has been refactored, providing up to 2 orders of magnitude of speed-up for kinetic monochromatic measurements. The new algorithm :ref:`SANSILLMultiProcess <algm-SANSILLMultiProcess>` steers the reduction of the whole experiment, using the new version of the :ref:`SANSILLReduction-v2 <algm-SANSILLReduction-v2>`.
- :ref:`CalculateDynamicRange <algm-CalculateDynamicRange>` will now work also for workspaces where the x-axis unit is not wavelength, provided that the wavelength is present in the sample logs.
- :ref:`ParallaxCorrection <algm-ParallaxCorrection>` will now accept optional angle offsets per detector bank, to be subtracted from the scattering angles before evaluating the formulae.
- :ref:`ApplyTransmissionCorrection <algm-ApplyTransmissionCorrection>` no longer requires the input workspace to be histogram data in wavelengths.
- The SANS TOML format has been bumped from V0 to V1. This formally represents forwards compatibility of the .TOML format. All V1 files will continue to be supported in future versions of Mantid without requiring changes. See :ref:`sans_toml_v1-ref` for details.


Bugfixes
--------

- Merged mode now correctly saves can workspaces when "Save Can" is ticked
- The ISIS SANS Interface now will correctly find "txt", "Txt", "TXT", "toml", "Toml", and "TOML" extensions on case sensitive platforms
- The ISIS SANS row copy, cut and paste has been overhauled fixing numerous edge-cases. These include: the paste order being reversed, unexpected rows appearing or clearing whilst pasting and blank rows appearing randomly.
- The :ref:`SANSILLMultiProcess <algm-SANSILLMultiProcess>` now properly handles the RuntimError coming from user requesting masks that do not exist, with a relevant issue message displayed.

:ref:`Release 6.4.0 <v6.4.0>`
