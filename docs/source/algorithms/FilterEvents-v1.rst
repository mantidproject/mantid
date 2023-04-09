.. algorithm::

.. summary::

.. relatedalgorithms::

.. properties::

Description
-----------

This algorithm filters events from a single :ref:`EventWorkspace` to
one or multiple :ref:`EventWorkspaces <EventWorkspace>` according to
the ``SplittersWorkspace`` property. The :ref:`EventFiltering` concept
page has a detailed introduction to event filtering.

Specifying the splitting strategy
---------------------------------

The ``SplittersWorkspace`` describes much of the information for
splitting the ``InputWorkspace`` into the various output
workspaces. It can have one of three types

+--------------------------------------------------------------+-------------+----------+
| workspace class                                              | units       | rel/abs  |
+==============================================================+=============+==========+
| :ref:`MatrixWorkspace <MatrixWorkspace>`                     | seconds     | either   |
+--------------------------------------------------------------+-------------+----------+
| :class:`SplittersWorkspace <mantid.api.ISplittersWorkspace>` | nanoseconds | absolute |
+--------------------------------------------------------------+-------------+----------+
| :ref:`TableWorkspace <Table Workspaces>`                     | seconds     | either   |
+--------------------------------------------------------------+-------------+----------+

Whether the values in :ref:`MatrixWorkspace <MatrixWorkspace>` and
:ref:`TableWorkspace <Table Workspaces>` is treated as relative or
absolute time is dependent on the value of ``RelativeTime``. In the
case of ``RelativeTime=True``, the time is relative to the start of
the run (in the ``ws.run()['run_start']``) or, if specified, the
``FilterStartTime``. In the case of ``RelativeTime=False``, the times
are relative to the :class:`GPS epoch <mantid.kernel.DateAndTime>`.

Both :ref:`TableWorkspace <Table Workspaces>` and
:class:`SplittersWorkspace <mantid.api.ISplittersWorkspace>` have 3
columns, ``start``, ``stop``, and ``target`` which should be a float,
float, and string. The :ref:`event filtering <EventFiltering>` concept
page has details on creating the :ref:`TableWorkspace <Table
Workspaces>` by hand.

If the ``SplittersWorkspace`` is a :ref:`MatrixWorkspace
<MatrixWorkspace>`, it must have a single spectrum with the x-value is
the time boundaries and the y-value is the workspace group index.

The optional ``InformationWorkspace`` is a :ref:`TableWorkspace <Table
Workspaces>` for information of splitters.

Unfiltered Events
-----------------

Some events are not inside any splitters. They are put to a workspace
name ended with ``_unfiltered``. If
``OutputUnfilteredEvents=False``, then this workspace will not be
created.

Using FilterEvents with fast-changing logs
------------------------------------------

There are a few parameters to consider when the log filtering is
expected to produce a large splitter table. An example of such a case
would be a data file for which the events need to be split according
to a log with two or more states changing in the kHz range. To reduce
the filtering time, one may do the following:

- Make sure the ``SplitterWorkspace`` input is a :ref:`MatrixWorkspace
  <MatrixWorkspace>`. Such a workspace can be produced by using the
  ``FastLog = True`` option when calling :ref:`GenerateEventsFilter
  <algm-GenerateEventsFilter>`.

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


Difference from FilterByLogValue
--------------------------------

In :ref:`FilterByLogValue <algm-FilterByLogValue>`,
``EventList.splitByTime()`` is used. In FilterEvents, it only uses
this when ``FilterByPulse=True``. Otherwise,
``EventList.splitByFullTime()`` is used. The difference between
``splitByTime`` and ``splitByFullTime`` is that ``splitByTime``
filters events by pulse time, and ``splitByFullTime`` considers both
pulse time and TOF.

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
        splitters = time_roi.toSplitters()
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

    # create MatrixWorkspace
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
        splitters = time_roi.toSplitters()
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
