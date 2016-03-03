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

Improvements
------------

- :ref:`IqtFitMultiple <algm-IqtFitMultiple>` (previously the function ``FuryFitMult`` in IndirectDataAnalysis.py)
   has been adapted to be a Mantid algorithm. This has not effected the way this script is used in the Indirect Data Analysis:
   I(Q,t) Fit tab, but it does now have a dialogue box interface from the algorithm list.
   This also allows for better testing, progress tracking and documentation of the algorithm.


Bugfixes
--------

- *BayesQuasi* no longer crashes if the you supply data with trailing or leading zeros
- :ref:`ISISIndirectEnergyTransfer <algm-ISISIndirectEnergyTransfer>` only corrects for detailed balance when one is actually specified as input.

`Full list of changes on GitHub <http://github.com/mantidproject/mantid/pulls?q=is%3Apr+milestone%3A%22Release+3.7%22+is%3Amerged+label%3A%22Component%3A+Indirect+Inelastic%22>`_
