============
SANS Changes
============

.. contents:: Table of Contents
   :local:

Improvements
############


Algorithms and instruments
--------------------------

Improvements
############

- In the :ref:`SANSILLAutoProcess <algm-SANSILLAutoProcess>` algorithm, the beam radius can made be different for each
  distance with the ``TransmissionBeamRadius`` parameter. The default value of all beam radii is now 0.1m.
- In the :ref:`SANSILLAutoProcess <algm-SANSILLAutoProcess>` algorithm, if sample thickness is set to -1, the algorithm
  will try to get it from the nexus file.
- In the :ref:`SANSILLAutoProcess <algm-SANSILLAutoProcess>` algorithm, the output workspace will get its title from the
  nexus file.
- The :ref:`SANSILLAutoProcess <algm-SANSILLAutoProcess>` algorithm now utilises the WavelengthRange for TOF reduction.
- The Rectangle option in the :ref:`SolidAngle <algm-SolidAngle>` algorithm is now supported for ILL's D22 and D33.
- The :ref:`SANSILLAutoProcess <algm-SANSILLAutoProcess>` algorithm nopw accepts more than one transmission per
  processing, this is consistent with number of sample runs.
- Support has been added for D11, D16, D22 and D33 in the :ref:`MaskBTP <algm-MaskBTP>` algorithm.
- Instrument definitions have been added for the two new PSD based multi-panel SANS instruments D11B and D22B at the ILL.
- The Rectangle option for :ref:`SolidAngle <algm-SolidAngle>` is now supported for ILL's D22 and D33.
- Loader and MaskBTP support have been added for D11B and D22B.

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
- ISIS SANS will print the name of any missing maskfiles instead of an empty name.
- A bug has been fixed in the ISIS SANS GUI sum runs tab, where clicking browse and then cancelling the file picker
  dialog caused a crash.

:ref:`Release 6.0.0 <v6.0.0>`
