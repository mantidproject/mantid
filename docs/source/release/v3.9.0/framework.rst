=================
Framework Changes
=================

.. contents:: Table of Contents
   :local:

Facility Updates
----------------

- Mantid now supports automatic updates to the facilities.xml file in the same way that it does the instrument definitions.  This allows extensions and changes to the list of supported instruments without needing to install a new release of Mantid.
  - This has been initially put into place to support a data file naming change for Vesuvio, but will help for future changes as well.

Changes
-------

- Mantid is aware of the change to file naming for Vesuvio, you can continue to use EVS or VESUVIO as a prefix when trying to load files from this instrument, and Mantid will map that to the correct filenames.

Algorithms
----------

New
###

- :ref:`ConvertToConstantL2 <algm-ConvertToConstantL2>` is the new name for CorrectFlightPaths.
- :ref:`BinWidthAtX <algm-BinWidthAtX>` and :ref:`MedianBinWidth <algm-MedianBinWidth>` provide information about the bin widths of histograms.


Improved
########

- :ref:`CalculateFlatBackground <algm-CalculateFlatBackground>` has a new mode 'Moving Average' which takes the minimum of a moving window average as the flat background.
- :ref:`StartLiveData <algm-StartLiveData>` and its dialog now support dynamic listener properties, based on the specific LiveListener being used.
- All algorithms using AsciiPointBase now have a new property 'Separator' which allows the delimiter to be set to either comma, space or tab. This affects :ref:`SaveReflCustomAscii <algm-SaveReflCustomAscii>`, :ref:`SaveReflThreeColumnAscii <algm-SaveReflThreeColumnAscii>`, :ref:`SaveANSTOAscii <algm-SaveANSTOAscii>` and :ref:`SaveILLCosmosAscii <algm-SaveILLCosmosAscii>`.
- :ref:`ReplaceSpecialValues <algm-ReplaceSpecialValues>` now can replace 'small' values below a user specified threshold.
- :ref:`Stitch1DMany <algm-Stitch1DMany>` has a new property 'ScaleFactorFromPeriod' which enables it to apply scale factors from a particular period when stitching group workspaces. The documentation for this algorithm has also been improved.
- :ref:`SaveMDWorkspaceToVTK <algm-SaveMDWorkspaceToVTK>` has a working progress bar.
- :ref:`SumSpectra <algm-SumSpectra>` has an option to ignore special floating point values called 'RemoveSpecialValues'. This is off by default but when enabled will ignore values such as NaN or Infinity during the summation of the spectra.  It was also updated to fix special values being used in some cases when the option was selected.
- :ref:`MonteCarloAbsorption <algm-MonteCarloAbsorption>`:
   * an `Interpolation` option has been added. Availabile options are: `Linear` & `CSpline`.
   * the method of selecting the scattering point has ben updated to give better agreement with numerical algorithms such as :ref:`CylinderAbsorption <algm-CylinderAbsorption>`.
- :ref:`SetSample <algm-SetSample>` now accepts an Angle argument for defining a rotated flat plate sample.
- :ref:`SavePlot1D <algm-SavePlot1D>` now supports optional ``SpectraList`` for plotting
- :ref:`MaskDetectors <algm-MaskDetectors>` has now a new option to mask detectors by instrument component name.
- :ref:`MayersSampleCorrection <algm-MayersSampleCorrection>`: The calculation of the azimuth angle has been fixed. Previously it was set equal to the Mantid definition of phi but the old code defined it as the angle away from the scattering plane.

Renamed
#######

- ref:`CorrectFlightPaths <algm-ConvertToConstantL2>` has been renamed to :ref:`ConvertToConstantL2 <algm-ConvertToConstantL2>`.

Deprecated
##########

- :ref:`AbortRemoteJob	 <algm-AbortRemoteJob>` use version 2 instead.
- :ref:`Authenticate	 <algm-Authenticate>` use version 2 instead.
- :ref:`CentroidPeaksMD	 <algm-CentroidPeaksMD>` use version 2 instead.
- :ref:`ConvertEmptyToTof	 <algm-ConvertEmptyToTof>`.
- :ref:`ConvertUnitsUsingDetectorTable	 <algm-ConvertUnitsUsingDetectorTable>`.
- :ref:`DownloadRemoteFile	 <algm-DownloadRemoteFile>` use version 2 instead.
- :ref:`FFTSmooth	 <algm-FFTSmooth>` use version 2 instead.
- :ref:`OneStepMDEW	 <algm-OneStepMDEW>`.
- :ref:`QueryAllRemoteJobs	 <algm-QueryAllRemoteJobs>` use version 2 instead.
- :ref:`RefinePowderInstrumentParameters	 <algm-RefinePowderInstrumentParameters>` use version 2 instead.
- :ref:`SetupILLD33Reduction	 <algm-SetupILLD33Reduction>`.
- :ref:`StartRemoteTransaction	 <algm-StartRemoteTransaction>` use version 2 instead.
- :ref:`LoadILLAscii	 <algm-LoadILLAscii>`.
- :ref:`StopRemoteTransaction	 <algm-StopRemoteTransaction>` use version 2 instead.
- :ref:`SubmitRemoteJob	 <algm-SubmitRemoteJob>` use version 2 instead.
- :ref:`Transpose3D	 <algm-Transpose3D>` use TransposeMD instead.

Removed
#######

The following (previously deprecated) algorithms versions have now been removed:

- LoadEventPreNexus v1
- LoadLogsForSNSPulsedMagnet v1
- Lorentzian1D v1
- ProcessDasNexusLog v1
- LoadILL v1
- SANSDirectBeamScaling v1


MD Algorithms (VATES CLI)
#########################

Performance
-----------

CurveFitting
------------

- Systemtest, FittingBenchmarks, added for testing fit minimizer benchmarking scripts generating the tables displayed on :ref:`FittingMinimzers page <FittingMinimizers>`. This Systemtest also demo how these tables can be created as a standard Mantid script.
- Recommendations for which fitting to use added to :ref:`FittingMinimzers page <FittingMinimizers>`.
- Algorithm :ref:`CalculateCostFunction <algm-CalculateCostFunction>` calculates a value of any available cost function.
- Algorithm :ref:`EstimateFitParameters <algm-EstimateFitParameters>` estimates initial values of a fiting function in given intervals.
- New property of :ref:`Fit <algm-Fit>` `Exclude` sets ranges that need to be excluded from a fit.
- Fit Function :ref:`FunctionQDepends <func-FunctionQDepends>` as the base class for QENS models depending on Q.

Improved
########

- The `Peak Radius` global setting for 1D peaks that limits the interval on which they are calculated is replaced with `PeakRadius` property of the :ref:`Fit <algm-Fit>` algorithm (see algorithm's description for the details).

.. figure:: ../../images/NoPeakRadius_3.9.png
   :class: screenshot
   :width: 550px

- The output and normalization MDHistoWorkspaces from :ref:`MDNormSCD <algm-MDNormSCD>` and :ref:`MDNormDirectSC <algm-MDNormDirectSC>` have the 'displayNormalization' set to 'NoNormalization'. For older outputs, the `setDisplayNormalization` function is now exposed to python.

Python
------

- The function `IMDDimension.getName()` has been deprecated. Use the propery `IMDDimension.name` instead.
- The duplicate function `Workspace.getName()` has been deprecated. Use `Workspace.name()` instead.

Python Algorithms
#################

- :ref:`MatchPeaks <algm-MatchPeaks>` performs circular shift operation (numpy roll) along the x-axis to align the peaks in the spectra.
- :ref:`FindEPP <algm-FindEPP>` is improved to better determine the initial parameters and range for the fitting.
- :ref:`StartLiveData <algm-StartLiveData>` can now accept LiveListener properties as parameters, based on the value of the "Instrument" parameter.

Bug Fixes
---------

- Bin masking information was wrongly saved when saving workspaces into nexus files, which is now fixed.
- :ref:`LoadEventNexus <algm-LoadEventNexus>` should no longer leak memory when the execution is cancelled.
- :ref:`LoadNexusProcessed <algm-LoadNexusProcessed>` will now load the stored workspace names from a processed Nexus file in the case of multiperiod data.
- If a run is aborted and restarted, the ``running`` log in the workspace will correctly reflect this. (``running`` will be false at all times before the abort.)
- Fixed several issues with masked detectors and neighbour counts in the nearest-neighbour code used by a few algorithms.
- Issues with :ref:`CalculateFlatBackground <algm-CalculateFlatBackground>` sometimes returning bogus values when the **Return Background** option was used were fixed.

Full list of
`Framework <http://github.com/mantidproject/mantid/pulls?q=is%3Apr+milestone%3A%22Release+3.9%22+is%3Amerged+label%3A%22Component%3A+Framework%22>`__
and
`Python <http://github.com/mantidproject/mantid/pulls?q=is%3Apr+milestone%3A%22Release+3.9%22+is%3Amerged+label%3A%22Component%3A+Python%22>`__
changes on GitHub
