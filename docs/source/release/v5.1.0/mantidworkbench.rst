=================
Workbench Changes
=================

.. contents:: Table of Contents
   :local:

New
###

- The Advanced Plotting menu is now in Workbench. This enables creating surface and contour plots of three or more workspaces, and choosing which log value to plot against.
- There is now a dialog for project saving that allows you to choose between saving all workspaces or only saving workspaces which have been altered.

Improvements
############

.. figure:: ../../images/Plot1DSelectionDialog5-1.png
   :align: right

- The plot selection dialog now correctly shows the full range of valid spectra to plot, not just the min to max range.
- We have added a Copy to Clipboard button to the plot window.
- Tile plots are now reloaded correctly by project recovery.
- When you stop a script running in workbench it will now automatically attempt to cancel the algorithm the script is running, rather than wait for the current algorthm to end.
  This is similar to what Mantidplot does, and should result in the script stopping much sooner.
- The ``Python extensions directory`` setting from MantidPlot is now available in Workbench within the Manage User Directories window.

.. figure:: ../../images/wb_invalid_log_shading.png
   :align: right

- Logs that contain invalid data indicated by NeXus log alarm states are now shaded in red, and a tool tip will tell you how many values are invalid.
- Fixed an issue where some scripts were running slower if a  plot was open at the same time.
- The Help Menu now has an About screen that will pop up automatically on startup to provide links to the release notes and various other resources, and allow you to set some important setting such as Facility, instrument and accept usage tracing.
  You can choose to hide it until the next release.
- There is now an option to create a 3D plot (surface, wireframe, contour) when you right-click a workspace.
- The Sample Logs Dialog now lets you view the complete log data as well as filtered data which only includes values for the current period, the running status, and with invalid values removed.  Just click the "Filtered Data" checkbox to swap between them.
- The axes tab in the figure options can now be used to set the limits, label, and scale of the z-axis on 3D plots.
- The "Show sample logs" dialog will now hide the plot and statistics display if there are no suitable logs in the workspace that need it.  This is particularly applicable for some of the reactor based instruments.
- The plot toolbar now shows the correct buttons for 3D plots.
- Plots now have a Help button on the toolbar, which will direct the user to a relevant documentation page informing them about that type of plot.
- On 3D plots you can now double-click on the z-axis to change its limits or label.
- Plots extracted from "Show Sample Logs" by double clicking the plot can now be converted to a python script, just like other workbench plots.
- The workspace sample logs interface now responds to keyboard input from the cursor keys to move between logs.
- We have neatened up the the slice viewer user interface, to reduce the amount of wasted space and devote more to the data view itself.
- We have implemented the interactive  plot details functionality from the SpectrumViewer in Mantidplot into the slice viewer there is now a table of details that update as you move your cursor over the data this includes the diension value and signal for multidimensional workspaces, and for matrix workspaces with a spectrum axis:
   - Signal
   - Spectra Number
   - Detector ID
   - Two Theta
   - Azimuthal Angle
   - Time of Flight
   - Wavelength
   - Energy
   - d-Spacing
   - Mod Q
- Surface plots no longer spill over the axes when their limits are reduced.
- The instrument view now ignores non-finite (infinity and NaN) values and should now display workspaces containing those values.
  If there are no valid values for that detector the value will appear as invalid (grayed out).
  It can also now display negative values in workspaces correctly.
- The gray and plasma colormaps have been added to the instrument view.
- Slow running algorithms will now display an estimated time to completion in their progress bars.
- The x-axis tick labels on colorfill plots are now horizontal.
- Improved the usability of the fit function and peak selection pop-up menus by allowing the user to immediately search for the desired function and activate autocompletion by pressing "enter" if there is just a single possible function.
- The figure options menu now has a help button which opens the documentation for the menu.
- Variables assigned in python scripts are now cleared when a script is run in its entirety.
- Monitors are no longer shown on bin and colorfill plots.
- The colorbar on colorfill plots is now labelled.
- User data directories are no longer checked at startup, reducing launch times with slow network drives.
- Added an option to set the default ```drawstyle``` within the workbench settings window. Additionally, the ```linestyle``` can now be set to 'None'.
- Improved the handling of ``WorkspaceSingleValue`` workspaces in workbench. This fixes a crash which occurred when interacting with workspaces of this type.

Bugfixes
########

- Fixed new tab names not incrementing correctly on KDE display environments (i.e. KUbuntu).
- Fixed a bug where setting columns to Y error in table workspaces wasn't working. The links between the Y error and Y columns weren't being set up properly.
- Fixed a crash when you selected a spectra to plot that was not present in a workspace.
- Fixed a crash when opening the plot options for a sample logs plot.
- Fixed a crash when you defined a new Fit Function after deleting a plot.
- Fixed a crash when plotting the logs from a multi-dimensional workspace, that combines several different original workspaces.
- The scale of the color bars on colorfill plots of ragged workspaces now uses the maximum and minimum values of the data.
- Fixed a bug where setting columns to Y error in table workspaces wasn't working. The links between the Y error and Y columns weren't being set up properly
- Opening figure options on a plot with an empty legend no longer causes an unhandled exception.
- Fixed being able to zoom in and out of colorbars on colorfill plots.
- Deleting a workspace now correctly deletes colorfill plots and waterfall plots that have been filled in.
- Fixed the default axis scale settings applying to the wrong axis.
- Performing an overplot by dragging workspaces onto colorfill plots now correctly replaces the workspace.
- Removed gridlines from the colorbar on colorfill plots.
- The Instrument View now passes through useful error messages to the workbench if it fails to start.
- The correct interpolation now appears in the plot figure options for colorfill plots.
- Changing the axis scale on a colourfill plot now has the same result if it is done from either the context menu or figure options.
- The plot guess of the Bk2BkExpConvPV is now correct.
- A sign error has been fixed in the Bk2Bk2ExpConvPV function.
- `plt.show()` now shows the most recently created figure.
- Removed error when changing the normalisation of a ragged workspace with a log scaled colorbar.
- The SavePlot1D algorithm can now be run in Workbench.
- Changing the settings on tiled colorbars now applys to all the plots if there is only one colorbar.
- Colorfill plots now correctly use the workspace name as the plot title.
- Overplotting no longer resets the axes scales.
- Fixed a bug with the peak cursor immediately resetting to the default cursor when trying to add a peak.

:ref:`Release 5.1.0 <v5.1.0>`
