============
SANS Changes
============

.. contents:: Table of Contents
   :local:

.. warning:: **Developers:** Sort changes under appropriate heading
    putting new features at the top of the section, followed by
    improvements, followed by bug fixes.

:ref:`Release 4.1.0 <v4.1.0>`

ISIS SANS Interface
-------------------

Improvements
############

- Increased font size in run table.
- For ZOOM, SHIFT user file command now moves monitor 5.
- canSAS output mode will be disabled if 2D reduction mode is selected to avoid accidental errors with data dimension.
- **Load Batch File** opens to the directory of your last selected batch file. **Load User File** opens to the directory of your last selected user file.
- Batch files created by exporting the runs table have the same order of keys as in the table.
- Batch files no longer require an output name to load. When processing, an auto-generated name is used instead.
- When the main save directory is changed, the add runs save directory is also updated. Add runs save directory can be still changed independently of the main save directory.

Bug Fixes
#########

- Fixed the error seen when saving to file with event slice data. Event slice output files now contain transmission workspaces.
- Exporting table as a batch file is fixed for Mantid Workbench.
- The warning message raised when you have supplied a transmission run without a direct run has been suppressed when data is still being input. The warning will still be raised if you load or process the data.
- The algorithm :ref:`Load <algm-Load>` can now load NXcanSAS files.
- A bug in which the final column in a batch file was sometimes ignored if empty, and therefore impossible to load, has been fixed.