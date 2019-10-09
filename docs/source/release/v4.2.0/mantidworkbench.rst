=======================
MantidWorkbench Changes
=======================

.. contents:: Table of Contents
   :local:


User Interface
##############

- The zoom icon in the SliceViewer and plot toolbars have been replaced with clearer icons.
- Plots now allow the insertion of draggable horizontal and vertical markers.
- Marker label, color and line style can be edited on a per-marker basis.
- The button to remove a curve in the figure options is now the same size as the drop-down list of curves.
- Uses of "MantidPlot" in window titles have been removed.
- Figure options now has a Legend tab so that a plot's legend can be customised.

New
###
- Added a 'Generate Recovery Script' button to Workbench under the File menu bar, it generates a script that is essentially what project recovery uses. However it is only the script and does not include the workspace clean up and other features Project Recovery offers.

Improvements
############
- The keyboard shortcut Ctrl+N now opens a new tab in the script editor.
- Project Save and Load will no longer freeze when saving and loading large amounts of workspaces and/or interfaces.
- Attempting to save files that are larger than (by default) 10GB now results in a dialog box to inform the user that it may take a long time and gives them the opportunity to cancel.
- Added basic tiled plots to workbench interface.
- Changing the axis' scale, by right-clicking on a figure with multiple plots, changes only the plot that was clicked on.
- If a spectrum has a label, this will now be used instead of the spectrum number in the legend when the spectrum is plotted.
- The dialog for selecting spectra to plot now has the spectrum number input field selected by default.
- There are now icons alongside the colormap names in the plot options dialog.
- The help button in the Manage User Directories widget has been restored.
- Hex codes can now be inputted directly into the color selectors in figure options.
- There is now a button on the plot window's toolbar to generate a script that will re-create the current figure.
- There is now a "Filter by" menu in the message display's context menu, allowing you to filter output by script.
- It is now possible to fit table workspaces in the fit browser and in a script.
- The sub-tabs in the Curves tab in plot options now contain an "Apply to All" button which copies the properties of the current curve to all other curves in the plot.

Bugfixes
########
- Pressing the tab key while in the axis quick editor now selects each input field in the correct order.
- Clicking Cancel after attempting to save a project upon closing now keeps Workbench open instead of closing without saving.
- Dialog windows no longer contain a useless ? button in their title bar.
- Instrument view now keeps the saved rendering option when loading projects.
- Fixes an issue where choosing to not overwrite an existing project when attempting to save upon closing would cause Workbench to close without saving.
- Fit results on normalised plots are now also normalised to match the plot.
- A crash in the Fit Browser when the default peak was not a registered peak type has been fixed.
- Fixed an issue where you could not edit table workspaces to enter negative numbers.
- The data display will now update automatically when deleting a column in a table workspace.
- The colorbar in the colorfill plot window now correctly resizes when the scale is changed by double-clicking on the colorbar axis.
- Fixes an issue in the Slice Viewer where changing the colormap, min value, or max value via the figure options would not update the scale.
- Fixes an issue where changing the curve properties in the figure options menu would reset the plot's axes scales.
- Fixed an issue with fitting where the difference would be plotted even if the Plot Difference option in the fit property browser was not enabled.
- Fixed an issue where the plot legend would no longer be movable after removing a plot guess.
- An error is no longer raised when attempting to open plot options, or the fitting tab, on a figure containing a line plotted using a script without a spectrum number being specified.

:ref:`Release 4.2.0 <v4.2.0>`
