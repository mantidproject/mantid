.. _ISIS_SANS_Runs_Tab-ref:

Runs Tab
========

.. contents:: Table of Contents
  :local:

The *Runs* tab is where the majority of processing takes place. You can select
your user file, batch file or manually enter data sets for reduction in the
data table.

Users can also control how they would like to save their processed data
using the Save Options controls, which are described below.

.. image:: /images/ISISSansInterface/runs_page.png
   :align: center
   :width: 800px

Using the SANS GUI
------------------
All of the data required for a SANS reduction can be entered into this page.
First load the appropriate user file, which is usually written by an instrument
scientist and contains the settings that relate to a series of runs.

Once the settings file is loaded other pages, documented in other sections,
are activated allowing settings to be viewed and edited.

There are two ways of specifying the run numbers to analyze: as single run
or batch mode. In batch mode the runs are specified in a text file separated
by commas as in the batch mode specification. In both cases the directory in
where those run files can be found must be present in Mantid's
"Manage Directories" dialog.

In single run mode the sample run, empty can, transmission file, etc.
run numbers are entered into each box.

Next click "Load Data" and an error dialog box will appear if one of the
runs is incorrect or its file cannot be found. Once the data is loaded you
have the choice of a 1D or 2D reduction.

Once this is complete the name of the results workspace name will be saved into
the selected save directory.


Main Controls
-------------

+--------------------------+-------------------------------------------------------------------------+
| **Manage Directories**   | Allows a user to select the location of data and mask files             |
+--------------------------+-------------------------------------------------------------------------+
| **Load User File**       | To select the user file to load and use                                 |
+--------------------------+-------------------------------------------------------------------------+
| **Load Batch File**      | Selects a batch file which will pre-populate the table with run numbers |
+--------------------------+-------------------------------------------------------------------------+
| **Process Selected**     | Processes the selected row from the table                               |
+--------------------------+-------------------------------------------------------------------------+
| **Process All**          | Processes all rows in the table                                         |
+--------------------------+-------------------------------------------------------------------------+
| **Load**                 | Loads the run data associated with the selected row from the table      |
+--------------------------+-------------------------------------------------------------------------+

Table Controls
--------------


.. image:: /images/ISISSansInterface/runs_page_table.png
   :align: center
   :width: 800px

+--------------------------+-------------------------------------------------------------------------+
| **Insert row after**     | Adds a row after the currently selected row                             |
+--------------------------+-------------------------------------------------------------------------+
| **Delete row**           | Deletes the selected row                                                |
+--------------------------+-------------------------------------------------------------------------+
| **Copy selected**        | Creates a copy of the selected rows                                     |
+--------------------------+-------------------------------------------------------------------------+
| **Cut selected**         | Cuts the selected rows                                                  |
+--------------------------+-------------------------------------------------------------------------+
| **Paste selected**       | Pastes rows from the clipboard                                          |
+--------------------------+-------------------------------------------------------------------------+
| **Clear selected**       | Clears the entries from selected rows without deleting them             |
+--------------------------+-------------------------------------------------------------------------+

Table Columns
-------------

+--------------------------+-------------------------------------------------------------------------------------------------+
| **SampleScatter**        |   Scattering data file or run number to use. This is the only mandatory field                   |
+--------------------------+-------------------------------------------------------------------------------------------------+
| **SampleTrans**          |   Transmission data file or run number to use                                                   |
+--------------------------+-------------------------------------------------------------------------------------------------+
| **SampleDirect**         |   Direct data file or run number to use                                                         |
+--------------------------+-------------------------------------------------------------------------------------------------+
| **CanScatter**           |   Scattering datafile or run number for the can run                                             |
+--------------------------+-------------------------------------------------------------------------------------------------+
| **CanTrans**             |   Transmission datafile or run number for can run                                               |
+--------------------------+-------------------------------------------------------------------------------------------------+
| **CanDirect**            |   Direct datafile or run number for can run                                                     |
+--------------------------+-------------------------------------------------------------------------------------------------+
| **OutputName**           |   Name of output workspace                                                                      |
+--------------------------+-------------------------------------------------------------------------------------------------+
| **User File**            |   User file to use for this row. If specified it will override any options set in the GUI,      |
|                          |   with those set in the specified user file                                                     |
+--------------------------+-------------------------------------------------------------------------------------------------+
| **Sample Thickness**     |   Sets the sample thickness to be used in the reduction                                         |
+--------------------------+-------------------------------------------------------------------------------------------------+
| **Options**              |   This column allows the user to provide row specific settings. Currently only                  |
|                          |   **WavelengthMin**, **WavelengthMax** and **EventSlices** can be set (see below for details)   |
+--------------------------+-------------------------------------------------------------------------------------------------+

Save Options
------------

.. image::  /images/ISISSansInterface/runs_page_save_opts.png
   :align: center
   :width: 500px

+--------------------------+-----------------------------------------------------------------------------------------+
| **Save Other**           | Opens up the save a dialog box :ref:`Save Other <save-other>` which allows users        |
|                          | to manually save processed data                                                         |
+--------------------------+-----------------------------------------------------------------------------------------+
| **Save Options - Memory**| Keeps the workspaces in memory, but does not save.                                      |
+--------------------------+-----------------------------------------------------------------------------------------+
| **Save Options - Load**  | Saves the workspace to the user's output directory and removes from memory afterwards   |
+--------------------------+-----------------------------------------------------------------------------------------+
| **Save Options - Both**  | Saves the workspace to the user's output directory and keeps it in memory               |
+--------------------------+-----------------------------------------------------------------------------------------+
| **CanSAS/NxCanSAS/RKH**  | Tick boxes which allow the user to select the file formats to save into                 |
+--------------------------+-----------------------------------------------------------------------------------------+
| **Zero Error Free**      | Ensures that zero error entries get artificially inflated when the data is saved        |
|                          | This is beneficial if you wish to load the processed data into different analysis tools |
+--------------------------+-----------------------------------------------------------------------------------------+
| **Use optimizations**    | (Strongly Recommended) This will reuse already loaded data rather than reloading it     |
|                          | for each run in the table, speeding up processing considerably.                         |
+--------------------------+-----------------------------------------------------------------------------------------+
| **Plot results**         | If enabled, data is automatically plotted on a graph as it is processed.                |
+--------------------------+-----------------------------------------------------------------------------------------+
