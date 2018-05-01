=================
Framework Changes
=================

.. contents:: Table of Contents
   :local:

Instrument Definitinons
-----------------------

- The reference frame in :ref:`IDF <InstrumentDefinitionFile>` can now be customized in terms of setting the axis defining the 2theta sign.
- The detector efficiency correction formulas for ILL's IN4, IN5 and IN6 spectrometers have been revised. IN4 has now separate formulas for the low and large angle detectors while the previously incorrect IN5 and IN6 formulas were fixed.

Algorithms
----------

New
###

- :ref:`algm-CalculatePolynomialBackground` fits a polynomial background to a workspace.
- :ref:`ConjoinXRuns <algm-ConjoinXRuns>` performs concatenation of the workspaces into a single one by handling the sample logs merging as in :ref:`MergeRuns <algm-MergeRuns>`.
- :ref:`LoadSESANS <algm-LoadSESANS>` Loading SESANS data to a MatrixWorkspace is now supported.
- :ref:`SaveSESANS <algm-SaveSESANS>` Saving a workspace using the SESANS format is now supported.
- :ref:`PaddingAndApodization <algm-PaddingAndApodization-v1>` a new algorithm for padding data and adding an apodization function.
- :ref:`algm-IntegrateEPP` integrates a workspace around the elastic peak positions given in an EPP table.

Improved
########

- :ref:`ConvertSpectrumAxis <algm-ConvertSpectrumAxis-v2>` is extended to support conversion to elastic d-spacing.
- :ref:`SumSpectra <algm-SumSpectra-v1>`: Fixed a bug where a wrong fallback value would be used in case of invalid values being set for min/max worspace index, and improved input validation for those properties.
- :ref:`SetUncertainties <algm-SetUncertainties-v1>` now provides a "custom" mode, which lets the user specify both an arbitrary error value whose occurences are to be replaced in the input workspace, as well as the value to replace it with.
- :ref:`LoadBBY <algm-LoadBBY-v1>` is now better at handling sample information.
- :ref:`ConjoinWorkspaces <algm-ConjoinWorkspaces-v1>` provides option to change Y axis unit and label.
- :ref:`FilterEvents <algm-FilterEvents-v1>` has refactored on splitting sample logs.
- :ref:`FilterEvents <algm-FilterEvents-v1>` now copies units for the logs in the filtered workspaces
- :ref:`GroupDetectors <algm-GroupDetectors-v2>` now supports workspaces with detector scans.
- :ref:`FindPeaksMD <algm-FindPeaksMD-v1>` now allows users to normalize by the number of events. This can improve results for data that was originally based on histogram data which has been converted to event-mode.
- :ref:`FindSXPeaks <algm-FindSXPeaks-v1>` now finds all peaks in each spectrum. It also allows for setting more fine-grained resolutions and takes into account any goniometer set on the workspace.
- :ref:`SimpleShapeMonteCarloAbsorption <algm-SimpleShapeMonteCarloAbsorption>` has been added to simplify sample environment inputs for MonteCarloAbsorption
- :ref:`IntegreatePeaksMD <algm-IntegratePeaksMD-v2>` makes the culling of the top one percent of the background events optional.
- :ref:`IntegrateEllipsoids <algm-IntegrateEllipsoids-v1>` has the culling of the top one percent of the background events now as an optional input.
- :ref:`IntegrateEllipsoidsTwoStep <algm-IntegrateEllipsoidsTwoStep-v1>` has the culling of the top one percent of the background events now as an optional input.
- :ref:`IntegreatePeaksMD <algm-IntegratePeaksMD-v2>` makes the culling of the top one percent of the background events optional.
- :ref:`Load <algm-Load-v1>` now supports use of tilde in file paths in Python, for example Load(Filename="~/data/test.nxs", ...)
- :ref:`LoadBBY <algm-LoadBBY-v1>` is now better at handling sample information.
- :ref:`MonteCarloAbsorption <algm-MonteCarloAbsorption-v1>` has had several improvements:

  * It now supports approximating the input instrument with a sparse grid of detectors enabling quick simulation of huge pixel arrays.
  * The NumberOfWavelengthPoints input property is now validated more rigorously.
  * A new MaxScatterPtAttempts input has been added to control how many tries are made to generate a random point in the object. Useful for cases such as thin annuli that require a higher number of tries. The previous version was hard coded internally.
- :ref:`SaveGSS <algm-SaveGSS-v1>` now supports saving in the legacy GSAS ALT format. This is useful for older tools however the default format FXYE should be used whenever possible.
- :ref:`SaveMDWorkspaceToVTK <algm-SaveMDWorkspaceToVTK-v1>` and :ref:`LoadVTK <algm-LoadVTK-v1>` algorithms are now accessible from python.
- :ref:`MergeRuns <algm-MergeRuns-v1>` will now merge workspaces with detector scans.
- :ref:`SetUncertainties <algm-SetUncertainties-v1>` now provides a "custom" mode, which lets the user specify both an arbitrary error value whose occurences are to be replaced in the input workspace, as well as the value to replace it with.
- :ref:`SimpleShapeMonteCarloAbsorption <algm-SimpleShapeMonteCarloAbsorption>` has been added to simplify sample environment inputs for MonteCarloAbsorption
- :ref:`SumSpectra <algm-SumSpectra-v1>`: Fixed a bug where a wrong fallback value would be used in case of invalid values being set for min/max worspace index, and improved input validation for those properties.
- :ref:`LoadBBY <algm-LoadBBY-v1>`: Fixed bug where the logManager did not work with sample_name, sample_aperture and source_aperture. Also added more information regarding the sample and the selected choppers.
- :ref:`ConvertSpectrumAxis <algm-ConvertSpectrumAxis-v2>`: Added an option to disable the sorting of the resulting axis making it useful especially for scanning workspaces. Also reduced the complexity of the operation for the default (ordered axis) case from *Nˆ2* to *N*.
- :ref:`MSDFit <algm-MSDFit>` now supports model selection. Currently has the option of 3 models: MsdGauss, MsdPeters and MsdYi.
- :ref:`algm-LineProfile`: Fixed a bug which could cause crashes when the line extended over the right or bottom edge of a workspace.
- :ref:`algm-Mean`: Added error messages if the data is not appropriate.
- :ref:`algm-LoadLiveData`: Fixed a bug affecting Live Data Processing in "Replace" mode. The bug meant that changes to Instrument position/rotation were overwitten by defaults on every load. Now fixed so that Instrument state is persistent across loads.
- :ref:`CalMuonDetectorPhasees <algm-CalMuonDetectorPhases-v1>` now fits a cos instead of a sin function.

Performance
-----------
- Performance of UB indexing routines addressed. :ref:`FindUBUsingLatticeParameters <algm-FindUBUsingLatticeParameters-v1>` running 2x faster than before.
- Several changes to the core of how instrument geometry is stored and accessed. These changes have resulted in a few noteworthy performance improvements.

  * Partial loading of event nexus files has improved by 22%.
  * The LoadNexusMonitors algorithm has improved by 30%.
  * The ConvertSpectrumAxis algorithm has improved by 8%.

Core functionality
------------------

- The ``IndexProperty`` has been added to the list of property types.
- The ``blocksize()`` of a workspace now throws an exception if the number of bins is not constant. ``size()`` has been modified to the sum of the number of bins in each ``Histogram``.


CurveFitting
------------

New
###

- :ref:`PrimStretchedExpFT <func-PrimStretchedExpFT>` Provides the Fourier Transform of the Symmetrized Stretched Exponential Function integrated over each energy bin. Use in place of :ref:`StretchedExpFT <func-StretchedExpFT>` for fitting sample data featuring relaxation times longer than the resolution of the instrument.
- :ref:`GramCharlier <func-GramCharlier>` is a new fit function primarily for use in neutron compton scattering.
- :ref:`SplineInterpolation <algm-SplineInterpolation>` is extended to support also linear interpolation, if only 2 points are given.

Improved
########

- :ref:`Fit <algm-Fit>` outputs a function object containing the optimized parameter values. See the usage examples for more detail.
- :ref:`CubicSpline <func-CubicSpline>` is fixed to sort the y-values and x-values correctly.
- Fix displayed type name for optional boolean properties.
- Fix parameters that are tied to functions can now be untied correctly.
- Table workspaces do not have the values changed by viewing them (no rounding).

Python
------

- :py:obj:`mantid.kernel.MaterialBuilder` had an issue when setting the mass density with more than one atom in the chemical formula. This is now fixed, so the number density is correctly set in :py:obj:`mantid.kernel.Material` and the cross sections correctly calculated.

Python Algorithms
#################

- Exposed `StringContainsValidator` to python to enable python algorithms to place requirement on input string to contain certain substrings.

Bugfixes
########

- :ref:`MatchPeaks <algm-MatchPeaks-v1>` is fixed to not to leave temporary hidden workspaces behind.
- Fix a bug where Mantid could crash when trying to update live data if the network connection is lost.

Python Fit Functions
####################

- A bug that makes it difficult to define and use attributes in python fit functions has been fixed.
- The usability of the fit functions has been improved, enabling users to construct and modify the functions as objects rather than strings
  as described :ref:`here <FitFunctionsInPython>`.



|

Full list of
`Framework <http://github.com/mantidproject/mantid/pulls?q=is%3Apr+milestone%3A%22Release+3.11%22+is%3Amerged+label%3A%22Component%3A+Framework%22>`__
and
`Python <http://github.com/mantidproject/mantid/pulls?q=is%3Apr+milestone%3A%22Release+3.11%22+is%3Amerged+label%3A%22Component%3A+Python%22>`__
changes on GitHub
