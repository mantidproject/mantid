======================
UI & Usability Changes
======================

.. contents:: Table of Contents
   :local:

.. warning:: **Developers:** Sort changes under appropriate heading
    putting new features at the top of the section, followed by
    improvements, followed by bug fixes.

- Added time standard deviation to the sample log dialog

Installation
------------
Changes
#######
The following changes have been made _only_ on Windows, bringing it in line with Mantid's installation naming on Linux.

- The Mantid Nightly build will now be installed in a different directory by default, to avoid overwriting the release Mantid installation.
- The Mantid Nightly build desktop and start menu shortcuts will have the "Nightly" suffix appended, to distinguish from the release Mantid installation.

Project Recovery
----------------
New
###
- Project Recovery can now make a recovery checkpoint on command using mantidplot.app.saveRecoveryCheckpoint() in either the interpreter or script windows in python
- Project Recovery now adds a lock file at the start of saving so if MantidPlot crashes when saving it will no longer use that checkpoint as it is incomplete.
- If project recovery fails when attempting to recover a checkpoint it will open a new GUI offering multiple checkpoints to the user and the ability to open them in a script window. (See image below)

.. figure:: ../../images/ProjectRecoveryFailureDialog.png
    :class: screenshot
    :align: right
    :figwidth: 70%

Changes
#######
- Project Recovery will no longer save groups, this fixes an issue where it would cause crashes if you deleted a workspace from a group.
- MantidPlot no longer checks for the existence of files in the "Recent Files" menu. Fixes case where files on slow mounted network drives can cause a lag on MantidPlot startup.
- Workspaces now save locally as a number of how many workspaces have already been saved instead of workspace names
- Project Recovery will now attempt to recover multiple instances of mantid that are ran at the same time.
- The project recovery prompt on mantid restart is improved and shows which checkpoint you will be getting. (See image below)
- Project Recovery will now output less unhelpful logging information into the results log
.. figure:: ../../images/ProjectRecoveryDialog.png
    :class: screenshot
    :align: right
    :figwidth: 70%

Bugfixes
########
- Workspaces with a '#' in their name will no longer cause issues in the loading of a recovered project
- Project Recovery will actually recover fully cases where multiple workspaces were passed as a list to an algorithm (Fixes a known bug with GroupWorkspaces as well)
- Project Recovery will now run normally when you select no or the recovery fails when recovering from a ungraceful exit.
- When autosaving or saving a recovery checkpoint with the Instrument View open the results log would be filled with excess logging and no longer does this.
- Fixed an issue where Project Recovery would start regardless of the config options

MantidPlot
----------

Changes
#######

- All File Browser dialog boxes will now (by default) display all valid file extensions as the first file filter.

BugFixes
########

- Fixed issue where an open set of data from ITableWorkspace wouldn't update if the data was changed via python
- Fixed an issue where MantidPlot would crash when renaming workspaces.

:ref:`Release 3.14.0 <v3.14.0>`
