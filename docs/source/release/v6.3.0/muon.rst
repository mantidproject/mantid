============
Muon Changes
============

.. contents:: Table of Contents
   :local:

.. warning:: **Developers:** Sort changes under appropriate heading
    putting new features at the top of the section, followed by
    improvements, followed by bug fixes.


Frequency Domain Analysis
-------------------------

New Features
############

Improvements
############

Bugfixes
########

- Fixed a bug that prevented the frequency spectra from being plotted when data was binned.

Muon Analysis
-------------

New Features
############


Muon Analysis and Frequency Domain Analysis
-------------------------------------------

New Features
############

Improvements
############

- The `alpha` values on grouping tab are now to six decimal places.
- The numerical values in the `run info` box on the home tab are now rounded to either 4 significant figures or a whole number, whichever is more precise.
- Changes have been made to improve the speed of Muon Analysis and Frequency Domain Analysis

Bugfixes
########

- On the model analysis tab, the fit range will now update when the x axis is changed.
- Fixed a bug that prevented the model analysis plot showing when data was binned.
- When a new results table is created the Model Analysis tab selects the default parameters to plot based on log values or parameters in the results table.

ALC
---

New Features
############

- Can now read `nxs_v2` files.

Improvements
############

Algorithms
----------

Improvements
############

- :ref:`LoadPSIMuonBin <algm-LoadPSIMuonBin>` can now load a subset of the spectra.

Bugfixes
########


Fitting Functions
-----------------
New Features
############
- Added a :ref:`Muonium-style Decoupling Curve <func-MuoniumDecouplingCurve>` function to MuonModelling Fit Functions.
- Added a :ref:`Power Law <func-PowerLaw>` function to MuonModelling Fit Functions.

Improvements
############
- :ref:`Gaussian <func-Gaussian>`, :ref:`Lorentzian <func-Lorentzian>` and :ref:`Polynomial fitting <func-Polynomial>` functions can now also be found under MuonModelling in the Fitting Functions Tree.

:ref:`Release 6.3.0 <v6.3.0>`

..
  Model Fitting
  -------------

  BugFixes
  ########
  - A bug has been fixed that caused Model fitting to not update it's results table list.
  - Plotting in Model fitting now features a greater number of units for parameters and sample logs.


