=================
Framework Changes
=================

.. contents:: Table of Contents
   :local:

Algorithms
----------

New features
############
- :ref:`algm-PolarizationCorrectionFredrikze` and :ref:`algm-PolarizationEfficiencyCor` now have ``InputSpinStates`` and ``OutputSpinStates`` properties for specifying the spin state order in input and output workspace groups.
- ``Mantid::Kernel::FloatingPointComparison`` has new functions for efficiently calculating absolute and relative differences.
- On Linux the :ref:`algorithm profiler <AlgorithmProfiler>` is now built by default. To enable profiling, the :ref:`Algorithm Profiling Settings <Algorithm_Profiling>` must be set.
- The :ref:`AlgoTimeRegister <AlgoTimeRegister>` class is now exposed to python.
- :ref:`ExtractMask <algm-ExtractMask>` has a new property ``UngroupDetectors`` to allow the output ``MaskWorkspace`` to expand the spectra to individual detectors.
- :ref:`PolarizationCorrectionWildes <algm-PolarizationCorrectionWildes>` and :ref:`PolarizationCorrectionFredrikze <algm-PolarizationCorrectionFredrikze>` have a new ``AddSpinStateToLog`` property to add the final spin state into the sample log of each child workspace in the output group.
- :ref:`EditInstrumentGeometry <algm-EditInstrumentGeometry>` now adds detectors to a single detector bank, rather than directly to the ``ComponentInfo`` root.
- :ref:`SaveNexusESS <algm-SaveNexusESS>` now supports append mode, allowing multiple workspaces to be written either one at a time, or as a group workspace, to a single NeXus HDF5 file.
- New algorithm :ref:`algm-MagneticFormFactorCorrectionMD` to scale the MDEvents by the magnetic form factor.
- :ref:`RebinRagged <algm-RebinRagged>` now exposes ``FullBinsOnly`` from the :ref:`Rebin <algm-Rebin>` Algorithm.
- New algorithm :ref:`CreateMonteCarloWorkspace <algm-CreateMonteCarloWorkspace>` that creates a randomly distributed workspace by sampling from the probability distribution of the input workspace.
- :ref:`CompareWorkspaces <algm-CompareWorkspaces>` has a new ``NaNsEqual`` boolean property to specify whether ``NaN`` values compare as equal.

Bugfixes
############
- :ref:`FindPeaksConvolve <algm-FindPeaksConvolve>` will no longer segfault due to a racing condition in the parallel loop.
- :ref:`RemovePromptPulse <algm-RemovePromptPulse>` will now correctly account for the first pulse.
- :ref:`CompareWorkspaces <algm-CompareWorkspaces>` will no longer evaluate ``NaN`` values as equal to any floating point (including ``inf`` values).
- :ref:`ConjoinWorkspaces <algm-ConjoinWorkspaces>` will now throw an error if the input workspaces' bins do not match. A new ``CheckMatchingBins`` boolean property can be set to ``False`` to disable this check.
- Some :ref:`algm-HeliumAnalyserEfficiency` properties have been renamed for consistency (any scripts using the old names will need to be updated):

  - ``GasPressureTimesCellLength`` is now ``PxD``
  - ``GasPressureTimesCellLengthError`` is now ``PXDError``
  - ``StartLambda`` is now ``StartX``
  - ``EndLambda`` is now ``EndX``
- :ref:`LoadNexusProcessed <algm-LoadNexusProcessed>` now correctly determines the number of workspaces in a NeXus HDF5 file. It now counts the number of root-level ``NX_class: NXentry`` groups. Previously, it simply counted the number of root-level groups, assuming all were of ``NX_class: NXentry``.
- :ref:`Divide <algm-Divide>` will now correctly set the ``isDistribution`` flag to ``true`` when dividing two ragged workspaces.
- :ref:`ConvertUnits <algm-ConvertUnits>` now supports histogram :ref:`ragged workspaces <Ragged_Workspace>`. Ragged workspaces with bin centers (Point rather than BinEdges) still generate errors.
- :ref:`LoadNGEM <algm-LoadNGEM>` now respects ``Min/MaxEventsPerFrame`` inputs.
- :ref:`LoadErrorEventsNexus <algm-LoadErrorEventsNexus>` no longer hangs when when the error bank has zero events.


Data Objects
------------

New features
############
- ``EnumeratedStringProperty``, which uses ``EnumeratedString``, can be used in C++ based algorithms.

Bugfixes
############
- `TableWorkspace::getMemorySize()` now sums memory correctly and returns a more reliable estimate of memory use.


Geometry
--------

Bugfixes
############
- ``CrystalStructure`` will now store and display Deuterium as ``D`` rather than ``H``.


Python
------

New features
############
- A new ``TableWorkspaceNotEmptyValidator``.
- :class:`PropertyWithValue <mantid.kernel.FloatPropertyWithValue>` types can now be used as output properties from the python API.
- A new testing function :ref:`assert_not_equal <mantid.testing.assert_not_equal>` to make testing inequality between workspaces more convenient.

Bugfixes
############
- Renaming a plotted workspace, where the plot also contains a line, will no longer cause an exception.

Dependencies
------------------

New features
############
- Updated Matplotlib from version 3.7 to version 3.9. See release notes for `version 3.8 <https://matplotlib.org/stable/users/prev_whats_new/whats_new_3.8.0.html>`_  and `version 3.9 <https://matplotlib.org/stable/users/prev_whats_new/whats_new_3.9.0.html>`_.
- Updated compiler on Linux to gcc version 13, which should improve performance in some circumstances. The release notes can be found here https://gcc.gnu.org/gcc-13/changes.html.
- Drop support for NumPy version 1. We now build against NumPy v2.0 and support up to v2.1. `Read about the changes <https://numpy.org/news/#numpy-200-released>`_. **Users should note that NumPy 2 introduces some breaking API changes. See the `NumPy 2 Migration Guide <https://numpy.org/devdocs/numpy_2_0_migration_guide.html>`_ for more details**


MantidWorkbench
---------------

See :doc:`mantidworkbench`.
:ref:`Release 6.12.0 <v6.12.0>`
