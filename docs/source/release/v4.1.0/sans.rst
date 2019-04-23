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
- Batch files no longer require an output name to load. When processing, an auto-generated name is used instead.
- canSAS output mode will be disable if 2D reduction mode is selected to avoid accidental errors with data dimension.

Bug Fixes
#########

- Fixed the error seen when saving to file with event slice data. Event slice output files now contain transmission workspaces.
- Exporting table as a batch file is fixed for Mantid Workbench.
- The warning message raised when you have supplied a transmission run without a direct run has been suppressed when data is still being input. The warning will still be raised if you load or process the data.