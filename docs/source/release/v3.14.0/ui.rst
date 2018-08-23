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
- Workspaces now save locally as a number of how many workspaces have already been saved instead of workspace names

Bugfixes
########
- Workspaces with a '#' in their name will no longer cause issues in the loading of a recovered project

MantidPlot
----------

BugFixes
########

- Fixed issue where an open set of data from ITableWorkspace wouldn't update if the data was changed via python

:ref:`Release 3.14.0 <v3.14.0>`