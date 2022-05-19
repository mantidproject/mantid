========================
Mantid Workbench Changes
========================

.. contents:: Table of Contents
   :local:

New Features
------------

- Added the Fit Script Generator interface to the 'General' category of the interface menu.
- New general setting: `Provide auto-completion suggestions in the script editor` for making auto-completion optional in the script editor
- An additional memory bar has been added to the memory widget to enable users to see how much memory Mantid is using.

.. image::  ../../images/mantid_memory_bar.png
            :align: center

- New feature: `FitPropertyBrowser` has been extended to allow for Function Attribute Validators.
  This allows the value of attributes to be restricted in numerous ways such as being bounded by a numeric min/max value or to be selected from a list of values using a drop box.
- Add support for the new SANS algorithm in DrILL.
- When loading and saving a project or a script, the working directory of the dialogs is set to the current default save directory.
- In DrILL, the working directory of save and load dialogs is set to the default save directory

Bugfixes
--------

- Suppressed a spurious warning where matplotlib warns about creating a figure outside of the main thread when using the script window.
- In DrILL interface, scrolling down in the settings dialog no longer affects the comboxes
- Cleaned up the appearance of the main window and the Indirect, Reflectometry, and Engineering GUIs on macOS.
- Fixed where matplotlib 3.5 caused 3D plots to throw a warning in multiple places.
- Fixed where matplotlib 3.5 caused a plot to raise an exception when right clicked, or showing the context menu.
- Fixed where matplotlib 3.5 caused a warning when opening plots.
- Fixed where matplotlib 3.5 caused any 3D plot to raise an exception when opened.
- Fixed where matplotlib 3.5 caused a crash when closing workbench with a plot open.
- Fixed where matplotlib 3.5 caused a project to be unable to save and load plots.
- Fixed where matplotlib 3.5 didn't allow for waterfall plots to functions
- Fixed a bug with the plotting status bar in Conda builds. The status bar no longer flickers when users move over the plot image or the z axis image.
- Fixed a memory leak whereby copies of workspaces were being retained even though the workspace had been deleted.
- Removed the load ILL tab from the interface in "Indirect" -> "Tools". Users should use LoadILLIndirect algorithm or the Load button instead.
- RebinnedOutput workspaces are now supported in the Superplot
- Copying a spectrum in a MatrixWorkspace is now supported for ragged workspaces.

Instrument Viewer
-----------------

New Features
############

- New option to see basic detector data in a tooltip when hovering.
- It is now possible to replace the workspace and save the image of the instrument viewer using its python API.

Bugfixes
########




Sliceviewer
-----------

New Features
############

- New cut viewer tool for non-axis aligned cuts in slice viewer (currently only supports 3D MD workspaces where all dimensions are Q).

Bugfixes
########

- Color limit autoscaling now works for MDHisto workspaces in non-orthogonal view in sliceviewer.
- Fixed a bug caused by moving to matplotlib 3.5 that prevented users from zooming in on a plot in SliceViewer.



:ref:`Release 6.4.0 <v6.4.0>`
