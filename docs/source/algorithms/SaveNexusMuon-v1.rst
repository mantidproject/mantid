.. algorithm::

.. summary::

.. relatedalgorithms::

.. properties::

Description
-----------

Saves a muon workspace to a NeXus file in the ISIS Muon NeXus V2 (HDF5) format. The saved file can be loaded again
with :ref:`Load <algm-Load>`, which uses :ref:`LoadMuonNexusV2 <algm-LoadMuonNexusV2>`, and opened in the Muon
Analysis and Frequency Domain Analysis interfaces.

The usual way to use the algorithm is to save a workspace loaded from a muon file, after any changes such as removing
bad counts or editing sample logs. Pass the ``TimeZero``, ``DeadTimeTable`` and ``DetectorGroupingTable`` outputs of
:ref:`Load <algm-Load>` to the matching properties so they are kept in the saved file.

Time zero and good data range
#############################

By default :ref:`Load <algm-Load>` subtracts time zero from the time axis (``CorrectTime=True``) and stores the
``FirstGoodData`` sample log relative to time zero. ``TimeZero`` must always be set. If ``TimeZeroCorrected`` is true,
which is the default, ``TimeZero`` is added back to the time axis and to the ``FirstGoodData`` log before saving. If
the workspace was loaded with ``CorrectTime=False``, set ``TimeZeroCorrected`` to false.

``FirstGoodData`` can also be given directly. By default it is measured from time zero, as shown in the Muon Analysis
interface. To give it from the start of the raw time axis instead, as returned by the ``FirstGoodData`` output of
:ref:`Load <algm-Load>`, set ``GoodDataRelativeToTimeZero`` to false. For example, if time zero is 0.213
microseconds and first good data is 0.107 microseconds after it, give ``FirstGoodData=0.107``, or
``FirstGoodData=0.32`` with ``GoodDataRelativeToTimeZero=False``. First good data before time zero is rejected.

Workspaces do not record the last good data, so to keep it, pass the ``LastGoodData`` output of
:ref:`Load <algm-Load>` with ``GoodDataRelativeToTimeZero=False``, or give it measured from time zero. It must be
after the first good data and not after the end of the time axis. If it is not given, the data is good up to the end
of the time axis, as in ISIS muon files.

The file stores first and last good data as bin numbers, in units of the resolution, which is the width of the first
time bin. The values saved are rounded to the nearest whole bin, but the first good bin is never before time zero and
the last good bin is never after the end of the time axis.

What is saved
#############

======================================= ===============================================================================
Field in the file                       Source
======================================= ===============================================================================
``counts``                              The counts of each spectrum. Errors are not saved: they are recalculated as
                                        the square root of the counts on loading.
``raw_time``                            The time bin edges, with ``TimeZero`` added back if ``TimeZeroCorrected`` is
                                        true.
``resolution``                          The width of the first time bin.
``time_zero``                           The ``TimeZero`` property.
``first_good_bin``                      The ``FirstGoodData`` property, or the ``FirstGoodData`` sample log.
``last_good_bin``                       The ``LastGoodData`` property, or the end of the time axis.
``dead_time``                           The ``DeadTimeTable`` property. Detectors not in the table, or all detectors
                                        if no table is given, have a dead time of zero.
``grouping``                            The ``DetectorGroupingTable`` property. If no table is given, grouping is not
                                        saved and loading uses the default grouping of the instrument.
``orientation``                         The ``MainFieldDirection`` property, or the ``main_field_direction`` sample log.
``sample/temperature``,                 The ``sample_temp`` and ``sample_magn_field`` sample logs.
``sample/magnetic_field``
``good_frames``, ``run_number``,        The ``goodfrm``, ``run_number``, ``run_start`` and ``run_end`` sample logs.
``start_time``, ``end_time``
``periods``                             The ``period_*`` sample logs, or the values for a single period.
``runlog``                              Every time series sample log of numbers or text. Logs that loading creates
                                        itself, such as ``running`` and ``periods``, are not saved.
======================================= ===============================================================================

The proton charge, raw frames and experiment identifier are taken from the ``gd_prtn_chrg``, ``tot_prtn_chrg``,
``rawfrm`` and ``rb_proposal`` sample logs. Workspaces loaded from older muon files do not have these logs, so they are
saved as zero or blank. Muon analysis does not use them.

Restrictions on the input workspace
###################################

- Only single-period data can be saved. To save multi-period data, load and save each period separately using the
  ``EntryNumber`` property of :ref:`Load <algm-Load>`.
- The workspace must contain histogram data whose time bins are the same for every spectrum, in microseconds.
- The counts must be non-negative whole numbers, as in raw muon data. Data that has been dead time corrected,
  normalised or rebinned to fractional counts cannot be saved.
- Each spectrum must contain a single detector whose ID equals its spectrum number.
- The workspace must have an instrument and the ``run_number``, ``run_start``, ``run_end``, ``goodfrm``,
  ``sample_temp`` and ``sample_magn_field`` sample logs, which are needed to load the file.

Usage
-----

**Example - Save a muon run and load it again**

.. testcode:: ExSaveNexusMuonRoundTrip

   import os

   loaded = Load("EMU00102347.nxs_v2", DeadTimeTable="dead_times", DetectorGroupingTable="grouping")
   file_path = os.path.join(config["defaultsave.directory"], "EMU00102347_saved.nxs")
   SaveNexusMuon(InputWorkspace=loaded.OutputWorkspace, Filename=file_path, TimeZero=loaded.TimeZero,
                 DeadTimeTable="dead_times", DetectorGroupingTable="grouping")

   reloaded = Load(file_path, DeadTimeTable="saved_dead_times", DetectorGroupingTable="saved_grouping")
   print("The saved file has {} spectra".format(reloaded.OutputWorkspace.getNumberHistograms()))
   print("Time zero is {:.2f} microseconds".format(reloaded.TimeZero))
   print("First good data is {:.3f} microseconds".format(reloaded.FirstGoodData))

.. testcleanup:: ExSaveNexusMuonRoundTrip

   os.remove(file_path)

Output:

.. testoutput:: ExSaveNexusMuonRoundTrip

   The saved file has 96 spectra
   Time zero is 0.16 microseconds
   First good data is 0.384 microseconds

.. categories::

.. sourcelink::
