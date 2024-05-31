========================
Mantid Workbench Changes
========================

.. contents:: Table of Contents
   :local:

New Features
------------



Bugfixes
--------
- Fixed bug where selecting and deselecting different curves with error bars in the plot settings caused an error.
- Fixed bug where overplotting workspaces onto a regular Matplotlib plot could cause an error.
- Fixed a memory leak caused when loading nexus files from the disk.
- Fixed bug that caused a plot script to unhide all previously hidden plots.
- Switch to using ``fig.show`` in the generated plot scripts to avoid hanging problem in Workbench.
- Remove Fit and Superplot buttons from toolbar of non mantid axes and plots of 1D :ref:`MDHistoWorkspace <MDHistoWorkspace>` (for which the data have no workspace index).
- Fixed a bug in the fitting property browser, where removing a function from a nested composite function could cause a crash.
- Fixed a bug where renaming a plot in the plots widget would redraw the plot title regardless of the Show Title setting.
- Fixed potential crash when opening a file to edit where that file / directory had been deleted.
- Fixed bug where a corrupted recovery file could stop workbench from opening.
- Fixed a bug where changing the scale on a 3D plot would cause a crash.
- Fixed bug in the plot settings axes widget where setting an invalid axes limit and switching tabs could cause an error.
- Fixed a bug where an algorithm alias would always get the highest version of the parent algorithm instead of the specific version it was assigned to.
- Fixed a bug where dragging a table workspace onto a plot could cause a crash.
- Fix incorrect scaling when operating on the same workspace on both sides.


InstrumentViewer
----------------

New features
############
- Added method to python API of Instrument Viewer to set maintain aspect ratio option by calling `myiv.set_maintain_aspect_ratio(False)`.

Bugfixes
############
- Fixed bug where title on InstrumentView window did not update upon renaming the workspace.
- Fix rendering issue on linux when OpenGl is disabled.


SliceViewer
-----------

New features
############
- Added controls for the x and y limits to the main view of Sliceviewer.
- In non-orthogonal view, the signal will now be hidden (previously was ``-``).

Bugfixes
############



:ref:`Release 6.10.0 <v6.10.0>`