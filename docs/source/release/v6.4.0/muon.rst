============
MuSR Changes
============

.. contents:: Table of Contents
   :local:

.. warning:: **Developers:** Sort changes under appropriate heading
    putting new features at the top of the section, followed by
    improvements, followed by bug fixes.

:ref:`Release 6.4.0 <v6.4.0>`


..
  Model Fitting
  -------------

  BugFixes
  ########
  - A bug has been fixed that caused Model fitting to not update it's results table list.
  - Plotting in Model Fitting now features a greater number of units for parameters and sample logs.
  - The dates and times for relevant parameters in Model Fitting have been formatted so that they can be plotted with relative spacing.
  - On the Model Fitting Tab, the fit range will now update when the x axis is changed.
  - The Model Fitting tab no longer resets when the instrument is changed.
  - When a new results table is created the Model Fitting tab selects the default parameters to plot based on log values or parameters in the results table.
  - Fixed a bug that prevented the Model Fitting plot showing when data was binned.
