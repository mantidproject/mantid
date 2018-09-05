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

Changes
#######
- Project Recovery will no longer save groups, this fixes an issue where it would cause crashes if you deleted a workspace from a group.

Bugfixes
########
- Workspaces with a '#' in their name will no longer cause issues in the loading of a recovered project
- Project recovery will now run normally when you select no or the recovery fails when recvoering from a ungraceful exit.

MantidPlot
----------

BugFixes
########

- Fixed issue where an open set of data from ITableWorkspace wouldn't update if the data was changed via python

:ref:`Release 3.14.0 <v3.14.0>`