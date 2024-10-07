=================
Framework Changes
=================

.. contents:: Table of Contents
   :local:

Supported Operating Systems
---------------------------

- We now support Ubuntu Xenial Xerus (16.04) and this will be the final release to support Trusty Tahr (14.04).
- We now support OS X Yosemite (10.10) and support for OS X Mavericks (10.9) has been dropped.

HistogramData
-------------

A new module for dealing with histogram data has been added, it is now being used internally in Mantid to store data in various workspace types.

- For C++ users, details can be found in the `transition documentation <http://docs.mantidproject.org/nightly/concepts/HistogramData.html>`_.
- For Python users, the interface is so far unchanged.
  However, to ensure data consistency and to reduce the risk of bugs, histograms now enforce length limitations. For example, there must be one bin edge more than data (Y and E) values.
  If you experience trouble, in particular exceptions about size mismatch, please refer to the section `Dealing with problems <http://docs.mantidproject.org/nightly/concepts/HistogramData.html#dealing-with-problems>`_.

Concepts
--------

- ``MatrixWorkspace`` : When masking bins or detectors with non-zero weights,
  undefined and infinite values and errors will be zeroed.
- ``Lattice`` : Allow setting a UB matrix with negative determinant (improper rotation)

- ``MultipleFileProperty`` : will now support also ``OptionalLoad`` ``FileAction`` (similar to ``FileProperty``).

Algorithms
----------

New
###

-  :ref:`ClearCache <algm-ClearCache>` an algorithm to simplify the clearance of several in memory or disk caches used in Mantid.

- :ref:`LoadPreNexusLive <algm-LoadPreNexusLive>` will load "live"
  data from file on legacy SNS DAS instruments.

- :ref:`CropToComponent <algm-CropToComponent>` allows for cropping a workspace to a list of component names.
- :ref:`CreateUserDefinedBackground <algm-CreateUserDefinedBackground>` takes a set of points
  that the user has chosen and creates a background workspace out of them. It interpolates the
  points so the resulting background can be subtracted from the original data.

- SaveDiffFittingAscii an algorithm which saves a TableWorkspace containing
  diffraction fitting results as an ASCII file

- :ref:`UnwrapMonitorsInTOF <algm-UnwrapMonitorsInTOF>` handles the data which was collected beyond the end of a frame.

- :ref:`ExtractMonitors <algm-ExtractMonitors>` an algorithm to extract the monitor spectra into a new workspace. Can also be
  used to create a workspace with just the detectors, or two workspaces, one with the monitors and one with the detectors.

Improved
########

- :ref:`FlatPlatePaalmanPingsCorrection <algm-FlatPlatePaalmanPingsCorrection>` & :ref:`CylinderPaalmanPingsCorrection <algm-CylinderPaalmanPingsCorrection>`
  now accept 'Direct' as a possible ``EMode`` parameter.

- :ref:`FilterEvents <algm-FilterEvents>` now produces output
  workspaces with the same workspace numbers as specified by the
  ``SplittersWorkspace``.
- :ref:`ConvertAxisByFormula <algm-ConvertAxisByFormula>` now supports instrument geometry vairables and several constants within the formula.  Axes are now reversed if the need to be to maintain increasing axis values.

- :ref:`SavePlot1D <algm-SavePlot1D>` has options for writing out
  plotly html files.

- :ref:`SofQW <algm-SofQW>` has option to replace any NaNs in output workspace
  with zeroes.

- :ref:`ConvertTableToMatrixWorkspace <algm-ConvertTableToMatrixWorkspace>`
  had a bug where the table columns were in a reversed order in the dialogue's combo boxes.
  This is now fixed and the order is correct.

- :ref:`ConvertUnits <algm-ConvertUnits>` and :ref:`ConvertUnitsUsingDetectorTable <algm-ConvertUnitsUsingDetectorTable>` will no longer corrupt a workspace used as input and output if the algorithm fails.

- :ref:`SetSample <algm-SetSample>`: Fixed a bug with interpreting the `Center` attribute for cylinders/annuli

- :ref:`MonteCarloAbsorption <algm-MonteCarloAbsorption>` had a bug in cases where the beam was larger than the
  sample, which lead to the attenuation factor being too high. This has been fixed.

- :ref:`ConvertUnits <algm-ConvertUnits>` now has the option to take a workspace with Points as input.
  A property has been added that will make the algorithm convert the workspace to Bins automatically. The output space will be converted back to Points.

- :ref:`RenameWorkspace <algm-RenameWorkspace>` and `RenameWorkspaces <algm-RenameWorkspaces>`
  now check if a Workspace with that name already exists in the ADS and gives
  the option to override it.

- :ref:`FindSXPeaks <algm-FindSXPeaks>`: Fixed a bug where peaks with an incorrect TOF would stored for some instrument geometries.

- :ref:`LoadILL <algm-LoadILLTOF>` was renamed to :ref:`LoadILLTOF <algm-LoadILLTOF>` to better reflect what it does. The new algorithm can also handle cases where the monitor IDs are greater than the detector IDs.

- :ref:`FFT <algm-FFT>` deals correctly with histogram input data. Internally, it converts to point data, and the output is always a point data workspace. (It can be converted to histogram data using :ref:`ConvertToHistogram <algm-ConvertToHistogram>` if required).

-  :ref:`StartLiveData <algm-StartLiveData>` has additional properties for specifying scripts to run for processing and post-processing.

- :ref:`LoadEmptyInstrument <algm-LoadEmptyInstrument>` now also accepts a workspace name as input, as an alternative to an instrument definition xml file.

- :ref:`Mergeruns <algm-MergeRuns>` can now also deal with non-time series sample logs when merging. Behaviour can be to create a time series, a list of values and warn or fail if different.


MD Algorithms (VATES CLI)
#########################

- :ref:`MergeMD <algm-MergeMD>` now preserves the display normalization from the first workspace in the list

- :ref:`BinMD <algm-BinMD>` fixed bug where algorithm would default to using orthogonal basis vectors when supplied with 4 bases and 4 dimensions

Performance
-----------

- An internal change that is a preliminary step for "Instrument-2.0" can yield slight to moderate performance improvements of the following algorithms (and other algorithms that use one of these):
  AppendSpectra, ApplyTransmissionCorrection, CalculateEfficiency, CalculateFlatBackground, ConjoinSpectra, ConvertAxesToRealSpace, ConvertAxisByFormula, ConvertEmptyToTof, ConvertSpectrumAxis2, ConvertUnitsUsingDetectorTable, CorelliCrossCorrelate, DetectorEfficiencyVariation, EQSANSTofStructure, FilterEvents, FindCenterOfMassPosition, FindCenterOfMassPosition2, FindDetectorsOutsideLimits, GetEi, IntegrateByComponent, LorentzCorrection, MultipleScatteringCylinderAbsorption, NormaliseToMonitor, Q1D2, Q1DWeighted, RadiusSum, RemoveBackground, RemoveBins, RemoveMaskedSpectra, RingProfile, SANSDirectBeamScaling, SumSpectra, TOFSANSResolution, UnwrapMonitor, UnwrapSNS, VesuvioCalculateMS, and WeightedMeanOfWorkspace.

- The introduction of the HistogramData module may have influenced the performance of some algorithms and many workflows.
  Some algorithms (listed below) experience a speedup and reduced memory consumption.
  If you experience unusual slowdowns, please contact the developer team.

  The following algorithms were adapted and show a noticeable speedup:

  - :ref:`ApplyTransmissionCorrection <algm-ApplyTransmissionCorrection>`: 20% speedup
  - :ref:`ConvertSpectrumAxis <algm-ConvertSpectrumAxis>`: 25% speedup
  - :ref:`ConvertToHistogram <algm-ConvertToHistogram>`: 3x to 4x speedup
  - :ref:`ConvertToPointData <algm-ConvertToPointData>`: 3x to 4x speedup
  - :ref:`CorrectFlightPaths <algm-ConvertToConstantL2>`: 10% speedup
  - :ref:`ExtractSpectra <algm-ExtractSpectra>`: no change when X-range changes, otherwise 50x to 100x speedup for Workspace2D and up to 3x speedup for EventWorkspace
  - :ref:`GetAllEi <algm-GetAllEi>`: 5-10% speedup
  - `GetDetOffsetsMultiPeaks`: 5-10% speedup
  - :ref:`GetEi <algm-GetEi>`: 20% speedup
  - :ref:`MaxEnt <algm-MaxEnt>`: 5% speedup
  - :ref:`ModeratorTzero <algm-ModeratorTzero>`: 30% speedup
  - :ref:`ModeratorTzeroLinear <algm-ModeratorTzeroLinear>`: 40% speedup
  - :ref:`RebinByPulseTimes <algm-RebinByPulseTimes>`: 5-10% speedup
  - :ref:`ScaleX <algm-ScaleX>`: 20% speedup

  In most of these cases memory consumption has also reduced.
  The performance improvements will vary from machine to machine, and will be different or even non-existent depending on the type and size of the input workspace and algorithm parameters.

  The following algorithms were adapted and do not show any speedup, however the memory consumption may have reduced slightly:

  AbsorptionCorrection, CalculateEfficiency, CalculateFlatBackground, CalculateZscore, ConvertEmptyToTof, ConvertToMatrixWorkspace, CrossCorrelate, ExtractFFTSpectrum, FindPeaks, GeneratePeaks, PolarizationCorrection, Rebin2D, RebinByTimeAtSample, ReflectometryTransform, StripPeaks

  Algorithms that are run after one of those listed above may also benefit from the improved data sharing that lead to speedup and reduced memory consumption.
  In some cases, however, follow-up algorithms may run slower (typically this can happen for algorithms that do in-place modification of data).
  However, the total runtime (sum of the runtimes of the improved *and* the degraded algorithm) should be unchanged in the worst case.

- A race condition when accessing a singleton from multiple threads was fixed.

- Log file buffers are no longer flushed by default for each newline received, increasing the speed of some system tests on Windows by 4.5x.


CurveFitting
------------

- Added a new minimizer belonging to the trust region family of algorithms developed for Mantid by the SCD
  Numerical Analysis Group at RAL. It has better performance characteristics compared to the existing
  minimizers especially when applied to the most difficult fitting problems.
- Added new property `EvaluationType` to Fit algorithm. If set to "Histogram" and the input dataset
  is a histogram with large bins it can improve accuracy of the fit.
- The concept page for :ref:`Comparing fit minimizers <FittingMinimizers>` has been updated to include the new
  minimizer and a comparison against neutron data examples.

The work on benchmarking fitting has received funding from the Horizon 2020 Framework Programme of the European Union under the SINE2020 project Grant No 654000.

Others
------

- ``Facilities.xml`` was updated for changes to the SNS live data servers.

- A cmake parameter ``ENABLE_MANTIDPLOT`` (default ``True``) was added to facilitate framework only builds.

- The case search in ``DataService`` has been replaced with a case-insensitive comparison function. Behavior
  is almost identical, but a small number of cases (such as adding the workspaces ``Z`` and ``z``) will work
  in a more predictable manner.


Python
------

- :py:obj:`mantid.kernel.MaterialBuilder` has been exposed to python
  and :py:obj:`mantid.kernel.Material` has been modified to expose the
  individual atoms.
- :py:obj:`mantid.geometry.OrientedLattice` set U with determinant -1 exposed to python
- The setDisplayNormalization and setDisplayNormalizationHisto methods for MDEventWorkspaces are now exposed to Python
- Tube calibration now has ``saveCalibration`` and ``readCalibrationFile`` functions similar to ``savePeak`` and ``readPeakFile``.

Python Algorithms
#################

- New algorithm :ref:`SelectNexusFilesByMetadata <algm-SelectNexusFilesByMetadata>` provides quick filtering of nexus files based on criteria imposed on metadata.

Bug Fixes
---------
- Scripts generated from history including algorithms that added dynamic properties at run time (for example Fit, and Load) will not not include those dynamic properties in their script.  This means they will execute without warnings.
- Cloning a ``MultiDomainFunction``, or serializing to a string and recreating it, now preserves the domains.
- :ref:`EvaluateFunction <algm-EvaluateFunction>` now works from its dialog in the GUI as well as from a script
- :ref:`ConvertToMD <algm-ConvertToMD>` ConvertToMD will now work on powder diffraction samples stored .nxspe files. This is because if a Goniometer contains a NaN value it will report itself as undefined.

|

Full list of
`Framework <http://github.com/mantidproject/mantid/pulls?q=is%3Apr+milestone%3A%22Release+3.8%22+is%3Amerged+label%3A%22Component%3A+Framework%22>`__
and
`Python <http://github.com/mantidproject/mantid/pulls?q=is%3Apr+milestone%3A%22Release+3.8%22+is%3Amerged+label%3A%22Component%3A+Python%22>`__
changes on GitHub
