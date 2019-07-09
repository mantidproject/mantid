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

The following major changes are included in this release:

- The interface is now available in Workbench as well as MantidPlot
- Major usability improvements: functionality is largely the same and the changes should be intuitive, but a few things have been moved around, and there is new functionality for keyboard navigation, input validation and processing transmission runs. The rest of this section describes the improvements in more detail.

Batches and Settings
####################
- Tabs are now grouped inside "Batches" rather than having two separate "Groups" within each tab to make it easier to see which settings will be used together.
- Any number of Batches is now supported - batches can be added/removed using the menu and the Batch tabs on the left.
- The Settings tab has been split into two separate tabs or Experiment and Instrument Settings.
- Default values for the Experiment and Instrument Settings tabs are automatically populated for the selected instrument.

Transmission Runs
#################
- There are now separate input boxes for the first and second transmission runs.
- Multiple runs can be specified for each transmission input - these will be summed prior to reduction. Multiple values are entered as a comma-separated list, similarly to the Run(s) column.
- Specific spectra (i.e. ``processing instructions``) can be specified for the transmission runs on the Experiment Settings tab. If none are specified then the same spectra will be used as for the input runs.
- A new ``Transmission stitch params`` input allows you to stitch parameters specifically for the transmission runs rather than using the same parameters as for the output IvsQ workspaces.
- A new ``Scale RHS transmission workspace`` check box allows you to control which workspace is scaled when stitching transmission runs.

The Toobar
##########
.. |filldown| image:: ../../images/arrow-expand-down.png

- A "Fill Down" |filldown| button has been added, which allows filling all selected cells below the highest selected cell, in the column that is selected.
- Filtering by run or group name is now possible using the search bar above the table. This accepts regular expressions.
- Icons have been updated to be consistent between Workbench and MantidPlot.

.. figure:: /images/ISISReflectometryInterface_filter.png
  :align: center
  :alt: Filtering the runs table

  *Filtering the runs table*

.. figure:: /images/ISISReflectometryInterface_toolbar.png
  :align: center
  :alt: The new toolbar icons
        
  *The new toolbar icons*
  
Keyboard shortcuts
##################

Navigation by keyboard shortcuts has been added to the Runs table:

- ``F2`` edits a cell
- ``Esc`` cancels the current edit operation
- ``Tab``/``Shift-Tab`` moves to the next/previous cell
- Pressing ``Tab`` when in the last cell of a row adds a new row and moves to the first cell in it
- ``Enter`` adds a new row/group at the same level
- ``Ctrl-I`` inserts a new child row to a group
- ``Ctrl-X``/``Ctrl-C``/``Ctrl-V`` perform cut/copy/paste. Note that 
- The ``Delete`` key removes the selected rows/groups
- Copy/paste functionality is more intuitive - you can select the destination rows/groups to paste over or paste into the "root" of the table to create a new group

Processing and highlighting
###########################
- Input validation is more intuitive - invalid values are highlighted with a red background, or, in the table, cells with an invalid value are marked with a red star
- Additional highlighting has been added for rows and groups in the table
- A row or group's state is reset if its final output workspace(s) have been deleted
- Renamed workspaces now remain associated with the correct row/group in the table, so they can still be plotted
- Q min, Q max and dQ/Q are greyed out when they have been populated from the algorithm outputs so that you can easily distinguish between inputs and outputs
- The progress bar is more accurate, and remembers previous progress when you pause and restart processing.
- Processing in event mode is now done asynchronously, so it no longer locks up MantidPlot.

.. figure:: /images/ISISReflectometryInterface_processing.png
  :align: center
  :alt: Highlighting on the runs table

  *Highlighting on the runs table: Yellow is processing; green is complete; blue indicates an error (hover over the row to see the error message); grey rows are invalid; invalid/missing cell values are marked with a star*

Bug fixes
#########

- Fixed an error about an unknown property value when starting the live data monitor from the reflectometry interface.
- Fixed a problem where auto-saving would fail if the output for a row is a group workspace.
- Fixed a problem where the live data monitor would not start. Also fixed an issue where the output workspace is created prematurely as a clone of the TOF workspace.

Removed/updated
###############

- The ``Generate Notebook`` checkbox has been removed.
- The deprecation of ISIS Reflectometry (Old) GUI has been pushed back to November 2019.

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
