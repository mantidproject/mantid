=================
Framework Changes
=================

.. contents:: Table of Contents
   :local:


Removals
--------
- Both ``RemoteAlgorithms`` and ``RemoteJobManagers`` subpackages have been removed due to lack of use since v3.7.

Algorithms
----------
New Features
############
- Introduced a naming convention for ``algorithms``, and ``deprecated aliases`` as the preferred method for renaming a C++ or Python algorithms.
- Enabled deprecation of Python algorithms; instructions on how to deprecate a C++ or Python algorithm are available in the developer documentation.
- Improvement for the Liquids Reflectometer at SNS. Added functionality to the :ref:`LRReflectivityOutput <algm-LRReflectivityOutput>` algorithm to automatically compute the Q resolution from the data. An option has been added to the :ref:`LRAutoReduction <algm-LRAutoReduction>` algorithm to switch this feature on and off.
- Added a :ref:`Power Law <func-PowerLaw>` function to General Fit Functions.

Improvements
############
- A relative error option, ``ToleranceRelErr``, is now enabled for peak table workspaces in :ref:`CompareWorkspaces <algm-CompareWorkspaces>`.
- Added the option ``SetErrors`` to :ref:`CreateSimulationWorkspace <algm-CreateSimulationWorkspace>` algorithm to set bin errors.
- The option ``ImportanceSampling`` has been added to :ref:`DiscusMultipleScatteringCorrection <algm-DiscusMultipleScatteringCorrection>` so that it handles spikes in the structure factor S(Q) better.
- Added the parameter ``MaxScatterPtAttempts`` to :ref:`DiscusMultipleScatteringCorrection <algm-DiscusMultipleScatteringCorrection>` to control number of attempts to generate initial scatter point.
- :ref:`GenerateLogbook <algm-GenerateLogbook>` now allows to perform binary operations even when certain entries do not exist, e.g. to create a string with all polarisation orientations contained in a collection of data files.
- Event nexuses produced at ILL can now be loaded using :ref:`LoadEventNexus <algm-LoadEventNexus>`.
- :ref:`Rebin <algm-Rebin>` now has an option ``UseReverseLogarithmic`` for binning with reverse logarithmic and inverse power bins.
- :ref:`SaveAscii <algm-SaveAscii>` and :ref:`SaveCanSAS1D <algm-SaveCanSAS1D>` have a new property ``OneSpectrumPerFile``, controlling whether or not to save each spectrum in an individual file or all the spectra into a single file.
- :ref:`SetSample <algm-SetSample>` can now load sample environment XML files from any directory using ``SetSample(ws, Environment={'Name': 'NameOfXMLFile', 'Path':'/path/to/file/'})``.
- :ref:`SetSampleFromLogs <algm-SetSampleFromLogs>` will now fail if the resulting sample shape has a volume of 0.

Bugfixes
########
- Fixed a bug with :ref:`CalculatePlaczek <algm-CalculatePlaczek>` algorithm for computing Placzek correction factors that fixed the previously implemented formula for transforming k to e, and the summation for second order Placezek corrections.
- :ref:`ConvertAxesToRealSpace <algm-ConvertAxesToRealSpace>` no longer crashes Mantid if using an invalid file.
- Calculation of log binning factor in :ref:`DiffractionFocussing <algm-DiffractionFocussing>` adjusted to remove a small discrepancy between total x range before/after rebinning.
- Fixed a bug in :ref:`DiscusMultipleScatteringCorrection <algm-DiscusMultipleScatteringCorrection>` where the calculation aborts with an exception due to a floating point rounding error when the track segment is close to vertical. Also fixed bug in calculation of track direction after scatter if pre-scatter track was pointing exactly down - sign of z component of new direction was incorrect.
- Fixed a bug in :ref:`Integration <algm-Integration>` when using ``UsePartialBinsOption`` with integration limits that are either equal or close together.
- The :ref:`Load <algm-Load>` algorithm now reports the correct history.
- Fixed a bug in :ref:`LoadAndMerge <algm-LoadAndMerge>` where ``LoaderVersion`` choice was previously ignored.
- Fixed a bug in :ref:`LoadEventNexus <algm-LoadEventNexus>` in checking valid event ID's and to make sure to always exclude data in ``error`` and ``unmapped`` banks.
- Fixed a bug in :ref:`MonteCarloAbsorption <algm-MonteCarloAbsorption>`. If the algorithm was run with the Sparse Workspace feature enabled on a workspace containing spectra
  that didn't have any detectors, it failed with an error.
- Fixed a bug in :ref:`Integration <algm-Integration>` when using it with a ``RebinnedOutput`` workspace (e.g from :ref:`SofQWNormalisedPolygon <algm-SofQWNormalisedPolygon>`) where the output was not correctly normalised.
- Fixed a bug in :ref:`SaveNexus <algm-SaveNexus>` - ragged workspace x-values are now saved correctly when workspace indices are supplied.


Documentation
-------------
Improvements
############
- The source links at the bottom of each algorithm page no longer include the last-modified date. The dates were misleading in most cases as no meaningful
  changes to the algorithm had actually occurred and could be something as simple as formatting changes. Moving forward, each algorithm will now document any significant changes of behaviour across versions in a dedicated
  section within its own page.


Fit Functions
-------------
New Features
############
- Fixed a bug in :ref:`UserFunction<func-UserFunction>` where the view would not be updated with the parameters in the formula entered.

Fitting
-------
Bugfixes
########
- The errors/confidence-bounds on a fitted curve are determined using the covariance matrix without scaling (since v6.0 the matrix was scaled by the reduced chi-squared). This change makes them consistent with the errors on the best fit parameters.

Geometry
---------
Bugfixes
########
- Fixed an issue in ``CSGObject`` such that the intercept type is no longer tied to an arbitrary value that make Track returns unstable results.

Python
------
New features
############
* ``isGroup`` can now be used to determine if a workspace/table workspace is a grouped workspace object.
* ``createChildAlgorithm`` now accepts property keyword arguments to set the child algorithm's properties during creation:

  *  Existing arguments, such as ``version``, ``start`` and ``end progress`` etc. are unaffected by this change.
  *  E.g. ``createChildAlgorithm("CreateSampleWorkspace", version=1, XUnit="Wavelength")``.

* The package on Windows now includes the `euphonic <https://pypi.org/project/euphonic/>`_ package
  for calculating phonon bandstructures.


MantidWorkbench
---------------
See :doc:`mantidworkbench`.


:ref:`Release 6.3.0 <v6.3.0>`
