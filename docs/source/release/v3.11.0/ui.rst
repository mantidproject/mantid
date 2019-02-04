======================
UI & Usability Changes
======================

.. contents:: Table of Contents
   :local:


User Interface
--------------

- Fixed the ability to drag & drop files into Mantid on OSX >=10.10
- The welcome dialog has been redesigned to use less of the screen to fit better onto laptops with small high resolution screens that use screen scaling.

Scanning workspaces
###################

- Choosing 'Show Detectors' for a workspace will now also show the time indexes for scanning workspaces.

Workspace History Window
########################

.. figure:: ../../images/WorkspaceHistoryShortenedValues.png
   :class: screenshot
   :align: right

- Algorithm proprties with long values are now shown in a shortened format both in this display and the log.  The full history is still retained, and generated python scripts will still work as before.

Custom Interfaces
#################

- General > Multi dataset fitting interface and the general fitting dock now display the status string returned by the `Fit` algorithm. If an error occurred during fitting it will be reported in this string.
- Indirect ILL calibration tab, deprecated since v3.9 is now dropped.
- SANS > ISIS SANS v2 experimental interface has become available. It has basic reduction functionalities and makes use of the new reduction backend.

SliceViewer Improvements
########################

- SliceViewer input of number of bins, thickness, and slice point now waits until the editing is finished to rebin or changing slice point instead of changing with each digit entered.
- Fixed the ability to drag & drop files into Mantid OSX >=10.10
- The welcome dialog has been redesigned to use less of the screen to fit better onto laptops with small high resolution screens that use screen scaling.

VSI Improvements
################

- ParaView has been updated to to `v5.4.0 <https://blog.kitware.com/paraview-5-4-0-release-notes/>`_.

.. figure:: ../../images/LaNaF4_3D_Slices.png
   :class: screenshot
   :align: right

- Upstream ParaView Contributions

  - Fix scaling, orientation and position inputs when nonorthogonal axes are present.
  - Speed up MultiSlice view by avoiding repeated allocating and freeing memory.
  - Fix the resample to image mapper, which was failing when input contains cell data.
  - Expose the SamplingDimensions property when using the resample to image mapper with vtkStructuredGrids.

- Upstream VTK Contributions

  - Improve surface filter performance using vtkSMPTools
  - Refactor the vtkDataSetTriangleFilter for better performance.
  - Refactor the threshold filter to take advantage of structured data.
  - Minimize duplicate code in vtkDataArrayPrivate and parallelize range calculation.

- Multislice view uses a custom `representation <https://www.paraview.org/ParaView/index.php/Views_And_Representations>`_ to speed up slicing by taking advantage of the consistent bin
  sizes in a MDHistoWorkspace. Smooth interaction with typical data sizes (< 10 million cells) is now possible.

Bugs Resolved
-------------
- Fixed a bug causing table windows with string values to appear truncated if the string contained a space.
- Fixed a bug where setting a table column's plot type would not be saved to the workspace correctly.
- We have improved the documentation for the `RewriteSpectraMap` property of the `LoadInstrument`
  algorithm
- Fixed a bug where replacing a workspace with X errors with a workspace without the errors while Data View was open would result in a crash.

Full list of
`GUI <http://github.com/mantidproject/mantid/pulls?q=is%3Apr+milestone%3A%22Release+3.11%22+is%3Amerged+label%3A%22Component%3A+GUI%22>`_
and
`Documentation <http://github.com/mantidproject/mantid/pulls?q=is%3Apr+milestone%3A%22Release+3.11%22+is%3Amerged+label%3A%22Component%3A+Documentation%22>`_
changes on GitHub
