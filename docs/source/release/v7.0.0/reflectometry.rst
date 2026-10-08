=====================
Reflectometry Changes
=====================

New Features
------------
- (`#41868 <https://github.com/mantidproject/mantid/pull/41868>`_) The `orsopy <https://www.reflectometry.org/orsopy/history.html>`_ library dependency has been updated from version ``1.2.1`` to ``1.2.3``.
- (`#41637 <https://github.com/mantidproject/mantid/pull/41637>`_) A new plotting tab has been added to the :ref:`ISIS Reflectometry Interface <interface-isis-refl>`. It lists successful reduction outputs by group and run, identifies each item and output type, supports plotting selected workspaces individually, overlaid, or tiled, and shows output workspace groups with their member workspaces as selectable child entries. It also includes reflectivity, detector map, spin asymmetry, and alignment plot output presets.
- (`#41455 <https://github.com/mantidproject/mantid/pull/41455>`_) :ref:`algm-SaveISISReflectometryORSO` can now save ORSO files using metadata from workspace history, manual inputs, or history with manual overrides. The default metadata source remains workspace history, preserving the previous behaviour for existing scripts.
- (`#41455 <https://github.com/mantidproject/mantid/pull/41455>`_) The `IncludeAdditionalColumns` property of :ref:`algm-SaveISISReflectometryORSO` no longer controls writing the Q resolution column; use `WriteResolution` to include Q resolution and `IncludeAdditionalColumns` for wavelength and incident angle columns. The ISIS Reflectometry GUI keeps both options selected by default, preserving the previous default output.
- (`#41829 <https://github.com/mantidproject/mantid/pull/41829>`_) :ref:`ReflectometryISISCalibration <algm-ReflectometryISISCalibration>` now has an ``InstrumentWorkflow`` option. The ``POLREF`` workflow supports spectrum-number-indexed absolute theta calibration files, using the supplied experiment theta and ``SpecularPixelSpectrumNo`` to correct the incident angle and to subsquently align the detector geometry. It is of note that the current inverted POLREF calibration-map angle convention is expected.
- (`#41958 <https://github.com/mantidproject/mantid/pull/41958>`_) :ref:`FindReflectometryLines <algm-FindReflectometryLines-v3>` version 3 adds configurable background fitting, fit-window and fit-status handling, fallback behaviour, and optional profile and fit outputs. Unlike version 2, it returns the fractional workspace index only through the scalar ``LineCentre`` property and does not provide a single-valued ``OutputWorkspace``. Callers requiring the version 2 API can select it explicitly with ``FindReflectometryLines(..., Version=2, OutputWorkspace="position")``. :ref:`ReflectometryILLPreprocess <algm-ReflectometryILLPreprocess>`, :ref:`LoadILLReflectometry <algm-LoadILLReflectometry>`, and alignment plots in the :ref:`ISIS Reflectometry Interface <interface-isis-refl>` now use version 3.
- (`#42028 <https://github.com/mantidproject/mantid/pull/42028>`_) ``ReflectometryISISSumBanks``, :ref:`ReflectometryReductionOneAuto <algm-ReflectometryReductionOneAuto>`,
  and :ref:`ReflectometryISISLoadAndProcess <algm-ReflectometryISISLoadAndProcess>` can now apply ``ROIDetectorIDs`` bank summing to workspace groups.
- (`#42042 <https://github.com/mantidproject/mantid/pull/42042>`_) The ISIS Reflectometry Preview tab now supports workspace groups. Individual group members can be inspected while
  retaining detector and time-of-flight regions, masks are retained separately for each member, and the reduced
  reflectivity curves can be overplotted for comparison.
- (`#42064 <https://github.com/mantidproject/mantid/pull/42064>`_) :ref:`ReflectometryISISLoadAndProcess <algm-ReflectometryISISLoadAndProcess>` now utilises the POLREF calibration workflow as part of :ref:`ReflectometryISISCalibration <algm-ReflectometryISISCalibration>` through ``ReflectometryISISPreprocess``.
- (`#42131 <https://github.com/mantidproject/mantid/pull/42131>`_) Created a new version of :ref:`algm-ReflectometrySliceEventWorkspace-v2`, which produces one workspace group per slice when a workspace group is used as the ``InputWorkspace``.
  These output groups have the same structure as the input group so they can be used in workflows that require a certain set of member workspaces in a group, such as polarization corrections.
- (`#42388 <https://github.com/mantidproject/mantid/pull/42388>`_) The ``ReflectometryISISPreprocess`` algorithm no longer applies detector calibration. The
  :ref:`ISIS Reflectometry Interface <interface-isis-refl>` instead applies calibration through
  :ref:`ReflectometryISISLoadAndProcess <algm-ReflectometryISISLoadAndProcess>` after workspace summation. The detector
  image and TOF plot in the reduction preview now display the raw workspace instead of the calibrated workspace, but no
  visual impact is expected because calibration changes detector geometry rather than the plotted signal.
- (`#42423 <https://github.com/mantidproject/mantid/pull/42423>`_) ``POLREF_Parameters.xml`` now specifies ``CorrectDetectors=0``. This ensures that detector correction is turned off when reducing POLREF data through the ISIS Reflectometry GUI, as for POLREF the correction of detector positions is currently handled by :ref:`algm-ReflectometryISISCalibration`


Bugfixes
--------
- (`#41457 <https://github.com/mantidproject/mantid/pull/41457>`_) Modifications made to :ref:`algm-ReflectometryReductionOneAuto` and :ref:`algm-Stitch1DMany` so that all workspaces output from ISIS polarized Reflectometry reductions use consistent spin-state suffixes.
- (`#41950 <https://github.com/mantidproject/mantid/pull/41950>`_) The Search table in the ``Runs`` tab of the :ref:`ISIS Reflectometry <ISISReflectometryInterface>` interface now has a **Model** column to add the model description of the run.
- (`#41601 <https://github.com/mantidproject/mantid/pull/41601>`_) ISIS Reflectometry GUI stitching now uses the updated stitch algorithms so invalid values do not overwrite valid data from other workspaces in stitched overlap regions.
- (`#41814 <https://github.com/mantidproject/mantid/pull/41814>`_) The ``MantidORSODataset`` has been updated to make **model** and **validate** optional arguments.
- (`#42066 <https://github.com/mantidproject/mantid/pull/42066>`_) A new configuration property ``isisjournal.url_prefix`` has been added to ``Mantid.properties`` file to specify the URL prefix of the ISIS journal allowing it to be configurable as necessary whenever needed.
- (`#41996 <https://github.com/mantidproject/mantid/pull/41996>`_) Fixed a bug in the DataHandling library that caused files to become very large and extremely slow to load. This particulary effected the loading of processed FIGARO data.
- (`#42064 <https://github.com/mantidproject/mantid/pull/42064>`_) Fixed ISIS Reflectometry reductions reusing uncalibrated data from the ADS after a calibration file was selected. The requested calibration is now applied on the first reduction.

:ref:`Release 7.0.0 <v7.0.0>`
