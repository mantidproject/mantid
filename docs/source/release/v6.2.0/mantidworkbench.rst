========================
Mantid Workbench Changes
========================

.. contents:: Table of Contents
   :local:

New and Improved
----------------

- **Superplot is a new decorator widget for the plot window. It facilitates over-plotting and manipulation of overplotted data. See** :ref:`Superplot documentation <WorkbenchSuperplot>` **for more information.**

.. figure:: ../../images/superplot_1.png
    :width: 500px
    :align: center

- **New widget and workbench plugin:** :ref:`WorkbenchWorkspaceCalculator` **, allows to perform binary operations and scaling by a floating number on workspaces;
  This will require your widget layout to be reset when starting workbench v6.2.0 for the first time. Previously saved layouts accessible from** ``View > User Layouts``
  **may need to be saved again to include the** :ref:`WorkbenchWorkspaceCalculator` **widget.**

.. image::  ../../images/Workbench/WorkspaceCalculator/WorkspaceCalculator.png
    :align: right

- Workflow diagrams in help pages are now ``.svg`` rather than ``.png``.
- Peaks can now be added or removed from a PeaksWorkspace using the :ref:`peaks overlay <sliceviewer_peaks_overlay>` in :ref:`sliceviewer`.
- When fitting a plot, selecting the peak type will only update the default peak shape in the settings if the ``Set as global default`` checkbox is ticked.
- Plot legends can be shown or hidden from the plot context menu.
- The plot config dialog notifies the user when there has been an error applying the config to the plot, and allows them to change the config further.
- When fitting a plot, selecting the peak type will only update the default peak shape in the settings if the "Set as global default" checkbox is ticked.
- Added help button to the sliceviewer
- SliceViewer can toggle between different scales again without any issue.
- SliceViewer uses a more visible divider between the main data view and the peaks table view.
- SliceViewer now loads customized colormap from the colorbar directory upon starting.
- The list of eligible workspaces in the `WorkspaceSelector` can now be sorted by name.
- Added tooltips to all the widgets in the :ref:`sliceviewer`. Please contact the developers if any are missing.
- Script editor function call tip support for Numpy 1.21.
- The visibility of a component parameter in the Pick tab of the :ref:`InstrumentViewer` is now steered by the 'visible' attribute of a parameter in :ref:`IPF <InstrumentParameterFile>`.
- ADS signal handlers are now synchronized in `WorkspaceSelector`. This reduces the probability of hard crash when interacting with the widget while a script manipulating a large number of workspaces is being run.
- Added help button to the :ref:`sliceviewer`.
- :ref:`sliceviewer` can toggle between different scales again without any issue.
- :ref:`sliceviewer` uses a more visible divider between the main data view and the peaks table view.

Bugfixes
--------

- ``Grid`` checkbox in ``Edit axis`` dialog, and ``Grids on/off`` toolbar button will now have the correct checked state when running a plot script with major grid lines.
- Fixed a bug in colorfill plots which lead to the loss of a spectrum from the resulting image.
- Fixed a bug where removing the plot guess line in the fit browser could lead to an exception being thrown.
- Fixed the advanced plotting dialog incorrectly laying out, causing the options to be partially occluded.
- Scroll bars added to about dialog if screen resolution is too low.
- Fixed missing 'on top' windowing behaviour for the matrix and table workspace data displays.
- :ref:`sliceviewer` now doesn't normalise basis vectors for HKL data such that Bragg peaks appear at integer HKL for cuts along e.g. HH0.
- Uninstalling from Windows "Apps & features" list will now run the uninstaller as the current Windows user and delete all shortcuts.
- Added missing icon for the uninstaller in Windows "Apps & features" list.
- Fixed a bug where parameters wouldn't update in the fit property browser when fitting a single function with ties.
- Fixed a bug where the workspace index spinbox in the fit browser wouldn't update when the user added or removed curves from the figure.
- Fixed a bug retrieving algorithm history from a workspace when the retrieval methods were chained together.
- Fixed a bug where output workspaces of different types would interfere with successive calls to binary operations, such as multiply.
- Fixed JSON serialization issue of MantidAxType by explicitly extracting its value.
- Fixed a bug in the :ref:`sliceviewer` when transposing MDE workspaces multiple times would cause the data to become all zeros.
- Fixed a bug where the errorbar tab in the figure options was wrongly enabled while selecting multiple curves.
- Fixed a bug where marker formatting options were disabled upon opening the figure options.
- Fixed out of range errors in the :ref:`sliceviewer` that sometimes occurred whilst hovering over transposed data.
- Fixed the help icon not showing on OSX and high-resolution monitors.
- Tabbing between fields in the error reporter now works as expected, rather than jumping to a random place each time.
- Fixed the advanced plotting dialog incorrectly laying out, causing the options to be partially occluded.
- Fixed a bug in the Instrument Viewer causing the projection to not be updated when different axis views were selected in Full 3D.


:ref:`Release 6.2.0 <v6.2.0>`
