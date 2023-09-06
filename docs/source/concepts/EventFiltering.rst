.. _EventFiltering:

===============
Event Filtering
===============

.. contents::
   :local:

Given an :class:`EventWorkspace <<mantid.api.IEventWorkspace>` (:ref:`additional docs <EventWorkspace>`)
one will generally want to either remove events (commonly called filtering) or divide them into separate output workspaces (splitting).
While the full list of algorithms can be found in the event filtering algorithm category, a high level summary of them is included here:

* :ref:`FilterByTime <algm-FilterByTime>` and :ref:`FilterByLogValue <algm-FilterByLogValue>` which will create a filter and apply it in a single step
* :ref:`GenerateEventsFilter <algm-GenerateEventsFilter>` which can create an event filter to be used by :ref:`FilterEvents <algm-FilterEvents>`
* :ref:`FilterEvents <algm-FilterEvents>` which allows for a variety of workspaces to specify how an :ref:`EventWorkspace` is split.


This document focuses on how to create workspaces for filtering and will largely
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
``SplitterWorkspace``. The events are split into output workspaces
according to their pulse times or the times they arrive at detectors.
Note, times in ``MatrixWorkspace`` and ``TableWorkspace`` are in seconds,
while times in ``SplittersWorkspace`` are in nanoseconds.

:ref:`GenerateEventsFilter <algm-GenerateEventsFilter>` will create a
:class:`SplittersWorkspace <mantid.api.ISplittersWorkspace>` based on
its various options. This result can be supplied as the
``SplitterWorkspace`` input property of :ref:`FilterEvents <algm-FilterEvents>`.
It will also generate an ``InformationWorkspace`` which can be passed
along to :ref:`FilterEvents <algm-FilterEvents>`.
Depending on the parameters in :ref:`GenerateEventsFilter
<algm-GenerateEventsFilter>`, the events will be filtered based on
their pulse times or their absolute times.  A neutron event's
absolute time is the sum of its pulse time and TOF.

Custom event filters
====================

Sometimes one wants to filter events based on arbitrary conditions. In
this case, one needs to go beyond what existing algorithms can do. For
this, one must generate their own splitters workspace. The workspace
is generally 3 columns, with the first two being start and stop times
and the third being the workspace index to put the events into. For
filtering with time relative to the start of the run, the first two
columns can be either integer or floating-point values. To specify the
times as absolute, the first two columns should be of ``long64`` type.
For both of the examples below, the filter workspaces are created using
the following function:

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
relative to the start time of the run. Note, the times in the
table are in seconds.

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
time must be processed somewhat to go into the table workspace:

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

Time average mean and stddev of logs
====================================

In general, the simple mathematical mean of a log is not the value of interest.
It is the mean weighted by time, referred to here as the time-average mean.
The method for calculating the time-average mean and standard deviation is explained in detail in [1]_.
We define that a log is represented by the right-continuous `multi-step function <https://en.wikipedia.org/wiki/Step_function>`_ :math:`L(t)` (the ``Kernel::TimeSeriesProperty`` class) and a region of interest in time (the ``Kernel::TimeROI`` class) is represented by the function :math:`M(t)` which is zero when the data should not be included and one when it should be.
The time-average mean, :math:`\mu_T` is given by

.. math::

   \mu_T = \frac{\int_0^T M(t) L(t) dt}{\int_0^T M(t) dt}

The denominator is correctly observed to be the duration.
The variance (standard deviation squared) is

.. math::

   \sigma_T^2 = \frac{\int_0^T M(t) (L(t) - \mu_T)^2 dt}{\int_0^T M(t) dt}

In the cases of properties (including time series) with only a single value, :math:`L`, these values become :math:`\mu_T = L` and :math:`\sigma_T^2=0` independent of the time region of interest, as expected.
When all data is to be used (i.e. :math:`M(t) = 1`), the equations simplify to the values weighted by their observed durations, or

.. math::

   \mu_T = \frac{\int_0^T L(t) dt}{\int_0^T dt} = \frac{1}{T} \int_0^T L(t) dt


References
----------

.. [1] P.F. Peterson, D. Olds, A.T. Savici, and W. Zhou *Advances in utilizing event based data structures for neutron scattering experiments* Review of Scientific Instruments **89** (2018) 093001. doi: `10.1063/1.5034782 <https://doi.org/10.1063/1.5034782>`_


.. categories:: Concepts
