.. algorithm::

.. summary::

.. alias::

.. properties::

Description
-----------

The :ref:`LoadNexusLogs <algm-LoadNexusLogs>`
algorithm loads the sample logs from the given `NeXus <http://www.nexusformat.org>`__
file. The logs are visible from MantidPlot if you right-click on a workspace and
select "Sample Logs...".

If you use :ref:`LoadEventNexus <algm-LoadEventNexus>`
or
:ref:`LoadISISNexus <algm-LoadISISNexus>`,
calling this algorithm is not necessary, since it called as a child algorithm.

Usage
-----

As described above, normal usage of this algorithm is not necessary. however, at SNS
there are preNeXus files available. The following uses this mechanism and then adds
the logs.

.. include:: ../usagedata-note.txt

.. testcode:: Ex

    ws = LoadEventPreNexus("CNCS_7860_neutron_event.dat")
    # Five logs are already present
    print "Number of original logs =", len(ws.getRun().keys())
    phase_log = "Phase1"
    # Try to get a log that doesn't exist yet
    try:
        log = ws.getRun().getLogData(phase_log)
    except RuntimeError:
        print phase_log, "log does not exist!"
    LoadNexusLogs(ws, "CNCS_7860_event.nxs")
    print "Number of final logs =", len(ws.getRun().keys())
    # Try getting the log again
    try:
        log = ws.getRun().getLogData(phase_log)
        print phase_log, "log size =", log.size()
    except RuntimeError:
        print phase_log, "log does not exist!"

Output:

.. testoutput:: Ex

    Number of original logs = 5
    Phase1 log does not exist!
    Number of final logs = 44
    Phase1 log size = 46

.. categories::
