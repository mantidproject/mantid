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

Changes
#######

- Pushed back deprecation of ISIS Reflectometry (Old) GUI to November 2019 from July 2019.

New
###

- Tabs are now grouped inside "Batches" rather than having two separate "Groups" within each tab. This makes it easier to see which set of settings will be used together. Batches can be added/removed using the menu and the Batch tabs on the left.
- The Settings tab has been split into two separate tabs, Experiment Settings and Instrument Settings.
- The first and second transmission runs are now entered via two separate boxes. Multiple runs can be summed for either of these inputs by entering the run numbers as a comma-separated list.
- New inputs have been added to control stitching of transmission runs: you can select whether to scale the left or right workspace; you can enter rebin parameters specifically for transmission run stitching (previously the same parameters as stitching of the output workspaces were used).
- Spectra of interest can be specified for the transmission runs via a new column on the Experiment Settings tab (previously the same spectra as the run workspaces was used).
- Default values for the current instrument are automatically set in the Experiment and Instrument Settings tabs when the interface is opened or the instrument is changed.
- Processing in event mode is now done asynchronously. Previously this used to lock up MantidPlot.
- Error handling has been improved to catch invalid inputs earlier. Errors are highlighted in red or (for the table) with a red star.
- The progress bar now remembers previous progress when you pause and restart processing.

The Runs Table
^^^^^^^^^^^^^^
.. |filldown| image:: ../../images/arrow-expand-down.png

- Added a "Fill Down" (|filldown|) functionality which allows filling all selected cells below the highest selected cell, in the column that is selected.
- Filtering by run or group name is now possible using the search bar above the table. This accepts regular expressions.

- Navigation by keyboard shortcuts has been added:
  - F2 edits a cell
  - Tab/Shift-Tab moves to the next/previous cell
  - Pressing Tab when in the last cell of a row adds a new row and moves to the first cell in it
  - Enter adds a new row/group at the same level
  - Ctrl-I inserts a new child row to a group
  - Ctrl-X/Ctrl-C/Ctrl-V perform cut/copy/paste
  - The Delete key removes the selected rows/groups
  - Copy/paste functionality is more intuitive - you can select the destination rows/groups to paste over or paste into the "root" of the table to create a new group

- Additional highlighting has been added for rows in the table:

  - Grey rows are invalid and will not be processed
  - Yellow rows are currently processing
  - Green rows have completed successfully
  - Blue rows completed with an error - a tooltip will display the error message
  - Rows are also highlighted in blue if their mandatory output workspaces have been deleted - again, a tooltip will explain the issue
  - Renamed workspaces are now tracked, so that the row state is no longer reset if a mandatory output workspace is renamed
  - Cells are greyed out when they have been populated from the algorithm outputs so that you can easily distinguish between inputs and outputs


Bug fixes
#########

- Fixed an error about an unknown property value when starting the live data monitor from the reflectometry interface.
- Fixed a problem where auto-saving would fail if the output for a row is a group workspace.
- Fixed a problem where the live data monitor would not start. Also fixed an issue where the output workspace is created prematurely as a clone of the TOF workspace.
	
Removed
#######

- The ``Generate Notebook`` checkbox has been removed.

Algorithms
----------

Improvements
############

- An additional method to calculate background has been added to :ref:`algm-ReflectometryBackgroundSubtraction`.
- The output workspaces of :ref:`algm-ReflectometrySliceEventWorkspace` now have names which describe the slice.
- In :ref:`algm-ReflectometryISISLoadAndProcess` all output workspaces have names which give information about the slice.
- In :ref:`algm-ReflectometryISISLoadAndProcess` the TOF workspaces are now grouped together.

Bug fixes
#########

- Fixed a bug in :ref:`ReflectometrySumInQ <algm-ReflectometrySumInQ>` that resulted in slightly too small bins in the output workspace.

Removed
#######

- Version 1 of `FindReflectometryLines` has been removed. Use :ref:`FindReflectometryLines-v2 <algm-FindReflectometryLines>` instead.

:ref:`Release 4.1.0 <v4.1.0>`
