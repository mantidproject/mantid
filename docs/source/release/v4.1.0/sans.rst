============
SANS Changes
============

.. contents:: Table of Contents
   :local:

:ref:`Release 4.1.0 <v4.1.0>`

ISIS SANS Interface
-------------------

New
###
- Non-compatibility reduction mode has been reactivated. In non-compatibility mode, workspaces are kept as event slices for longer for a quicker reduction. To use non-compatibility mode, uncheck **Compatibility mode** in the settings tab.
- Reductions can be optimised for event slices. To use this mode, check **Event slice optimization** and uncheck **Compatibility mode** in the settings tab.


Improvements
############

- Increased font size in run table.
- For ZOOM, SHIFT user file command now moves monitor 5.
- canSAS output mode will be disabled if 2D reduction mode is selected to avoid accidental errors with data dimension.
- **Load Batch File** opens to the directory of your last selected batch file. **Load User File** opens to the directory of your last selected user file.
- Batch files created by exporting the runs table have the same order of keys as in the table.
- Batch files no longer require an output name to load. When processing, an auto-generated name is used instead.
- When the main save directory is changed, the add runs save directory is also updated. Add runs save directory can be still changed independently of the main save directory.
- File type buttons are disabled when memory mode is selected to make it clearer that SANS will not save to file.
- The path to the user file used to reduce the data is now added to the workspace sample logs. This user file path is added to canSAS file metadata.
- The **diagnostic** page icon has been changed from a question mark to a stethoscope, to distinguish it from the **Help** page icon. The **Export Table** button now has an icon. There have been minor icon changes elsewhere on the interface.
- Sample thickness, height, and width can be read from a batch file. These parameters are also included in the batch file generated from exporting the table.
- The beam centre HAB/LAB update checkboxes are automatically disabled if you select LAB/HAB as the reduction mode, respectively.
- The run numbers for sample transmission, sample direct, can scatter, and can direct workspaces are now added to NXCanSAS and CanSAS files.


Bug Fixes
#########

- Fixed the error seen when saving to file with event slice data. Event slice output files now contain transmission workspaces.
- Exporting table as a batch file is fixed for Mantid Workbench.
- The warning message raised when you have supplied a transmission run without a direct run has been suppressed when data is still being input. The warning will still be raised if you load or process the data.
- The algorithm :ref:`Load <algm-Load>` can now load NXcanSAS files.
- You can now process in **memory** mode with no file type buttons selected. A warning box will open if you process with no file types while in **file** or **both** mode, and processing will not continue.
- A bug in which the final column in a batch file was sometimes ignored if empty, and therefore impossible to load, has been fixed.
- The differences between non-compatibility and compatibility modes has been minimised. Compatibility mode can now be turned off in the settings tab. When off, workspaces stay as EventWorkspaces until the penultimate stage of **SANSReductionCore**. Bin masking is not performed until workspace has been converted to a histogram.
- When adding files, bad proton charges in run logs are identified and automatically corrected.
- A bug in which a row was added if the only row is the table was erased, has been fixed.

Algorithms
----------

New
###
- New algorithm :ref:`EQSANSCorrectFrame <algm-EQSANSCorrectFrame>`
- New IDF for ``EQ-SANS``
- New algorithm :ref:`LoadHFIRSANS <algm-LoadHFIRSANS>` and speed improvements to :ref:`LoadSpice2D <algm-LoadSpice2D>`
- New version of algorithm :ref:`CalculateEfficiency <algm-CalculateEfficiency-v2>`

Improved
########
- Added support for ``BIOSANS``, ``EQSANS``, and ``GPSANS`` to :ref:`MaskBTP <algm-MaskBTP>`. This includes an additional parameter ``Components`` to mask a particular list of instrument components.

