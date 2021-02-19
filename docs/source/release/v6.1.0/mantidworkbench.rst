========================
Mantid Workbench Changes
========================

.. contents:: Table of Contents
   :local:

New and Improved
----------------

- New plot interactions: Double click a legend to hide it, double click a curve to open it in the plot config dialog.
- It is now possible to overplot bin data from the matrix workspace view.
- Improved the performance of the table workspace display for large datasets
- A new facility named "NONE" is the default facility now, and
  user has to select their choice of facility and instrument for the first time, including ISIS

Bugfixes
--------

- The background shell of spherical and elliptical peaks is now correctly plotted when viewing slices that do not cut through the peak center
- For the elliptical shell of integrated peaks, the background is correct when plotting with varying background thicknesses
- Fixed a bug which occurred when switching to a log scale in sliceviewer with negative data.
- Fixed a bug that use wrong help links in certain interfaces
- First time dialog box will not appear recurrently

:ref:`Release 6.1.0 <v6.1.0>`
