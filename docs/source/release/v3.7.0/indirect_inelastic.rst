==========================
Indirect Inelastic Changes
==========================

.. contents:: Table of Contents
   :local:

New features
------------

Algorithms
##########

- *IndirectNormSpectra* algorithm is designed to normalise all spectra in a
  MatrixWorkspace so that the maximum value for any spectra is 1

Data Analysis
#############

Jump Fit
~~~~~~~~

- The interface now has the option to plot a guess of what the fit will look like before running the algorithm.
- The Plot button is no longer present in the interface as it is no longer used.

Vesuvio
#######

- The following Mantid algorithms used for Vesuvio have been added:
    - :ref:`VesuvioPreFit <algm-VesuvioPreFit>`
    - :ref:`VesuvioTOFFit <algm-VesuvioTOFFit>`
    - :ref:`VesuvioCorrections <algm-VesuvioCorrections>`

- The script used to process data for Vesuvio has also been added. This used to be called ``VesuvioWorkflow.py``, but is now named ``VesuvioCommands.py``.

- The following Vesuvio specific algorithms have been updated to have their name prefixed by Vesuvio:
    - :ref:`VesuvioCalculateGammaBackground <algm-VesuvioCalculateGammaBackground>` previously ``CalculateGammaBackground``
    - :ref:`VesuvioCalculateMS <algm-VesuvioCalculateMS>` previously ``CalculateMSVesuvio``
    - :ref:`VesuvioDiffractionReduction <algm-VesuvioDiffractionReduction>` previously ``EVSDiffractionReduction``


- :ref:`LoadVesuvio <algm-LoadVesuvio>` now has the option to load the monitor data in addition to its normal operation. This is  loaded as an additional separate workspace.

- Added a fit function to fit a multivariate Gaussian profile (:ref:`MultivariateGaussianComptonProfile <func-MultivariateGaussianComptonProfile>`)


Improvements
------------

- :ref:`IqtFitMultiple <algm-IqtFitMultiple>` (previously the function ``FuryFitMult`` in IndirectDataAnalysis.py)
   has been adapted to be a Mantid algorithm. This has not effected the way this script is used in the Indirect Data Analysis:
   I(Q,t) Fit tab, but it does now have a dialogue box interface from the algorithm list.
   This also allows for better testing, progress tracking and documentation of the algorithm.

- :ref:`BASISReduction <algm-BASISReduction>` now accepts Vanadium runs for normalization.

- :ref:`QECoverage <Interfaces>` planning tool has now been updated, There is now an Emin option included for direct tab,
    If Emin or Emax are left empty; appropriate values are set automatically, the negative values of Ei are treated as
    positive, appropriate informative pop-up messages displayed for invalid values and minor calculations updates.

- :ref:`OSIRISDiffractionReduction <algm-OSIRISDiffractionReduction>` has an additional DRange added for conversion from time regime.


Bugfixes
--------

- *BayesQuasi* no longer crashes if the you supply data with trailing or leading zeros
- :ref:`ISISIndirectEnergyTransfer <algm-ISISIndirectEnergyTransfer>` only corrects for detailed balance when one is actually specified as input.
- :ref:`SimulatedDensityOfStates <algm-SimulatedDensityOfStates>` should no longer manipulate the actual data values and only rebins the data to the desired bin width.
- :ref:`VesuvioCorrections <algm-VesuvioCorrections>` no longer always fits using only the first spectrum in the input workspace.

`Full list of changes on GitHub <http://github.com/mantidproject/mantid/pulls?q=is%3Apr+milestone%3A%22Release+3.7%22+is%3Amerged+label%3A%22Component%3A+Indirect+Inelastic%22>`_
