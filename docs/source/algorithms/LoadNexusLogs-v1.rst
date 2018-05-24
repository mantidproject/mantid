.. algorithm::

.. summary::

.. relatedalgorithms::

.. properties::

Description
-----------

The **LoadNexusLogs**
algorithm loads the sample logs from the given `NeXus <http://www.nexusformat.org>`__
file. The logs are visible from MantidPlot if you right-click on a workspace and
select "Sample Logs...".

If you use :ref:`LoadEventNexus <algm-LoadEventNexus>`
or
:ref:`LoadISISNexus <algm-LoadISISNexus>`,
calling this algorithm is not necessary, since it called as a child algorithm.

Data loaded from Nexus File
###########################

Not all of the nexus file is loaded. This section tells you what is loaded and where it goes in the workspace. 
Items missing from the Nexus file are simply not loaded.


+----------------------------------+----------------------------------------------------+---------------------------------------+
| Description of Data              | Found in Nexus file                                | Placed in Workspace (Workspace2D)     |
+==================================+====================================================+=======================================+
| Log group                        | Each group of class ``IXrunlog`` or ``IXselog``    | See below                             |
|                                  | or with name ``"DASLogs"`` or ``"framelog"``       |                                       |
|                                  | (henceforth referred as [LOG])                     |                                       |
+----------------------------------+----------------------------------------------------+---------------------------------------+
| Periods group                    | Each group of class ``IXperiods``                  | See below                             |
|                                  | (henceforth referred as [PERIODS])                 |                                       |
+----------------------------------+----------------------------------------------------+---------------------------------------+
| Time series                      | Each group of class ``NXlog`` or ``NXpositioner``  | Time series in workspace run object   |
|                                  | within [LOG]                                       | *Only these are affected by the       |
|                                  |                                                    | OverwriteLogs algorithm property.*    |
+----------------------------------+----------------------------------------------------+---------------------------------------+
| SE logs                          | Each group of class ``IXseblock``                  | Workspace run object. Item prefixed   |
|                                  | within [LOG]                                       | with ``selog_``,                      |
|                                  |                                                    | if name is already in use.            |
+----------------------------------+----------------------------------------------------+---------------------------------------+
| Periods                          | Each group of class ``IXperiods`` and  name        | ``nperiods`` item in run object,      |
|                                  | ``"periods"`` within [PERIODS]                     | if it does not already exist          |
+----------------------------------+----------------------------------------------------+---------------------------------------+
| Start and end times              | Groups ``"start_time"`` and ``"end time"``         | start and end times in run object     |
|                                  |                                                    | *Existing values are always           |
|                                  |                                                    | overwritten.*                         |
+----------------------------------+----------------------------------------------------+---------------------------------------+
| Proton charge                    | Group ``"proton_charge"``, if it exists,           | Proton charge in run object           |
|                                  | else integration of proton charge time series      | if it does not already exist          |
+----------------------------------+----------------------------------------------------+---------------------------------------+
| Measurement information          | Group ``"measurement"`` of class ``NXcollection``  | Run object items with name from       |
|                                  |                                                    | Nexus group prefixed with             |
|                                  |                                                    | ``measurement_``                      |
|                                  |                                                    | *Existing values are always           |
|                                  |                                                    | overwritten.*                         |
+----------------------------------+----------------------------------------------------+---------------------------------------+
| Run title                        | Entry ``"title"``                                  | Title in run object if it exists      |
+----------------------------------+----------------------------------------------------+---------------------------------------+

If the nexus file has a ``"proton_log"`` group, then this algorithm will do some event filtering to allow SANS2D files to load.

Usage
-----

As described above, normal usage of this algorithm is not necessary. however, at SNS
there are preNeXus files available. The following uses this mechanism and then adds
the logs.

.. include:: ../usagedata-note.txt

.. testcode:: Ex

    ws = LoadEventPreNexus("CNCS_7860_neutron_event.dat")
    # Five logs are already present
    print("Number of original logs = {}".format(len(ws.getRun().keys())))
    phase_log = "Phase1"
    # Try to get a log that doesn't exist yet
    try:
        log = ws.getRun().getLogData(phase_log)
    except RuntimeError:
        print("{} log does not exist!".format(phase_log))
    LoadNexusLogs(ws, "CNCS_7860_event.nxs")
    print("Number of final logs = {}".format(len(ws.getRun().keys())))
    # Try getting the log again
    try:
        log = ws.getRun().getLogData(phase_log)
        print("{} log size = {}".format(phase_log, log.size()))
    except RuntimeError:
        print("{} log does not exist!".format(phase_log))

Output:

.. testoutput:: Ex

    Number of original logs = 5
    Phase1 log does not exist!
    Number of final logs = 44
    Phase1 log size = 46

.. categories::

.. sourcelink::
