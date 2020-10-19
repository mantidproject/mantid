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

- Enable workbench to plot 1D spectrum (plot, overplot, plot_with_error, overplot_with_error) from an IMDHistoWorkspace whose non-integrated dimension is equal to 1 but not to launch slice viewer.
- Enabled the plotting of individual functions in the Workbench fit browser. This replicates a feature that was in MantidPlot.
  Plotting of each function can be performed by right clicking on the function within the browser and selecting plot.
- Add the possibility to copy and paste shapes in the instrument viewer using Ctrl + C and Ctrl + V.

Bugfixes
########

- Only display slice viewer widget for MDEventWorkspaces with 2 or more dimensions.

:ref:`Release 6.0.0 <v6.0.0>`
