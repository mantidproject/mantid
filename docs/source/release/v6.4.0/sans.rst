============
SANS Changes
============

.. contents:: Table of Contents
   :local:

.. warning:: **Developers:** Sort changes under appropriate heading
    putting new features at the top of the section, followed by
    improvements, followed by bug fixes.

New
---

- ILL SANS reduction suite has been refactored, providing up to 2 orders of magnitude of speed-up for kinetic monochromatic measurements. The new algorithm :ref:`SANSILLMultiProcess <algm-SANSILLMultiProcess>` steers the reduction of the whole experiment, using the new version of the :ref:`SANSILLReduction-v2 <algm-SANSILLReduction-v2>`.
- :ref:`CalculateDynamicRange <algm-CalculateDynamicRange>` will now work also for workspaces where the x-axis unit is not wavelength, provided that the wavelength is present in the sample logs.
- :ref:`ParallaxCorrection <algm-ParallaxCorrection>` will now accept optional angle offsets per detector bank, to be subtracted from the scattering angles before evaluating the formulae.
- :ref:`ApplyTransmissionCorrection <algm-ApplyTransmissionCorrection>` no longer requires the input workspace to be histogram data in wavelengths.

Bugfixes
--------

- The ISIS SANS row copy, cut and paste has been overhauled fixing numerous edge-cases. These include: the paste order being reversed, unexpected rows appearing or clearing whilst pasting and blank rows appearing randomly.


:ref:`Release 6.4.0 <v6.4.0>`
