.. algorithm::

.. summary::

.. relatedalgorithms::

.. properties::

Description
-----------

This algorithm filters events from an :ref:`EventWorkspace` to one or
multiple :ref:`EventWorkspaces <EventWorkspace>` according to an input
:ref:`SplittersWorkspace` containing a series of splitters (i.e.,
:ref:`splitting intervals <SplittingInterval>`).

Inputs
######

FilterEvents takes 2 mandatory input Workspaces and 1 optional
Workspace.  One of mandatory workspace is the :ref:`EventWorkspace`
where the events are filtered from.  The other mandatory workspace is
workspace containing splitters.  It can be a MatrixWorkspace, a TableWorkspace or
a :ref:`SplittersWorkspace <SplittersWorkspace>`.

The optional workspace is a :ref:`TableWorkspace <Table Workspaces>`
for information of splitters.

Workspace containing splitters
==============================

This algorithm accepts three types of workspace that contains event splitters.
- TableWorkspace: a general TableWorkspace with at three columns
- MatrixWorkspace: a 1-spectrum MatrixWorkspace
- SplittersWorkspace: an extended TableWorkspace with restrict definition on start and stop time.

Event splitter
++++++++++++++

An event splitter contains three items, start time, stop time and splitting target (index).
All the events belonged to the same splitting target will be saved to a same output EventWorkspace.

Unit of input splitters
+++++++++++++++++++++++

- MatrixWorkspace:  the unit must be second.
- TableWorkspace: the unit must be second.
- SplittersWorkspace: by the definition of SplittersWorkspace, the unit has to be nanosecond.


How to generate input workspace containing splitters
++++++++++++++++++++++++++++++++++++++++++++++++++++

There are two ways to generate

Algorithm :ref:`GenerateEventsFilter <algm-GenerateEventsFilter>`
creates both the :ref:`SplittersWorkspace <SplittersWorkspace>` and
splitter information workspace.


Splitters in relative time or absolute time
+++++++++++++++++++++++++++++++++++++++++++

As the SplittersWorkspace is in format of :ref:`MatrixWorkspace
<MatrixWorkspace>`, its time, i.e., the value in X vector, can be
relative time.

Property ``RelativeTime`` flags that the splitters' time is relative.
Property ``FilterStartTime`` specifies the starting time of the filter.
Or the shift of time of the splitters.
If it is not specified, then the algorithm will search for sample log ``run_start``.

Outputs
#######

The output will be one or multiple workspaces according to the number of
index in splitters. The output workspace name is the combination of
parameter OutputWorkspaceBaseName and the index in splitter.

Calibration File
################

The calibration, or say correction, from the detector to sample must be
consider in fast log. Thus a calibration file is required. The math is

``TOF_calibrated = TOF_raw * correction(detector ID).``

The calibration is in column data format.

A reasonable approximation of the correction is

``correction(detector_ID) = L1/(L1+L2(detector_ID))``

Unfiltered Events
#################

Some events are not inside any splitters. They are put to a workspace
name ended with '\_unfiltered'.

If input property 'OutputWorkspaceIndexedFrom1' is set to True, then
this workspace shall not be outputed.

Using FilterEvents with fast-changing logs
##########################################

There are a few parameters to consider when the log filtering is expected to produce a large
splitter table. An example of such a case would be a data file for which the events need to be split
according to a log with two or more states changing in the kHz range. To reduce the filtering time,
one may do the following:

- Make sure the ``SplitterWorkspace`` input is a :ref:`MatrixWorkspace <MatrixWorkspace>`. Such a workspace can be produced by using the ``FastLog = True`` option when calling :ref:`GenerateEventsFilter <algm-GenerateEventsFilter>`.
- Choose the logs to split. Filtering the logs can take a substantial amount of time. To save time, you may want to split only the logs you will need for analysis. To do so, set ``ExcludeSpecifiedLogs = False`` and list the logs you need in ``TimeSeriesPropertyLogs``. For example, if we only need to know the accumulated proton charge for each filtered workspace, we would set ``TimeSeriesPropertyLogs = proton_charge``.

Difference from FilterByLogValue
################################

In FilterByLogValue(), EventList.splitByTime() is used.

In FilterEvents, if FilterByPulse is selected true,
EventList.SplitByTime is called; otherwise, EventList.SplitByFullTime()
is called instead.

The difference between splitByTime and splitByFullTime is that
splitByTime filters events by pulse time, and splitByFullTime considers
both pulse time and TOF.

Therefore, FilterByLogValue is not suitable for fast log filtering.

Comparing with other event filtering algorithms
###############################################

Wiki page :ref:`EventFiltering` has a detailed introduction on event
filtering in MantidPlot.


Developer's Note
----------------

Splitters given by TableWorkspace
#################################

- The ``start/stop time`` is converted to ``m_vecSplitterTime``.
- The splitting target (in string) is mapped to a set of continuous integers that are stored in ``m_vecSplitterGroup``.
  - The mapping will be recorded in ``m_targetIndexMap`` and ``m_wsGroupIndexTargetMap``.
  - Class variable ``m_maxTargetIndex`` is set up to record the highest target group/index,i.e., the max value of ``m_vecSplitterGroup``.


Undefined splitting target
##########################

Indexed as ``0`` in m_vecSplitterGroup.


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
