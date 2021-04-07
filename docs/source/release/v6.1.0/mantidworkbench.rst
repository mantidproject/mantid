========================
Mantid Workbench Changes
========================

.. contents:: Table of Contents
   :local:

New and Improved
----------------

- Added Floating/On Top setting for all the windows that are opened by workbench (plots, interfaces, etc.)
- New plot interactions: Double click a legend to hide it, double click a curve to open it in the plot config dialog.
- It is now possible to overplot bin data from the matrix workspace view.
- New command line options ``--version`` will print the version on mantid and exit. ``--error-on-warning`` will convert python warnings into exceptions. This is intended for developers so they can find deprecation warnings more easily.
- Improved the performance of the table workspace display for large datasets
- Added a sample material dialog that is accessed via the context menu in the workspace widget.
- When a workspace is renamed it now updates the relevant plot labels with the new workspace name.
- Add a checkbox to freeze the rotation in the instrument viewer in Full 3D mode.
- Calling python's `input` now raises an input dialog in the script editor and the iPython shell.
- A new empty facility with empty instrument is the default facility now, and
  user has to select their choice of facility (including ISIS) and instrument for the first time.
- Added an algorithm ProfileChiSquared1D to profile chi squared after a fit. This can be used
  to find better estimates of parameter errors.
- Instrument view: when in tube selection mode, the sum of pixel counts is now output to the selection pane.
- Added memory widget to display total memory usage.


Bugfixes
--------

- Display Debug and Information messages generated during workbench start up
- The background shell of spherical and elliptical peaks is now correctly plotted when viewing slices that do not cut through the peak center
- For the elliptical shell of integrated peaks, the background is correct when plotting with varying background thicknesses
- Fixed a bug which occurred when switching to a log scale in sliceviewer with negative data.
- Fixed a bug that use wrong help links in certain interfaces
- Fixed a bug that would not let the user input the bounding box of a shape in the instrument viewer.
- If the facility in Mantid.user.properties is empty, it is consistently reflected as empty in the GUI
- First time dialog box will not appear recurrently, if user selected their choice of facility
  and instrument at least once and checked "Do not show again until next version".
- Fixed a bug that would cause a crash if the user right clicked on the plot in the instrument view pick tab after the stored curves were cleared.

:ref:`Release 6.1.0 <v6.1.0>`
