========================
Mantid Workbench Changes
========================

.. contents:: Table of Contents
   :local:

New and Improved
----------------

- New plot interactions: Double click a legend to hide it, double click a curve to open it in the plot config dialog.
- It is now possible to overplot bin data from the matrix workspace view.
- New command line options ``--version`` will print the version on mantid and exit. ``--error-on-warning`` will convert python warnings into exceptions. This is intended for developers so they can find deprecation warnings more easily.
- Improved the performance of the table workspace display for large datasets
- A new empty facility with empty instrument is the default facility now, and
  user has to select their choice of facility (including ISIS) and instrument for the first time


- Added memory widget to display total memory usage

Bugfixes
--------

- The background shell of spherical and elliptical peaks is now correctly plotted when viewing slices that do not cut through the peak center
- For the elliptical shell of integrated peaks, the background is correct when plotting with varying background thicknesses
- Fixed a bug which occurred when switching to a log scale in sliceviewer with negative data.
- Fixed a bug that use wrong help links in certain interfaces
- Fixed a bug that would not let the user input the bounding box of a shape in the instrument viewer.
- If the facility in Mantid.user.properties is empty, it is consistently reflected as empty in the GUI
- First time dialog box will not appear recurrently, if user selected their choice of facility
  and instrument at least once and checked "Do not show again until next version"

:ref:`Release 6.1.0 <v6.1.0>`
