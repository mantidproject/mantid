============
SANS Changes
============

.. contents:: Table of Contents
   :local:

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
    - There can be multiple inputs for sensitivity calculation. A new parameter, SensitivityWithOffsets, has been added
      to mark that these multiple sensitivities should be processed separately and specially merged before calculation
      of the efficiency to remove gaps caused by beam stop mask.
    - More than one transmission per processing, consistent with number of sample runs, are now accepted.
- The Rectangle option for :ref:`SolidAngle <algm-SolidAngle>` is now supported for ILL's D22 and D33.
- Added loader and MaskBTP support for D11B and D22B.

Bugfixes
########

- A bug has been fixed in ISIS SANS GUI where all changes to settings on the adjustment page would be ignored, so that
  it only used parameters from the user file instead.
- A bug has been fixed where "Falsey" values such as 0.0 or False were replaced with a default value in the ISIS SANS
  settings. For example, a Phi limit of 0.0 remains at 0.0 rather than defaulting back to -90.
- During a 2d rotation detector IDs are no longer copied. This also resolves a bug where the first two spectra were
  marked as monitors and would not appear in a colour fill plot on Workbench.
- A bug has been fixed where Wavelength limits entered with comma ranges larger than 10, e.g. `1,5,10,15` would throw a
  Runtime Error.
- A bug has been Fixed in ISIS SANS GUI where the `Save Other` dialog caused an error.
- ISIS SANS will print the name of any missing mask files instead of an empty name.
- A bug has been fixed in the ISIS SANS GUI sum runs tab, where clicking browse and then cancelling the file picker
  dialog caused a crash.
- Fixes a bug in :ref:`SaveNXcanSAS <algm-SaveNXcanSAS>` where a bin-edge transmission array would be saved instead points
  if the input workspace was a histogram. :ref:`LoadNXcanSAS <algm-LoadNXcanSAS>` has been modified to load existing files
  that might contain bin-edge transmissoin data.

:ref:`Release 6.0.0 <v6.0.0>`
