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

Progress Reporting
##################

- The progress reporting for algorithms has been improved, so that the progress is reported correctly when processin workspace groups or multi-period workspaces.


Documentation
#############
* Added Ragged Workspace as a concept page


Bugs Resolved
-------------
- Floating windows now always stay on top of the main window in OSX

SliceViewer Improvements
------------------------
* When opening the sliceviewer, it will default to showing the first two non-integrated dimensions

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
