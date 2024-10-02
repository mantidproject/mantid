========================
Mantid Workbench Changes
========================

.. contents:: Table of Contents
   :local:

New Features
------------
- Updated compiler on macOS from clang version 15 to 16, which should result in performance improvements.
- Clarified ``Report Usage Data`` checkbox on the Workbench About page to explain that usage reporting is required to use the Error Reporter.
- expose :class:`ConfigService::remove() <mantid.kernel.ConfigServiceImpl.remove>` to python API
- Handled error where parsing malformed XML code in the code editor caused mantid to crash.
- Force Style for QComboBoxes and text, in order to make text visible on OSX dark mode
- expose :class:`ConfigService::configureLogging() <mantid.kernel.ConfigServiceImpl.configureLogging>` to python API
- expose :class:`ConfigService::getLogLevel() <mantid.kernel.ConfigServiceImpl.getLogLevel>` to python API
- fix handling of removed propertyies in :class:`ConfigService::saveConfig() <mantid.kernel.ConfigServiceImpl.saveConfig>`
- Added a new :ref:`Deprecation Policy <deprecation_policy>` for Mantid.
- Added tooltips to the run and abort buttons, displaying their functionality and shortcut keys.  Tooltips are displayed when hovering over the relevant buttons.
- Updated selection method  to `ExtendedSelection <https://doc.qt.io/qt-5/qabstractitemview.html#SelectionMode-enum>`_ when adding workspaces to the sequential fit dialog.


Bugfixes
--------
- Fixed a bug where right-clicking a 3D plot to change the colour bar scale could toggle the zoom tool.
- Fixed issue with dragging text onto the plot: Previously, dragging invalid text onto the plot resulted in unexpected behavior Now, when invalid text is dragged onto the plot, a warning is displayed to the user indicating that the workspace is invalid.
- Creating a script from a workspace's history, where that history includes algorithms using `StoreInADS=False`, should now create a functioning script.
- Fixed error that appered when editing the y-axis of plots generated in a python script within Workbench
- Fixed a bug where editing plot settings could reorder the line order.
- Fixed error when plotting a histogram of live data.
- Fixed crash when attempting to plot data with negative error values.
- Fixed key shortcuts `k` and `l` in mantid plot for quickly switching between `linear` and `log` scales.
- Disabled the broken Linear/Log context menu when right clicking on the colour bar of a 3D surface plot. The menu is now only accessible by right clicking inside the plot axes, which is consistent with other types of plotting (colorfill and contour).
- Fixed error when cropping a workspace after using the ``Plot All`` button
- Legend in plots is no longer accidentally picked up by a mouse scroll
- Fixed bug where the TOF converter window was not displaying properly on high resolution screens.


InstrumentViewer
----------------

New features
############


Bugfixes
############
- Fixed a bug where opening the instrument viewer with old muon data (DEVA) would cause a crash.
- Fixed drawing artifacts appearing (in some cases) when zooming in on the Instrument View with a 2D projection
- Fixed crash when selecting multiple ROI banks
- Fixed crash in the instrument view when trying to overlay peaks that do not have a corresponding detector.


SliceViewer
-----------

New features
############


Bugfixes
############
- Fixed issue causing unhandled exception when changing normalisation with the "gist_rainbow" colourmap.
- A warning will now be displayed if a workspace with unordered spectrum numbers is opened in the :ref:sliceviewer. These workspaces can fail to display correctly and may result in errors.


:ref:`Release 6.11.0 <v6.11.0>`