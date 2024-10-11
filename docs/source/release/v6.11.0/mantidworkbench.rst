========================
Mantid Workbench Changes
========================

.. contents:: Table of Contents
   :local:

New Features
------------
- Updated compiler on macOS from clang version 15 to 16, which should result in performance improvements.
- Clarified ``Report Usage Data`` checkbox on the Workbench About page to explain that usage reporting is required to use the Error Reporter.
- Exposed :class:`ConfigService::remove() <mantid.kernel.ConfigServiceImpl.remove>` to python API.
- Exposed :class:`ConfigService::configureLogging() <mantid.kernel.ConfigServiceImpl.configureLogging>` to python API.
- Exposed :class:`ConfigService::getLogLevel() <mantid.kernel.ConfigServiceImpl.getLogLevel>` to python API.
- Fixed handling of removed propertyies in :class:`ConfigService::saveConfig() <mantid.kernel.ConfigServiceImpl.saveConfig>`.
- All text on the Workbench About screen is now readable when using OSX dark mode.
- There is a new :ref:`Deprecation Policy <deprecation_policy>` for Mantid.
- ``Run`` and ``Abort`` buttons now display tooltips explaining their functionality and shortcut keys.
  Tooltips are displayed when hovering over the relevant buttons:

  .. figure::  ../../images/6_11_release/run_abort_tooltip.gif
     :width: 450px

- Sequential Fit Dialog has a new selection method to `ExtendedSelection <https://doc.qt.io/qt-5/qabstractitemview.html#SelectionMode-enum>`_ when adding workspaces.


Bugfixes
--------
- Code editor no longer crashes when parsing malformed XML code.
- Right-clicking on a 3D plot to change the colour bar scale no longer triggers the zoom tool.
- Dragging arbitary text onto a plot now displays a warning indicating that the workspace name is invalid, rather than causing a crash.
- Creating a script from a workspace's history, where that history includes algorithms using `StoreInADS=False`, should now create a functioning script.
- Editing the y-axis of plots generated in a python script within Workbench no longer causes an error.
- Editing plot settings no longer reorders the line order.
- Plotting a histogram of live data for frequent updates is now functioning.
- Ploting data with negative error values no longer causes a crash.
- Fixed key shortcuts ``k`` and ``l`` in mantid plots for quickly switching between ``linear`` and ``log`` scales.
- Disabled the broken Linear/Log context menu when right clicking on the colour bar of a 3D surface plot.
  The menu is now only accessible by right clicking inside the plot axes, which is consistent with other types of plotting (colorfill and contour).
- Cropping a workspace after using the ``Plot All`` button no longer causes an error.
- Legend in plots is no longer accidentally picked up by a mouse scroll.
- Fixed bug where the TOF converter window was not displaying properly on high resolution screens.


InstrumentViewer
----------------

Bugfixes
############
- Opening the instrument viewer with old muon data (DEVA) no longer causes a crash.
- Zooming in on :ref:`Instrument View <InstrumentViewer>` with a 2D projection no longer causes drawing artifacts.
- Fixed crash in the :ref:`Instrument View <InstrumentViewer>` when trying to overlay peaks that do not have a corresponding detector.
- Fixed crash when selecting multiple ROI banks.


SliceViewer
-----------

Bugfixes
############
- Changing normalisation with the ``gist_rainbow`` colourmap no longer causes an error.
- A warning will now be displayed if a workspace with unordered spectrum numbers is opened in the :ref:`Slice Viewer <sliceviewer>`.
  These workspaces can fail to display correctly and may result in errors.


:ref:`Release 6.11.0 <v6.11.0>`
