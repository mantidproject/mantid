=================
Framework Changes
=================

.. contents:: Table of Contents
   :local:


Changes
-------

- Mantid is aware of the change to file naming for Vesuvio, you can continue to use EVS or VESUVIO as a prefix when trying to load files from this instrument, and Mantid will map that to the correct filenames.


Algorithms
----------

New
###

- :ref:`ConvertToConstantL2 <algm-ConvertToConstantL2>` is the new name for CorrectFlightPaths.
- :ref:`BinWidthAtX <algm-BinWidthAtX>` calculates the bin width at X, averaged over all histograms. 
- :ref:`MedianBinWidth <algm-MedianBinWidth>` provides the median bin widths of histograms.


Improved
########

- :ref:`CalculateFlatBackground <algm-CalculateFlatBackground>` has a new mode 'Moving Average', which takes the minimum of a moving window average as the flat background.
- :ref:`StartLiveData <algm-StartLiveData>` and its dialog now support dynamic listener properties, based on the specific LiveListener being used.
- All algorithms using AsciiPointBase now have a new property 'Separator' which allows the delimiter to be set to either comma, space or tab. This affects :ref:`SaveReflCustomAscii <algm-SaveReflCustomAscii>`, :ref:`SaveReflThreeColumnAscii <algm-SaveReflThreeColumnAscii>`, :ref:`SaveANSTOAscii <algm-SaveANSTOAscii>` and :ref:`SaveILLCosmosAscii <algm-SaveILLCosmosAscii>`.
- :ref:`ReplaceSpecialValues <algm-ReplaceSpecialValues>` allows for 'small' values, which are below a user specified threshold, to be replaced.
- :ref:`Stitch1DMany <algm-Stitch1DMany>` has a new property 'ScaleFactorFromPeriod', which enables it to apply scale factors from a particular period when stitching group workspaces. The documentation for this algorithm has also been improved.
- :ref:`SaveMDWorkspaceToVTK <algm-SaveMDWorkspaceToVTK>` has a working progress bar.
- :ref:`SumSpectra <algm-SumSpectra>` has an option to ignore special floating point values called 'RemoveSpecialValues'. This is off by default. When enabled it will ignore values such as NaN or Infinity during the summation of the spectra.  It was also updated to fix special values being used in some cases when the option was selected.
- :ref:`MonteCarloAbsorption <algm-MonteCarloAbsorption>`:
   - An `Interpolation` option has been added. The availabile options are: `Linear` & `CSpline`.
   - The method of selecting the scattering point has ben updated to give better agreement with numerical algorithms (e.g. :ref:`CylinderAbsorption <algm-CylinderAbsorption>`).
- :ref:`SetSample <algm-SetSample>` now accepts an Angle argument for defining a rotated flat plate sample.
- :ref:`MaskDetectors <algm-MaskDetectors>` has a new option to mask detectors by the instrument's component name. It can accept a masked workspace with a differing number of spectra to the input workspace, providing that the number of detectors match. This can be useful in the case of hardware grouped detectors.
- :ref:`SavePlot1D <algm-SavePlot1D>` now supports optional ``SpectraList`` for plotting
- :ref:`MayersSampleCorrection <algm-MayersSampleCorrection>`: The calculation of the azimuth angle has been fixed. Previously it was set equal to the Mantid definition of phi but the old code defined it as the angle away from the scattering plane.
Renamed
#######

- :ref:`CorrectFlightPaths <algm-ConvertToConstantL2>` has been renamed to :ref:`ConvertToConstantL2 <algm-ConvertToConstantL2>`.

Deprecated
##########

- :ref:`AbortRemoteJob	 <algm-AbortRemoteJob>` is deprecated in favour of v2.
- :ref:`Authenticate	 <algm-Authenticate>`  is deprecated in favour of v2.
- :ref:`CentroidPeaksMD	 <algm-CentroidPeaksMD>`  is deprecated in favour of v2.
- :ref:`ConvertEmptyToTof	 <algm-ConvertEmptyToTof>`.
- :ref:`ConvertUnitsUsingDetectorTable	 <algm-ConvertUnitsUsingDetectorTable>`.
- :ref:`DownloadRemoteFile	 <algm-DownloadRemoteFile>` is deprecated in favour of v2.
- :ref:`FFTSmooth	 <algm-FFTSmooth>` is deprecated in favour of v2.
- :ref:`OneStepMDEW	 <algm-OneStepMDEW>`.
- :ref:`QueryAllRemoteJobs	 <algm-QueryAllRemoteJobs>` is deprecated in favour of v2.
- :ref:`RefinePowderInstrumentParameters	 <algm-RefinePowderInstrumentParameters>` is deprecated in favour of v2.
- :ref:`SetupILLD33Reduction	 <algm-SetupILLD33Reduction>`.
- :ref:`StartRemoteTransaction	 <algm-StartRemoteTransaction>` is deprecated in favour of v2.
- :ref:`LoadILLAscii	 <algm-LoadILLAscii>`.
- :ref:`StopRemoteTransaction	 <algm-StopRemoteTransaction>` is deprecated in favour of v2.
- :ref:`SubmitRemoteJob	 <algm-SubmitRemoteJob>` is deprecated in favour of v2.
- :ref:`Transpose3D	 <algm-Transpose3D>` is deprecated in favour :ref:'TransposeMD <algm_TransposeMD>'.

Removed
#######

The following (previously deprecated) algorithms versions have now been removed:

- LoadEventPreNexus v1
- LoadLogsForSNSPulsedMagnet v1
- Lorentzian1D v1
- ProcessDasNexusLog v1
- LoadILL v1
- SANSDirectBeamScaling v1

CurveFitting
------------

- Systemtest and FittingBenchmarks have been added for testing the minimizer, the scripts generate the tables displayed on :ref:`FittingMinimzers page <FittingMinimizers>`. This Systemtest also demo how these tables can be created as a standard Mantid script.
- Recommendations for which fitting method to use for a given data set has been added to :ref:`FittingMinimzers page <FittingMinimizers>`.
- Algorithm :ref:`CalculateCostFunction <algm-CalculateCostFunction>` calculates a value of any available cost function.
- Algorithm :ref:`EstimateFitParameters <algm-EstimateFitParameters>` estimates the initial values of a fiting function in given intervals.
- `Exclude` is new property of :ref:`Fit <algm-Fit>`, which allows for a user defined range to be excluded from a fit. 
- Fit Function :ref:`FunctionQDepends <func-FunctionQDepends>` as the base class for QENS models depending on Q.

Improved
########

- The `Peak Radius` global setting for 1D peaks is replaced with `PeakRadius` property of the :ref:`Fit <algm-Fit>` algorithm (see algorithm's description for the details).

.. figure:: ../../images/NoPeakRadius_3.9.png
   :class: screenshot
   :width: 550px

- The output and normalization MDHistoWorkspaces from :ref:`MDNormSCD <algm-MDNormSCD>` and :ref:`MDNormDirectSC <algm-MDNormDirectSC>` have the 'displayNormalization' set to 'NoNormalization'. For older outputs, the `setDisplayNormalization` function is now exposed to python.

Python
------

- The function `IMDDimension.getName()` has been deprecated in favour of `IMDDimension.name'.
- The duplicate function `Workspace.getName()` has been deprecated in favour of `Workspace.name()'.

Python Algorithms
#################

- :ref:`MatchPeaks <algm-MatchPeaks>` performs circular shift operation (numpy roll) along the x-axis to align the peaks in the spectra.
- :ref:`FindEPP <algm-FindEPP>` is improved to better determine the initial parameters and range for the fitting.
- :ref:`StartLiveData <algm-StartLiveData>` can now accept LiveListener properties as parameters, based on the value of the "Instrument" parameter.
- Bin masking information was incorrectly saved when converting workspaces into nexus files, which is now fixed.
- :ref:`LoadEventNexus <algm-LoadEventNexus>` should no longer leak memory when the execution is cancelled.
- :ref:`LoadNexusProcessed <algm-LoadNexusProcessed>` will now load the stored workspace names from a processed Nexus file in the case of multiperiod data.
- If a run is aborted and restarted, the ``running`` log in the workspace will correctly reflect this. (``running`` will be false at all times before the abort.)
- Fixed several issues with masked detectors and neighbour counts in the nearest-neighbour code used by a few algorithms.
- Issues with :ref:`CalculateFlatBackground <algm-CalculateFlatBackground>` with  **Return Background** option returning fake values has been fixed.

Full list of
`Framework <http://github.com/mantidproject/mantid/pulls?q=is%3Apr+milestone%3A%22Release+3.9%22+is%3Amerged+label%3A%22Component%3A+Framework%22>`__
and
`Python <http://github.com/mantidproject/mantid/pulls?q=is%3Apr+milestone%3A%22Release+3.9%22+is%3Amerged+label%3A%22Component%3A+Python%22>`__
changes on GitHub
