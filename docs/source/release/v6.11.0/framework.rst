=================
Framework Changes
=================

.. contents:: Table of Contents
   :local:

Algorithms
----------

New features
############
- H5 exceptions are now caught with improved error reporting.
- :ref:`algm-Load` will now give a warning if the extension specified on the filename, eg. `MUSR15189.txt`, is not found.
  Loading files from interfaces or other algorithms should also give this warning.
  The algorithm will still load the file by looking for other extensions, as was the case before.
- New version 2 of :ref:`GenerateGroupingPowder <algm-GenerateGroupingPowder>` that will save the grouping file with groups starting at 1 instead of 0 to make them consisted with ``GroupingWorkspace``. The ``FileFormat`` parameter has also been removed as it will now be determined from the extension of ``GroupingFilename``.
- Implemented the :ref:`FilterBadPulses <algm-FilterBadPulses>` functionality in :ref:`LoadEventNexus <algm-LoadEventNexus>`, adding the new ``FilterBadPulsesLowerCutoff`` parameter.
- A new ``SpinStates`` property has been added to :ref:`algm-PolarizationCorrectionWildes` and
  :ref:`algm-PolarizationEfficiencyCor` to allow the order of the workspaces in the output Workspace Group to be set.
- Improve time-scaling performance of CombineDiffCal, and add extra validations.
- Add support for :ref:`Ragged Workspaces <Ragged_Workspace>` to :ref:`CompareWorkspaces <algm-CompareWorkspaces>`.
- Add a new method to :ref:`CompressEvents <algm-CompressEvents>` that will compress events without sorting first. This will be faster when there is a large density of events.
- Add support for :ref:`Ragged Workspaces <Ragged_Workspace>` to binary operations. :ref:`Plus <algm-Plus>`, :ref:`Minus <algm-Minus>`, :ref:`Divide <algm-Divide>` and :ref:`Multiply <algm-Multiply>`.
- Add OutputSuffix parameter to :ref:`ISISIndirectEnergyTransfer <algm-ISISIndirectEnergyTransfer>` algorithm that will append a suffix to the end of output workspace names.
- Added a load compression parameter ``MinSizeCompressOnLoad`` for ref: :ref:`AlignAndFocusPowderFromFiles <algm-AlignAndFocusPowderFromFiles>` and ref: :ref:`SNSPowderReduction <algm-SNSPowderReduction>`
- Changed use of `scipy.interpolate.interp2d` in `NMoldyn4Interpolation` to use `scipy.interpolate.RectBivariateSpline`, since `interp2d` has been removed in version 1.14 of `scipy`. See reference documentation here (https://docs.scipy.org/doc/scipy/reference/generated/scipy.interpolate.interp2d.html)
- Add support for a radial collimator in :ref:`DiscusMultipleScatteringCorrection <algm-DiscusMultipleScatteringCorrection-v1>` algorithm that restricts scatter points within a small region within the larger sample volume. The algorithm was modified to assign zero weight to tracks where the final scatter is not in a position that allows the final track segment to pass through the collimator toward detectors.
- New version 2 of :ref:`LoadEventAsWorkspace2D <algm-LoadEventAsWorkspace2D>` that adds FilterByTime functionality.
- Created new algorithm :ref:`ScaleInstrumentComponent <algm-ScaleInstrumentComponent>` to scale all detectors in an instrument component around the component's geometrical position.
- Implemented an alternative input option :ref:`InstrumentDefinitionFile` in :ref:`GenerateGroupingSNSInelastic <algm-GenerateGroupingSNSInelastic>`, selecting the new ``InstrumentDefinitionFile`` option in instrument drop down menu will create a new field allowing users selecting older instrument definition files.
- add flag "CheckUncertainty" to :ref:`CompareWorkspaces <algm-CompareWorkspaces>` to turn off comparing the y-value uncertainties.

Bugfixes
############
- Modify :ref:`LoadEventNexus <algm-LoadEventNexus>` so the minimum histogram bin edge is equal to the lowest time-of-flight event rather than one less. The effect is that there is no longer negative bin edges unless there is actually a negative time-of-flight in the data.
- Updated isotope densities, see https://pypi.org/project/periodictable/1.6.1/#history for notes about the updates from NIST
- Remove reference to `numpy.distutils` in `IntegratePeaks1DProfile`, since as of NumPy 1.23.0 it is deprecated.
- The :ref:`ScaleX <algm-ScaleX>` algorithm has had a 95% speedup when using the "InstrumentParameter" property.
- Fixed bug when all sample times are before 01/01/1991
- Fixed an unreliable segmentation fault in the :ref:`ExtractFFTSpectrum <algm-ExtractFFTSpectrum>` algorithm.
- The :ref:`LoadIsawUB <algm-LoadIsawUB>` algorithm now correctly adds the UB to the first experiment info when the input workspace has more than one.
- :ref:`SaveNexusProcessed <algm-SaveNexusProcessed>` and :ref:`LoadNexusProcessed <algm-LoadNexusProcessed>` has been fixed to correctly work with :ref:`Ragged Workspaces <Ragged_Workspace>`.

Removed
#######
- The SaveDiffFittingAscii algorithm was deprecated in :ref:`Release 3.13.0 <v3.13.0>` and has now been removed. Use :ref:`EnggSaveSinglePeakFitResultsToHDF5 <algm-EnggSaveSinglePeakFitResultsToHDF5>` instead.
- The AddNote algorithm was deprecated in :ref:`Release 3.6.0 <v3.6.0>` and has now been removed. Please use :ref:`Comment <algm-Comment>` instead.
- The FilterEventsByLogValuePreNexus algorithm was deprecated in 2014 and has now been removed.
- The FindUBUsingMinMaxD algorithm was deprecated in 2013 and has now been removed. Use :ref:`FindUBUsingFFT<algm-FindUBUsingFFT>` instead.
- The ConvertEmptyToTof algorithm was deprecated in :ref:`Release 3.9.0 <v3.9.0>` and has now been removed.
- The RecordPythonScript algorithm was deprecated in :ref:`Release 5.1.0 <v5.1.0>` and has now been removed. Use :ref:`GeneratePythonScript<algm-generatepythonscript>` instead.
- The CheckWorkspacesMatch algorithm was deprecated in :ref:`Release 3.9.0 <v3.9.0>` and has now been removed. Use :ref:`CompareWorkspaces<algm-compareworkspaces>` instead.

Fit Functions
-------------

New features
############
- Added the :ref:`PearsonIV <func-PearsonIV>` fit function to model prompt pulses.
- Added the :ref:`SpinDiffusion <func-SpinDiffusion>` fit function in the Muon category.

Bugfixes
############



Data Objects
------------

New features
############


Bugfixes
############
- Added a `+ 1` to `EventWorkspace::sortAll` to prevent grainsize from being 0.
- Loading ``ENGIN-X`` data on IDAaaS from the instrument data cache no longer throws a "path not found" error.


Python
------

New features
############
- Creates documentation for :mod:`mantid.dataobjects` python bindings
- Fix python fuction ``assert_almost_equal`` to fail for non-equal workspaces
- The python function ``assert_almost_equal`` for testing if two modules are within a tolerance was reworked

Bugfixes
############



Dependencies
------------------

New features
############
- Updated compiler on Linux to gcc version 12, which should improve performance in some circumstances
- Changing from ``Poco::File`` to ``std::filesystem`` in the Kernel module. There will be no noticeable effect to the users.

Bugfixes
############
- Add a run constraint to the mantid package to constrain the optional matplotlib dependency to v3.7. Previously it was possible to install any version of matplotlib alongside mantid in a conda environment, but we cannot guarantee compatibility for any version other than 3.7.
- Allow versions of `pycifrw` greater than 4.4.1


MantidWorkbench
---------------

See :doc:`mantidworkbench`.
:ref:`Release 6.11.0 <v6.11.0>`