.. _SampleLogs Dev:

============
Sample Logs
============

.. contents::
  :local:

The following information will be useful to you if you want to write an
:ref:`algorithm` that manipulates sample log data.

Sample logs
###########

When you load a nexus file using Mantid, a set of sample logs relevant to the experimental run are loaded into the
workspace's run object, which can be accessed through:

.. code-block:: cpp

  #include "MantidAPI/Run.h"

  // Declare property with default settings
  // IndexType::WorkspaceIndex is default
  MatrixWorkspace_sptr workspace;
  ....
  Run &run = workspace->mutableRun()
  // get all log data
  auto logData = run.getLogData()
  // access individual log
  auto protonCharge = run.getLogData("proton_charge")


Log data can be added and removed from the run object using the `addLogData` and `removeLogData` methods.

Multiperiod workspace sample logs
#################################

When you load multiperiod data using :ref:`algm-LoadISISNexus-v2` or :ref:`algm-LoadEventNexus` a set of
periods logs, describing the period information for each workspace are created using the `ISISRunLogs` class.
This class adds three logs, which are summarised as:

period n
~~~~~~~~~~~~

This log contains times and boolean flags describing when a period started, but not necessarily when data collection
for that period occurred. For example, a typical sample log entry for this log would read:

+----------------------+---------------------+--------------------+
| Time                 | Boolean flag value  | Description        |
+======================+=====================+====================+
| 2020-Oct-20 12:19:40 | 0                   | -                  |
+----------------------+---------------------+--------------------+
| 2020-Oct-20 12:19:41 | 1                   | nth period started |
+----------------------+---------------------+--------------------+
| 2020-Oct-20 12:19:48 | 0                   | nth period ended   |
+----------------------+---------------------+--------------------+

running
~~~~~~~~~~~

This log contains a series of times and boolean describing when a data collection was occurring, for example:

+----------------------+---------------------+--------------------------+
| Time                 | Boolean flag value  | Description              |
+======================+=====================+==========================+
| 2020-Oct-20 12:19:40 | 0                   | -                        |
+----------------------+---------------------+--------------------------+
| 2020-Oct-20 12:19:41 | 1                   | Data collection started  |
+----------------------+---------------------+--------------------------+
| 2020-Oct-20 12:19:46 | 0                   | Data collection ended    |
+----------------------+---------------------+--------------------------+

periods
~~~~~~~~

This log contains times and period numbers describing when each period started, e.g.

+----------------------+---------------------+--------------------------+
| Time                 | Period number       | Description              |
+======================+=====================+==========================+
| 2020-Oct-20 12:19:40 | 1                   | Period 1 started         |
+----------------------+---------------------+--------------------------+
| 2020-Oct-20 12:19:49 | 2                   | Period 2 started         |
+----------------------+---------------------+--------------------------+
| 2020-Oct-20 12:19:56 | 3                   | Period 3 started         |
+----------------------+---------------------+--------------------------+

These three logs are constructed from the ICP_EVENT which tells you when various “events” of significance to the
software happen, such as beginning or end of a run or a change of period number.


Filtering
~~~~~~~~~~

For multiperiod workspaces, time-series data, such as Theta will be filtered by combining the `period n` and `running`
logs, to create a filter describing when data collection for each period occurred. For example, a filter for the nth
period would be described as follows, where we note that each period is described entirely be two boolean values, the
first describing when the data collection started for that period and the second stating when data collection ended.

+----------------------+---------------------+-----------------------------------------+
| Time                 | Boolean flag        | Description                             |
+======================+=====================+=========================================+
| 2020-Oct-20 12:19:40 | 0                   | -                                       |
+----------------------+---------------------+-----------------------------------------+
| 2020-Oct-20 12:19:41 | 1                   | Data collection for period n started    |
+----------------------+---------------------+-----------------------------------------+
| 2020-Oct-20 12:19:46 | 0                   | Data collection for period n finished   |
+----------------------+---------------------+-----------------------------------------+

This filtering is performed by the static method `ISISRunLogs::applyLogFiltering`.

Event data
~~~~~~~~~~

For event data, an additional `period_log` is present in the workspace, which is created from the `framelog/period_log` entry.
This entry contains the value of various items on a frame by frame (pulse by pulse) basis.
So it is showing you what the neutron acquisition electronics believe the period number to be at the point that bit
of neutron data was recorded.

The `framelog/period_log` entry will therefore contain a list of `m` times and the corresponding period number
which was active during that frame:

+----------------------+---------------------+--------------------------+
| Time                 | Period number       | Description              |
+======================+=====================+==========================+
| 2020-Oct-20 12:19:40 | 1                   | Period 1 data            |
+----------------------+---------------------+--------------------------+
| 2020-Oct-20 12:19:41 | 1                   | Period 1 data            |
+----------------------+---------------------+--------------------------+
| 2020-Oct-20 12:19:44 | 1                   | Period 1 data            |
+----------------------+---------------------+--------------------------+
| 2020-Oct-20 12:19:50 | 2                   | Period 2 data            |
+----------------------+---------------------+--------------------------+
| 2020-Oct-20 12:19:51 | 2                   | Period 2 data            |
+----------------------+---------------------+--------------------------+
| 2020-Oct-20 12:19:52 | 3                   | Period 3 data            |
+----------------------+---------------------+--------------------------+
| 2020-Oct-20 12:20:01 | 3                   | Period 3 data            |
+----------------------+---------------------+--------------------------+
| 2020-Oct-20 12:20:02 | 3                   | Period 3 data            |
+----------------------+---------------------+--------------------------+

This log is therefore a combination of the `period n` and `running logs` defined above, whereby it records the times and
periods during data collection. However, rather than recording a single value describing when
the data collection for that period started, the `framelog` data contains a discrete number of time recordings
corresponding to the period that each "bit" of neutron data was collected in.
