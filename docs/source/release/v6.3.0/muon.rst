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

- Instead of plotting the confidence interval of a fit as an error bar, it is now represented by a shaded region.

Improvements
############

- The `alpha` values on grouping tab are now to six decimal places.
- The numerical values in the `run info` box on the home tab are now rounded to either 4 significant figures or a whole number, whichever is more precise.
- The results table now produces errors for log values (when they are available).
- Changes have been made to improve the speed of Muon Analysis and Frequency Domain Analysis
- The results tab will now dispay a warning (red text and a tooltip) if the results table already exists.
- The plots no longer use scientific notation for the axis values.
- On resizing priority is given to plotting.
- The Sequentially Fit all button is now visible for 4K displays
- the plot guess option in Fitting can now have its range interpolated or extrapolated.

Bugfixes
########

- Fixed a bug that prevented the model analysis plot showing when data was binned.
- When a new results table is created the Model Analysis tab selects the default parameters to plot based on log values or parameters in the results table.
- Fixed a bug that prevented the GUI working with workspace history and project recovery.
- Detaching tabs, then closing Mantid no longer causes a crash.
- When a new fit is performed in Muon Analysis it no longer reselects all parameter workspaces in the results tab.

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
- Added an :ref:`Activation <func-Activation>` fitting function to MuonModelling Fit Functions.
- Added a :ref:`Magentic Order Parameter<func-MagneticOrderParameter>` function to MuonModelling\Magentism Fit Functions.
- Added a :ref:`Muonium-style Decoupling Curve <func-MuoniumDecouplingCurve>` function to MuonModelling Fit Functions.
- Added a :ref:`Power Law <func-PowerLaw>` fitting function to MuonModelling Fit Functions.
- Added a ref:`Smooth Transition <func-SmoothTransition>` function to MuonModelling Fit Functions.


Improvements
############
- created a new category, 'Magnetism', in the MuonModelling Fit Functions list.
- :ref:`Gaussian <func-Gaussian>`, :ref:`Lorentzian <func-Lorentzian>` and :ref:`Polynomial fitting <func-Polynomial>` functions can now also be found under MuonModelling in the Fitting Functions Tree.

:ref:`Release 6.3.0 <v6.3.0>`

..
  Model Fitting
  -------------

  BugFixes
  ########
  - A bug has been fixed that caused Model fitting to not update it's results table list.
  - Plotting in Model fitting now features a greater number of units for parameters and sample logs.
  - The dates and times for relevant parameters in model fitting have been formatted so that they can be plotted with relative spacing.
  - On the model analysis tab, the fit range will now update when the x axis is changed.
  - The model analysis tab no longer resets when the instrument is changed.


