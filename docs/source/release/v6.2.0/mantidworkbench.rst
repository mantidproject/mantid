========================
Mantid Workbench Changes
========================

.. contents:: Table of Contents
   :local:

New and Improved
----------------

- Superplot is a new decorator widget for the plot window. It facilitates over-plotting and manipulation of overplotted data. See :ref:`Superplot documentation <WorkbenchSuperplot>` for more information.

.. figure:: ../../images/superplot_1.png
    :width: 500px
    :align: center

- Workflow diagrams in help pages are now ``.svg`` rather than ``.png``
- Peaks can now be added or removed from a PeaksWorkspace using the :ref:`peaks overlay <sliceviewer_peaks_overlay>` in :ref:`sliceviewer`.
- The list of eligible workspaces in the `WorkspaceSelector` can now be sorted by name
- New widget and workbench plugin: `WorkspaceCalculator`, allows to perform binary operations and scaling by a floating number on workspaces;
  This will require your widget layout to be reset when starting workbench v6.2.0 for the first time. Previously saved layouts accessible from ``View > User Layouts``
  may need to be saved again to include the workspace calculator widget.
- Added tooltips to all the widgets in the Slice Viewer. Please contact the developers if any are missing.
- Script editor tab completion and call tip support for Numpy 1.21
- The visibility of a component parameter in the Pick tab of the InstrumentViewer is now steered by the 'visible' atrribute of a parameter in IPF
- Plot legends can be shown or hidden from the plot context menu.

Bugfixes
--------

- "Grid" checkbox in "Edit axis" dialog, and "Grids on/off" toolbar button will now have the correct checked state when running a plot script with major grid lines.
- Scroll bars added to about dialog if screen resolution is too low.
- Fixed missing 'on top' windowing behaviour for the matrix and table workspace data displays.
- Sliceviewer now doesn't normalise basis vectors for HKL data such that Bragg peaks appear at integer HKL for cuts along e.g. HH0
- Uninstalling from Windows "Apps & features" list will now run the uninstaller as the current Windows user and delete all shortcuts.
- Fixed a bug where parameters wouldn't update in the fit property browser when fitting a single function with ties.
- Fixed a bug retrieving algorithm history from a workspace when the retrieval methods were chained together.
- Added missing icon for the uninstaller in Windows "Apps & features" list.
- Fixed a bug where output workspaces of different types would interfere with successive calls to binary operations, such as multiply.
- Fixed JSON serialization issue of MantidAxType by explicitly extracting its value
- Fixed a bug in colorfill plots which lead to the loss of a spectrum from the resulting image.

:ref:`Release 6.2.0 <v6.2.0>`
