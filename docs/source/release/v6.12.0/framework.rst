=================
Framework Changes
=================

.. contents:: Table of Contents
   :local:

Algorithms
----------

New features
############
- A new ``InputSpinStates`` and ``OutputSpinStates`` property has been added to :ref:`algm-PolarizationCorrectionFredrikze` and
  :ref:`algm-PolarizationEfficiencyCor` to allow the order of the workspaces in the input and output Workspace Groups to be set.
- add new functions for efficiently calculating absolute and relative differences
- On Linux the algorithm profiler is now built by default but to enable profiling, the :ref:`properties <Algorithm_Profiling>` must be set.
- The AlgoTimeRegister class is exposed to python to measure time taken by other algorithms to run.
- A new property ``UngroupDetectors`` was added to :ref:`ExtractMask <algm-ExtractMask>` allowing the output ``MaskWorkspace`` to expand the spectra to individal detectors.
- A new property ``AddSpinStateToLog`` was added to :ref:`PolarizationCorrectionWildes <algm-PolarizationCorrectionWildes>` to give the option to add the final spin state into the sample log of each child workspace in the output group.
- A new property ``AddSpinStateToLog`` was added to :ref:`PolarizationCorrectionFredrikze <algm-PolarizationCorrectionFredrikze>` to give the option to add the final spin state into the sample log of each child workspace in the output group.
- The algorithm EditInstrumentGeometry now adds detectors to a single detector bank, rather than directly to the ComponentInfo root.
- Algorithm SaveNexusESS now supports append mode, allowing multiple workspaces to be written either one at a time, or as a group workspace, to a single NeXus HDF5 file.
- New algorithm :ref:`algm-MagneticFormFactorCorrectionMD` to scale the MDEvents by the magnetic form factor.
- :ref:`RebinRagged <algm-RebinRagged>` exposes FullBinsOnly from Rebin Algo.
- New algorithm :ref:`CreateMonteCarloWorkspace <algm-CreateMonteCarloWorkspace>` that creates a randomly simulated workspace by sampling from the probability distribution of input data.

Bugfixes
############
- Fixed a segmentation fault in :ref:`FindPeaksConvolve <algm-FindPeaksConvolve>` algorithm due to a racing condition in the parallel loop. The issue was first observed as a flaky unit test failure in the CI.
- :ref:`RemovePromptPulse <algm-RemovePromptPulse>` has been fixed to correctly account for the first pulse.
- Fixed a bug in :ref:`sample_transmission_calculator` (Interfaces > General > Sample Transmission Calculator) interface to restrict entering commas mixed with decimal point in the double spin boxes for Low, Width and High fields
- fixes bug in :ref:`CompareWorkspaces <algm-CompareWorkspaces>` that evaluated ``NaN`` values as equal to any floating point (including ``inf`` and finite values).
- adds new flag NaNsEqual to :ref:`CompareWorkspaces <algm-CompareWorkspaces>` to control how ``NaN`` compares to other ``NaN`` s.
- :ref:`ConjoinWorkspaces <algm-ConjoinWorkspaces>` now performs a check and throws an error if input workspace's bins are mismatched. The mismatch check can be disabled setting the ``CheckMatchingBins`` property to ``False``.
- The following parameter names in :ref:`algm-HeliumAnalyserEfficiency` have been updated for consistency: `GasPressureTimesCellLength` is now `PxD`, `GasPressureTimesCellLengthError` is now `PXDError`, `StartLambda` is now `StartX`, and `EndLambda` is now `EndX`. Any scripts using the old names will need to be updated.
- fixes a bug in :ref:`LoadNexusProcessed <algm-LoadNexusProcessed>` when determining the number of workspaces in a NeXus HDF5 file.  It now counts the number of root-level "NX_class: NXentry" groups. Previously, it simply counted the number of root-level groups, assuming all were of "NX_class: NXentry".
- Corrects `isDistribution` property of result of division of two ragged workspaces, now `true`
- Add support for histogram :ref:`ragged workspaces <Ragged_Workspace>` to :ref:`ConvertUnits <algm-ConvertUnits>`. Ragged workspaces with bin centers (Point rather than BinEdges) still generate errors.
- Fixed bug in:ref:`LoadNGEM <algm-LoadNGEM>` where ``Min/MaxEventsPerFrame`` inputs were not being respected.
- Fixed a bug in :ref:`LoadErrorEventsNexus <algm-LoadErrorEventsNexus>` that would cause it to hang when the error bank had zero events.

Deprecated
############


Removed
############


Fit Functions
-------------

New features
############


Bugfixes
############
- Fixed logic in FitParameter so that if only a Minimum constraint or only a Maximum constraint is available the string returned will not record a 0 for the unavailable constraint.

Deprecated
############


Removed
############



Data Objects
------------

New features
############
- ``EnumeratedStringProperty`` which uses ``EnumeratedString`` can be used in C++ based algorithms

Bugfixes
############
- Fixed bug in `TableWorkspace::getMemorySize()` where the calculation was not summing memory correctly, leading to an underestimate of memory use..
- Fix ``CrystalStructure`` to display deuterium when it is one of the atoms


Python
------

New features
############
- Adds `TableWorkspaceNotEmptyValidator`
- allows for declaring the many :class:`PropertyWithValue <mantid.kernel.FloatPropertyWithValue>` types as output properties from the python API.
- Added a new testing function :ref:`assert_not_equal <mantid.testing.assert_not_equal>` to make testing inequality between workspaces more convenient.
- `MatrixWorkspace` now has a `plotType` property

Bugfixes
############
Fix for bug in ``mantid.plots.MantidAxes`` where an exception would occur when a workspace plot, which also had data not tied to a workspace on the same axes, had its workspace renamed.


Dependencies
------------------

New features
############
- Updated Matplotlib from version 3.7 to version 3.9. The release notes for `version 3.8 <https://matplotlib.org/stable/users/prev_whats_new/whats_new_3.8.0.html>`_  and `version 3.9 <https://matplotlib.org/stable/users/prev_whats_new/whats_new_3.9.0.html>`_
- Updated compiler on Linux to gcc version 13, which should improve performance in some circumstances. The release notes can be found here https://gcc.gnu.org/gcc-13/changes.html
- Drop support for NumPy version 1. We now build against NumPy v2.0 and support up to v2.1. `Read about the changes <https://numpy.org/news/#numpy-200-released>`_. **Users should note that NumPy 2 introduces some breaking API changes. See the `NumPy 2 Migration Guide <https://numpy.org/devdocs/numpy_2_0_migration_guide.html>`_ for more details**

Bugfixes
############



MantidWorkbench
---------------

See :doc:`mantidworkbench`.
:ref:`Release 6.12.0 <v6.12.0>`
