=================
Framework Changes
=================

.. contents:: Table of Contents
   :local:

Algorithms
----------

New features
############
- (`#40367 <https://github.com/mantidproject/mantid/pull/40367>`_) `UnaryOperation <https://github.com/mantidproject/mantid/blob/main/Framework/Algorithms/src/UnaryOperation.cpp>`_ algorithms like :ref:`ReplaceSpecialValues <algm-ReplaceSpecialValues>`, :ref:`Logarithm <algm-Logarithm>` now support ragged workspaces.
- (`#40063 <https://github.com/mantidproject/mantid/pull/40063>`_) Documenting flowchart diagram for :ref:`algm-LoadWANDSCD`.
- (`#40067 <https://github.com/mantidproject/mantid/pull/40067>`_) :ref:`AlignAndFocusPowderSlim <algm-AlignAndFocusPowderSlim>` now has the parameter ``OutputSpectrumNumber`` for selecting to load only a single bank of data.
- (`#40107 <https://github.com/mantidproject/mantid/pull/40107>`_) Created new algorithm :ref:`HFIRPowderReduction <algm-HFIRPowderReduction>`, just added UI for loading data to reduce
- (`#40120 <https://github.com/mantidproject/mantid/pull/40120>`_) Documenting flowchart diagram for :ref:`algm-ConvertWANDSCDtoQ`.
- (`#40122 <https://github.com/mantidproject/mantid/pull/40122>`_) Added a new implementation for splitting into multiple workspace to :ref:`AlignAndFocusPowderSlim <algm-AlignAndFocusPowderSlim>` that can be tested by setting ``ProcessBankSplitTask=True``.
- (`#40128 <https://github.com/mantidproject/mantid/pull/40128>`_) :ref:`CreateDetectorTable <algm-CreateDetectorTable>` now supports option ``PickOneDetectorID`` for returning column of detector IDs as a column of integers containing only one detector ID per row.
  By default this option is disabled, so by default the column of detector IDs will consist of strings of one or more detector ID per row.
  This new option was introduced to be used in the background of the new instrument view interface, but might also be useful if the user wants a single detector ID per row.
- (`#40213 <https://github.com/mantidproject/mantid/pull/40213>`_) Added UI elements for the reduction part of :ref:`HFIRPowderReduction <algm-HFIRPowderReduction>`
- (`#40227 <https://github.com/mantidproject/mantid/pull/40227>`_) New algorithm, :ref:`CreateGroupingByComponent <algm-CreateGroupingByComponent>`, allows for creation of ``GroupingWorkspaces`` where the groups are formed by combining components, under a given parent component. Components which should be combined are identified by providing strings which their names should/should not contain. Each of these groups can be optionally subdivided into a further, ``GroupSubdivision``, number of groups.
- (`#40234 <https://github.com/mantidproject/mantid/pull/40234>`_) Documenting flowchart diagram for :ref:`algm-ConvertHFIRSCDtoMDE`
- (`#40327 <https://github.com/mantidproject/mantid/pull/40327>`_) New algorithm :ref:`ReorientUnitCell <algm-ReorientUnitCell>` to select the unit cell most aligned to goniometer axes.
- (`#40332 <https://github.com/mantidproject/mantid/pull/40332>`_) New algorithm :ref:`AddLogSmoothed <algm-AddLogSmoothed>` for smoothing time series log data.
- (`#40377 <https://github.com/mantidproject/mantid/pull/40377>`_) :ref:`AlignAndFocusPowderSlim <algm-AlignAndFocusPowderSlim>` can now can split events based on full time (pulsetime+TOF) with the parameter ``UseFullTime``.
- (`#40420 <https://github.com/mantidproject/mantid/pull/40420>`_) Algorithm :ref:`LoadNGEM <algm-LoadNGEM>` has been extended to allow for data to be loaded directly into histograms, via the setting of the PreserveEvents property to ``False``. For workflows where preserving events is not necessary, this reduces load time and memory usage by a large factor.
-  Updated the :ref:`MaskDetectorsIf <algm-MaskDetectorsIf>` algorithm to allow the user to input the start and end index of the range to apply the mask on.
- (`#40450 <https://github.com/mantidproject/mantid/pull/40450>`_) New algorithm :ref:`AddLogInterpolated <algm-AddLogInterpolated>` for interpolating time series sample logs.
- (`#40493 <https://github.com/mantidproject/mantid/pull/40493>`_) :ref:`AlignAndFocusPowderSlim <algm-AlignAndFocusPowderSlim>` will now set the uncertainty values on the output workspace.
- (`#40656 <https://github.com/mantidproject/mantid/pull/40656>`_) The :ref:`algm-Load` algorithm, and the file loader search process, now use a lazy file descriptor to check for the correct loading algorithm, further reducing time for :ref:`algm-Load`.
-  Updated default parameters ``ReadSizeFromDisk``, ``EventsPerThread``, and ``LogBlockList`` for :ref:`AlignAndFocusPowderSlim <algm-AlignAndFocusPowderSlim>` to give faster execution.
- (`#40444 <https://github.com/mantidproject/mantid/pull/40444>`_) New property in :ref:`ApplyDetailedBalanceMD <algm-ApplyDetailedBalanceMD>` allows scaling data from one temperature to another.

Bugfixes
############
- (`#40029 <https://github.com/mantidproject/mantid/pull/40029>`_) Removed uses of ``MatrixWorkspace::getNumberBins()``, which is deprecated, and replaced those uses with ``MatrixWorkspace::blocksize()``.
- (`#40606 <https://github.com/mantidproject/mantid/pull/40606>`_) :ref:`GenerateEventsFilter <algm-GenerateEventsFilter>` will now be able to handle cases where there is no second pulse in the ROI.
- (`#40336 <https://github.com/mantidproject/mantid/pull/40336>`_) ``CreateDetectorTable`` will now return the workspace index as an integer, not a floating point number.

Deprecated
############
- (`#40380 <https://github.com/mantidproject/mantid/pull/40380>`_) :ref:`GetTimeSeriesLogInformation <algm-GetTimeSeriesLogInformation>` has been deprecated. Use ``workspace.run().getStatistics("log_name")`` instead to access time series log statistics directly.
- (`#40625 <https://github.com/mantidproject/mantid/pull/40625>`_) :ref:`LoadMcStasNexus <algm-LoadMcStasNexus>` has been deprecated. Use :ref:`LoadMcStas <algm-LoadMcStas>` instead as it is more up to date.

Removed
############

Fit Functions
-------------

New features
############

Bugfixes
############
- (`#40267 <https://github.com/mantidproject/mantid/pull/40267>`_) Improve stabiltiy of :ref:`func-Gaussian` fits by change of internal 'active' parameter to ``1/sigma`` instead of ``1/sigma^2``.
- (`#40267 <https://github.com/mantidproject/mantid/pull/40267>`_) Fix normalisation in histogram evaluation of :ref:`func-Gaussian` function and Jacobian such that they are now consistent with ``CentrePoint`` evaluation for small bin-widths.

Deprecated
############

Removed
############


Data Objects
------------

New features
############
- (`#40054 <https://github.com/mantidproject/mantid/pull/40054>`_) :py:obj:`Table Workspaces <mantid.api.ITableWorkspace>` now support :py:meth:`mantid.api.ITableWorkspace.columnArray` for retrieving a column as a NumPy array.
  For example, with ``import numpy as np``: ``table_ws.columnArray("Index")`` returns ``np.array([0, 1, 2, 3])``.

Bugfixes
############
- (`#40551 <https://github.com/mantidproject/mantid/pull/40551>`_) Fix issue where ``MatrixWorkspace.getMemorySize()`` reported the wrong value for :ref:`ragged workspaces <Ragged_Workspace>`


Python
------

New features
############

Bugfixes
############
- (`#40239 <https://github.com/mantidproject/mantid/pull/40239>`_) ``SampleShapePlot`` (from ``mantidqt/plotting/sample_shape``) no longer crashes upon being called from a script in workbench. Previously, this would crash as the plotting methods expected to be on the user interface thread.
- (`#40545 <https://github.com/mantidproject/mantid/pull/40545>`_) ``jemalloc`` has been removed from the conda activation scripts because it was causing crashes with unrelated applications on Linux due to LD_PRELOAD being set globally.
  Behaviour is now reverted to Mantid v6.13 where ``jemalloc`` is used only when launching ``mantidworkbench``.


Dependencies
------------------

New features
############
- (`#39957 <https://github.com/mantidproject/mantid/pull/39957>`_) Add the ability to select mirrors for test data in builds using the ``DATA_STORE_MIRROR`` cmake variable. See :doc:`data files for testing <mantid-dev:DataFilesForTesting>`
- (`#40289 <https://github.com/mantidproject/mantid/pull/40289>`_) Add `AGENTS.md <https://github.com/mantidproject/mantid/blob/main/AGENTS.md>`_ which gives instructions to coding agents in a way consistent with the https://agents.md standard

Bugfixes
############
- (`#40246 <https://github.com/mantidproject/mantid/pull/40246>`_) Pin `occt` to 7.9.2 to avoid conda/rattler version number parsing issue with 7_9_1 (See: https://github.com/conda-forge/occt-feedstock/pull/124)


Kernel
------

New features
############
- (`#40343 <https://github.com/mantidproject/mantid/pull/40343>`_) Created function ``strmakef``, which allows developers to create ``std::string`` objects using C-style formatting strings.
- (`#40487 <https://github.com/mantidproject/mantid/pull/40487>`_) :py:obj:`Property <mantid.kernel.Property>` and :py:obj:`IPropertyManager <mantid.kernel.IPropertyManager>` now support the application of multiple :py:obj:`IPropertySettings <mantid.kernel.IPropertySettings>` instances to a single property instance.  From Python, a property's ``getSettings`` method will now return a *vector* of ``IPropertySetting`` when appropriate.


MantidWorkbench
---------------

See :doc:`mantidworkbench`.

:ref:`Release 6.15.0 <v6.15.0>`
