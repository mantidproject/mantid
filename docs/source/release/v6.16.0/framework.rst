=================
Framework Changes
=================

Algorithms
----------

New features
############
- (`#40714 <https://github.com/mantidproject/mantid/pull/40714>`_) :ref:`algm-SCDCalibratePanels-v2` now writes the full name for bank components.
- (`#40824 <https://github.com/mantidproject/mantid/pull/40824>`_) Include IMAGINE instrument in :ref:`algm-MaskBTP`.
- (`#40952 <https://github.com/mantidproject/mantid/pull/40952>`_) MPI Algorithms :ref:`BroadcastWorkspace <algm-BroadcastWorkspace>` and :ref:`GatherWorkspaces <algm-GatherWorkspaces>` have been added to mantid to allow for MPI processing that is workspace friendly. These are available on linux in the ``mantidmpi`` package.
- (`#41137 <https://github.com/mantidproject/mantid/pull/41137>`_) :ref:`algm-HFIRPowderReduction` now has logic for loading MIDAS data
-  :ref:`algm-HFIRPowderReduction` now logs warnings for unset optional fields.
- (`#41294 <https://github.com/mantidproject/mantid/pull/41294>`_) Algorithm :ref:`algm-CreateDetectorTable` now has option `OneRowPerDetectorID`, which ensures a new row in the Detector Table is created for each detector id. The rows become ordered by the component index of the detector ids, instead of being ordered by workspace index. In workspaces that have grouped detectors, there will be several rows with the same workspace index.
- (`#41320 <https://github.com/mantidproject/mantid/pull/41320>`_) The `Rebin` algorithm will now throw an error if the number of bins requested is expected to exceed available memory.
- (`#41321 <https://github.com/mantidproject/mantid/pull/41321>`_) A new c++ algorithm type ``TaskBasedAlgorithm`` has been added. Such an algorithm consists of tasks, linked via an input-output dependency structure. A user can control the workflow by specifying a custom task execution order.
- (`#41334 <https://github.com/mantidproject/mantid/pull/41334>`_) Algorithm :ref:`algm-PolarizationCorrectionWildes` now accepts an input workspace group as an alternative to a string list of workspace names.
-  :ref:`algm-HFIRPowderReduction` now does absorption correction for the sample and vanadium using :ref:`CylinderAbsorptionCW <algm-CylinderAbsorptionCW>`

Bugfixes
############
-  Updated the :ref:`SaveSESANS <algm-SaveSESANS>` and :ref:`SaveCanSAS1D <algm-SaveCanSAS1D>` algorithms to include the decimal point if the data written to file is an integer.
-  :ref:`ConjoinWorkspaces <algm-ConjoinWorkspaces>` no longer has a fencepost error when determining spectrum numbers.
- (`#40755 <https://github.com/mantidproject/mantid/pull/40755>`_) :ref:`LoadEmptyInstrument <algm-LoadEmptyInstrument>` will now load instruments from a wider class of NeXus files.
- (`#40817 <https://github.com/mantidproject/mantid/pull/40817>`_) :ref:`FileFinder <mantid.api.FileFinderImpl>` has been update to more reliably determine the instrument from the hint particularly when the instrument name contains digits.
- (`#41053 <https://github.com/mantidproject/mantid/pull/41053>`_) Fix crash in :ref:`SaveDiffCal <algm-SaveDiffCal>` produced by calibration tables with repeated entries.
- (`#40855 <https://github.com/mantidproject/mantid/pull/40855>`_) Fix performance issue in :ref:`MaskDetectors <algm-MaskDetectors>` using a ``MatrixWorkspace`` as the source of the mask
- (`#40963 <https://github.com/mantidproject/mantid/pull/40963>`_) Algorithm :ref:`LoadEventAsWorkspace2D <algm-LoadEventAsWorkspace2D>` now updates the run duration log when using
  parameters ``FilterByTimeStart`` and/or ``FilterByTimeStop``. The behaviour is now consistent with
  :ref:`LoadEventNexus <algm-LoadEventNexus>` when using the same parameters.
- (`#40963 <https://github.com/mantidproject/mantid/pull/40963>`_) Loading with :ref:`LoadEventAsWorkspace2D <algm-LoadEventAsWorkspace2D>` and
  :ref:`LoadEventNexus <algm-LoadEventNexus>` with time filtering when there are banks with no events in that time
  range no longer causes the loading to hang due to an infinite loop.
- (`#40856 <https://github.com/mantidproject/mantid/pull/40856>`_) The :ref:`GenerateFlatCellWorkspaceLOQ <algm-GenerateFlatCellWorkspaceLOQ>` algorithm has been updated so that the output is prepended with the monitor spectrums. An option to save the output using :ref:`SaveRKH <algm-SaveRKH>` has also been added.
- (`#41039 <https://github.com/mantidproject/mantid/pull/41039>`_) The :ref:`SumSpectra <algm-SumSpectra>` algorithm has been updated to correctly propagate errors for weighted sum calculations.
- (`#41131 <https://github.com/mantidproject/mantid/pull/41131>`_) `EnggEstimateFocussedBackground` will no longer throw an error related to writing to a read-only destination.
- (`#41150 <https://github.com/mantidproject/mantid/pull/41150>`_) :ref:`MaskBTP <algm-MaskBTP>` now supports name and short-name for instrument IMAGINE.
-  Fixed a bug in :ref:`WeightedMeanOfWorkspace <algm-WeightedMeanOfWorkspace>` where output errors were computed as :math:`\sqrt{\mathrm{weightSum}}` instead of :math:`1/\sqrt{\mathrm{weightSum}}`. This affects :ref:`DgsReduction <algm-DgsReduction>` (including the Absolute Units tab in the GUI), DPDFReduction, and :ref:`MDNorm <algm-MDNorm>` workflows. **Warning:** Results from previous releases may contain inflated uncertainties and should be reprocessed.
- (`#41215 <https://github.com/mantidproject/mantid/pull/41215>`_) The algorithm `LoadEventNexus` when loading a single bank on the TOPAZ or MANDI instruments
    will no longer appear to hang, and will no longer throw an error on completion.
    The loaded instrument will contain only the specified bank, with no other components.
- (`#41256 <https://github.com/mantidproject/mantid/pull/41256>`_) Algorithm `LoadMask` will now throw an error if given an invalid file.
- (`#41259 <https://github.com/mantidproject/mantid/pull/41259>`_) The algorithm `BASISReduction` has been updated to throw an error if a mask workspace cannot be found.
- (`#41330 <https://github.com/mantidproject/mantid/pull/41330>`_) Rebinning operations (such as in ``Rebin`` or ``RebinRagged``) that specify a range with a start and end value that are not in the correct order (start < end) will now throw an exception instead of silently producing incorrect results. This is to prevent users from accidentally specifying invalid rebinning parameters and getting unexpected outputs.
- (`#41343 <https://github.com/mantidproject/mantid/pull/41343>`_) The algorithm `BinMD` will throw an error if the total number of bins requested is larger than the available memory.
- (`#41343 <https://github.com/mantidproject/mantid/pull/41343>`_) The algorithm `SliceMD` will throw an error if the total number of bins requested is larger than the available memory.
- (`#41343 <https://github.com/mantidproject/mantid/pull/41343>`_) The algorithm `MDNorm` will throw an error if the total number of bins requested is larger than the available memory.
-  Fix symmetry bug in :ref:`MDNorm <algm-MDNorm>`

Deprecated
############

Removed
############

Fit Functions
-------------

New features
############
- (`#41167 <https://github.com/mantidproject/mantid/pull/41167>`_) :ref:`IkedaCarpenterPV <func-IkedaCarpenterPV>` peak function now supports analytical derivative calculation, for more information see :ref:`IkedaCarpenterPV <func-IkedaCarpenterPV-derivative>`.
- (`#41416 <https://github.com/mantidproject/mantid/pull/41416>`_) The `PDF` parameter of the :ref:`FABADA Minimizer <FABADA>` has been changed from a bool to a string. This allows the user to specify the name of the output workspace for the PDF.

Bugfixes
############

Deprecated
############

Removed
############


Data Objects
------------

New features
############
- (`#40761 <https://github.com/mantidproject/mantid/pull/40761>`_) Sample Shapes generated from ``.STL`` mesh files can now be saved and loaded from nexus files.
-  Added ``getMemorySize()`` to :py:class:`~mantid.geometry.Instrument`, :py:class:`~mantid.geometry.DetectorInfo`, and :py:class:`~mantid.geometry.ComponentInfo` to report the memory footprint of instrument geometry objects in bytes.

Bugfixes
############
- (`#41072 <https://github.com/mantidproject/mantid/pull/41072>`_) adds persistence and round-trip support for the MonitorCount field on peaks
  when saving/loading Mantid "processed" NeXus peak workspaces.


Python
------

New features
############
-  :py:obj:`SplittersWorkspaces <mantid.dataobjects.SplittersWorkspace>` can now be created directly from python.
- (`#40748 <https://github.com/mantidproject/mantid/pull/40748>`_) ADARA-packet replay utility: for ORNL-facility instruments which use the ADARA system, it is now possible
  to record and playback an ADARA-packet stream.
- (`#41086 <https://github.com/mantidproject/mantid/pull/41086>`_) First batch of concepts pages migrated to corresponding API pages

Bugfixes
############
- (`#41288 <https://github.com/mantidproject/mantid/pull/41288>`_) Loading :ref:`Python extensions<04_loading_extensions_on_startup>` does not allow to import hidden folders or Python files with protected names to avoid crashing Mantid on startup.


Dependencies
------------------

New features
############
- (`#40737 <https://github.com/mantidproject/mantid/pull/40737>`_) Add support for markdown in developer docs using `myst-parser <https://myst-parser.readthedocs.io/en/latest/index.html>`_
- (`#40766 <https://github.com/mantidproject/mantid/pull/40766>`_) Add :ref:`mantidprofiler <AlgorithmProfiler>` as a developer dependency on linux
- (`#40771 <https://github.com/mantidproject/mantid/pull/40771>`_) The memory allocator set by ``LD_PRELOAD`` has been change from jemalloc to tbbmalloc.
- (`#40973 <https://github.com/mantidproject/mantid/pull/40973>`_) Add information for writing docs using MyST flavored markdown
- (`#41058 <https://github.com/mantidproject/mantid/pull/41058>`_) Upgraded python to 3.12. See the changes made to python `here <https://docs.python.org/3/whatsnew/3.12.html>`__.
- (`#41017 <https://github.com/mantidproject/mantid/pull/41017>`_) Remove dependency ``pytz`` to prefer using python's built-in ``zoneinfo`` for time zone information
- (`#41034 <https://github.com/mantidproject/mantid/pull/41034>`_) Remove ``python-dateutil`` in favor of python's ``datetime`` module
- (`#41071 <https://github.com/mantidproject/mantid/pull/41071>`_) New configuration files have been added for GitHub Copilot.
-  Move to `euphonic <https://github.com/pace-neutrons/Euphonic>`_ v1.6.0
- (`#41153 <https://github.com/mantidproject/mantid/pull/41153>`_) Updated Matplotlib from version 3.9 to version 3.10. The release notes for `version 3.10 can be found in <https://matplotlib.org/stable/users/prev_whats_new/whats_new_3.10.0.html>`_.

Bugfixes
############


Nexus
-----

Bugfixes
############
- (`#41354 <https://github.com/mantidproject/mantid/pull/41354>`_) `LoadNexusLogs` can once again load string logs from Nexus files as in some old (ca. 2016) CORELLI data files, where the string was stored with `DataType` length of 1, and the `DataSpace` having rank 1 with the correct size in the first dimension.


MantidWorkbench
---------------

See :doc:`mantidworkbench`.

:ref:`Release 6.16.0 <v6.16.0>`
