=======================
MantidWorkbench Changes
=======================

.. contents:: Table of Contents
   :local:

Improvements
############
- Linux desktops now have an icon to launch the workbench without using the terminal.
- The log level of help page notices has been reduced to declutter your message window.
- Single line commenting in the script editor is now enabled without needing to highlight any text.
- You can now import from local python files that are in the same directory as the script you're executing.
- All icons have been replaced with icons from Material Design: https://material.io/tools/icons.
- You can now double-click the numbers on a figure axis to adjust the axis' scale.
- You can now plot workspaces on top of figures you've created using scripts. Simply create a matplotlib figure in the
  script window, then drag and drop a workspace on top of it.
- Mantid's offline help is now available in Workbench.
- A colorfill plot of a workspace with logarithmic bins is plotted on a log scale.
- You can now toggle the normalization of plots in Workbench from the plot's context menu
- You can now save, load and delete custom layouts from the settings menu with quick access to user layout in the view
  menu.

Bugfixes
########
- An error raised when double-clicking an arrow in the algorithm toolbox
  when no algorithm was selected has been fixed.
- Help documentation for the manage user directories interface now correctly displays when launched from the interface.
- A Colorfill plot of a workspace with one spectrum plots correctly and no longer raises an error.
- The units on the y-axis of distribution workspace plots have been corrected

:ref:`Release 4.1.0 <v4.1.0>`