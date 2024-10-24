==========================
Indirect Inelastic Changes
==========================

.. contents:: Table of Contents
   :local:

New features
------------

Algorithms
##########

* Remove CylinderPaalmanPingsCorrection v1. This algorithm has been replaced by :ref:`CylinderPaalmanPingsCorrection <algm-CylinderPaalmanPingsCorrection>`


Bayes
#####

Stretch
~~~~~~~

- Previously the Quest script was used to drive the Bayes stretch interface. This functionality has been ported to the algorithm :ref:`BayesStretch <algm-BayesStretch>`.

Corrections
###########

Absorption
~~~~~~~~~~

- Mantid plotting is now handled in the interface rather than the respective algorithm


Data Reduction
##############

ISIS Calibration
~~~~~~~~~~~~~~~~

- Add load log option to ISIS calibration interface

Data Analysis
#############

Elwin
~~~~~

- Additional option to ungroup Elwin output
- When using multiple input files, the naming convention for the outputworkspace contains the `first-final` run number.
  An example of this would be `osi92764-92767_graphite002_red_elwin_elf` for OSIRIS run between 92764-92767


Simulations
###########

Density Of States
~~~~~~~~~~~~~~~~~

- :ref:`SimulatedDensityOfStates <algm-SimulatedDensityOfStates>` now allows for the parsing of isotopes from the \*.phonon or \*.castep file in the form 'element:isotope'

- Allow for the loading of separate indexes for each element via the Output Format combo box in the interface.

Load nMoldyn
~~~~~~~~~~~~

- New algorithm :ref:`LoadNMoldyn4Ascii1D <algm-LoadNMoldyn4Ascii1D>` has been added to allow 1D nmoldyn data to be loaded in Mantid

Correlations
~~~~~~~~~~~~

- New algorithms :ref:`VelocityCrossCorrelations <algm-VelocityCrossCorrelations>` and :ref:`VelocityAutoCorrelations <algm-VelocityAutoCorrelations>`
- New algorithms :ref:`AngularAutoCorrelationsSingleAxis <algm-AngularAutoCorrelationsSingleAxis>` and :ref:`AngularAutoCorrelationsTwoAxes <algm-AngularAutoCorrelationsTwoAxes>`

VESUVIO
#######

- Add the functionality for ties between internal parameters within each mass profile. This allows for the creation of a BivariateGaussian profile from the MultivariateGaussian profile.
  Ties can be added within the definition of the mass profile with the following:

  *flags['masses'] = [{'value':1.0079, 'function': 'MultivariateGaussian', 'SigmaX': 5, 'SigmaY': 5, 'SigmaZ': 5, 'ties': 'SigmaX=SigmaY'}]*

  The above will tie SigmaX to SigmaY for this MultivariateGaussian in the driver script

Improvements
------------

- :ref:`LoadVesuvio <algm-LoadVesuvio>` now uses the whole TOF range for loaded monitor data (0-20000)
- Physical positions were included to the 311 reflection of BASIS instrument for improved instrument view.
- Algorithm BASISReduction311 has been included in algorithm :ref:`BASISReduction <algm-BASISReduction>`.
- Range bars colours in the *ISIS Calibration* interface have been updated to match the convention in the fit wizard.
- Vesuvio sigma_theta value updated for single and double differencing in both forward and back scattering. The new value is 0.016 for all.
- The Elwin interface now uses the resolution of the instrument to create the range bars when possible
- Shift of container data and conversion of units to wavelength have been removed from ApplyPaalmanPings interface and added to ApplyPaalmanPingsCorrection algorithm.
- The plotting and saving of the results of all Inelastic Interfaces, apart from EnergyTransfer has been re-factored to be only accessible via the interface and once the algorithm is completed.
- Improvements to FABADA minimizer have been added (ergodicity, ties and false convergences).

Bugfixes
--------


* :ref:`IqtFitMultiple <algm-IqtFitMultiple>` no longer creates an unwanted temporary workspace when executed
* The documentation for :ref:`TransformToIqt <algm-TransformToIqt>` now correctly states that the ParameterWorkspace is a TableWorkspace
* Fix memory leak in :ref:`LoadSassena <algm-LoadSassena>`
* The *ResNorm* interface should no longer crash when using workspaces (rather than files) as input.
* Fix bug showing incorrect doublet peaks in :ref:`ISISIndirectDiffractionReduction <algm-ISISIndirectDiffractionReduction>`
* Fix end of line issue when loading ascii files in *LoadILL* interface
* *BayesQuasi* now displays correct spectrum number in progress bar

`Full list of changes on GitHub <http://github.com/mantidproject/mantid/pulls?q=is%3Apr+milestone%3A%22Release+3.8%22+is%3Amerged+label%3A%22Component%3A+Indirect+Inelastic%22>`_
