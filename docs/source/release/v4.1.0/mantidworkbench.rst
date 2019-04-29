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
- You can now double-click the numbers on a figure axis to adjust the axis' scale.
- You can now plot workspaces on top of figures you've created using scripts. Simply create a matplotlib figure in the
  script window, then drag and drop a workspace on top of it.

Bugfixes
########
- An error raised when double-clicking an arrow in the algorithm toolbox
  when no algorithm was selected has been fixed.
- Help documentation for the manage user directories interface now correctly displays when launched from the interface.
- A Colorfill plot of a workspace with one spectrum plots correctly and no longer raises an error.

:ref:`Release 4.1.0 <v4.1.0>`
