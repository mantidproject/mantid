.. algorithm::

.. summary::

.. relatedalgorithms::

.. properties::

Description
-----------

This algorithm filters events from a single :ref:`EventWorkspace` to
one or multiple target event workspaces according to
the ``SplitterWorkspace`` property. The :ref:`EventFiltering` concept
page has a detailed introduction to event filtering.

Specifying the splitting strategy
#################################

The ``SplitterWorkspace`` provides information for
splitting the ``InputWorkspace`` into target output
workspaces. It can have one of three types:

+--------------------------------------------------------------+-------------+----------+
| workspace class                                              | units       | rel/abs  |
+==============================================================+=============+==========+
| :ref:`MatrixWorkspace <MatrixWorkspace>`                     | seconds     | either   |
+--------------------------------------------------------------+-------------+----------+
| :class:`SplittersWorkspace <mantid.api.ISplittersWorkspace>` | nanoseconds | absolute |
+--------------------------------------------------------------+-------------+----------+
| :ref:`TableWorkspace <Table Workspaces>`                     | seconds     | either   |
+--------------------------------------------------------------+-------------+----------+

Whether the times in ``SplitterWorkspace`` are treated as relative or
absolute is dependent on the value of ``RelativeTime``. In the
case of ``RelativeTime=True``, the times are relative to the start of
the run (in the ``ws.run()['run_start']``) or, if specified, the
``FilterStartTime``. In the case of ``RelativeTime=False``, the times
are relative to the :class:`GPS epoch <mantid.kernel.DateAndTime>`.

Both :ref:`TableWorkspace <Table Workspaces>` and
:class:`SplittersWorkspace <mantid.api.ISplittersWorkspace>` have 3
columns, ``start``, ``stop``, and ``target``. :ref:`MatrixWorkspace
<MatrixWorkspace>` has a single spectrum where X-values
represent the time boundaries and Y-values represent the target workspace indexes.
The :ref:`filter-events-usage-label` examples and :ref:`Event Filtering <EventFiltering>` concept
page have details on creating the ``SplitterWorkspace`` by hand.
Note that event filtering treats all time intervals as [inclusive,exclusive).

The optional ``InformationWorkspace`` is a :ref:`TableWorkspace <Table
Workspaces>` for information on splitters.

Unfiltered Events
#################

Some events do not fall into any time-splitting intervals. They are put into a workspace
with the name ending with ``_unfiltered``. If ``OutputUnfilteredEvents=False``,
this workspace will not be created.

Correcting time neutron was at the sample
#########################################

When filtering fast logs, the time to filter by is the time that the
neutron was at the sample. This can be specified using the
``CorrectionToSample`` parameter. Either the user specifies the
correction parameter for every pixel, or one is calculated. The
correction parameters are applied as

.. math::

   TOF_{sample} = TOF_{detector} * scale[detectorID] + shift[detectorID]

and stored in the ``OutputTOFCorrectionWorkspace``.

* ``CorrectionToSample="None"`` applies no correction
* ``CorrectionToSample="Elastic"`` applies :math:`shift = 0` with
  :math:`scale = L1/(L1+L2)` for detectors and :math:`scale = L1/L_{monitor}`
  for monitors
* ``CorrectionToSample="Direct"`` applies :math:`scale = 0` and
  :math:`shift = L1 / \sqrt{2 E_{fix} / m_n}`.  The value supplied in
  ``IncidentEnergy`` will override the value found in the workspace's
  value of ``Ei``.
* ``CorrectionToSample="Indirect"`` applies :math:`scale = 1` and
  :math:`shift = -1 * L2 / \sqrt{2 E_{fix} / m_n}` for detectors. For
  monitors, uses the same corrections as ``Elastic``.

* ``CorrectionToSample="Customized"`` applies the correction supplied
  in the ``DetectorTOFCorrectionWorkspace``.


Filter by pulse time vs. full time
##################################

In the case of ``FilterByPulseTime=True``, events will be filtered by pulse time.
This is recommended for slow sample environment logs. The algorithm
will run faster, but with lower precision. In the case of ``FilterByPulseTime=False``,
events will be filtered by full time, i.e. pulse time plus TOF.

.. _filter-events-usage-label:

Usage
-----

**Example - Filtering events without correction on TOF**

.. testcode:: FilterEventsNoCorrection

    ws = Load(Filename='CNCS_7860_event.nxs')
    splitws, infows = GenerateEventsFilter(InputWorkspace=ws, UnitOfTime='Nanoseconds', LogName='SampleTemp',
            MinimumLogValue=279.9,  MaximumLogValue=279.98, LogValueInterval=0.01)

    FilterEvents(InputWorkspace=ws, SplitterWorkspace=splitws, InformationWorkspace=infows,
            OutputWorkspaceBaseName='tempsplitws',  GroupWorkspaces=True,
            FilterByPulseTime = False, OutputWorkspaceIndexedFrom1 = False,
            CorrectionToSample = "None", SpectrumWithoutDetector = "Skip",
            OutputTOFCorrectionWorkspace='mock', OutputUnfilteredEvents = True)

    # Print result
    wsgroup = mtd["tempsplitws"]
    wsnames = wsgroup.getNames()
    for name in sorted(wsnames):
        tmpws = mtd[name]
        print("workspace %s has %d events" % (name, tmpws.getNumberEvents()))

Output:

.. testoutput:: FilterEventsNoCorrection

    workspace tempsplitws_0 has 124 events
    workspace tempsplitws_1 has 16915 events
    workspace tempsplitws_2 has 10009 events
    workspace tempsplitws_3 has 6962 events
    workspace tempsplitws_4 has 22520 events
    workspace tempsplitws_5 has 5133 events
    workspace tempsplitws_unfiltered has 50603 events

**Example - Filtering events by a user-generated TableWorkspace**

.. testcode:: FilterEventsNoCorrection

    import numpy as np
    ws = Load(Filename='CNCS_7860_event.nxs')

    # create TableWorkspace
    split_table_ws = CreateEmptyTableWorkspace()
    split_table_ws.addColumn('float', 'start')
    split_table_ws.addColumn('float', 'stop')
    split_table_ws.addColumn('str', 'target')

    split_table_ws.addRow([0., 100., 'a'])
    split_table_ws.addRow([200., 300., 'b'])
    split_table_ws.addRow([400., 600., 'c'])
    split_table_ws.addRow([600., 650., 'b'])

    # filter events
    FilterEvents(InputWorkspace=ws, SplitterWorkspace=split_table_ws,
            OutputWorkspaceBaseName='tempsplitws3',  GroupWorkspaces=True,
            FilterByPulseTime = False, OutputWorkspaceIndexedFrom1 = False,
            CorrectionToSample = "None", SpectrumWithoutDetector = "Skip",
            OutputTOFCorrectionWorkspace='mock',
            RelativeTime=True, OutputUnfilteredEvents = True)

    # print result
    wsgroup = mtd["tempsplitws3"]
    wsnames = wsgroup.getNames()
    for name in sorted(wsnames):
        tmpws = mtd[name]
        print("workspace %s has %d events" % (name, tmpws.getNumberEvents()))
        time_roi = tmpws.run().getTimeROI()
        splitters = time_roi.toTimeIntervals()
        for index, splitter in enumerate(splitters, 1):
            times = np.array(splitter, dtype=np.int64) * np.timedelta64(1, 'ns') + np.datetime64('1990-01-01T00:00')
            print("event splitter " + str(index) + ": from " + np.datetime_as_string(times[0], timezone='UTC') + " to " + np.datetime_as_string(times[1], timezone='UTC'))

Output:

.. testoutput:: FilterEventsNoCorrection

    workspace tempsplitws3_a has 77580 events
    event splitter 1: from 2010-03-25T16:08:37.000000000Z to 2010-03-25T16:10:17.000000000Z
    workspace tempsplitws3_b has 0 events
    event splitter 1: from 2010-03-25T16:11:57.000000000Z to 2010-03-25T16:13:37.000000000Z
    event splitter 2: from 2010-03-25T16:18:37.000000000Z to 2010-03-25T16:19:27.000000000Z
    workspace tempsplitws3_c has 0 events
    event splitter 1: from 2010-03-25T16:15:17.000000000Z to 2010-03-25T16:18:37.000000000Z
    workspace tempsplitws3_unfiltered has 34686 events
    event splitter 1: from 2010-03-25T16:10:17.000000000Z to 2010-03-25T16:11:57.000000000Z
    event splitter 2: from 2010-03-25T16:13:37.000000000Z to 2010-03-25T16:15:17.000000000Z

**Example - Filtering events by a user-generated MatrixWorkspace**

.. testcode:: FilterEventsNoCorrection

    import numpy as np
    ws = Load(Filename='CNCS_7860_event.nxs')

    # create MatrixWorkspace. Use -1 as a target for unfiltered events.
    times = [0, 100, 200, 300, 400, 600, 650]
    targets = [0, -1, 1, -1, 2, 1]
    split_matrix_ws = CreateWorkspace(DataX=times, DataY=targets, NSpec=1)

    # filter events
    FilterEvents(InputWorkspace=ws, SplitterWorkspace=split_matrix_ws,
            OutputWorkspaceBaseName='tempsplitws4',  GroupWorkspaces=True,
            FilterByPulseTime = False, OutputWorkspaceIndexedFrom1 = False,
            CorrectionToSample = "None", SpectrumWithoutDetector = "Skip",
            OutputTOFCorrectionWorkspace='mock',
            RelativeTime=True, OutputUnfilteredEvents = True)

    # print result
    wsgroup = mtd["tempsplitws4"]
    wsnames = wsgroup.getNames()
    for name in sorted(wsnames):
        tmpws = mtd[name]
        print("workspace %s has %d events" % (name, tmpws.getNumberEvents()))
        time_roi = tmpws.run().getTimeROI()
        splitters = time_roi.toTimeIntervals()
        for index, splitter in enumerate(splitters, 1):
            times = np.array(splitter, dtype=np.int64) * np.timedelta64(1, 'ns') + np.datetime64('1990-01-01T00:00')
            print("event splitter " + str(index) + ": from " + np.datetime_as_string(times[0], timezone='UTC') + " to " + np.datetime_as_string(times[1], timezone='UTC'))

Output:

.. testoutput:: FilterEventsNoCorrection

    workspace tempsplitws4_0 has 77580 events
    event splitter 1: from 2010-03-25T16:08:37.000000000Z to 2010-03-25T16:10:17.000000000Z
    workspace tempsplitws4_1 has 0 events
    event splitter 1: from 2010-03-25T16:11:57.000000000Z to 2010-03-25T16:13:37.000000000Z
    event splitter 2: from 2010-03-25T16:18:37.000000000Z to 2010-03-25T16:19:27.000000000Z
    workspace tempsplitws4_2 has 0 events
    event splitter 1: from 2010-03-25T16:15:17.000000000Z to 2010-03-25T16:18:37.000000000Z
    workspace tempsplitws4_unfiltered has 34686 events
    event splitter 1: from 2010-03-25T16:10:17.000000000Z to 2010-03-25T16:11:57.000000000Z
    event splitter 2: from 2010-03-25T16:13:37.000000000Z to 2010-03-25T16:15:17.000000000Z

**Example - Filtering events by pulse time**

.. testcode:: FilterEventsByPulseTime

    ws = Load(Filename='CNCS_7860_event.nxs')
    splitws, infows = GenerateEventsFilter(InputWorkspace=ws, UnitOfTime='Nanoseconds', LogName='SampleTemp',
            MinimumLogValue=279.9,  MaximumLogValue=279.98, LogValueInterval=0.01)

    FilterEvents(InputWorkspace=ws,
        SplitterWorkspace=splitws,
        InformationWorkspace=infows,
        OutputWorkspaceBaseName='tempsplitws',
        GroupWorkspaces=True,
        FilterByPulseTime = True,
        OutputWorkspaceIndexedFrom1 = True,
        CorrectionToSample = "None",
        SpectrumWithoutDetector = "Skip",
        OutputTOFCorrectionWorkspace='mock')

    # Print result
    wsgroup = mtd["tempsplitws"]
    wsnames = wsgroup.getNames()
    for name in sorted(wsnames):
        tmpws = mtd[name]
        print("workspace %s has %d events" % (name, tmpws.getNumberEvents()))

Output:

.. testoutput:: FilterEventsByPulseTime

    workspace tempsplitws_1 has 123 events
    workspace tempsplitws_2 has 16951 events
    workspace tempsplitws_3 has 9972 events
    workspace tempsplitws_4 has 7019 events
    workspace tempsplitws_5 has 22529 events
    workspace tempsplitws_6 has 5067 events


**Example - Filtering events with correction on TOF**

.. testcode:: FilterEventsTOFCorrection

    ws = Load(Filename='CNCS_7860_event.nxs')
    splitws, infows = GenerateEventsFilter(InputWorkspace=ws, UnitOfTime='Nanoseconds', LogName='SampleTemp',
            MinimumLogValue=279.9,  MaximumLogValue=279.98, LogValueInterval=0.01)

    FilterEvents(InputWorkspace=ws, SplitterWorkspace=splitws, InformationWorkspace=infows,
        OutputWorkspaceBaseName='tempsplitws',
        GroupWorkspaces=True,
        FilterByPulseTime = False,
        OutputWorkspaceIndexedFrom1 = False,
        CorrectionToSample = "Direct",
        IncidentEnergy=3,
        SpectrumWithoutDetector = "Skip",
        OutputTOFCorrectionWorkspace='mock',
        OutputUnfilteredEvents = True)

    # Print result
    wsgroup = mtd["tempsplitws"]
    wsnames = wsgroup.getNames()
    for name in sorted(wsnames):
        tmpws = mtd[name]
        print("workspace %s has %d events" % (name, tmpws.getNumberEvents()))

Output:

.. testoutput:: FilterEventsTOFCorrection

    workspace tempsplitws_0 has 123 events
    workspace tempsplitws_1 has 16951 events
    workspace tempsplitws_2 has 9972 events
    workspace tempsplitws_3 has 7019 events
    workspace tempsplitws_4 has 22514 events
    workspace tempsplitws_5 has 5082 events
    workspace tempsplitws_unfiltered has 50605 events

.. categories::

.. sourcelink::
    :h: Framework/Algorithms/inc/MantidAlgorithms/FilterEvents.h
    :cpp: Framework/Algorithms/src/FilterEvents.cpp
    :py: None
