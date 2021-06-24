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

- Peaks can now be added or removed from a PeaksWorkspace using the :ref:`peaks overlay <sliceviewer_peaks_overlay>` in :ref:`sliceviewer`.
- The list of eligible workspaces in the `WorkspaceSelector` is now sorted alphabetically
- New widget and workbench plugin: `WorkspaceCalculator`, allows to perform binary operations and scaling by a floating number on workspaces

Bugfixes
--------

- Scroll bars added to about dialog if screen resolution is too low.
- Sliceviewer now doesn't normalise basis vectors for HKL data such that Bragg peaks appear at integer HKL for cuts along e.g. HH0

:ref:`Release 6.2.0 <v6.2.0>`
