=======================
MantidWorkbench Changes
=======================

.. contents:: Table of Contents
   :local:


User Interface
##############

- The zoom icon in the SliceViewer and plot toolbars have been replaced with clearer icons.

Improvements
############
- The keyboard shortcut Ctrl+N now opens a new tab in the script editor.

- Attempting to save files that are larger than (by default) 10GB now results in a dialog box to inform the user that it may take a long time and gives them the opportunity to cancel.

Bugfixes
########
- Clicking Cancel after attempting to save a project upon closing now keeps Workbench open instead of closing without saving.
- Dialog windows no longer contain a useless ? button in their title bar.
- Instrument view now keeps the saved rendering option when loading projects. 
- Fixes an issue where choosing to not overwrite an existing project when attempting to save upon closing would cause Workbench to close without saving.
- Fit results on normalised plots are now also normalised to match the plot.
- A crash in the Fit Browser when the default peak was not a registered peak type has been fixed.
- Fixed an issue where you could not edit table workspaces to enter negative numbers.
- Fixes an issue where changing the curve properties in the figure options menu would reset the plot's axes scales.

:ref:`Release 4.2.0 <v4.2.0>`
