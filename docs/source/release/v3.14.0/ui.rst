======================
UI & Usability Changes
======================

.. contents:: Table of Contents
   :local:

.. warning:: **Developers:** Sort changes under appropriate heading
    putting new features at the top of the section, followed by
    improvements, followed by bug fixes.

- Added time standard deviation to the sample log dialog

Project Recovery
----------------
New
###
-Project recovery can now make a recovery checkpoint on command using mantidplot.app.saveRecoveryCheckpoint() in either the interpreter or script windows in python

Changes
#######
- Project Recovery will no longer save groups, this fixes an issue where it would cause crashes if you deleted a workspace from a group.
- MantidPlot no longer checks for the existence of files in the "Recent Files" menu. Fixes case where files on slow mounted network drives can cause a lag on MantidPlot startup.
- Workspaces now save locally as a number of how many workspaces have already been saved instead of workspace names
- Project Recovery will now attempt to recover multiple instances of mantid that are ran at the same time.

Bugfixes
########
- Workspaces with a '#' in their name will no longer cause issues in the loading of a recovered project
- Project Recovery will actually recover fully cases where multiple workspaces were passed as a list to an algorithm (Fixes a known bug with GroupWorkspaces aswell)
- Project Recovery will now run normally when you select no or the recovery fails when recovering from a ungraceful exit.

MantidPlot
----------

BugFixes
########

- Fixed issue where an open set of data from ITableWorkspace wouldn't update if the data was changed via python
- Fixed an issue where MantidPlot would crash when renaming workspaces.

:ref:`Release 3.14.0 <v3.14.0>`
