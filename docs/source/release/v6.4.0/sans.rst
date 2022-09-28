============
SANS Changes
============

.. contents:: Table of Contents
   :local:

New Features
------------

- The ILL SANS reduction suite has been refactored, improving performance by two orders of magnitude for kinetic monochromatic measurements.
  The new algorithm, :ref:`SANSILLMultiProcess <algm-SANSILLMultiProcess>`, steers the reduction of the whole experiment, using the new version of :ref:`SANSILLReduction-v2 <algm-SANSILLReduction-v2>`.
- :ref:`CalculateDynamicRange <algm-CalculateDynamicRange>` can now accept workspaces where the x-axis unit is not wavelength, provided that the wavelength is present in the sample logs.
- :ref:`ParallaxCorrection <algm-ParallaxCorrection>` now accepts optional angle offsets per detector bank, which are subtracted from the scattering angles before evaluating the formulae.
- :ref:`ApplyTransmissionCorrection <algm-ApplyTransmissionCorrection>` no longer requires that the input workspace contains histogram data in wavelengths.
- The SANS TOML format has been bumped from V0 to V1. This formally represents forwards compatibility of the ``.TOML`` format. All V1 files will continue to be supported in future versions of Mantid without requiring changes. See :ref:`sans_toml_v1-ref` for details.


Bugfixes
--------

- Merged mode now correctly saves can workspaces when "Save Can" is ticked.
- The ISIS SANS interface will now correctly find ``.txt``, ``.Txt``, ``.TXT``, ``.toml``, ``.Toml``, and ``.TOML`` extensions on case-sensitive platforms.
- The copy, cut and paste functions for rows have been overhauled to fix numerous edge-cases in the ISIS SANS interface. These include: the paste order being reversed, unexpected rows appearing or clearing while pasting, and blank rows appearing randomly.
- The :ref:`SANSILLMultiProcess <algm-SANSILLMultiProcess>` algorithm now properly handles the ``RuntimeError`` that occurs when a mask that does not exist is requested, and displays a relevant error message.
- Fixed a bug handling invalid user files in the table. Previously the error reporter would appear, with a confusing error about a tuple index out of range. Now you get a sensible error message saying the file cannot be found.
- Fixed a bug in the :ref:`SANS GUI<ISIS_Sans_interface_contents>` where subsequent periods after the first for summed, multi-period runs were not saved.

:ref:`Release 6.4.0 <v6.4.0>`
