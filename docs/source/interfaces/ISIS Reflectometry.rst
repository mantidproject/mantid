.. _interface-isis-refl:


============================
ISIS Reflectometry Interface
============================

.. contents:: Table of Contents
  :local:

.. |process| image:: /images/icons/sigma.png
.. |pause| image:: /images/icons/pause.png
.. |expandall| image:: /images/icons/expand-all.png
.. |collapseall| image:: /images/icons/collapse-all.png
.. |plotrow| image:: /images/icons/chart-line.png
.. |plotgroup| image:: /images/icons/chart-areaspline.png
.. |insertrow| image:: /images/icons/table-row-plus-after.png
.. |removerow| image:: /images/icons/table-row-remove.png
.. |insertgroup| image:: /images/icons/table-plus.png
.. |removegroup| image:: /images/icons/table-remove.png
.. |copy| image:: /images/icons/content-copy.png
.. |paste| image:: /images/icons/content-paste.png
.. |cut| image:: /images/icons/content-cut.png
.. |filldown| image:: /images/icons/arrow-expand-down.png

Purpose
-------
This user interface allows for batch processing of data reduction for
reflectometry data. The actual data reduction is performed with
:ref:`ReflectometryReductionOneAuto <algm-ReflectometryReductionOneAuto>`.
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

Sample Data
~~~~~~~~~~~

To follow this example you will need the ISIS reflectometry example materials:

* ``INTER00013460.nxs``
* ``INTER00013462.nxs``
* ``INTER00013463.nxs``
* ``INTER00013464.nxs``

These can be downloaded as part of the `ISIS example data <http://download.mantidproject.org/>`_.

Once they are downloaded, place the nxs files in one of Mantid's user directories.
To see a list of directories, click on **File -> Manage User Directories**.

Processing Runs
~~~~~~~~~~~~~~~

Open either ``MantidWorkbench`` or ``MantidPlot``, and open the ISIS
Reflectometry interface from the menu: **Interfaces -> Reflectometry -> ISIS
Reflectometry**

First, we want to enter the runs that we will process into the table. Enter the
values as shown in the figure below. Just the run number and the angle is the
minimum requirement.

.. tip:: You can use the `Toolbar`_ or `Keyboard Shortcuts`_ to edit the table,
  e.g.  ``Tab`` between cells, ``Enter`` to add a new row, ``Ctrl-I`` to insert
  a child row.

.. figure:: /images/ISISReflectometryInterface/workflow_runs.png
  :class: screenshot
  :width: 700px
  :align: center
  :alt: Run numbers and angles entered into the runs table

  *Run numbers and angles entered into the runs table*

Let's process the first group, which consists of the first two rows of the
table (13460 and 13462). Select the group we want to process, and then click on
|process| **Process**.

.. figure:: /images/ISISReflectometryInterface/workflow_processed.png
  :class: screenshot
  :width: 700px
  :align: center
  :alt: The runs table after the first group has been processed

  *The runs table after the first group has been processed with default settings*

The rows within the group should turn yellow (probably very briefly) to
indicate that they are processing, and then green when they have
completed. Once both rows have been processed, the group will be post-processed
and it will also turn green. The ``Q min``, ``Q max`` and ``dQ/Q`` cells will
also be updated with the values that were calculated in the reduction.

.. tip:: If a row or group turns blue, it has an error. Hover over the row to
  see the error message and consult the `Troubleshooting`_ section of this
  document for guidance on fixing it.

Editing Settings
~~~~~~~~~~~~~~~~

The above is a minimal reduction. We also want to use some transmission runs to
correct this data. We could enter these into the runs table but instead we will
use the ``Experiment Settings`` tab to set them as defaults for all runs.  We
will also set the limits and resolution for the final rebinning in ``Q``, and
output the debug workspaces.

Enter the following information on the table on the ``Experiment Settings``
tab, and tick the ``Debug`` option. Then re-process the group.

.. figure:: /images/ISISReflectometryInterface/workflow_settings.png
  :class: screenshot
  :width: 800px
  :align: center
  :alt: Editing experiment settings

  *Editing experiment settings*
  
Viewing Results
~~~~~~~~~~~~~~~

You should now have several workspaces in the ADS. Amongst them should be:

+-----------------+----------------------------------------------------------------------------+
|Workspace        | Description                                                                |
+=================+============================================================================+
|TOF_13460        | This is the data before processing. The X axis is time of flight in        |
|                 | :math:`\mu s`.                                                             |
+-----------------+----------------------------------------------------------------------------+
|IvsQ_13460       | This is the output workspace of                                            |
|                 | :ref:`ReflectometryReductionOneAuto <algm-ReflectometryReductionOneAuto>`. |
|                 | The X axis is momentum transfer in Å\ :sup:`-1`\ .                         |
+-----------------+----------------------------------------------------------------------------+
|IvsQ_13460_13462 | This workspace is the result of stitching ``IvsQ_13460`` and ``IvsQ_13462``|
|                 | together using :ref:`Stitch1DMany <algm-Stitch1DMany>`. The X axis is      |
|                 | momentum transfer in Å\ :sup:`-1`\ .                                       |
+-----------------+----------------------------------------------------------------------------+
|IvsLam_13460     | This is the wavelength output workspace of                                 |
|                 | :ref:`ReflectometryReductionOneAuto <algm-ReflectometryReductionOneAuto>`. |
|                 | The X axis is wavelength in Å. It is only output if the ``Debug`` option is|
|                 | ticked.                                                                    |
+-----------------+----------------------------------------------------------------------------+
|TRANS_13463_13464| This is a transmission run, created by running                             |
|                 | :ref:`CreateTransmissionWorkspace <algm-CreateTransmissionWorkspace>`      |
|                 | on ``TOF_13463`` and ``TOF_13464``. The X axis is wavelength in Å.         |
+-----------------+----------------------------------------------------------------------------+

For convenience, the interface provides tools to easily plot the main
outputs. The plot-rows |plotrow| button plots the reduced runs
(i.e. ``IvsQ_binned_13460`` and ``IvsQ_binned_13462``) for the selected row(s),
and/or all rows in the selected group(s). The plot-groups |plotgroup| button
plots the stitched output for the selected group(s)
(i.e. ``IvsQ_13460_13462``).

.. figure:: /images/ISISReflectometryInterface/workflow_plot.png
  :class: screenshot
  :width: 700px
  :align: center
  :alt: Plotting the results of the reduction

  *Plotting the results of the reduction*

       
Layout
------

Batches
~~~~~~~

The main window contains one or more "Batches", which are shown as vertical
tabs on the left. Each Batch contains a group of settings tabs (Runs, Event
Handling, Experiment, Instrument and Save ASCII). Together, these provide all
of the settings for a particular reduction.

.. figure:: /images/ISISReflectometryInterface/batches.png
  :class: screenshot
  :width: 700px
  :align: center
  :alt: Batch tabs on the ISIS Reflectometry interface

  *Batch tabs on the left contain all of the reduction settings for a particular batch of runs*

Using multiple batches is useful when users need to apply different options to
runs measured during the same experiment. For instance, if some runs need to be
analyzed with a wavelength range of ``LambdaMin=1, LambdaMax=17`` but others
need a wavelength range of ``LambdaMin=1.5, LambdaMax=15``, users may want to
enter the first set of runs in the processing table in one batch and the
second set in the processing table in another batch, and update the settings in
each batch accordingly. The interface will use the settings from the relevant
batch to reduce runs in that batch's processing table.

Menu Bar
~~~~~~~~

The main menu currently just contains options for managing batches via the
**Batch** menu:

+------------------+----------------------------------------------------------+
| Action           | Effect                                                   |
+==================+==========================================================+
| New              | Add a new Batch tab                                      |
+------------------+----------------------------------------------------------+

Runs Tab
~~~~~~~~

This section describes the different elements in the *Runs* tab.

.. figure:: /images/ISISReflectometryInterface/runs_tab.png
  :class: screenshot
  :width: 700px
  :align: center
  :alt: The runs tab

  *The runs tab*

Processing Table
^^^^^^^^^^^^^^^^

The processing table is where the bulk of the work takes place. It is used to
specify which runs to process, the properties that should be used to process
them, and how the different runs should be joined together.

Each row represents a single reduction (i.e. execution of
:ref:`ReflectometryReductionOneAuto <algm-ReflectometryReductionOneAuto>`),
and belongs to a group. Rows that are grouped together will have their output stitched
together using :ref:`Stitch1DMany <algm-Stitch1DMany>`.

Above the processing table is a `Toolbar`_ containing various actions for
manipulating the processing table, and a filter bar to allow filtering of the
table by group or run name. Various `Keyboard Shortcuts`_ are available to help
with quickly editing the table.

To process, simply select the rows or groups you want to process and click
``Process`` |process|. Alternatively, if nothing is selected, the entire table
will be processed.

Below the table is a progress bar, which shows the current progress of any
processing that is in progress. When processing the entire table, this will
show the percentage of the entire table that is complete. When processing a
selection it will show the percentage of that selection that is complete.

.. figure:: /images/ISISReflectometryInterface/processing_table.png
  :class: screenshot
  :width: 800px
  :align: center
  :alt: The processing table

  *The processing table*

Rows or groups that are currently processing will be highlighted in yellow, and
those that are successfully complete will be highlighted in green. Note that
groups that only have a single row do not have any stitching to do so will not
be processed and therefore will not turn green. If processing fails for any
reason, the row/group will be highlighted in blue and you can over over it to
see a tooltip displaying the error message.

Editing any settings that may change the outputs will reset the state for all
rows and groups. If any rows are added to or removed from a group, the group's
state will be reset. Deleting any of the mandatory output workspaces will also
reset the relevant row or group states. Note however that if you rename a
workspace, the interface will track it, so it will remain associated with its
original row or group.

If reduction stops and is then resumed, the interface will re-process any rows
and groups within the current selection that have not been processed, or whose
state has been reset. If you manually select rows/groups that have an error
then they too will be reprocessed. However if you process the entire table
(i.e. click ``Process`` |process| when nothing is selected), rows/groups that
have errors will **not** be reprocessed - you can manually select all rows in
the table if you want to reprocess them.

**Note**: The interface cannot be closed while runs are being processed. To
close the interface, you must first stop the reduction by clicking on the
``Pause`` |pause| button.

Columns
=======

The processing table contains the following columns:

+---------------------+-----------+---------------------------------------------------------------------------------+
| Column Title        | Required? |  Description                                                                    |
+=====================+===========+=================================================================================+
| Run(s)              | **Yes**   | Contains the sample runs to be processed.                                       |
|                     |           | Runs may be given as run numbers or workspace                                   |
|                     |           | names. Multiple runs may be added together by                                   |
|                     |           | separating them with a ``+`` or ``,``.                                          |
|                     |           |                                                                                 |
|                     |           | Example: ``1234+1235+1236``                                                     |
|                     |           |                                                                                 |
|                     |           | Note that if a workspace name contains ``+`` or ``,`` you must enter it in      |
|                     |           | quotes, e.g. ``"TOF_1234+1235+1236"``                                           |
+---------------------+-----------+---------------------------------------------------------------------------------+
| Angle               | **Yes**   | Contains the angle used during the run, in                                      |
|                     |           | degrees. If left blank,                                                         |
|                     |           | :ref:`ReflectometryReductionOneAuto <algm-ReflectometryReductionOneAuto>`       |
|                     |           | will calculate theta using                                                      |
|                     |           | :ref:`SpecularReflectionCalculateTheta <algm-SpecularReflectionCalculateTheta>`.|
|                     |           |                                                                                 |
|                     |           |                                                                                 |
|                     |           | Example: ``0.7``                                                                |
+---------------------+-----------+---------------------------------------------------------------------------------+
| 1st Trans Run(s)    | No        | Contains the transmission run(s) used to                                        |
|                     |           | normalise the sample runs. To specify two                                       |
| 2nd Trans Run(s)    |           | transmission runs, enter them in each input box.                                |
|                     |           | Note that as per the Run(s) column, you can sum multiple                        |
|                     |           | runs for each input by entering multiple values separated by ``+`` or ``,`.     |
|                     |           | If left blank, the sample runs will be                                          |
|                     |           | normalised by monitor only.                                                     |
|                     |           |                                                                                 |
|                     |           | Example: ``1234,1235``                                                          |
+---------------------+-----------+---------------------------------------------------------------------------------+
| Q min               | No        | Contains the minimum value of Q to be used in                                   |
|                     |           | Å\ :sup:`−1`\ . Data with a value of Q lower                                    |
|                     |           | than this will be discarded. If left blank,                                     |
|                     |           | this is set to the lowest Q value found. This                                   |
|                     |           | is useful for discarding noisy data.                                            |
|                     |           |                                                                                 |
|                     |           | Example: ``0.1``                                                                |
+---------------------+-----------+---------------------------------------------------------------------------------+
| Q max               | No        | Contains the maximum value of Q to be used in                                   |
|                     |           | Å\ :sup:`−1`\ . Data with a value of Q higher                                   |
|                     |           | than this will be discarded. If left blank,                                     |
|                     |           | this is set to the highest Q value found. This                                  |
|                     |           | is useful for discarding noisy data.                                            |
|                     |           |                                                                                 |
|                     |           | Example: ``0.9``                                                                |
+---------------------+-----------+---------------------------------------------------------------------------------+
| dQ/Q                | No        | Contains the resolution used when rebinning                                     |
|                     |           | output workspaces. If left blank, this is                                       |
|                     |           | calculated for you using the                                                    |
|                     |           | NRCalculateSlitResolution algorithm. This value is                              |
|                     |           | negated so that Logarithmic binning can be                                      |
|                     |           | applied for the IvsQ workspace.                                                 |
|                     |           | If you desire linear binning then you                                           |
|                     |           | may negate the value in the processing table                                    |
|                     |           | and a linear binning will be applied.                                           |
|                     |           |                                                                                 |
|                     |           | Example: ``0.9``                                                                |
+---------------------+-----------+---------------------------------------------------------------------------------+
| Scale               | No        | Contains the factor used to scale output                                        |
|                     |           | IvsQ workspaces. The IvsQ workspaces are                                        |
|                     |           | scaled by ``1/i`` where i is the value of                                       |
|                     |           | this column.                                                                    |
|                     |           |                                                                                 |
|                     |           | Example: ``1.0``                                                                |
+---------------------+-----------+---------------------------------------------------------------------------------+
| Options             | No        | Contains options that allow you to override                                     |
|                     |           | ReflectometryReductionOne's properties. To                                      |
|                     |           | override a property, just use the property's                                    |
|                     |           | name as a key, and the desired value as the                                     |
|                     |           | value.                                                                          |
|                     |           | Options are specified in ``key=value`` pairs,                                   |
|                     |           | separated by commas. Values containing commas                                   |
|                     |           | must be quoted. Options specified via this                                      |
|                     |           | column will prevail over options specified                                      |
|                     |           | in the **Settings** tab.                                                        |
|                     |           |                                                                                 |
|                     |           | Example: ``RegionOfDirectBeam="0,2", Params="1,2,3"``                           |
+---------------------+-----------+---------------------------------------------------------------------------------+

Toolbar
=======

This table details the behaviour of the actions in the tool bar, from left to right.

.. figure:: /images/ISISReflectometryInterface/toolbar.png
  :class: screenshot
  :align: center
  :alt: The runs table toolbar

  *The runs table toolbar*

+----------------------------------------+----------------------------------------------------------+
| Action                                 | Effect                                                   |
+========================================+==========================================================+
| |process| Process                      | Processes the selected runs, or, if no runs are selected,|
|                                        | all of the runs in the table. When a group is selected,  |
|                                        | runs belonging to the same group are stitched together.  |
+----------------------------------------+----------------------------------------------------------+
| |pause| Pause                          | Pauses processing any selected runs. Processing may be   |
|                                        | resumed by clicking on the 'Process' button. If the      |
|                                        | selection has changed, the new selection will be         |
|                                        | processed.                                               |
+----------------------------------------+----------------------------------------------------------+
| |expandall| Expand Groups              | Expand all groups so that you can see all child rows.    |
+----------------------------------------+----------------------------------------------------------+
| |collapseall| Collapse Groups          | Collapse all groups to hide all child rows.              |
+----------------------------------------+----------------------------------------------------------+
| |plotrow| Plot Selected                | Creates a plot of the IvsQ workspaces generated by any of|
| Rows                                   | the selected rows (or all child rows of the selected     |
| Rows                                   | groups).                                                 |
+----------------------------------------+----------------------------------------------------------+
| |plotgroup| Plot Selected              | Creates a plot of the stitched IvsQ workspaces generated |
| Groups                                 | by any of the selected groups.                           |
+----------------------------------------+----------------------------------------------------------+
| |insertrow| Insert Row                 | Inserts a new child row into the selected group          |
+----------------------------------------+----------------------------------------------------------+
| |removerow| Delete Row                 | Deletes any selected rows. If no rows are selected,      |
|                                        | nothing happens.                                         |
+----------------------------------------+----------------------------------------------------------+
| |insertgroup| Insert Group             | Adds a new group after the first selected group, or at   |
|                                        | the end of the table if no groups were selected.         |
+----------------------------------------+----------------------------------------------------------+
| |removegroup| Delete Group             | Deletes any selected Groups. If no groups are selected,  |
|                                        | nothing happens.                                         |
+----------------------------------------+----------------------------------------------------------+
| |copy| Copy Rows                       | Copies the selected rows or groups into the clipboard.   |
+----------------------------------------+----------------------------------------------------------+
| |paste| Paste Rows                     | Pastes the contents of the clipboard onto the selected   |
|                                        | rows or groups. For groups, if no destination is selected|
|                                        | the they will be pasted as new groups at the end of the  |
|                                        | table. Rows must always be pasted onto a destination     |
|                                        | selection of the same size.                              |
+----------------------------------------+----------------------------------------------------------+
| |cut| Cut Rows                         | Copies the selected rows, and then deletes them.         |
+----------------------------------------+----------------------------------------------------------+

Keyboard Shortcuts
==================

The following keyboard shortcuts are available for editing in the the runs
table.

+-----------------------------+---------------------------------------+
| Shortcut                    | Action                                |
+=============================+=======================================+
|``F2``                       | Edit the current cell                 |
+-----------------------------+---------------------------------------+
|``Esc``                      | Cancel editing                        |
+-----------------------------+---------------------------------------+
|``Tab``                      | Next cell                             |
+-----------------------------+---------------------------------------+
|``Shift-Tab``                | Previous cell                         |
+-----------------------------+---------------------------------------+
|``Enter``                    | Edit the next row / append a new row  |
+-----------------------------+---------------------------------------+
|``Ctrl-I``                   | Insert child row                      |
+-----------------------------+---------------------------------------+
|``Ctrl-X``                   | Cut                                   |
+-----------------------------+---------------------------------------+
|``Ctrl-C``                   | Copy                                  |
+-----------------------------+---------------------------------------+
|``Ctrl-V``                   | Paste                                 |
+-----------------------------+---------------------------------------+
|``Delete``                   | Delete the selected rows/groups       |
+-----------------------------+---------------------------------------+
|``Up``/``Down``              | Select next/previous row              |
+-----------------------------+---------------------------------------+
|``Shift-Up``/``Shift-Down``  | Extend selection to next/previous row |
+-----------------------------+---------------------------------------+
|``Ctrl-A``                   | Select all                            |
+-----------------------------+---------------------------------------+

Search Interface
^^^^^^^^^^^^^^^^

.. figure:: /images/ISISReflectometryInterface/search.png
  :class: screenshot
  :align: right
  :alt: The search interface

  *The search interface*

To search for runs, select the instrument the runs are from, enter the id of
the investigation the runs are part of, and click on ``Search``.

In the table below, valid runs and their descriptions will be listed. You can
then transfer runs to the processing table by selecting the runs you wish to
transfer, and click the ``Transfer`` |transfer| button. You can also
right-click on one of the selected runs and select *Transfer* in the context
menu that appears.

Search Transfer
===============

Search transfer uses the descriptions associated with raw files from the experiment.

If a run's description contains the text ``th=0.7`` at the end of the
description then the interface will deduce that the run's angle (also known as
theta), was ``0.7``, and enter this value into the angle column for you.  This
holds true for any numeric value.

When multiple runs are selected and transferred simultaneously, the interface
will attempt to organise them appropriately in the processing table. The exact
behaviour of this is as follows:

- Any runs with the same description, excluding their theta value, will be
  placed into the same group.
- Any runs with the same description, including their theta value, will be
  merged into a single row, with all the runs listed in the **Run(s)** column
  in the format, ``123+124+125``.
- Rows within a group will be sorted by angle.

Failed Transfers
================

When transferring a run from the Search table to the Processing table there may
exist invalid runs. For example, where theta could not be found or is zero. In
the image below we have selected four runs from the Search table that we have
transfered to the processing table.

.. figure:: /images/ISISReflectometryInterface/transfer.png
  :class: screenshot
  :width: 500px
  :align: center
  :alt: Selecting runs from search table to transfer to processing table

  *Selecting runs from search table to transfer to processing table*

Attempting to transfer an invalid run will result in that run not being
transferred to the processing table. If the transfer was not successful then
that specific run will be highlighted in blue in the Search table. Hovering
over the highlighted run with your cursor will allow you to see why the run was
invalid.

Autoprocessing
^^^^^^^^^^^^^^

The interface provides **Autoprocessing**, which allows fully automatic
processing of runs for a particular investigation. Enter the instrument and
investigation ID and then click `Autoprocess` to start. This then:

- Searches for runs that are part of the investigation the id was supplied for.
- Transfers any initial runs found for that investigation from the Search table
  into the Processing table and processes them.y
- Polls for new runs and transfers and processes any as they are found.

If the investigation has not started yet, polling will begin straight away and
the Processing table will remain empty until runs are created.
  
Like the `Process` button in the Processing table, the `Autoprocess` button
will be disabled while autoprocessing is in progress. If autoprocessing has
been paused, the button will be enabled again. Clicking `Autoprocess` again
will resume processing from where it left off.

Rows that do not contain a valid theta value will not be included in
autoprocessing - they will be highlighted as failed rows in the Search
table. The error message will be displayed as a tooltip if you hover over the
row. These rows can be transferred manually by first pausing autoprocessing and
then selecting the rows and clicking `Transfer`.

Successfully reduced rows are highlighted in green. If a group has been
post-processed successfully then it is also highlighted in green. If the group
only contains a single row then post-processing is not applicable, and the
group will be highlighted in a paler shade of green to indicate that all of its
rows have been reduced successfully but that post-processing was not performed.

If row or group processing fails, the row will be highlighted in blue. The
error message will be displayed as a tooltip if you hover over the row. Failed
rows will not be reprocessed automatically, but you can manually re-process
them by pausing autoprocessing, selecting the required rows, and clicking
`Process`.

The Processing table is not editable while autoprocessing is running but can be
edited while paused. Any changes to a row that will affect the result of the
reduction will cause the row's state to be reset to unprocessed, and the row
will be re-processed when autoprocessing is resumed. You can also manually
process selected rows while autoprocessing is paused using the `Process` button.

Rows can be deleted and new rows can be added to the table while autoprocessing
is paused. Use the buttons at the top of the Processing table, or manually
transfer them from the Search table. They will then be included when you resume
autoprocessing.

If workspaces are deleted while autoprocessing is running, or before resuming
autoprocessing, then affected rows/groups will be reprocessed if their
mandatory output workspaces no longer exist. If you do not want a row/group to
be reprocessed, then you must first remove it from the table. Deleting interim
workspaces such as IvsLam will not cause rows to be reprocessed.

Changing the instrument, investigation id or transfer method while paused and
then clicking `Autoprocess` will start a new autoprocessing operation, and the
current contents of the Processing table will be cleared. You will be warned if
this will cause unsaved changes to be lost.

Live Data Monitoring
^^^^^^^^^^^^^^^^^^^^

The *Live data* section on the *Runs* tab allows you to start a monitoring
algorithm that will periodically load live data from the instrument and reduce
it with :ref:`ReflectometryReductionOneAuto
<algm-ReflectometryReductionOneAuto>`. It outputs two workspaces, `TOF_live`
for the original data and `IvsQ_binned_live` for the reduced data.

Live values for `ThetaIn` and the slit gaps are checked and used each time the
reduction runs. Other algorithm properties are taken from `Group 1` on the
*Settings* tab. Make any changes you want to the settings and press `Start
monitor` to begin monitoring. Note that **any changes to the settings will not
be updated** in the live data reduction unless you stop and re-start
monitoring.

You can stop monitoring at any time using the `Stop monitor` button or by
cancelling the algorithm from the *Algorithm progress* dialog. If you close the
interface, monitoring will continue running in the background. You can cancel
the `MonitorLiveData` algorithm from the *Algorithm progress* dialog.

If `MonitorLiveData` stops due to an error, the `Start monitor` button will be
re-enabled so that it can be re-started from the Interface.

Note that if you close and re-open the Interface, the link to any running
monitor algorithm will be lost. You will not be able to start a new version of
the monitor due to a clash in the output names. Stop the algorithm from the
*Algorithm process* dialog and re-start it from the new instance of the
Interface to re-link it.

Live data monitoring has the following requirements:

- CaChannel must be installed in Mantid. See the instructions `here <https://www.mantidproject.org/CaChannel_In_Mantid>`_.
- The instrument must be on IBEX or have additional processes installed to supply the EPICS values. If it does not, you will get an error that live values could not be found for `Theta` and the slits.


Event Handling Tab
~~~~~~~~~~~~~~~~~~

.. figure:: /images/ISISReflectometryInterface/event_handling_tab.png
  :class: screenshot
  :width: 800px
  :align: center
  :alt: The event handling tab

  *The event handling tab*

The ``Event Handling`` tab can be used to analyze event workspaces. It contains four text boxes for
specifying uniform even, uniform, custom and log value slicing respectively. Each of these slicing
options are exclusive, no more than one can be applied. If the text box for the selected slicing
method is empty no event analysis will be performed, runs will be loaded using
:ref:`LoadISISNexus <algm-LoadISISNexus>` and analyzed as histogram workspaces. When this text box
is not empty, runs will be loaded using :ref:`LoadEventNexus <algm-LoadEventNexus>` and the
interface will try to parse the user input to obtain a set of start and stop values. These define
different time slices that will be passed on to the filtering algorithms
(:ref:`GenerateEventsFilter <algm-GenerateEventsFilter>` and :ref:`FilterEvents <algm-FilterEvents>`). Each time slice will be
normalized by the total proton charge and reduced as described in the previous section. Note that,
if any of the runs in a group could not be loaded as an event workspace, you will get an error message
and the reduction will not be performed.

The four slicing options are described in more detail below:

- **Uniform Even** - The interface obtains the start and end times of the run and divides it into
  a specified number of evenly-sized slices. For example given a run of duration 100 seconds,
  specifying 4 uniform even slices will produce slices with ranges of ``0 - 25``, ``25 - 50``,
  ``50 - 75`` and ``75 - 100`` seconds respectively.
- **Uniform** - The interface obtains the start and end times of the run and divides it into
  several slices of a specified duration. If the total duration does not divide evenly by the
  slice duration, then the last slice will be shorter than the others. For example, given a run
  of duration 100 seconds, specifying slices of duration 30 seconds will produce slices with
  ranges of ``0 - 30``, ``30 - 60``, ``60 - 90`` and ``90 - 100`` seconds respectively.
- **Custom** - This takes a list if comma-separated numbers that indicate the start and end of
  each time slice. There are different possibilities:

  * If a single number is provided, e.g. ``100``, the interface will extract a single slice
    starting at the start of the run, and ending at ``100`` seconds.
  * If two numbers are provided, e.g. ``100, 200``, the interface will extract a single slice
    starting ``100`` seconds after the start of the run and stopping at 200 seconds after the
    start of the run.
  * If more than two numbers are provided, e.g. ``100, 200, 300``, the interface will extract two
    slices, the first one starting at ``100`` seconds after the start of the run and ending at
    ``200`` seconds after the start of the run, and the second one starting at ``200`` seconds
    and ending at ``300`` seconds.

- **LogValue** - This takes a single value which is the log value interval, and also the log name
  which is the name of the log we wish to filter the run for. For example, given a run and entries
  of ``100`` and ``proton_charge`` for slicing values and log name respectively, we would
  produce a number of slices each with interval ``100``.

Workspaces will be named with a suffix providing information about the slice, e.g
``IvsQ_13460_slice_50_75``, ``IvsQ_13460_slice_75_100``, etc.

Experiment and Instrument Settings Tabs
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

.. figure:: /images/ISISReflectometryInterface/experiment_settings_tab.png
  :class: screenshot
  :width: 800px
  :align: center
  :alt: The experiment settings tab

  *The experiment Settings tab*

The ``Experiment Settings`` and ``Instrument Settings`` tabs can be used to
specify options for the reduction and post-processing releting to a specific
experiment. The Experiment settings are variables that are mostly set by the
user, whereas the ``Instrument Settings`` are variables relating to the
instrument used to perform the reduction. Both are populated with default
values for the current instrument. The ``Restore Defaults`` button allows you
to revert the settings to the default values for the instrument.

The majority of these options are used by the interface to provide argument
values for the pre-processing and reduction steps, which are handled by the
algorithm: :ref:`ReflectometryISISLoadAndProcess
<algm-ReflectometryISISLoadAndProcess>`

The exception is ``Output Stitch Params``, which is used for the final
stitching done by the algorithm :ref:`Stitch1DMany <algm-Stitch1DMany>`. Note
however that if a bin width is not provided, for instance ``Params="-0.03"``,
then ``-dQ/Q`` will be used, if specified; otherwise a default value will be
calculated from the slits, if possible.

Note that when conflicting options are specified for the reduction,
i.e. different values for the same property are specified via one of the
settings tabs and the cells in the *Runs* tab, the latter will take
precedence. Therefore, the Settings tabs should be used to specify
global options that will be applied to all the rows in the table, whereas the
row values will only be applicable to the specific row for which those options
are defined.

Per-Angle Defaults
^^^^^^^^^^^^^^^^^^

The **Experiment Settings** tab allows some options to be specified on a
per-angle basis, that is, to specify defaults that will apply only to runs with
a specific angle. Note that matching angles are searched for within a tolerance
of ``0.01``. In the per-angle defaults table, you can also specify a "wildcard"
row, which will apply to all runs that don't also have a matching angle - just
leave the angle blank to create a wildcard row. Only one wildcard row may
exist.

.. figure:: /images/ISISReflectometryInterface/workflow_settings.png
  :class: screenshot
  :width: 800px
  :align: center
  :alt: The per-angle defaults table

  *The per-angle defaults table*

Entries in the per-angle defaults table are similar to the table on the Runs
tab. Default transmission runs can be specified and each input can take a
single run/workspace or a number of runs/workspaces that will be summed before
processing. Specific spectra of interest can be specified for the input runs
and separate spectra, if required, can be specified for the transmission runs -
if the latter are not specified then the ``Run Spectra`` will also be used for
the transmission runs. If both a First and Second tranmission input is
specified, then they will be stitched using the options specified.
  
.. figure:: /images/ISISReflectometryInterface/transmission_runs.png
  :class: screenshot
  :width: 600px
  :align: center
  :alt: Transmission run options

  *Transmission run options*

Save ASCII Tab
~~~~~~~~~~~~~~

The ``Save ASCII`` tab allows for processed workspaces to be saved in specific
ASCII formats. The filenames are saved in the form [Prefix][Workspace Name].[ext].

.. figure:: /images/ISISReflectometryInterface/save_tab.png
  :class: screenshot
  :width: 800px
  :align: center
  :alt: The save ASCII tab

  *The save ASCII tab*

+-------------------------------+------------------------------------------------------+
| Name                          | Description                                          |
+===============================+======================================================+
| Save path                     | At present this dialog doesn't have a standard       |
|                               | file dialog so that path must be filled in manually. |
|                               | The path must already exist as this dialog doesn't   |
|                               | have the ability to create directories. As the       |
|                               | naming of files is automatic, the path must also     |
|                               | point to a directory rather than a file.             |
+-------------------------------+------------------------------------------------------+
| Prefix                        | The prefix is what is added to the beginning of      |
|                               | the workspace name to create the file name. No       |
|                               | underscore or space is added between them so they    |
|                               | must be manually added.                              |
+-------------------------------+------------------------------------------------------+
| Filter                        | This can be specified to filter out workspaces       |
|                               | in the workspace list whose name does not match      |
|                               | that of the filter text.                             |
+-------------------------------+------------------------------------------------------+
| Regex                         | Checking this option allows a regular expression     |
|                               | to be used for filtering workspace names.            |
+-------------------------------+------------------------------------------------------+
| List Of Workspaces            | The left listbox will contain any workspaces loaded  |
|                               | into mantid (excluding group and table workspaces).  |
|                               | Double clicking on one will fill the right list box  |
|                               | with the parameters it contains. This listbox        |
|                               | supports multi-select in order to allow for multiple |
|                               | workspaces to be saved out at the same time with the |
|                               | same settings.                                       |
+-------------------------------+------------------------------------------------------+
| List Of Logged Parameters     | The right listbox starts out empty, but will fill    |
|                               | with parameter names when a workspace in the left    |
|                               | listbox is double clicked. This listbox supports     |
|                               | multi-select in order to allow for the save output   |
|                               | to contain multiple parameter notes.                 |
+-------------------------------+------------------------------------------------------+
| File format                   | This dialog can save to ANSTO, ILL cosmos, 3-column, |
|                               | and a customisable format. It doesn't save from      |
|                               | the main interface's table, but from workspaces      |
|                               | loaded into mantid. All algorithms are also          |
|                               | available as save algorithms from mantid itself.     |
+-------------------------------+------------------------------------------------------+
| Custom Format Options         | When saving in 'Custom' this section allows you      |
|                               | to specify if you want a Title and/or Q Resolution   |
|                               | column as well as specifying the delimiter.          |
+-------------------------------+------------------------------------------------------+
| Automatic Save                | Automatically save the main output workspace for     |
|                               | groups in the runs table. Note that the stitched     |
|                               | group output will be saved if there is one or, for   |
|                               | a single-row group, the ``IvsQ_binned`` row output   |
|                               | will be saved instead.                               |
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

When I try to process I get an error: "Error encountered while stitching group ..."
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

This occurs when Mantid is unable to stitch a group. Please check that at you have
specified at least the bin width. This can be done either by setting a value in column
**dQ/Q** before processing the data, or by using the ``Output Stitch Params`` text
box in the **Experiment Settings** tab to provide the *Params* input property like this:
``Params="-0.03"`` (you may want to replace ``0.03`` with a bin size suitable for
your reduction). Note that the "-" sign in this case will produce a logarithmic binning in the
stitched workspace. For linear binning, use ``Params="0.03"``.

When I try to process I get an error: "Invalid key value pair, '...'"
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

This occurs when the contents of the options column are invalid.
Key value pairs must be given in the form ``key = value``, and if the value
contains commas it **must** be quoted, like so: ``key = "v,a,l,u,e"``.

When I try to process I get an error: "Invalid value for property ... Can not convert "False/True" to boolean"
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

This occurs when a boolean property is set to "True" or "False". Please, use ``1`` or ``0`` instead.

My IvsQ workspaces are not being stitched correctly
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

Stitching is controlled by the group a row is in. For stitching to occur, the
rows must be in the same group, and be processed simultaneously. To select all
rows in a group, just select the group itself - its child rows are implicitly
selected.

.. categories:: Interfaces Reflectometry
