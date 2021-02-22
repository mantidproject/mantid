========================
Mantid Workbench Changes
========================

.. contents:: Table of Contents
   :local:

New and Improved
----------------

- It is now possible to overplot bin data from the matrix workspace view.

Bugfixes
--------

- Display Debug and Information messages generated during workbench start up
- The background shell of spherical and elliptical peaks is now correctly plotted when viewing slices that do not cut through the peak center
- For the elliptical shell of integrated peaks, the background is correct when plotting with varying background thicknesses

Interfaces
----------

- Automatic data export in DrILL now creates a header for ASCII files, through the new property in :ref:`SaveAscii <algm-SaveAscii>`.
  See the :ref:`SaveAscii <algm-SaveAscii>` for more information.

:ref:`Release 6.1.0 <v6.1.0>`
