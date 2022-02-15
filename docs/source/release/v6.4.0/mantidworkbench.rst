========================
Mantid Workbench Changes
========================

.. contents:: Table of Contents
   :local:

New and Improved
----------------
- New general setting: `Provide auto-completion suggestions in the script editor` for making auto-completion optional in the script editor

Bugfixes
--------
- In DrILL interface, scrolling down in the settings dialog no longer affects the comboxes
- Cleaned up the appearance of the main window and the Indirect, Reflectometry, and Engineering GUIs on macOS.
- Fixed where matplotlib 3.5 caused 3D plots to throw a warning in multiple places.
- Fixed where matplotlib 3.5 caused a plot to raise an exception when right clicked, or showing the context menu.
- Fixed where matplotlib 3.5 caused a warning when opening plots.
- Fixed where matplotlib 3.5 caused any 3D plot to raise an exception when opened.
- Fixed where matplotlib 3.5 caused a crash when closing workbench with a plot open.
- Fixed where matplotlib 3.5 caused a project to be unable to save and load plots.
- Suppressed a spurious warning where matplotlib warns about creating a figure outside of the main thread when using the script window.

Sliceviewer
-----------

Bugfixes
########
- Color limit autoscaling now works for MDHisto workspaces in non-orthogonal view in sliceviewer.

Instrument Viewer
-----------------

- It is now possible to replace the workspace and save the image of the instrument viewer using its python API.

:ref:`Release 6.4.0 <v6.4.0>`
