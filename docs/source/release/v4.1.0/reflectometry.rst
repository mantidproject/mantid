=====================
Reflectometry Changes
=====================

.. contents:: Table of Contents
   :local:

.. warning:: **Developers:** Sort changes under appropriate heading
    putting new features at the top of the section, followed by
    improvements, followed by bug fixes.

ISIS Reflectometry Interface
----------------------------

New
###

- Tabs are now grouped inside "Batches" rather than having two separate "Groups" within each tab. This makes it easier to see which set of settings will be used together. Batches can be added/removed using the menu and the Batch tabs on the left.
- The Settings tab has been split into two separate tabs, Experiment Settings and Instrument Settings.
- The first and second transmission runs are now entered via two separate boxes. Multiple runs can be summed for either of these inputs by entering the run numbers as a comma-separated list.
- Default values for the current instrument are automatically set in the Experiment and Instrument Settings tabs when the interface is opened or the instrument is changed.
- Processing in event mode is now done asynchronously. Previously this used to lock up MantidPlot.
- Error handling has been improved to catch invalid inputs earlier. Errors are highlighted in red or (for the table) with a red star.

The Runs Table
^^^^^^^^^^^^^^

- Filtering by run or group name is now possible using the search bar above the table. This accepts regular expressions.

- Navigation by keyboard shortcuts has been added:
  - F2 edits a cell
  - Tab/Shift-Tab moves to the next/previous cell
  - Pressing Tab when in the last cell of a row adds a new row and moves to the first cell in it
  - Enter adds a new row/group at the same level
  - Ctrl-Enter adds a new child row to a group
  - Ctrl-X/Ctrl-C/Ctrl-V perform cut/copy/paste

- Additional highlighting has been added for rows in the table:

  - Grey rows are invalid and will not be processed
  - Yellow rows are currently processing
  - Green rows have completed successfully
  - Blue rows completed with an error - a tooltip will display the error message
  - Rows are also highlighted in blue if their mandatory output workspaces have been deleted - again, a tooltip will explain the issue
  - Renamed workspaces are now tracked, so that the row state is no longer reset if a mandatory output workspace is renamed


Bug fixes
#########

- Fixed an error about an unknown property value when starting the live data monitor from the reflectometry interface.
	
Algorithms
----------

Improvements
############

- An additional method to calculate background has been added to :ref:`algm-ReflectometryBackgroundSubtraction`.
- In ref:`ReflectometryISISLoadAndPreprocess` the TOF workspaces are now grouped together.

Bug fixes
#########

- Fixed a bug in :ref:`ReflectometrySumInQ <algm-ReflectometrySumInQ>` that resulted in slightly too small bins in the output workspace.

Removed
#######

- Version 1 of `FindReflectometryLines` has been removed. Use :ref:`FindReflectometryLines-v2 <algm-FindReflectometryLines>` instead.

:ref:`Release 4.1.0 <v4.1.0>`
