.. _EventFiltering:

===============
Event Filtering
===============

.. contents::
   :local:

In mantid, there are a variety of ways to filter events that are in an
:ref:`EventWorkspace`. They are :ref:`FilterByTime
<algm-FilterByTime>` and :ref:`FilterByLogValue
<algm-FilterByLogValue>` which will create a filter and apply it in a
single step. The other way to filter events is to use
:ref:`FilterEvents <algm-FilterEvents>` which allows for a variety of
workspaces to specify how an :ref:`EventWorkspace` is split. This
document focuses on how the create these workspaces and will largely
ignore :ref:`FilterByTime <algm-FilterByTime>` and
:ref:`FilterByLogValue <algm-FilterByLogValue>`.

How to generate event filters
=============================

Implicit filters
----------------

:ref:`algm-FilterByTime` and :ref:`algm-FilterByLogValue` internally
generate event filters during execution that are not exposed to the
user. These algorithms can only split the neutron events by pulse
time and do not provide the equivalent of a ``FastLog=True`` option.

Explicit filters
----------------

:ref:`algm-FilterEvents` takes either a :class:`SplittersWorkspace
<mantid.api.ISplittersWorkspace>`, :ref:`TableWorkspace <Table
Workspaces>`, or :ref:`MatrixWorkspace <MatrixWorkspace>` as the
``SplittersWorkspace``. The events are split into output workspaces
according to the times that they arrive detectors.

:ref:`GenerateEventsFilter <algm-GenerateEventsFilter>` will create a
:class:`SplittersWorkspace <mantid.api.ISplittersWorkspace>` based on
its various options. This result can be supplied as the
``SplittersWorkspace`` input property of ref:`algm-FilterEvents`. It
will also generate an ``InformationWorkspace`` which can be passed
along to :ref:`GenerateEventsFilter <algm-GenerateEventsFilter>`.
Depending on the parameters in :ref:`GenerateEventsFilter
<algm-GenerateEventsFilter>`, the events will be filtered based on
their pulse times or their absolute times.  An neutron event's
absolute time is the summation of its pulse time and TOF.

Custom event filters
====================

Sometimes one wants to filter events based on arbitrary conditions. In
this case, one needs to go beyond what existing algorithms can do. For
this, one must generate their own splitters workspace. The workspace
is generally 3 columns, with the first two being start and stop times
and the third being the workspace index to put the events into. For
filtering with time relative to the start of the run, the first two
columns are ``float``. To specify the times as absolute, in the case
of filtering files that will be summed together, the first two columns
should be ``int64``. For both of the examples below, the filter
workspaces are created using the following function:

.. code-block:: python

   def create_table_workspace(table_ws_name, column_def_list):
      CreateEmptyTableWorkspace(OutputWorkspace=table_ws_name)
      table_ws = mtd[table_ws_name]
      for col_tup in column_def_list:
          data_type = col_tup[0]
          col_name = col_tup[1]
          table_ws.addColumn(data_type, col_name)

      return table_ws

Relative time
-------------

The easiest way to generate a custom event filter is to make one
relative to the start time of the run or relative to a specified
epoch. As the times in the table are seconds, a table can be created
and used

.. code-block:: python

   filter_rel = create_table_workspace('custom_relative', [('float', 'start'), ('float', 'stop'), ('str', 'target')])
   filter_rel.addRow((0,9500, '0'))
   filter_rel.addRow((9500,19000, '1'))
   FilterEvents(InputWorkspace='ws', SplitterWorkspace=filter_rel,
                GroupWorkspaces=True, OutputWorkspaceBaseName='relative', RelativeTime=True)

This will generate an event filter relative to the start of the
run. Specifying the ``FilterStartTime`` in :ref:`FilterEvents
<algm-FilterEvents>`, one can specify a different time that filtering
will be relative to.

Absolute time
-------------

If instead a custom filter is to be created with absolute time, the
time must be processed somewhat to go into the table workspace. Much of the

.. code-block:: python

   abs_times = [datetime64('2014-12-12T09:11:22.538096666'), datetime64('2014-12-12T11:45:00'), datetime64('2014-12-12T14:14:00')]
   # convert to time relative to GPS epoch
   abs_times = [time - datetime64('1990-01-01T00:00') for time in abs_times]
   # convert to number of seconds
   abs_times = [float(time / timedelta64(1, 's')) for time in abs_times]

   filter_abs = create_table_workspace('custom_absolute', [('float', 'start'), ('float', 'stop'), ('str', 'target')])
   filter_abs.addRow((abs_times[0], abs_times[1], '0'))
   filter_abs.addRow((abs_times[1], abs_times[2], '1'))
   FilterEvents(InputWorkspace='PG3_21638', SplitterWorkspace=filter_abs,
                GroupWorkspaces=True, OutputWorkspaceBaseName='absolute', RelativeTime=False)

Be warned that specifying ``RelativeTime=True`` with a table full of
absolute times will almost certainly generate output workspaces
without any events in them.

.. categories:: Concepts
