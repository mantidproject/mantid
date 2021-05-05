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
- Added legend property commands to the plot script generator.
- Added memory widget to display total memory usage. This means that your widget layout will be reset when starting workbench v6.1.0 for the first time. Previously saved layouts accessible from ``View > User Layouts`` may need to be saved again to include the memory bar widget.


Bugfixes
--------

- Display Debug and Information messages generated during workbench start up
- The background shell of spherical and elliptical peaks is now correctly plotted when viewing slices that do not cut through the peak center
- For the elliptical shell of integrated peaks, the background is correct when plotting with varying background thicknesses
- Fixed a bug which occurred when switching to a log scale in sliceviewer with negative data.
- Fixed a bug that use wrong help links in certain interfaces
- Fixed a bug that would not let the user input the bounding box of a shape in the instrument viewer.
- The label of 1D curves in the legend of the plots is corrected to match the vertical axis bin center, if it is a BinEdgeAxis
- If the facility in Mantid.user.properties is empty, it is consistently reflected as empty in the GUI
- First time dialog box will not appear recurrently, if user selected their choice of facility
  and instrument at least once and checked "Do not show again until next version".
- Fixed a bug that would cause a crash if the user right clicked on the plot in the instrument view pick tab after the stored curves were cleared.
- Fixed a scenario where workbench could hang if the user closed a plot while live data was being read.
- Fixed a crash that happens when multiple plot windows are open, and the users closes one of them.
- The y-axis in the instrument view's pick tab will now rescale if the range changes.
- On the instrument widget pick tab, when the integration range is changed the current tool will stay selected.
- Fixed a bug where Workbench would hang on startup when running on Big Sur.
- Fixed a bug applying constraints with the conjugate gradient minimizer.
- Fixed a bug where panning on a colour fill plot stretches dataset along spectrum axis instead of panning
- Fixed a bug where TableWorkspace column names would not update correctly if the table was open.
- Fixed a problem with scripts generated from tiled plots.

Interfaces
----------

- Automatic data export in DrILL now creates a header for ASCII files, through the new property in :ref:`SaveAscii <algm-SaveAscii>`.
  See the :ref:`SaveAscii <algm-SaveAscii>` for more information.

:ref:`Release 6.1.0 <v6.1.0>`
