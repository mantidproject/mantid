=================
Framework Changes
=================

.. contents:: Table of Contents
   :local:

Algorithms
----------

New features
############
- Errors due to H5 file exceptions are now caught with improved error reporting.
- Algorithm :ref:`algm-Load` will now give a warning if the extension specified on the filename, eg. `MUSR15189.txt`, is not found.
  Loading files from interfaces or other algorithms should also give this warning.
  The algorithm will still load the file by looking for other extensions, as was the case before.
- A new version of :ref:`GenerateGroupingPowder <algm-GenerateGroupingPowder>` (version 2) that will save the grouping file with groups starting at 1 instead of 0 to make them consisted with ``GroupingWorkspace``.
  The ``FileFormat`` parameter has also been removed as it will now be determined from the extension of ``GroupingFilename``.
- Algorithm :ref:`LoadEventNexus <algm-LoadEventNexus>` now has a new ``FilterBadPulsesLowerCutoff`` parameter that implements the functionality of :ref:`FilterBadPulses <algm-FilterBadPulses>`.
- Algorithm :ref:`algm-PolarizationCorrectionWildes` and :ref:`algm-PolarizationEfficiencyCor` have a new ``SpinStates`` property to allow the order of the workspaces in the output workspace group to be set.
- Algorithm :ref:`algm-CombineDiffCal` has improved time-scaling performance and new extra validations.
- Algorithm :ref:`CompareWorkspaces <algm-CompareWorkspaces>` now supports :ref:`Ragged Workspaces <Ragged_Workspace>`.
- Binary operations :ref:`Plus <algm-Plus>`, :ref:`Minus <algm-Minus>`, :ref:`Divide <algm-Divide>` and :ref:`Multiply <algm-Multiply>` now support :ref:`Ragged Workspaces <Ragged_Workspace>`.
- Algorithm :ref:`CompressEvents <algm-CompressEvents>` a new ``SortFirst`` property that controls whether sorting happens before compressing events.
  If ``SortFirst=False`` then a different method is used to compress events that will not sort first. This is faster when you have a large number of events per compress tolerance.
- Algorithm :ref:`ISISIndirectEnergyTransfer <algm-ISISIndirectEnergyTransfer>` has the new property ``OutputSuffix`` that will append a suffix to the end of output workspace names.
- Algorithms :ref:`AlignAndFocusPowderFromFiles <algm-AlignAndFocusPowderFromFiles>` and ref: :ref:`SNSPowderReduction <algm-SNSPowderReduction>` have new a property called ``MinSizeCompressOnLoad`` for specifying load compression.
- Algorithm :ref:`NMoldyn4Interpolation <algm-NMoldyn4Interpolation>` now uses ``scipy.interpolate.RectBivariateSpline`` instead of ``scipy.interpolate.interp2d``, since ``interp2d`` has been removed in version 1.14 of ``scipy``.
  See reference documentation here (https://docs.scipy.org/doc/scipy/reference/generated/scipy.interpolate.interp2d.html).
- Algorithm :ref:`DiscusMultipleScatteringCorrection <algm-DiscusMultipleScatteringCorrection-v1>` now supports a radial collimator that restricts scatter points within a small region within the larger sample volume.
  The algorithm was modified to assign zero weight to tracks where the final scatter is not in a position that allows the final track segment to pass through the collimator toward detectors.
- Version 2 of the algorithm :ref:`LoadEventAsWorkspace2D <algm-LoadEventAsWorkspace2D>` that adds the property ``FilterByTime``.
- New algorithm :ref:`ScaleInstrumentComponent <algm-ScaleInstrumentComponent>` to scale all detectors in an instrument component around the component's geometrical position.

  .. figure::  ../../images/6_11_release/ScaleInstrumentComponent.png
     :width: 400px

- Algorithm :ref:`GenerateGroupingSNSInelastic <algm-GenerateGroupingSNSInelastic>` has the new input option :ref:`InstrumentDefinitionFile`.
  Selecting the new ``InstrumentDefinitionFile`` option in the instrument drop down menu will create a new field allowing users to select older instrument definition files.
- Algorithm :ref:`CompareWorkspaces <algm-CompareWorkspaces>` has a new ``CheckUncertainty`` property to turn off comparing the y-value uncertainties.
- Algorithm :ref:`ScaleX <algm-ScaleX>` is now 95% faster when using the ``InstrumentParameter`` property.

Bugfixes
############
- Algorithm :ref:`CompareWorkspaces <algm-CompareWorkspaces-v1>` is now fixed for relative differences of small values.
- Algorithm :ref:`LoadEventNexus <algm-LoadEventNexus>` now has the minimum histogram bin edge equal to the lowest time-of-flight event rather than one less.
  There are no longer negative bin edges unless there is actually a negative time-of-flight in the data.
- Isotope densities have been updated, see https://pypi.org/project/periodictable/1.6.1/#history for notes about the updates from NIST.
- Files where all sample times are before 01/01/1991 will no longer generate an error.
- Algorithm :ref:`ExtractFFTSpectrum <algm-ExtractFFTSpectrum>` no longer causes an unreliable segmentation fault.
- Algorithm :ref:`LoadIsawUB <algm-LoadIsawUB>` now correctly adds the UB to the first experiment info when the input workspace has more than one.
- Algorithms :ref:`SaveNexusProcessed <algm-SaveNexusProcessed>` and :ref:`LoadNexusProcessed <algm-LoadNexusProcessed>` have been fixed to correctly work with :ref:`Ragged Workspaces <Ragged_Workspace>`.

Removed
#######
- Algorithm ``SaveDiffFittingAscii`` was deprecated in :ref:`Release 3.13.0 <v3.13.0>` and has now been removed. Use :ref:`EnggSaveSinglePeakFitResultsToHDF5 <algm-EnggSaveSinglePeakFitResultsToHDF5>` instead.
- Algorithm ``AddNote`` was deprecated in :ref:`Release 3.6.0 <v3.6.0>` and has now been removed. Please use :ref:`Comment <algm-Comment>` instead.
- Algorithm ``FilterEventsByLogValuePreNexus`` was deprecated in 2014 and has now been removed.
- Algorithm ``FindUBUsingMinMaxD`` was deprecated in 2013 and has now been removed. Use :ref:`FindUBUsingFFT<algm-FindUBUsingFFT>` instead.
- Algorithm ``ConvertEmptyToTof`` was deprecated in :ref:`Release 3.9.0 <v3.9.0>` and has now been removed.
- Algorithm ``RecordPythonScript`` was deprecated in :ref:`Release 5.1.0 <v5.1.0>` and has now been removed. Use :ref:`GeneratePythonScript<algm-generatepythonscript>` instead.
- Algorithm ``CheckWorkspacesMatch`` was deprecated in :ref:`Release 3.9.0 <v3.9.0>` and has now been removed. Use :ref:`CompareWorkspaces<algm-compareworkspaces>` instead.

Fit Functions
-------------

New features
############
- Fit function :ref:`PearsonIV <func-PearsonIV>` now available to fit model prompt pulses.
- Fit function :ref:`SpinDiffusion <func-SpinDiffusion>` now available in the Muon category.


Data Objects
------------

Bugfixes
############
- Added a `+ 1` to `EventWorkspace::sortAll` to prevent grainsize from being 0.
- Loading ``ENGIN-X`` data on IDAaaS from the instrument data cache no longer throws a ``path not found`` error.


Python
------

New features
############
- Created documentation for :mod:`mantid.dataobjects` python bindings.
- Fix python fuction ``assert_almost_equal`` to fail for non-equal workspaces.
- The python function ``assert_almost_equal`` for testing if two modules are within a tolerance was reworked.


Dependencies
------------------

New features
############
- Linux compiler has been updated to gcc version 12, which should improve performance in some circumstances.
- Updated compiler on macOS from clang version 15 to 16, which should result in performance improvements.

Bugfixes
############
- Introduced a run constraint to the mantid package to constrain the optional matplotlib dependency to v3.7.
  Previously it was possible to install any version of matplotlib alongside mantid in a conda environment, but we cannot guarantee compatibility for any version other than 3.7.
- Versions of `pycifrw` are now allowed to be greater than 4.4.1.


MantidWorkbench
---------------

See :doc:`mantidworkbench`.
:ref:`Release 6.11.0 <v6.11.0>`
