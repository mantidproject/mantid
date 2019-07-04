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
``OutputWorkspaceIndexedFrom1=True``, then this workspace will not be
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
- Choose the logs to split. Filtering the logs can take a substantial
  amount of time. To save time, you may want to split only the logs
  you will need for analysis. To do so, set ``ExcludeSpecifiedLogs =
  False`` and list the logs you need in
  ``TimeSeriesPropertyLogs``. For example, if we only need to know the
  accumulated proton charge for each filtered workspace, we would set
  ``TimeSeriesPropertyLogs = proton_charge``.

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

**Example - Filtering event without correction on TOF**

.. testcode:: FilterEventNoCorrection

    ws = Load(Filename='CNCS_7860_event.nxs')
    splitws, infows = GenerateEventsFilter(InputWorkspace=ws, UnitOfTime='Nanoseconds', LogName='SampleTemp',
            MinimumLogValue=279.9,  MaximumLogValue=279.98, LogValueInterval=0.01)

    FilterEvents(InputWorkspace=ws, SplitterWorkspace=splitws, InformationWorkspace=infows,
            OutputWorkspaceBaseName='tempsplitws',  GroupWorkspaces=True,
            FilterByPulseTime = False, OutputWorkspaceIndexedFrom1 = False,
            CorrectionToSample = "None", SpectrumWithoutDetector = "Skip", SplitSampleLogs = False,
            OutputTOFCorrectionWorkspace='mock')

    # Print result
    wsgroup = mtd["tempsplitws"]
    wsnames = wsgroup.getNames()
    for name in sorted(wsnames):
        tmpws = mtd[name]
        print("workspace %s has %d events" % (name, tmpws.getNumberEvents()))


Output:

.. testoutput:: FilterEventNoCorrection

    workspace tempsplitws_0 has 124 events
    workspace tempsplitws_1 has 16915 events
    workspace tempsplitws_2 has 10009 events
    workspace tempsplitws_3 has 6962 events
    workspace tempsplitws_4 has 22520 events
    workspace tempsplitws_5 has 5133 events
    workspace tempsplitws_unfiltered has 50603 events

**Example - Filtering event by a user-generated TableWorkspace**

.. testcode:: FilterEventNoCorrection

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
            CorrectionToSample = "None", SpectrumWithoutDetector = "Skip", SplitSampleLogs = False,
            OutputTOFCorrectionWorkspace='mock',
            RelativeTime=True)

    # Print result
    wsgroup = mtd["tempsplitws3"]
    wsnames = wsgroup.getNames()
    for name in sorted(wsnames):
        tmpws = mtd[name]
        print("workspace %s has %d events" % (name, tmpws.getNumberEvents()))
        split_log = tmpws.run().getProperty('splitter')
        entry_0 = np.datetime_as_string(split_log.times[0].astype(np.dtype('M8[s]')), timezone='UTC')
        entry_1 = np.datetime_as_string(split_log.times[1].astype(np.dtype('M8[s]')), timezone='UTC')
        print('event splitter log: entry 0 and entry 1 are {0} and {1}.'.format(entry_0, entry_1))


Output:

.. testoutput:: FilterEventNoCorrection

    workspace tempsplitws3_a has 77580 events
    event splitter log: entry 0 and entry 1 are 2010-03-25T16:08:37Z and 2010-03-25T16:10:17Z.
    workspace tempsplitws3_b has 0 events
    event splitter log: entry 0 and entry 1 are 2010-03-25T16:08:37Z and 2010-03-25T16:11:57Z.
    workspace tempsplitws3_c has 0 events
    event splitter log: entry 0 and entry 1 are 2010-03-25T16:08:37Z and 2010-03-25T16:15:17Z.
    workspace tempsplitws3_unfiltered has 34686 events
    event splitter log: entry 0 and entry 1 are 2010-03-25T16:08:37Z and 2010-03-25T16:10:17Z.


**Example - Filtering event by pulse time**

.. testcode:: FilterEventByPulseTime

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
        SplitSampleLogs = False,
        OutputTOFCorrectionWorkspace='mock')

    # Print result
    wsgroup = mtd["tempsplitws"]
    wsnames = wsgroup.getNames()
    for name in sorted(wsnames):
        tmpws = mtd[name]
        print("workspace %s has %d events" % (name, tmpws.getNumberEvents()))


Output:

.. testoutput:: FilterEventByPulseTime

    workspace tempsplitws_1 has 123 events
    workspace tempsplitws_2 has 16951 events
    workspace tempsplitws_3 has 9972 events
    workspace tempsplitws_4 has 7019 events
    workspace tempsplitws_5 has 22529 events
    workspace tempsplitws_6 has 5067 events


**Example - Filtering event with correction on TOF**

.. testcode:: FilterEventTOFCorrection

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
        SplitSampleLogs = False,
        OutputTOFCorrectionWorkspace='mock')

    # Print result
    wsgroup = mtd["tempsplitws"]
    wsnames = wsgroup.getNames()
    for name in sorted(wsnames):
        tmpws = mtd[name]
        print("workspace %s has %d events" % (name, tmpws.getNumberEvents()))


Output:

.. testoutput:: FilterEventTOFCorrection

    workspace tempsplitws_0 has 123 events
    workspace tempsplitws_1 has 16951 events
    workspace tempsplitws_2 has 9972 events
    workspace tempsplitws_3 has 7019 events
    workspace tempsplitws_4 has 22514 events
    workspace tempsplitws_5 has 5082 events
    workspace tempsplitws_unfiltered has 50605 events

.. categories::

.. sourcelink::
