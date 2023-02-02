=====================
Reflectometry Changes
=====================

.. contents:: Table of Contents
   :local:

New Features
------------
- :ref:`ReflectometryISISLoadAndProcess <algm-ReflectometryISISLoadAndProcess>` can now take a polarization efficiencies workspace to make polarization corrections. This workspace can be set via the ``Polarization Efficiencies`` combo box on the ``Experiment Settings`` tab.
- A new graphical tool has been added to the ISIS Reflectometry interface to allow interactive selection of various regions of interest. It shows an on-the-fly preview showing the effect of the selections on the reduced reflectivity curve. It also adds the ability to select and sum across segments on a 2D detector. See the :ref:`Reduction Preview <refl_preview>` documentation for more detail.
- It is now possible to provide a calibration file on the Instrument Settings tab of the ISIS Reflectometry interface. The calibration step will be applied as part of pre-processing when a reduction is run or previewed. See :ref:`ReflectometryISISLoadAndProcess <algm-ReflectometryISISLoadAndProcess>` for more details.
- :ref:`LoadILLReflectometry <algm-LoadILLReflectometry>` can now load NeXus files v3 from D17 and FIGARO measurements from cycle 192 onwards.
- Gravity correction has been implemented for FIGARO data reduction in :ref:`ReflectometryILLPreProcess <algm-ReflectometryILLPreProcess>` and :ref:`ReflectometryILLConvertToQ <algm-ReflectometryILLConvertToQ>`.
- :ref:`ReflectometryILLAutoProcess <algm-ReflectometryILLAutoProcess>` now outputs configuration .out file containing a summary of the performed reduction.
- :ref:`ReflectometryILLAutoProcess <algm-ReflectometryILLAutoProcess>` does not have the `UseManualScaleFactors` boolean property to decide whether to use provided manual scale factors. The redundancy has been removed and now when the factors are provided, they will be used in :ref:`Stitch <algm-Stitch>` algorithm.
- `default-incident-monitor-spectrum` has been corrected for D17 from 257 to 256, allowing for proper monitor normalisation for data from cycle 211 and beyond.
- It is now possible to save the content of the ``Search Runs`` table into a ``.csv`` file, allowing the table to be used as a logbook.

Bugfixes
--------
- Fixed an issue where adding a new batch for instruments other than INTER causes settings on any existing batch tabs to be cleared.
- :ref:`ReflectometryILLSumForeground <algm-ReflectometryILLSumForeground>` now properly sums the foreground of the direct beam over the same range as the reflected beam, which fixes observed issues with reflectivity calculation.
- Fixed an issue where the live data output would update during processing of a new chunk of live data; it now only updates with the final IvsQ result, avoiding the axes changing during processing.
- Fix a bug where the live data monitor would stop running when it encounters a transmission run. This was because the reduction would fail when the angle is zero.
- Fixed an issue where Mantid could crash when workspaces were cleared from the ADS after deleting a batch from the ISIS Reflectometry GUI.
- Stop using the value of the ScaleRHSWorkspace parameter of Stitch1DMany in favour of the IndexOfReference parameter.

:ref:`Release 6.6.0 <v6.6.0>`