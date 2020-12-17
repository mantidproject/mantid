============
SANS Changes
============

.. contents:: Table of Contents
   :local:

Improvements
############

.. warning:: **Developers:** Sort changes under appropriate heading
    putting new features at the top of the section, followed by
    improvements, followed by bug fixes.

Algorithms and instruments
--------------------------

Improvements
############

- Added instrument definitions for the two new PSD based multi-panel SANS instruments D11B and D22B at the ILL.
- Added support for D11, D16, D22 and D33 in the :ref:`MaskBTP <algm-MaskBTP>` algorithm.
- Several improvements have been done in ILL SANS suite :ref:`SANSILLAutoProcess <algm-SANSILLAutoProcess>`:
    - The beam radius can be different for each distance.
    - A new parameter, TransmissionBeamRadius, has been added to set the beam radius for transmission measurements.
    - The default value of all the beam radii is now 0.1m.
    - If sample thickness is set to -1, the algorithm will try to get it from the nexus file itself.
    - The output workspace will get its title from the nexus file.
    - WavelengthRange is exposed to the algorithm, which is crucial for TOF reduction.
- The Rectangle option for :ref:`SolidAngle <algm-SolidAngle>` is now supported for ILL's D22 and D33.

Bugfixes
########

- Fixed a bug in ISIS SANS GUI where all changes to settings on the adjustment page would be ignored, so that
  it only used parameters from the user file instead.
- Fixed "Falsey" values such as 0.0 or False getting replaced with a default value in the ISIS SANS settings.
  For example, a Phi limit of 0.0 remains at 0.0 rather than defaulting back to -90
- Detector IDs are no longer copied during a 2D reduction. This also resolves
  a bug where the first two spectra were marked as monitors and would not appear
  in a colour fill plot on Workbench.
- Wavelength limits entered with comma ranges larger than 10, e.g. `1,5,10,15` no longer
  throw a Runtime Error.
- ISIS SANS will print the name of any missing maskfiles instead of an empty name.
- Fixed a bug in ISIS SANS GUI with the `Save Other` dialog.
- Fixed a bug in ISIS SANS GUI sum runs tab, where clicking browse and then cancelling the file picker dialog caused
  a crash.

:ref:`Release 6.0.0 <v6.0.0>`
