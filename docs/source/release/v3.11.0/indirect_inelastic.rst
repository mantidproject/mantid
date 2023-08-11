==========================
Indirect Inelastic Changes
==========================

.. contents:: Table of Contents
   :local:

Algorithms
----------

New
###
- :ref:`SimpleShapeMonteCarloAbsorption <algm-SimpleShapeMonteCarloAbsorption>` has been added to simplify sample environment inputs for MonteCarloAbsorption.
- :ref:`algm-CalculateMonteCarloAbsorption` calculates Paalman Pings absorption factors from a sample and container workspace, given shape specific input.

Improved
########
- The following changes were made to the :ref:`algm-MSDFit` algorithm:
   - Added model selection to the :ref:`algm-MSDFit` algorithm, with three current models: :ref:`MsdGauss <func-MsdGauss>`, :ref:`MsdYi <func-MsdYi>`, and :ref:`MsdYi <func-MsdYi>`. New models now work with workspaces in :math:`Q` not :math:`Q^2` (e.g. _eq workspaces 'Elastic Q').
- The following changes were made to the :ref:`algm-ConvolutionFitSequential` algorithm:
   - Added 'ExtractMembers' property to :ref:`algm-ConvolutionFitSequential` - this allows for extracting the members of the convolution fitting into their own workspaces.
   - Property to pass the workspace index added to :ref:`algm-ConvolutionFitSequential`.
   - The :ref:`algm-ConvolutionFitSequential` now performs correct treatment of the resolution function: convolve sample and resolution spectra with same momentum transfer.
- The following changes were made to the :ref:`algm-ISISIndirectDiffractionReduction` algorithm:
   - Manual D-Ranges can now be supplied as a list/range, to the :ref:`algm-ISISIndirectDiffractionReduction` algorithm, each corresponding to their respective runs, in the supplied order.
   - The Sum Files option in the :ref:`algm-ISISIndirectDiffractionReduction` algorithm now allows for correctly corresponding each sum of
     sample runs defined with a range (e.g. A-B, where A and B are run numbers) to the corresponding vanadium run, dependent on D-Range.
   - The 'Sample Runs' field in the :ref:`algm-ISISIndirectDiffractionReduction` algorithm now recognizes 3 operators: '-', '+', ':'. The '-' operator is used to supply a given range of runs and sum them when SumFiles is checked. The '+' operator is used to supply a given list of runs and sum when SumFiles is checked. The ':' operator is used to supply a range of runs, which will never be summed.
   - The Grouping Policy in :ref:`algm-ISISIndirectDiffractionReduction`, now allows for grouping with a Workspace.
- :ref:`FlatPlatePaalmanPingsCorrection <algm-FlatPlatePaalmanPingsCorrection>` now supports `Direct` and `Indirect` modes.
- :ref:`BASISReduction <algm-BASISReduction>` can save to NXSPE format.

Bugfixes
########
- :ref:`algm-ElasticWindowMultiple` now correctly normalizes by the lowest temperature - rather than the first one.
- An issue has been fixed in :ref:`algm-IndirectILLEnergyTransfer` when handling the data with mirror sense, that have shifted 0 monitor counts in the left and right wings. This was causing the left and right workspaces to have different x-axis binning and to fail to sum during the unmirroring step.
- An issue has been fixed in :ref:`algm-IndirectILLReductionFWS` when the scaling of the data after vanadium calibration was not applied.
- :ref:`algm-CalculateSampleTransmission` now divides by the tabulated wavelength when calculating the absorption cross section.

Indirect Interfaces
-------------------
- The Indirect Absorption Corrections interface has been replaced with Calculate Monte Carlo Absorption Corrections; using the new :ref:`algm-CalculateMonteCarloAbsorption` algorithm.
- In the Indirect ConvFit interface, EISF is now extracted as a parameter when performing a single fit using 'Fit Single Spectrum'.
- The Indirect *S(Q, W)* interface now automatically replaces NaN values with 0.
- The Save Result option in the Indirect Elwin interface now writes to file the temperature-dependent elastic intensity normalized to the lowest temperature.
- Model selection is available in the Indirect MSDFit interface, providing the option to choose one of the three models available in the :ref:`algm-MSDFit` algorithm.
- Removed fit option from plot options drop-down menu, in the Indirect Bayes interface.
- Use Manual Grouping in the Indirect Diffraction interface now functions in the same way as the equivalent option in the Indirect ISISEnergyTransfer interface; providing and option to choose the number of groups and subsequently grouping by detector.
- Plot Current Preview is now an available option across all Indirect interfaces, where a mini-plot is shown within the interface.

Vesuvio
-------
- Added flag for disabling multiple scattering corrections: flags['ms_flags']['ms_enabled'].
- Added method for specifying a mass by chemical symbol e.g. H for hydrogen, O for oxygen.
- Multiple scattering corrections for back-scattering spectra now approximate the hydrogen peak, this is done in the :ref:`algm-VesuvioCorrections` algorithm. This feature is incomplete for 3.11.
- :ref:`algm-VesuvioCorrections` has the additional property: 'MassIndexToSymbolMap'. MassIndexToSymbolMap is used to map from an index of mass in the 'Masses' property to a chemical symbol.
- :ref:`algm-VesuvioCorrections` takes the additional property: 'HydrogenConstraints'. HydrogenConstraints are used to constrain the hydrogen peak for multiple scattering corrections in back-scattering spectra.
- Gamma Corrections are no longer done for back-scattering spectra in the :ref:`algm-VesuvioCorrections` algorithm.

General
-------

Dropped
#######
- `LoadILLIndirect-v1 <http://docs.mantidproject.org/v3.10.1/algorithms/LoadILLIndirect-v1.html>`_, `IndirectILLReduction <http://docs.mantidproject.org/v3.10.1/algorithms/IndirectILLReduction-v1.html>`_, `ILLIN16BCalibration <http://docs.mantidproject.org/v3.10.1/algorithms/ILLIN16BCalibration-v1.html>`_ algorithms deprecated since v3.9, are now removed.

Bugfixes
########
- A number of Python indirect algorithms that use :py:obj:`mantid.kernel.MaterialBuilder` allowed setting the mass density for a material. The density was set incorrectly where the chemical formula had more than one atom, this is now fixed.

`Full list of changes on GitHub <http://github.com/mantidproject/mantid/pulls?q=is%3Apr+milestone%3A%22Release+3.11%22+is%3Amerged+label%3A%22Component%3A+Indirect+Inelastic%22>`_
