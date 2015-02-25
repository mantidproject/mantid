ISIS Reflectometry (Polref) Interface
=====================================

.. contents:: Table of Contents
  :local:

Purpose
-------
This user interface allows for batch processing of data reduction for
reflectometry data. The actual data reduction is performed with
:ref:`ReflectometryReductionOne <algm-ReflectometryReductionOne>`.
Wherever possible, this interface attempts to use reasonable defaults,
either loaded from the instruments' parameter files, or calculated from
the provided data, to minimise the amount of user input required.
This interface also strives to be transparent, making it clear how your
data is being processed, and easy to adjust any of the options used.

Integration with data archives is also provided, allowing for data to
be located and prepared for reduction automatically.

Information on how to resolve common problems can be found in the
`Troubleshooting`_ section of this document.

Example Workflow
----------------

To follow this example you will need the ISIS reflectometry example materials:

* ``INTER_NR_test2.tbl``
* ``INTER00013460.nxs``
* ``INTER00013462.nxs``
* ``INTER00013463.nxs``
* ``INTER00013464.nxs``

These can be downloaded as part of the `ISIS example data <http://download.mantidproject.org/>`_.

Once they are downloaded, place the nxs files in one of Mantid's user directories.
To see a list of directories, click on **File -> Manage User Directories**.
``INTER_NR_test2.tbl`` can be saved anywhere you like, as long as you know where it is.

Open MantidPlot, and open the ISIS Reflectometry (Polref) interface.
**Interfaces -> Reflectometry -> ISIS Reflectometry (Polref)**

Within the interface, we first want to import the tbl file as a TableWorkspace.
To do this, click on **Reflectometry -> Import .TBL**. A :ref:`LoadReflTBL <algm-LoadReflTBL>`
dialog will open. Select ``INTER_NR_test2.tbl`` as the file, and enter ``MyTable``
as the output workspace.

A table workspace called ``MyTable`` should now exist in the ADS (:ref:`Analysis Data Service <Analysis Data Service>`).
To open the table workspace go to **Reflectometry -> Open Table -> MyTable**.
The processing table (shown below) should now contain four rows (13460, 13462, 13469, 13470).

.. interface:: ISIS Reflectometry (Polref)
  :widget: viewTable

Let's process the first group, which consists of the first two rows of the
table (13460 and 13462). The simplest way to do this is simply to select the
two rows we want to process, and then click on **Process**.

.. tip::
  If you receive an error, consult the `Troubleshooting`_ section of this document for guidance on fixing it.

You should now have eleven workspaces in the ADS.

Amongst them should be:

TOF_13460
  This is the data before processing. The X axis is time of flight in µs.

TRANS_13463_13464
  This is a transmission run, created by running :ref:`CreateTransmissionWorkspace <algm-CreateTransmissionWorkspace>`
  on ``TOF_13463`` and ``TOF_13464``. The X axis is wavelength in Å.

IvsQ_13460
  This is the output workspace of :ref:`ReflectometryReductionOne <algm-ReflectometryReductionOne>`. The X
  axis is momentum transfer in Å\ :sup:`-1`\ .

IvsLam_13460
  This is the wavelength output workspace of :ref:`ReflectometryReductionOne <algm-ReflectometryReductionOne>`.
  The X axis is wavelength in Å.

IvsQ_13460_13462
  This workspace is the result of stitching ``IvsQ_13460`` and ``IvsQ_13462`` together using
  :ref:`Stitch1D <algm-Stitch1D>`. The X axis is momentum transfer in Å\ :sup:`-1`\ .

Layout
------

.. interface:: ISIS Reflectometry (Polref)

Menu bar
~~~~~~~~

.. interface:: ISIS Reflectometry (Polref)
  :widget: menuBar

The **Reflectometry** menu provides access to the following functionality:

+------------------+----------------------------------------------------------+
| Action           | Effect                                                   |
+==================+==========================================================+
| Open Table       | Opens a valid *TableWorkspace* in the `Processing Table`_|
|                  | for processing.                                          |
+------------------+----------------------------------------------------------+
| New Table        | Discards the current contents of the `Processing Table`_,|
|                  | presenting a blank table.                                |
+------------------+----------------------------------------------------------+
| Save Table       | Saves the current contents of the `Processing Table`_ to |
|                  | the *TableWorkspace* it came from. If no such workspace  |
|                  | already exists, a new one can be created.                |
+------------------+----------------------------------------------------------+
| Save Table As    | Saves the current contents of the `Processing Table`_ to |
|                  | a new *TableWorkspace*.                                  |
+------------------+----------------------------------------------------------+
| Import .TBL      | Opens a :ref:`LoadReflTBL <algm-LoadReflTBL>` dialog,    |
|                  | enabling you to load a ``.tbl`` file into a              |
|                  | *TableWorkspace*.                                        |
+------------------+----------------------------------------------------------+
| Export .TBL      | Opens a :ref:`SaveReflTBL <algm-SaveReflTBL>` dialog,    |
|                  | enabling you to save a *TableWorkspace* to a ``.tbl``    |
|                  | file.                                                    |
+------------------+----------------------------------------------------------+
| Slit Calculator  | Opens the slit calculator: a tool to help calculate the  |
|                  | correct geometry for the instruments' slits. It's powered|
|                  | by the :ref:`CalculateSlits <algm-CalculateSlits>`       |
|                  | algorithm.                                               |
+------------------+----------------------------------------------------------+
| Options          | Opens the `Options`_ menu.                               |
+------------------+----------------------------------------------------------+

The **Edit** menu provides access to the same actions found in the tool bar.
These are documented in the `Tool Bar`_ section of this document.

Processing Table
~~~~~~~~~~~~~~~~

.. interface:: ISIS Reflectometry (Polref)
  :widget: groupProcessPane

The processing table is where the bulk of the work takes place. It is used to
specify which runs to process, the properties that should be used to process
them, and how the different runs should be joined together.

Each row represents a single reduction (i.e. execution of
:ref:`ReflectometryReductionOne <algm-ReflectometryReductionOne>`).
Rows may be grouped together by setting their **Group** column to the same
value. Rows that are grouped together will have their output stitched
together using :ref:`Stitch1D <algm-Stitch1D>`.

Above the processing table is a tool bar containing various actions for
manipulating the processing table.

Below the table is a progress bar, which shows the current progress of any
processing that is in progress. And at the bottom right, by the **Process**
button is the processing instrument selector. The processing instrument is
used to help identify the correct data to load when processing runs.

Tool Bar
~~~~~~~~

This table details the behaviour of the actions in the tool bar, from left to right.

.. interface:: ISIS Reflectometry (Polref)
  :widget: rowToolBar

.. WARNING If you're updating this documentation, you probably also want to update the "What's This" tips in ReflMainWidget.ui

+------------------+----------------------------------------------------------+
| Action           | Effect                                                   |
+==================+==========================================================+
| Process          | Processes the selected runs, or, if no runs are selected,|
|                  | all of the runs in the table.                            |
+------------------+----------------------------------------------------------+
| Expand Selection | Expands your selection such that any rows in the same    |
|                  | group as a row you have selected are added to your       |
|                  | selection.                                               |
+------------------+----------------------------------------------------------+
| Plot Selected    | Creates a plot of the IvsQ workspaces generated by any of|
| Rows             | the selected rows.                                       |
+------------------+----------------------------------------------------------+
| Plot Selected    | Creates a plot of the stitched IvsQ workspaces generated |
| Groups           | by any of the selected groups.                           |
+------------------+----------------------------------------------------------+
| Insert Row       | Adds a new row after the first selected row, or at the   |
|                  | end of the table if no rows are selected.                |
+------------------+----------------------------------------------------------+
| Delete Row       | Deletes any selected rows. If no rows are selected,      |
|                  | nothing happens.                                         |
+------------------+----------------------------------------------------------+
| Group Rows       | Takes all the selected rows and places them in a group   |
|                  | together, separate from any other group.                 |
+------------------+----------------------------------------------------------+
| Copy Rows        | Copies the selected rows to the clipboard. In the        |
|                  | clipboard, each column's value is separated by a tab, and|
|                  | each row is placed on a new line.                        |
+------------------+----------------------------------------------------------+
| Cut Rows         | Copies the selected rows, and then deletes them.         |
+------------------+----------------------------------------------------------+
| Paste Rows       | Pastes the contents of the clipboard into the selected   |
|                  | rows. If no rows are selected, new rows are inserted.    |
+------------------+----------------------------------------------------------+
| Clear Rows       | Resets the cells in any selected rows to their initial   |
|                  | value, in other words, blank.                            |
+------------------+----------------------------------------------------------+
| Help             | Opens this documentation for viewing.                    |
+------------------+----------------------------------------------------------+
| What's This      | Provides guidance on what various parts of the interface |
|                  | are for.                                                 |
+------------------+----------------------------------------------------------+

Columns
~~~~~~~

.. WARNING If you're updating this documentation, you probably also want to update the "What's This" tips for the columns in QReflTableModel.cpp

+---------------------+-----------+-----------------------------------------------+
| Column Title        | Required? |  Description                                  |
+=====================+===========+===============================================+
| Run(s)              | **Yes**   | Contains the sample runs to be processed.     |
|                     |           | Runs may be given as run numbers or workspace |
|                     |           | names. Multiple runs may be added together by |
|                     |           | separating them with a '+'.                   |
|                     |           |                                               |
|                     |           | Example: ``1234+1235+1236``                   |
+---------------------+-----------+-----------------------------------------------+
| Angle               | No        | Contains the angle used during the run, in    |
|                     |           | degrees. If left blank, this is set to the    |
|                     |           | last value for ``THETA`` in the run's sample  |
|                     |           | log. If multiple runs were given in the Run(s)|
|                     |           | column, the first listed run's sample log will|
|                     |           | be used.                                      |
|                     |           |                                               |
|                     |           | Example: ``0.7``                              |
+---------------------+-----------+-----------------------------------------------+
| Transmission Run(s) | No        | Contains the transmission run(s) used to      |
|                     |           | normalise the sample runs. To specify two     |
|                     |           | transmission runs, separate them with a comma.|
|                     |           | If left blank, the sample runs will be        |
|                     |           | normalised by monitor only.                   |
|                     |           |                                               |
|                     |           | Example: ``1234,1235``                        |
+---------------------+-----------+-----------------------------------------------+
| Q min               | No        | Contains the minimum value of Q to be used in |
|                     |           | Å\ :sup:`−1`\ . Data with a value of Q lower  |
|                     |           | than this will be discarded. If left blank,   |
|                     |           | this is set to the lowest Q value found. This |
|                     |           | is useful for discarding noisy data.          |
|                     |           |                                               |
|                     |           | Example: ``0.1``                              |
+---------------------+-----------+-----------------------------------------------+
| Q max               | No        | Contains the maximum value of Q to be used in |
|                     |           | Å\ :sup:`−1`\ . Data with a value of Q higher |
|                     |           | than this will be discarded. If left blank,   |
|                     |           | this is set to the highest Q value found. This|
|                     |           | is useful for discarding noisy data.          |
|                     |           |                                               |
|                     |           | Example: ``0.9``                              |
+---------------------+-----------+-----------------------------------------------+
| dQ/Q                | No        | Contains the resolution used when rebinning   |
|                     |           | output workspaces. If left blank, this is     |
|                     |           | calculated for you using the                  |
|                     |           | CalculateResolution algorithm.                |
|                     |           |                                               |
|                     |           | Example: ``0.9``                              |
+---------------------+-----------+-----------------------------------------------+
| Scale               | **Yes**   | Contains the factor used to scale output      |
|                     |           | IvsQ workspaces. The IvsQ workspaces are      |
|                     |           | scaled by ``1/i`` where i is the value of     |
|                     |           | this column.                                  |
|                     |           |                                               |
|                     |           | Example: ``1.0``                              |
+---------------------+-----------+-----------------------------------------------+
| Group               | **Yes**   | Contains the group number used for stitching  |
|                     |           | output workspaces. The value of this column   |
|                     |           | determines which other rows this row's output |
|                     |           | will be stitched with. All rows with the same |
|                     |           | group number are stitched together.           |
+---------------------+-----------+-----------------------------------------------+
| Options             | No        | Contains options that allow you to override   |
|                     |           | ReflectometryReductionOne's properties. To    |
|                     |           | override a property, just use the property's  |
|                     |           | name as a key, and the desired value as the   |
|                     |           | value.                                        |
|                     |           | Options are specified in ``key=value`` pairs, |
|                     |           | separated by commas. Values containing commas |
|                     |           | must be quoted.                               |
|                     |           |                                               |
|                     |           | Example: ``StrictSpectrumChecking=0,``        |
|                     |           | ``RegionOfDirectBeam="0,2", Params="1,2,3"``  |
+---------------------+-----------+-----------------------------------------------+

Search Interface
~~~~~~~~~~~~~~~~

.. interface:: ISIS Reflectometry (Polref)
  :widget: groupSearchPane
  :align: right

To search for runs, select the instrument the runs are from, enter the id of
the investigation the runs are part of, and click on **Search**.

In the table below, valid runs and their descriptions will be listed. You
can then transfer runs to the processing table by selecting the runs you
wish to transfer, and click the **Transfer** button. You can also right-click
on one of the selected runs and select *Transfer* in the context menu that
appears.

If a run's description contains the text ``in 0.7 theta``, or ``th=0.7``, or
``th:0.7``, then the interface will deduce that the run's angle (also known
as theta), was ``0.7``, and enter this value into the angle column for you.
This holds true for any numeric value.

When multiple runs are selected and transferred simultaneously, the interface
will attempt to organise them appropriately in the processing table. The exact
behaviour of this is as follows:

- Any runs with the same description, excluding their theta value, will be
  placed into the same group.
- Any runs with the same description, including their theta value, will be
  merged into a single row, with all the runs listed in the **Run(s)** column
  in the format, ``123+124+125``.

Options
~~~~~~~

Through the options menu, a small number of options may be configured to adjust
the behaviour of the interface.

To open the options menu, click on **Reflectometry -> Options**.

+-------------------------------+------------------------------------------------------+
| Name                          | Description                                          |
+===============================+======================================================+
| Warn when processing all rows | When the **Process** button is pressed with no rows  |
|                               | selected, all rows will be processed.                |
|                               | If this is enabled, you will be asked if you're sure |
|                               | you want to process all rows first.                  |
+-------------------------------+------------------------------------------------------+
| Warn when processing only     | If this is enabled and you press **Process** with    |
| part of a group               | only a subset of a group's rows selected, you will be|
|                               | asked if you're sure you that's what you intended to |
|                               | do.                                                  |
+-------------------------------+------------------------------------------------------+
| Warn when discarding unsaved  | If this is enabled and you try to open an existing   |
| changes                       | table, or start a new table, with unsaved changes to |
|                               | the current table, you will be asked if you're sure  |
|                               | you want to discard the current table.               |
+-------------------------------+------------------------------------------------------+
| Rounding                      | When a column is left blank, the Reflectometry       |
|                               | interface will try to fill it with a sensible value  |
|                               | for you. This option allows you to configure whether |
|                               | the value should be rounded, and if so, to how many  |
|                               | decimal places.                                      |
+-------------------------------+------------------------------------------------------+

Troubleshooting
---------------

When I try to process I get an error: "Invalid value for property Filename (list of str lists) ..."
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

This occurs when Mantid is unable to load a run. If the run was given as a
workspace name, check the spelling. If the run was given as a number, check
that the run number is correct. If the run number is incorrect, check the
number given in the **Run(s)** or **Transmission Run(s)** columns. If the run
number is correct, check the instrument named in the error message is correct.
If the instrument is incorrect, check that the processing instrument selector
(at the bottom right of the interface) is correct.

If the run still isn't loading check Mantid's user directories are set
correctly, and that the desired run is in one of the given directories. To
manage the user directories, open **File -> Manage User Directories**.

When I try to process I get an error: "Invalid key value pair, '...'"
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

This occurs when the contents of the options column are invalid.
Key value pairs must be given in the form ``key = value``, and if the value
contains commas it **must** be quoted, like so: ``key = "v,a,l,u,e"``.

The *Open Table* menu doesn't do anything
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

The **Open Table** menu contains a list of valid table workspaces to open in the
processing table. If a workspace is not compatible, it will not be listed. So,
if there are no compatible workspaces the **Open Table** menu will be empty.

My IvsQ workspaces are not being stitched correctly
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

Stitching is controlled by the group a row is in. For stitching to occur, the
rows must be in the same group, and be processed simultaneously.

An easy way to check the runs are in the same group is to select one of the
rows you want stitched, and then in the menu bar select **Edit -> Expand Selection**.
All the rows in that group will be selected. If you have another row that you
would like to add to the group, you can do this easily by adding it to the
selection, and then in the menu bar selecting **Edit -> Group Selected**.

.. categories:: Interfaces Reflectometry
