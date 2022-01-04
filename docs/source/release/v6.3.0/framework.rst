=================
Framework Changes
=================

.. contents:: Table of Contents
   :local:

.. warning:: **Developers:** Sort changes under appropriate heading
    putting new features at the top of the section, followed by
    improvements, followed by bug fixes.

Removals
--------
- Both ``RemoteAlgorithms`` and ``RemoteJobManagers`` subpackages have been removed due to lack of use since v3.7.

Algorithms
----------
- Introducing a naming convention for algorithms, and *deprecated aliases* as the preferred method for renaming a C++ or Python algorithm.
- Enabling deprecation of Python algorithms; instructions on how to deprecate a C++ or Python algorithm in the developer documentation.

New Features
############
- Added a :ref:`Power Law <func-PowerLaw>` function to General Fit Functions.

Improvements
############

- :ref:`SaveAscii <algm-SaveAscii>` and :ref:`SaveCanSAS1D <algm-SaveCanSAS1D>` have a new property OneSpectrumPerFile, controlling whether or not to save each spectrum in an individual file or all the spectra into a single file.
- :ref:`GenerateLogbook <algm-GenerateLogbook>` now allows to perform binary operations even when certain entries do not exist, e.g. to create a string with all polarisation orientations contained in a collection of data files.
- Event nexuses produced at ILL can now be loaded using :ref:`LoadEventNexus <algm-LoadEventNexus>`.
- :ref:`Rebin <algm-Rebin>` now has an option for binning with reverse logarithmic and inverse power bins.
- :ref:`SetSampleFromLogs <algm-SetSampleFromLogs>` will now fail if the resulting sample shape has a volume of 0.
- :ref:`SetSample <algm-SetSample>` can now load sample environment XML files from any directory using ``SetSample(ws, Environment={'Name': 'NameOfXMLFile', 'Path':'/path/to/file/'})``.
- An importance sampling option has been added to :ref:`DiscusMultipleScatteringCorrection <algm-DiscusMultipleScatteringCorrection>` so that it handles spikes in the structure factor S(Q) better
- Added parameter to :ref:`DiscusMultipleScatteringCorrection <algm-DiscusMultipleScatteringCorrection>` to control number of attempts to generate initial scatter point
- Relative error option now enabled for peak table workspaces in :ref:`CompareWorkspaces <algm-CompareWorkspaces>`.

Bugfixes
########

- Fix bug in :ref:`LoadEventNexus <algm-LoadEventNexus>` in checking valid event ID's and make sure to always exclude data in ``error`` and ``unmapped`` banks.
- Fix bug in :ref:`Integration <algm-Integration>` when using UsePartialBinsOption with integration limits that are either equal or close together
- Fix bug in :ref:`DiscusMultipleScatteringCorrection <algm-DiscusMultipleScatteringCorrection>` where calculation aborts with exception due to floating point rounding error when track segment close to vertical
  Also fixed bug in calculation of track direction after scatter if pre-scatter track was pointing exactly down - sign of z component of new direction was incorrect
- The :ref:`Load <algm-Load>` algorithm now reports the correct history.
- Fix bug in :ref:`LoadAndMerge <algm-LoadAndMerge>` where LoaderVersion choice was previously ignored
- Fix bug in :ref:`SaveNexus <algm-SaveNexus>` - ragged workspace x-values are saved correctly when workspace indices are supplied.
- Fix bug in :ref:`MonteCarloAbsorption <algm-MonteCarloAbsorption>`. If the algorithm was run with the Sparse Workspace feature enabled on a workspace containing spectra
  that didn't have any detectors it failed with an error

Fit Functions
-------------
New Features
############
- Fixed a bug in :ref:`UserFunction<func-UserFunction>` where the view would not be updated with the parameters in the formula entered.

Data Objects
------------

Geometry
----------
- add additional unit test for Rasterize class.
- fix an issue in CSGObject such that the intercept type is no longer tied to an arbitrary value that make Track returns unstable results.

Python
------

- `isGroup` can now be used to determine if a workspace/table workspace is a grouped workspace object.
- `createChildAlgorithm` now accepts property keyword arguments to set the child algorithm's properties during creation:

  -  Existing arguments, such as version, start and end progress...etc. are unaffected by this change.
  -  E.g. `createChildAlgorithm("CreateSampleWorkspace", version=1, XUnit="Wavelength")`

Installation
------------

MantidWorkbench
---------------

See :doc:`mantidworkbench`.

SliceViewer
-----------

Improvements
############

Bugfixes
########
- Fix out-of-range error when trying to access the projection matrix in sliceviewer for a workspace with a non-Q axis before other Q axes.
- For MDHisto workspaces get the projection matrix from the basis vectors on the workspace rather than search for the W_MATRIX log.


- Fixed bug in :ref:`Run <Run>` goniometer when using :ref:`algm-Plus`.
- Fixed issue in SNSLiveEventDataListener when the instrument doesn't have monitors

:ref:`Release 6.3.0 <v6.3.0>`
