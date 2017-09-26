==========================
Indirect Inelastic Changes
==========================

.. contents:: Table of Contents
   :local:

New features
------------

Algorithms
##########

- :ref:`BASISReduction <algm-BASISReduction>` can save to NXSPE format.

Vesuvio
#######
- Added flag for disabling multiple scattering corrections: flags['ms_flags']['ms_enabled']
- Added method for specifying a mass by chemical symbol e.g. H for hydrogen, O for oxygen
- Gamma Corrections are no longer done for back-scattering spectra
- Multiple scattering corrections for back-scattering spectra now approximate hydrogen peak, this peak can be constrained with masses specified by symbol

Bayes
#####
- Removed fit option from plot options drop-down menu.
- :ref:`SimpleShapeMonteCarloAbsorption <algm-SimpleShapeMonteCarloAbsorption>` has been added to simplify sample environment inputs for MonteCarloAbsorption

Data Analysis
#############
- Added 'ExtractMembers' property to ConvolutionFitSequential algorithm - this allows for extracting the members of the
  convolution fitting into their own workspaces.

Elwin
~~~~~

Bugfixes
--------
- Save Result now writes to file the temperature-dependent elastic intensity normalized to the lowest temperature.
- Added 'ExtractMembers' property to ConvolutionFitSequential algorithm - this allows for extracting the members of the
  convolution fitting into their own workspaces.

ConvFit
~~~~~~~

Bugfixes
--------
- Correct treatment of the resolution function: convolve sample and resolution spectra with same momentum transfer.
- Property to pass the workspace index added to :ref:`algm-ConvolutionFitSequential`.


MSDFit
~~~~~~

Improvements
------------
- Added model selection to MSDFit, with three current models: MsdPeters, MsdYi and MsdPeters. New models now
  work with workspaces in Q not Q^2 (e.g. _eq workspaces 'Elastic Q')


Jump Fit
~~~~~~~~

General
~~~~~~~

Improvements
------------
- Added a flag 'ms_enabled' (default: True) for enabling/disabling multiple scattering in vesuvio user scripts.
- The *S(Q, W)* interface now automatically replaces NaN values with 0.
- EISF is now generated when performing a Single Fit, with a delta function, in the ConvFit interface.
- :ref:`FlatPlatePaalmanPingsCorrection <algm-FlatPlatePaalmanPingsCorrection>` now supports `Direct` and `Indirect` modes.

Dropped
-------
- `LoadILLIndirect-v1 <http://docs.mantidproject.org/v3.10.1/algorithms/LoadILLIndirect-v1.html>`_, `IndirectILLReduction <http://docs.mantidproject.org/v3.10.1/algorithms/IndirectILLReduction-v1.html>`_, `ILLIN16BCalibration <http://docs.mantidproject.org/v3.10.1/algorithms/ILLIN16BCalibration-v1.html>`_ algorithms deprecated since v3.9, are now removed.


Bugfixes
--------
- ElasticWindowMultiple now correctly normalizes by the lowest temperature - rather than the first one.
- A number of Python indirect algorithms that use :py:obj:`mantid.kernel.MaterialBuilder` allowed setting the mass density for a material. The density was set incorrectly where the chemical formula had more than one atom, this is now fixed.
- An issue has been fixed in :ref:`algm-IndirectILLEnergyTransfer` when handling the data with mirror sense, that have shifted 0 monitor counts in the left and right wings. This was causing the left and right workspaces to have different x-axis binning and to fail to sum during the unmirroring step. 
- An issue has been fixed in :ref:`algm-IndirectILLReductionFWS` when the scaling of the data after vanadium calibration was not applied.
- :ref:`algm-CalculateSampleTransmission` now divides by the tabulated wavelength when calculating the absorption cross section.

`Full list of changes on GitHub <http://github.com/mantidproject/mantid/pulls?q=is%3Apr+milestone%3A%22Release+3.11%22+is%3Amerged+label%3A%22Component%3A+Indirect+Inelastic%22>`_
