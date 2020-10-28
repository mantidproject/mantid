========================
Mantid Workbench Changes
========================

.. contents:: Table of Contents
   :local:

New Features
############

- Added a colorbar scale option to the workbench plot settings. This allows the user to choose between a linear (default) or logarithmic scale.
- Added a show legend checkbox to the workbench plot settings. This allows users to choose whether to display the legend on graphs by default.

Improvements
############

- Enable workbench to plot 1D spectrum from an IMDHistoWorkspace whose non-integrated dimension is equal to 1 but not to launch slice viewer.
- Removed dialogs for the Load and Fit algorithms as the dialogs were previously deprecated
- Enable workbench to plot 1D spectrum (plot, overplot, plot_with_error, overplot_with_error) from an IMDHistoWorkspace whose non-integrated dimension is equal to 1 but not to launch slice viewer.
- Enabled the plotting of individual functions in the Workbench fit browser. This replicates a feature that was in MantidPlot.
  Plotting of each function can be performed by right clicking on the function within the browser and selecting plot.
- Add the possibility to copy and paste shapes in the instrument viewer using Ctrl + C and Ctrl + V.
- Improved spectra selection within the sliceviewer, which should ensure that high counting spectra are shown immediately when the sliceviewer is opened.
- Powder diffraction support (instruments D2B and D20) has been added to DrILL interface. See


Bugfixes
########

- Only display slice viewer widget for MDEventWorkspaces with 2 or more dimensions.
- Fix Workbench crashes upon deleting rows or columns in a TableWorkspace.

:ref:`Release 6.0.0 <v6.0.0>`
