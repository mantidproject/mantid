========================
Mantid Workbench Changes
========================

.. contents:: Table of Contents
   :local:

New and Improved
----------------

- It is now possible to overplot bin data from the matrix workspace view.

- A new empty facility with empty instrument is the default facility now, and
  user has to select their choice of facility (including ISIS) and instrument for the first time

Bugfixes
--------

- Display Debug and Information messages generated during workbench start up
- The background shell of spherical and elliptical peaks is now correctly plotted when viewing slices that do not cut through the peak center
- For the elliptical shell of integrated peaks, the background is correct when plotting with varying background thicknesses
- Fixed a bug which occurred when switching to a log scale in sliceviewer with negative data.

- If the facility in Mantid.user.properties is empty, it is consistently reflected as empty in the GUI
- First time dialog box will not appear recurrently, if user selected their choice of facility
  and instrument at least once and checked "Do not show again until next version"

:ref:`Release 6.1.0 <v6.1.0>`
