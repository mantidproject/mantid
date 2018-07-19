=================
Framework Changes
=================

.. contents:: Table of Contents
   :local:

Instrument Definition Updates
-----------------------------

- The ALF IDF has been updated following a detector array alteration.
- The LARMOR IDF has been updated following the addition of a new detector to the instrument.
- Instrument definition for IN4, 5, and 6 at the ILL have been updated
- REF_L and CNCS has update instrument files and EQSANS, REF_M, ARCS and CNCS are now configured for loading live data.

Stability
---------

- We have introduced a Project Recovery mechanism for Mantidplot in order to be able to recover the lost state of the interface in the event of a crash or unexpected shutdown.  There are more details in the UI section of the release notes.
- The error reporter can now catches hard crashes to desktop, allowing us to get more information on causes of hangs or crashes in Mantid.  Since the last release error reports sent to us led directly to the identification of and fixes for 3 seperate bugs in Mantid.
- Mantid now handles poor network stability  better when reading live data from the ISIS DAE.  Mantid will now timeout after a couple of minutes of loss of network connectivity and remains responsive during this time.  
  You can alter the duration of this timeout by adding a line to the mantid.user.properties file like:
  ```
  ISISDAE.Timeout = 100 #seconds
  ```


Algorithms
----------

New feature
###########

- Sometimes the algorithm you are looking at is close to what you want, but not quite, to help you find the right one a list of Related Algorithms has been added to each algorithm, and is displayed in the documentation page of each algorithm as part of it's summary.

New Algorithms
##############

- :ref:`LoadSampleShape <algm-LoadSampleShape>` loads a shape into the sample in a workspace from an
  ASCII `STL <https://en.wikipedia.org/wiki/STL_(file_format)>`_  file,
  which contains a list of triangles or an
  ASCII `OFF <https://en.wikipedia.org/wiki/OFF_(file_format)>`_ file,
  which has a list of vertices and triangles.

- :ref:`CalculateCarpenterSampleCorrection <algm-CalculateCarpenterSampleCorrection>` outputs a group workspace with the separate absorption and multiple scattering corrections for flexibility to the User to apply them to the sample workspace

- :ref:`CarpenterSampleCorrection <algm-CarpenterSampleCorrection>` replaces *MultipleScatteringCylinderAbsorption* and uses :ref:`CalculateCarpenterSampleCorrection <algm-CalculateCarpenterSampleCorrection>` for calculating its corrections.

- :ref:`SaveBankScatteringAngles <algm-SaveBankScatteringAngles>` was added to save theta and phi values for diffraction banks to a MAUD-readable `grouping.new` file format

- :ref:`ExportSampleLogsToHDF5 <algm-ExportSampleLogsToHDF5>` saves a
  workspace's samples logs to an HDF5 file

- :ref:`SaveGDA <algm-SaveGDA>` saves a focused diffraction workspace to MAUD-readable ``.gda`` format

- :ref:`SaveGEMMAUDParamFile <algm-SaveGEMMAUDParamFile>`, which acts as a partner to :ref:`SaveGDA <algm-SaveGDA>`,
  saves a MAUD calibration file to convert the output of **SaveGDA** back to d-spacing

- Algorithm :ref:`FitPeaks <algm-FitPeaks>` is implemented as a generalized multiple-spectrum multiple-peak fitting algorithm.

Improvements
############

- :ref:`LoadMcStas <algm-LoadMcStas>` new alg property which controls the granularity of event data returned.
- :ref:`Maxent <algm-Maxent>` when outputting the results of the iterations, it no longer pads with zeroes but
  returns as many items as iterations done for each spectrum, making the iterations easy to count.
- XError values (Dx) can now be treated by the following algorithms: :ref:`ConjoinXRuns <algm-ConjoinXRuns>`, :ref:`ConvertToHistogram <algm-ConvertToHistogram>`, :ref:`ConvertToPointData <algm-ConvertToPointData>`, :ref:`CreateWorkspace <algm-CreateWorkspace>`, :ref:`SortXAxis <algm-SortXAxis>`, :ref:`algm-Stitch1D` and :ref:`algm-Stitch1DMany` (both with respect to point data).
- :ref:`Stitch1D <algm-Stitch1D>` can treat point data.
- The algorithm :ref:`SortXAxis <algm-SortXAxis>` has a new input option that allows ascending (default) and descending sorting. The documentation needed to be corrected in general.
- :ref:`LoadNexusMonitors <algm-LoadNexusMonitors>` has changed its properties for clarification. This has also propagated to :ref:`LoadEventNexus <algm-LoadEventNexus>` and :ref:`LoadEventAndCompress <algm-LoadEventAndCompress>`
- :class:`mantid.kernel.FacilityInfo` has an additional field with the timezone for use in converting ``numpy.datetime64`` values to strings

Bugfixes
########

- In :ref:`LoadMcStas <algm-LoadMcStas>` internally reduce number of event workspaces created. If n mcstas event components now create n*(n-1) fewer.
- The documentation of the algorithm :ref:`algm-CreateSampleWorkspace` did not match its implementation. The axis in beam direction will now be correctly described as Z instead of X.
- The :ref:`ExtractMask <algm-ExtractMask>` algorithm now returns a non-empty list of detector ID's when given a MaskWorkspace.
- Fixed a crash when the input workspace for :ref:`GroupDetectors <algm-GroupDetectors>` contained any other units than spectrum numbers.
- :ref:`ConvertToMD <algm-ConvertToMD>` can now be used with workspaces that aren't in the ADS.
- Fixed :ref:`SumSpectra <algm-SumSpectra>` to avoid a crash when validation of inputs was called with a WorkspaceGroup.
- Fixed a bug in TableWorkspaces where vector column data was set to 0 when the table was viewed
- The output workspace of :ref:`LineProfile <algm-LineProfile>` now has correct sample logs, instrument and history.
- TimeSeriesProperty::splitByTimeVector's behavior on a boundary condition is changed.  In the set of splitters toward a same target split workspace, if a splitter's beginning time is after the last entry of the TimeSeriesProperty to be split, then this last entry shall be included in its output TimeSeriesProperty.
- Fixed a bug in :ref:`MergeRuns <algm-MergeRuns>` which could cause the runs to be merged in a different sequence than indicated in the *InputWorkspaces* property.
- Fixed a bug where the values entered for basis vector properties in :ref:`BinMD <algm-BinMD>` were not being remembered.
- Fixed a bug which prevented :ref:`Load <algm-Load>` and :ref:`LoadAndMerge <algm-Load>` from parsing advanced run ranges such as ``1-3+5-7+10+15-20``.


Python
------

New
###

- Added a new ``MDFrameValidator`` which can check that a MD workspace passed to a python algorithm has the expected MD frame (e.g. HKL, QLab, QSample etc.).

Improvements
############

- Python fit functions that use from ``IPeakFunction`` as a base no longer require a ``functionDeriveLocal`` method to compute an analytical derivative. If
  the method is absent then a numerical derivative is calculate.

Bugfixes
########

- Checks on the structure of Python fit function classes have been improved to avoid scenarios, such as writing ``function1d`` rather than ``function1D``, which
  would previously have resulted in a hard crash.
- Fit functions defined in a python script can be used with the new fit function API right after subscription.
- Child algorithms now respect their parent algorithm's ``EnableLogging`` setting when invoked using the function-style calling. Previously, some messages could appear in the log even though ``EnableLogging`` was set to ``False``.
- Fixed a bug in ``detectorSignedTwoTheta`` method in ``MatrixWorkspace`` where the sign of the angle depended on the axis pointing up, not on the actual theta-sing axis defined in the IDF.
- Fixed a bug where calling ``setCell`` method on a :ref:`PeaksWorkspace <PeaksWorkspace>` did not set the cell value.


:ref:`Release 3.13.0 <v3.13.0>`
