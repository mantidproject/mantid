=================
Framework Changes
=================

.. contents:: Table of Contents
   :local:

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
- All algorithms using `AsciiPointBase` now have a new property 'Separator' which allows the delimiter to be set to either comma, space or tab. This affects `SaveReflCustomAscii <algm-SaveReflCustomAscii>`, `SaveReflThreeColumnAscii <algm-SaveReflThreeColumnAscii>`, `SaveANSTOAscii <algm-SaveANSTOAscii>` and `SaveILLCosmosAscii <algm-SaveILLCosmosAscii>`.
- :ref:`ReplaceSpecialValues <algm-ReplaceSpecialValues>` now can replace 'small' values below a user specified threshold.
- :ref:`SaveMDWorkspaceToVTK <algm-SaveMDWorkspaceToVTK>` has a working progress bar.
- :ref:`SumSpectra <algm-SumSpectra>` has an option to ignore special floating point values called 'RemoveSpecialValues'. This is off by default but when enabled will ignore values such as NaN or Infinity during the summation of the spectra.  It was also updated to fix special values being used in some cases when the option was selected.
- :ref:`MonteCarloAbsorption <algm-MonteCarloAbsorption>`:
   * an `Interpolation` option has been added. Availabile options are: `Linear` & `CSpline`.
   * the method of selecting the scattering point has ben updated to give better agreement with numerical algorithms such as :ref:`CylinderAbsorption <algm-CylinderAbsorption>`.
- :ref:`SetSample <algm-SetSample>` now accepts an Angle argument for defining a rotated flat plate sample.

Renamed
#######

- CorrectFlightPaths has been renamed to :ref:`ConvertToConstantL2 <algm-ConvertToConstantL2>`.

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
- :ref:`SetupILLD33Reduction	 <algm-SetupILLD33Reduction>.
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
- Algorithm :ref:`CalculateCostFunction <algm-CalculateCostFunction>` calculates a value of any available cost function.
- Algorithm :ref:`EstimateFitParameters <algm-EstimateFitParameters>` estimates initial values of a fiting function in given intervals.
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
