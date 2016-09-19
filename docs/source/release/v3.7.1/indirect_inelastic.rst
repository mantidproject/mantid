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
  MatrixWorkspace so that the maximum value for any spectra is 1.

- :ref:`IqtFitSequential <algm-IqtFitSequential>` algorithm has been added to sequential Iqt Fit data.
  This algorithm will be mainly used in the IqtFit interface.

Data Analysis
#############

Jump Fit
~~~~~~~~

- The interface now has the option to plot a guess of what the fit will look like before running the algorithm.
- The Plot button is no longer present in the interface as it is no longer used.


Diffraction
###########

- OSIRIS Diffraction DiffOnly interface and the :ref:`OSIRISDiffractionReduction <algm-OSIRISDiffractionReduction>` algorithm now support the use of multiple
  contianer runs. Additional validation also ensures you have the same number of sample/vanadium/container runs.


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

- Added :ref:`VesuvioThickness <algm-VesuvioThickness>` algorithm to calclulate the sample density


Improvements
------------

- :ref:`IqtFitMultiple <algm-IqtFitMultiple>` (previously the function ``FuryFitMult`` in IndirectDataAnalysis.py)
   has been adapted to be a Mantid algorithm. This has not effected the way this script is used in the Indirect Data Analysis:
   I(Q,t) Fit tab, but it does now have a dialogue box interface from the algorithm list.
   This also allows for better testing, progress tracking and documentation of the algorithm.

- :ref:`BASISReduction <algm-BASISReduction>` now accepts Vanadium runs for normalization, and one option to normalize by the maximum of the first spectrum.

- :ref:`QECoverage <QE Coverage>` planning tool has now been updated, There is now an Emin option included for direct tab,
    If Emin or Emax are left empty; appropriate values are set automatically, the negative values of Ei are treated as
    positive, appropriate informative pop-up messages displayed for invalid values and minor calculations updates.

- The file OSIRIS_GSS_Parameters.prm which describes OSIRIS in the .prm, format has been added to the instrument directory. This file can
    be used together with .gss files from OSIRIS diffraction.

- :ref:`OSIRISDiffractionReduction <algm-OSIRISDiffractionReduction>` has an additional DRange added for conversion from time regime.

- Updated the :ref:`SimulatedDensityOfStates <algm-SimulatedDensityOfStates>` workflow diagram to show an overview of the algorithm.

- the *Iqt* interface now validates that EMin is strictly less than EMax and that they are both not equal to 0


Bugfixes
--------

- *BayesQuasi* no longer crashes if the you supply data with trailing or leading zeros
- :ref:`ISISIndirectEnergyTransfer <algm-ISISIndirectEnergyTransfer>` only corrects for detailed balance when one is actually specified as input.
- :ref:`SimulatedDensityOfStates <algm-SimulatedDensityOfStates>` should no longer manipulate the actual data values and only rebins the data to the desired bin width.
- :ref:`VesuvioCorrections <algm-VesuvioCorrections>` no longer always fits using only the first spectrum in the input workspace.
- Fix bug with :ref: `BayesQuasi <algm-BayesQuasi>` docs not displaying online
- *BayesStretched* interface now gives the option of using the current working directory if no default save path is provided.
- The mini plot range bars in all interfaces now automatically update when a file is loaded.
- In the *I(Q, t) Fit* interface, checking the plot guess check box now correctly adds and removes the curve from the plot
- In the *BayesQuasi* interface ResNorm files are now automatically loaded from file locations when entered.
- :ref:`LoadVesuvio <algm-LoadVesuvio>` now correctly parses input in the form 10-20,30-40,50-60
- The *ApplyPaalmanPings* interface no longer crashes when attempting to preview different spectra when shift option is checked but the algorithm has not been run
- The *ContainerSubtraction* Interface should no longer crash when changing preview spectra in the miniplot
- Using the Spectra option in *S(Q,w)* interface now works correctly
- :ref:`IqtFitSequential <algm-IqtFitSequential>` and :ref:`IqtFitMultiple <algm-IqtFitMultiple>` now correctly add sample logs to their output workspaces
- It is now possible to properly update the Rebinning option in the *ISISCalibration* interface in *Indirect Data Reduction*
- The Plot Output options in the *I(Q, t) Fit* interface now update properly when switching between Fit Types
- The Data Reduction *ISIS Calibration* interface should now update the range bars correctly for OSIRIS



`Full list of changes on GitHub <http://github.com/mantidproject/mantid/pulls?q=is%3Apr+milestone%3A%22Release+3.7%22+is%3Amerged+label%3A%22Component%3A+Indirect+Inelastic%22>`_
