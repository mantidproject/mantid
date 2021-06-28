========================
Mantid Workbench Changes
========================

.. contents:: Table of Contents
   :local:

New and Improved
----------------

- Peaks can now be added or removed from a PeaksWorkspace using the :ref:`peaks overlay <sliceviewer_peaks_overlay>` in :ref:`sliceviewer`.
- The list of eligible workspaces in the `WorkspaceSelector` is now sorted alphabetically
- New widget and workbench plugin: `WorkspaceCalculator`, allows to perform binary operations and scaling by a floating number on workspaces
- Added tooltips to all the widgets in the Slice Viewer. Please contact the developers if any are missing.

Bugfixes
--------

- "Grid" checkbox in "Edit axis" dialog, and "Grids on/off" toolbar button will now have the correct checked state when running a plot script with major grid lines.
- Scroll bars added to about dialog if screen resolution is too low.
- Fixed missing 'on top' windowing behaviour for the matrix and table workspace data displays.
- Sliceviewer now doesn't normalise basis vectors for HKL data such that Bragg peaks appear at integer HKL for cuts along e.g. HH0
- Fixed a bug where parameters wouldn't update in the fit property browser when fitting a single function with ties.
- Added missing icon for the uninstaller in Windows "Apps & features" list.

:ref:`Release 6.2.0 <v6.2.0>`
