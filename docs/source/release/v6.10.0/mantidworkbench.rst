========================
Mantid Workbench Changes
========================

.. contents:: Table of Contents
   :local:


Bugfixes
--------

- Selecting or deselecting plot curves in plot settings no longer causes an error.
- Overplotting workspaces onto a regular Matplotlib plot no longer causes an error.
- Dragging a table workspace onto a plot no longer causes a crash.
- Loading files from disk no longer causes a memory leak.
- Previously hidden plots are no longer unhidden from some plot scripts.
- Plot scripts in documentation now use ``fig.show()`` instead of ``plt.show()`` to avoid hanging Workbench.
- Fit property browser no longer crashes when removing a function from a nested composite function.
- Renaming a plot in the plots widget no longer redraws the plot title regardless of the ``Show Title`` workbench setting.
- Opening a file to edit where that file/directory had been deleted no longer causes a crash.
- Corrupted recovery files no longer stop workbench from opening.
- Changing scale on a 3D plot no longer causes a crash.
- Plot settings axes widget no longer throws an error when setting an invalid axes limit and switching tabs.
- Algorithm alias no longer gets the highest version of the parent algorithm, and instead gets the specific version it was assigned to.
- Workspace scaling is no longer incorrect when operating on the same workspace on both sides.
- Using the full 3D view no longer causes a crash with certain instruments on some versions of Linux.
- Toolbar of Matplotlib plots (**non-mantid axes**) and plots of 1D :ref:`MDHistoWorkspace <MDHistoWorkspace>` (for which the data have no workspace index) no longer display Fit and Superplot buttons:

.. figure::  ../../images/6_10_release/remove-toolbar-buttons.png
   :width: 600px


InstrumentViewer
----------------

New features
############
- Python API of Instrument Viewer now has a method to set maintain aspect ratio option: `myiv.set_maintain_aspect_ratio(False)`. See documentation of :ref:`InstrumentViewer`.

Bugfixes
############
- InstrumentView window title now updates upon renaming the workspace.
- InstrumentView now avoids rendering issue on Linux when OpenGl is disabled.


SliceViewer
-----------

New features
############
- SliceViewer main window now has controls for the x and y limits:

  .. figure::  ../../images/6_10_release/sliceviewer-updated.png
     :width: 700px

- In non-orthogonal view the signal will now be hidden (previously was ``-``).


:ref:`Release 6.10.0 <v6.10.0>`
