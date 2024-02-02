========================
Mantid Workbench Changes
========================

.. contents:: Table of Contents
   :local:

New Features
------------
- Added extra tooltip for ISIS Linux users to help out anybody on IDAaaS who has not mounted the archive. This is due to a change in early 2024 that will require users to mount the archive on IDAaaS themselves using their federal ID.
- Added new options in the plot settings which let users set default X and Y ranges for spectra plots effectively zooming in on open.
- Editing a plot's title will now automatically update its name in the plot selector (and vice versa).
- Monitor for external changes to script files that are open in Mantid to prevent loss of work.
- An email is now required to submit an error report.


Bugfixes
--------
- Fixed bug that caused surface plots to hang and/or crash after changing properties such as axis labels or colour bar limits.
- Home button on a 3D plot will now reset the view to the default view.
- Fixed bug where Workbench can hang if you close a plot while an algorithm is processing.
- Fixed bug where the y axis in the superplot window did not scale immediately when selecting the "Normalise by max" option.
- Improved behaviour of the load dialog with event workspaces so that it doesn't expand off the bottom of the screen.
- Fixed bug where overplotting group workspaces would cause an unhandled exception.
- Fixed bug where, in certain cases, dragging a plot's legend could cause the plot to resize.
- Fixed a bug where double clicking a 3D plot to edit an axis label would grab hold of the plot for rotation and not release when the dialog was closed.
- Line Colour selection button is reenabled in the toolbar of contour plots.
- Fix for crash which could happen if workspaces are deleted while the project saves. Save now fails in a controlled way.
- Fix unhandled exception thrown when attempting to remove a curve from a plot that has a badly formatted label.
- Fixed bug where it was not possible to remove the grid lines on a plot once they had been added.
- The splash screen will no longer insist on being on top of every window.
- Fix bug that made settings window disappear behind the Workbench window on Linux when opening the font selection menu.
- Fixed bug where a 3D plot would shrink every time you changed a property of the colorbar.
- Fix issue when displaying large array properies in the sample logs viewer
- Fixed a bug where fixed properties in the fitting view could not be un-fixed.
- Fixed error in plot script generated for 1D MDHisto workspace plots.
- Fix bug with ``plotSpectrum`` where quickly over-plotting could cause Workbench to hang.
- Mantid's package size has been reduced by removing the algorithm dialog screenshots from the documentation.
- Fix for crash which could occur when opening file with unicode characters
- Fix a bug where changing waterfall x and y off sets would not update the plot axes limits.
- Fixed a bug where double clicking a plot to open the settings dialog would not end a pan/zoom event if either tool was selected.
- Fixed crash when opening the :ref:`algm-Rebin` algorithm dialog when a group workspace is selected as the input workspace.
- Fixed bug in the plot settings axes tab where editing an axis title and clicking ``Apply to all`` would clear your changes. The ui has been slightly reworked to make it clearer what ``Apply to all`` interacts with.
- Fixed bug where Workbench could crash if an editor tab was closed while executing.
- Fix for crash from setting both waterfall plot offsets to 0.


InstrumentViewer
----------------

New features
############
- Widgets in the collapsible stack in the pick tab are now resizable so users can change the height of the plot relative to the height of the info box.

Bugfixes
############
- Fixed intermittent crash in the Instrument View caused by a project recovery save happening while the Instrument View was opening.
- Fixed problem where spurious peaks can be added when picking single peaks in the Instrument View.


SliceViewer
-----------

New features
############


Bugfixes
############
- Fixed intermittent error in Slice Viewer when reopening after a change in support for nonorthogonal axes.
- Enables slice viewer's 'y' cut exports for event workspaces without giving errors in line plot and ROI modes.
- Fixed bug in the cut viewer where the plot would not update after changing plot settings until the window had been resized.
- Fixed layout bug when toggling the peaks overlays interface on/off.
- Fix issue when adding peaks was not taking into account the projection matrix when calculating HKL.
- Fixed a memory leak in the Slice Viewer colour bar.
- Fixed spin box showing an incorrect value when a peak was selected that was outside the range of the data.
- Fixed error in Slice Viewer when trying to click on the 2D plot after the `Add Peak` option is selected, but the peaks workspace has already been deleted.


:ref:`Release 6.9.0 <v6.9.0>`