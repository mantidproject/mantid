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
- :ref: All algorithms using `AsciiPointBase` now have a new property 'Separator' which allows the delimiter to be set to either comma, space or tab. This affects `SaveReflCustomAscii <algm-SaveReflCustomAscii>`, `SaveReflThreeColumnAscii <algm-SaveReflThreeColumnAscii>`, `SaveANSTOAscii <algm-SaveANSTOAscii>` and `SaveILLCosmosAscii <algm-SaveILLCosmosAscii>`.
- :ref:`ReplaceSpecialValues <algm-ReplaceSpecialValues>` now can replace 'small' values below a user specified threshold.
- :ref:`MonteCarloAbsorption <algm-MonteCarloAbsorption>` gained a new option: `Interpolation`.
  This controls the method used for interpolation. Availabile options are: `Linear` & `CSpline`.

Deprecated
##########

- :ref:`CorrectFlightPaths <algm-CorrectFlightPaths>` has been renamed to ConvertToConstantL2.

MD Algorithms (VATES CLI)
#########################

Performance
-----------

CurveFitting
------------

New
###

- Algorithm :ref:`CalculateCostFunction <algm-CalculateCostFunction>` calculates a value of any available cost function.
- Algorithm :ref:`EstimateFitParameters <algm-EstimateFitParameters>` estimates initial values of a fiting function in given intervals.

Improved
########

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
