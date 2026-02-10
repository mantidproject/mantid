=================
Framework Changes
=================

.. contents:: Table of Contents
   :local:

Algorithms
----------

New features
############
- (`#40367 <https://github.com/mantidproject/mantid/pull/40367>`_)
  `UnaryOperation <https://github.com/mantidproject/mantid/blob/main/Framework/Algorithms/src/UnaryOperation.cpp>`_
  algorithms like :ref:`ReplaceSpecialValues <algm-ReplaceSpecialValues>`, :ref:`Logarithm <algm-Logarithm>` now support
  ragged workspaces.
- Several algorithms have had flowcharts added to document their processing steps:

  - (`#40063 <https://github.com/mantidproject/mantid/pull/40063>`_) :ref:`algm-LoadWANDSCD`
  - (`#40120 <https://github.com/mantidproject/mantid/pull/40120>`_) :ref:`algm-ConvertWANDSCDtoQ`
  - (`#40234 <https://github.com/mantidproject/mantid/pull/40234>`_) :ref:`algm-ConvertHFIRSCDtoMDE`

- :ref:`algm-AlignAndFocusPowderSlim` has some new parameters:

  - (`#40067 <https://github.com/mantidproject/mantid/pull/40067>`_)
    ``OutputSpectrumNumber``, which allows a single bank of data to be loaded.
  - (`#40122 <https://github.com/mantidproject/mantid/pull/40122>`_)
    ``ProcessBankSplitTask``, which can split its output into multiple workspaces when set to ``True``.
  - (`#40377 <https://github.com/mantidproject/mantid/pull/40377>`_)
    ``UseFullTime``, which can split events based on full time (pulse time+TOF) when ``True``
  - (`#40669 <https://github.com/mantidproject/mantid/pull/40669>`_)
    ``ReadSizeFromDisk``, ``EventsPerThread``, and ``LogBlockList`` are existing parameters with new defaults to provide
    faster execution.

- (`#40493 <https://github.com/mantidproject/mantid/pull/40493>`_) :ref:`algm-AlignAndFocusPowderSlim` will now set the
  uncertainty values on the output workspace.
- (`#40107 <https://github.com/mantidproject/mantid/pull/40107>`_) :ref:`algm-HFIRPowderReduction` is a new algorithm,
  which is currently just the UI elements for loading and reducing data.
- (`#40128 <https://github.com/mantidproject/mantid/pull/40128>`_) :ref:`algm-CreateDetectorTable` now supports the
  option ``PickOneDetectorID``. This returns a column of detector IDs as a column of integers containing only one
  detector ID per row. By default this option is disabled, so by default the column of detector IDs will consist of
  strings of one or more detector ID per row. This new option was introduced to be used in the background of the new
  instrument view interface, but might also be useful if the user wants a single detector ID per row.
- (`#40227 <https://github.com/mantidproject/mantid/pull/40227>`_) :ref:`algm-CreateGroupingByComponent` is a new
  algorithm that allows for the creation of ``GroupingWorkspaces``, where the groups are formed by combining components
  under a given parent component. Components which should be combined are identified by providing strings to indicate
  what their names should/should not contain. Each of these groups can be optionally subdivided into a further,
  ``GroupSubdivision``, number of groups.
- (`#40327 <https://github.com/mantidproject/mantid/pull/40327>`_) :ref:`algm-ReorientUnitCell` is a new algorithm that
  allows the selection of the unit cell most aligned to goniometer axes.
- (`#40332 <https://github.com/mantidproject/mantid/pull/40332>`_) :ref:`algm-AddLogSmoothed` is also new, and allows
  for the addition of smoothed time-series log data.
- (`#40450 <https://github.com/mantidproject/mantid/pull/40450>`_) :ref:`algm-AddLogInterpolated` is new, and provides a
  method for interpolating time series sample logs.
- (`#40420 <https://github.com/mantidproject/mantid/pull/40420>`_) :ref:`algm-LoadNGEM` has been extended to allow for
  data to be loaded directly into histograms, by setting the ``PreserveEvents`` property to ``False``. For workflows
  where preserving events is not necessary, this significantly reduces load time and memory usage.
- (`#40429 <https://github.com/mantidproject/mantid/pull/40429>`_) :ref:`algm-MaskDetectorsIf` now allows the user to
  input the start and end index of the range to apply the mask on.
- (`#40656 <https://github.com/mantidproject/mantid/pull/40656>`_) :ref:`algm-Load` and the file loader search process
  now use a lazy file descriptor to check for the correct loading algorithm, further reducing the time taken to load files.
- (`#40444 <https://github.com/mantidproject/mantid/pull/40444>`_) :ref:`algm-ApplyDetailedBalanceMD`
  has a new parameter, ``RescaleToTemperature``, which allows the scaling of data from one temperature to another.

Bugfixes
########
- (`#40029 <https://github.com/mantidproject/mantid/pull/40029>`_) ``MatrixWorkspace::getNumberBins()`` has been
  replaced by ``MatrixWorkspace::blocksize()``, as the former is deprecated.
- (`#40606 <https://github.com/mantidproject/mantid/pull/40606>`_) :ref:`algm-GenerateEventsFilter`
  will now be able to handle cases where there is no second pulse in the ROI.
- (`#40336 <https://github.com/mantidproject/mantid/pull/40336>`_) :ref:`algm-CreateDetectorTable` will now return the
  workspace index as an integer, rather than a floating-point number.

Deprecated
##########
- (`#40380 <https://github.com/mantidproject/mantid/pull/40380>`_) :ref:`algm-GetTimeSeriesLogInformation` has been
  deprecated. Use ``workspace.run().getStatistics("log_name")`` instead to access time series log statistics directly.
- (`#40625 <https://github.com/mantidproject/mantid/pull/40625>`_) :ref:`algm-LoadMcStasNexus` has been deprecated.
  Use :ref:`algm-LoadMcStas` instead - it is more up-to-date.

Fit Functions
-------------

Bugfixes
########
- (`#40267 <https://github.com/mantidproject/mantid/pull/40267>`_) :ref:`func-Gaussian` has improved stability of fits
  by changing the internal 'active' parameter to ``1/sigma`` instead of ``1/sigma^2``.
- (`#40267 <https://github.com/mantidproject/mantid/pull/40267>`_) The normalisation in histogram evaluation of
  :ref:`func-Gaussian` function and :ref:`func-Jacobian` has been fixed such that they are now consistent with
  ``CentrePoint`` evaluation for small bin-widths.

Data Objects
------------

New features
############
- (`#40054 <https://github.com/mantidproject/mantid/pull/40054>`_) :ref:`Table Workspaces` now support
  :py:meth:`mantid.api.ITableWorkspace.columnArray` for retrieving a column as a NumPy array. For example, with
  ``import numpy as np``: ``table_ws.columnArray("Index")`` returns ``np.array([0, 1, 2, 3])``.

Bugfixes
########
- (`#40551 <https://github.com/mantidproject/mantid/pull/40551>`_) ``MatrixWorkspace.getMemorySize()`` now reports the
  correct value for a :ref:`Ragged_Workspace`.

Python
------

Bugfixes
########
- (`#40239 <https://github.com/mantidproject/mantid/pull/40239>`_) ``SampleShapePlot`` (from
  ``mantidqt/plotting/sample_shape``) no longer crashes upon being called from a script in workbench. Previously, this
  would crash as the plotting methods expected to be on the user interface thread.
- (`#40545 <https://github.com/mantidproject/mantid/pull/40545>`_) ``jemalloc`` has been removed from the conda
  activation scripts because it was causing crashes with unrelated applications on Linux due to ``LD_PRELOAD`` being set
  globally. Behaviour is now reverted to Mantid v6.13 where ``jemalloc`` is used only when launching ``mantidworkbench``.


Dependencies
------------

Bugfixes
############
- (`#40246 <https://github.com/mantidproject/mantid/pull/40246>`_) Pin `occt` to 7.9.2 to avoid conda/rattler version
  number parsing issue with 7.9.1 (See `here <https://github.com/conda-forge/occt-feedstock/pull/124>`_).

Kernel
------

New features
############
- (`#40343 <https://github.com/mantidproject/mantid/pull/40343>`_) Created function ``strmakef``, which allows
  developers to create ``std::string`` objects using C-style formatting strings.
- (`#40487 <https://github.com/mantidproject/mantid/pull/40487>`_) :py:obj:`Property <mantid.kernel.Property>` and
  :py:obj:`IPropertyManager <mantid.kernel.IPropertyManager>` now support the application of multiple
  :py:obj:`IPropertySettings <mantid.kernel.IPropertySettings>` instances to a single property instance. From Python, a
  property's ``getSettings`` method will now return a *vector* of ``IPropertySetting`` when appropriate.


MantidWorkbench
---------------

See :doc:`mantidworkbench`.

:ref:`Release 6.15.0 <v6.15.0>`
