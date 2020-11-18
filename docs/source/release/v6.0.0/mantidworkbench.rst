========================
Mantid Workbench Changes
========================

.. contents:: Table of Contents
   :local:

Improvements
############

- Expose :ref:`Instrument View <instrumentviewer>` control to Python.
- Enable workbench to plot 1D spectrum (plot, overplot, plot_with_error, overplot_with_error) from an IMDHistoWorkspace whose non-integrated dimension is equal to 1 but not to launch slice viewer.

Bugfixes
########

- Only display slice viewer widget for MDEventWorkspaces with 2 or more dimensions.
- Fix Workbench crashes upon deleting rows or columns in a TableWorkspace.

:ref:`Release 6.0.0 <v6.0.0>`
