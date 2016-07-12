.. algorithm::

.. summary::

.. alias::

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
workspace containing splitters.  It can be either a MatrixWorkspace or
a :ref:`SplittersWorkspace <SplittersWorkspace>`.

The optional workspace is a :ref:`TableWorkspace <Table Workspaces>`
for information of splitters.

Algorithm :ref:`GenerateEventsFilter <algm-GenerateEventsFilter>`
creates both the :ref:`SplittersWorkspace <SplittersWorkspace>` and
splitter information workspace.


Splitters in relative time
==========================

As the SplittersWorkspace is in format of :ref:`MatrixWorkspace
<MatrixWorkspace>`, its time, i.e., the value in X vector, can be
relative time.

Property *RelativeTime* flags that the splitters' time is relative.
Property *FilterStartTime* specifies the starting time of the filter.
Or the shift of time of the splitters.
If it is not specified, then the algorithm will search for sample log *run_start*.

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

Difference from FilterByLogValue
################################

In FilterByLogValue(), EventList.splitByTime() is used.

In FilterEvents(), if FilterByPulse is selected true,
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
        print "workspace %s has %d events" % (name, tmpws.getNumberEvents())


Output:

.. testoutput:: FilterEventNoCorrection

    workspace tempsplitws_0 has 124 events
    workspace tempsplitws_1 has 16915 events
    workspace tempsplitws_2 has 10009 events
    workspace tempsplitws_3 has 6962 events
    workspace tempsplitws_4 has 22520 events
    workspace tempsplitws_5 has 5133 events
    workspace tempsplitws_unfiltered has 50603 events


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
        print "workspace %s has %d events" % (name, tmpws.getNumberEvents())


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
        print "workspace %s has %d events" % (name, tmpws.getNumberEvents())


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
