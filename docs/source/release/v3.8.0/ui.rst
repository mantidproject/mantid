======================
UI & Usability Changes
======================

.. contents:: Table of Contents
   :local:

Installation
------------

Windows
#######

* IPython has been upgraded to version 3.2.1

OS X
####

User Interface
--------------

Instrument View
###############

Workspace Matrix View
#####################
* Masked spectra (including masked monitors) will be highlighted with background colour (typically light grey).
* Unmasked monitor spectra will have a dynamic background colour depending on the system settings (typically light yellow, as before).

.. figure::  ../../images/MaskedAndMonitor.png

Plotting Improvements
#####################

Algorithm Toolbox
#################

Algorithms
##########

.. figure::  ../../images/GroupWorkspaces_multipleInput.png
   :width: 487
   :align: right

- Algorithms can now use a multiple selection list box for property input, :ref:`MergeRuns <algm-MergeRuns>` and :ref:`GroupWorkspaces <algm-GroupWorkspaces>` use this so far. To select multiple rows use the Shift or Ctrl (Cmd) keys while clicking  with the mouse.


Scripting Window
################

 - All `matplotlib` examples now work out of the box when run inside the MantidPlot scripting environment.


Progress Reporting
##################

- The progress reporting for algorithms has been improved, so that the progress is reported correctly when processing workspace groups or multi-period workspaces.
- The progress reporting for algorithms has been improved, so that the progress is reported correctly when processin workspace groups or multi-period workspaces.


Documentation
#############

- Added Ragged Workspace as a concept page

Options Window
###############
- Within Preferences->Mantid->Options ticking a category off/on will now untick/tick all subcategories. Also having some subcategories on and off will now show a partially ticked box for that category. 


Bugs Resolved
-------------
- Floating windows now always stay on top of the main window in OSX
- The sliceviewer will now rebin an existing binned workspace correctly.
- 2D plots now display correctly for point data workspaces as well as for histogram data

SliceViewer Improvements
------------------------
* When opening the SliceViewer, it will default to showing the first two non-integrated dimensions
* The SliceViewer now uses bin centres instead of bin edges when slicing on a histogram workspace

VSI Improvements
----------------
* ParaView updated to version 5.1.0

Multi-dataset fitting interface improvements
--------------------------------------------
* Fitting a single dataset with a composite function no longer causes a crash

|

Full list of
`GUI <http://github.com/mantidproject/mantid/pulls?q=is%3Apr+milestone%3A%22Release+3.8%22+is%3Amerged+label%3A%22Component%3A+GUI%22>`_
and
`Documentation <http://github.com/mantidproject/mantid/pulls?q=is%3Apr+milestone%3A%22Release+3.8%22+is%3Amerged+label%3A%22Component%3A+Documentation%22>`_
changes on GitHub
