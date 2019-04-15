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

Bug Fixes
#########

- Exporting table as a batch file is fixed for Mantid Workbench.
- The warning message raised when you have supplied a transmission run without a direct run has been suppressed when data is still being input. The warning will still be raised if you load or process the data.