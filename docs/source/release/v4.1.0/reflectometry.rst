=====================
Reflectometry Changes
=====================

.. figure:: ../../images/ISISReflectometryInterface_processing.png
  :class: screenshot
  :width: 700px
  :align: right
  :alt: The improved ISIS Reflectometry Interface

  *Usability improvements in the ISIS Reflectometry Interface*

.. contents:: Table of Contents
   :local:

ISIS Reflectometry Interface
----------------------------

Workbench Support
#################

The ISIS Reflectometry interface is now fully supported in Workbench. You can also continue to use it in MantidPlot.

Usability Improvements
######################

Major usability improvements have been added to the interface in this release. There are general layout improvements as well as new functionality for:

- keyboard navigation;
- input validation; and
- pre-processing and stitching transmission runs.

The rest of this section describes the changes in more detail.

Significant under-the-hood changes have also taken place to update the interface to modern design and engineering standards, resulting in a much more stable, extensible interface.

Batches and Settings
^^^^^^^^^^^^^^^^^^^^
.. figure:: ../../images/ISISReflectometryInterface_batches.png
  :class: screenshot
  :width: 700px
  :align: center
  :alt: Batch tabs on the ISIS Reflectometry interface

  *Batch tabs on the left contain all of the settings for a particular reduction*

- Tabs are now grouped inside "Batches" rather than having separate "Groups" within each tab. This makes it easier to see which settings will be used for the current reduction.
- Any number of Batches is now supported - batches can be added using the Batch menu, or removed by clicking the X on the tab.
- The Settings tab has been split into two separate tabs for Experiment and Instrument Settings.
- Default values for the Experiment and Instrument Settings tabs are automatically populated for the selected instrument.

Transmission Runs
^^^^^^^^^^^^^^^^^

.. figure:: ../../images/ISISReflectometryInterface_transmission_runs.png
  :class: screenshot
  :width: 700px
  :align: center
  :alt: Additional transmission run settings

  *Additional transmission run settings: Multiple input runs can be summed; specific spectra can be extracted; stitching uses the given overlap range and rebin parameters, with the specified workspace being scaled*

- There are now separate input boxes for the first and second transmission runs.
- Multiple runs can be specified for each transmission input - these will be summed prior to reduction. Multiple values are entered as a comma-separated list, similarly to the Run(s) column.
- Specific spectra (i.e. ``processing instructions``) can be specified for the transmission runs on the Experiment Settings tab. If none are specified then the same spectra will be used as for the input runs.
- A new ``Transmission stitch params`` input allows you to stitch parameters specifically for the transmission runs rather than using the same parameters as for the output IvsQ workspaces.
- A new ``Scale RHS transmission workspace`` check box allows you to control which workspace is scaled when stitching transmission runs.

The Toobar
^^^^^^^^^^

.. |filldown| image:: ../../images/arrow-expand-down.png

- A "Fill Down" |filldown| button has been added, which allows filling all selected cells below the highest selected cell, in the column that is selected.
- Filtering by run or group name is now possible using the search bar above the table. This accepts regular expressions.
- Icons have been updated to be consistent between Workbench and MantidPlot.

.. figure:: ../../images/ISISReflectometryInterface_toolbar_and_filter.png
  :class: screenshot
  :align: center
  :alt: The new toolbar icons and filter box

  *The new toolbar icons and filter box*

Keyboard shortcuts
^^^^^^^^^^^^^^^^^^

.. figure:: ../../images/ISISReflectometryInterface_table_editing.png
  :class: screenshot
  :align: right
  :alt: Editing the Runs table

  *Editing the Runs table*

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
^^^^^^^^^^^^^^^^^^^^^^^^^^^

.. figure:: ../../images/ISISReflectometryInterface_row_highlighting.png
  :class: screenshot
  :width: 400px
  :align: center
  :alt: Highlighting on the runs table

  *Highlighting on the runs table: Yellow=processing; green=complete; grey=invalid; blue=error. A star indicates invalid or missing values.*

- Input validation is more intuitive - invalid values are highlighted with a red background, or, in the table, cells with an invalid value are marked with a red star
- Additional highlighting has been added for rows and groups in the table to indicate which row is currently processing (yellow) and rows that are invalid and will be ignored (grey).
- A row or group's state is reset if its final output workspace(s) have been deleted
- Renamed workspaces now remain associated with the correct row/group in the table, so they can still be plotted
- Q min, Q max and dQ/Q are greyed out when they have been populated from the algorithm outputs so that you can easily distinguish between inputs and outputs
- The progress bar is more accurate, and remembers previous progress when you pause and restart processing.
- Processing in event mode is now done asynchronously, so it no longer locks up MantidPlot.

.. figure:: ../../images/ISISReflectometryInterface_validation.png
  :class: screenshot
  :width: 700px
  :align: center
  :alt: The ISIS Reflectometry Interface showing invalid input values highlighted in red

  *Invalid input values are highlighted in red*

Bug fixes
#########

The following bugs have been fixed since the last release:

- Fixed an error about an unknown property value when starting the live data monitor from the reflectometry interface.
- Fixed a problem where auto-saving would fail if the output for a row is a group workspace.
- Fixed a problem where the live data monitor would not start. Also fixed an issue where the output workspace is created prematurely as a clone of the TOF workspace.

Removed/updated
###############

- The ``Generate Notebook`` checkbox has been removed as this was not used and not useful in its current state.
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
