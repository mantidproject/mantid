========================
Mantid Workbench Changes
========================

.. contents:: Table of Contents
   :local:

New and Improved
----------------
- New plot interactions: Double click a legend to hide it, double click a curve to open it in the plot config dialog.

- It is now possible to overplot bin data from the matrix workspace view.
- Added a sample material dialog that is accessed via the context menu in the workspace widget.

Bugfixes
--------

- Display Debug and Information messages generated during workbench start up
- The background shell of spherical and elliptical peaks is now correctly plotted when viewing slices that do not cut through the peak center
- For the elliptical shell of integrated peaks, the background is correct when plotting with varying background thicknesses
- Fixed a bug which occurred when switching to a log scale in sliceviewer with negative data.

:ref:`Release 6.1.0 <v6.1.0>`
