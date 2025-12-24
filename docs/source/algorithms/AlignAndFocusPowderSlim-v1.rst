
.. algorithm::

.. warning::

    This algorithm is currently for the VULCAN instrument testing purposes


.. summary::

.. relatedalgorithms::

.. properties::

Description
-----------

This is a simplified version of :ref:`algm-AlignAndFocusPowderFromFiles` which uses very few child algorithms.
The main feature is that this reads the events, filters and adjusts their time-of-flight, then increments the correct bin in the output workspace.
As a result, there is a significantly smaller memory usage and the processing is significantly faster.

Current limitations compared to ``AlignAndFocusPowderFromFiles``

- only supports the VULCAN instrument
- hard coded for 6 particular groups
- does not support copping data
- does not support removing prompt pulse

Child algorithms used are

- :ref:`algm-LoadDiffCal`
- :ref:`algm-LoadIDFFromNexus-v1`
- :ref:`algm-EditInstrumentGeometry`
- :ref:`algm-LoadNexusLogs`
- :ref:`algm-ConvertUnits`

Usage
-----

**Example - event filtering**

This algorithm accepts the same ``SplitterWorkspace`` inputs as :ref:`FilterEvents <algm-FilterEvents>` and more information can be found on the :ref:`Event Filtering <EventFiltering>` page.

.. code-block:: python

    # create splitter table using relative time
    splitter = CreateEmptyTableWorkspace()
    splitter.addColumn('float', 'start')
    splitter.addColumn('float', 'stop')
    splitter.addColumn('str', 'target')

    splitter.addRow((10,20, '0'))
    splitter.addRow((200,210, '0'))
    splitter.addRow((400,410, '0'))

    # pass the splitter table to AlignAndFocusPowderSlim
    ws = AlignAndFocusPowderSlim("VULCAN_218062.nxs.h5",
                                 SplitterWorkspace=splitter, RelativeTime=True,
                                 XMin=0, XMax=50000, XDelta=50000,
                                 BinningMode="Linear",
                                 BinningUnits="TOF",
                                 L1=43.755,
                                 L2=[2.296, 2.296, 2.070, 2.070, 2.070, 2.530],
                                 Polar=[90, 90, 120, 150, 157, 65.5],
                                 Azimuthal=[180, 0, 0, 0, 0, 0])

    # This is equivalent to using FilterEvents with the same splitter table.
    # But note that this example doesn't align the data so put everything in 1 big bin to compare.
    ws2 = LoadEventNexus("VULCAN_218062.nxs.h5", NumberOfBins=1)
    grp = CreateGroupingWorkspace(ws2, GroupDetectorsBy='bank')
    ws2 = GroupDetectors(ws2, CopyGroupingFromWorkspace="grp")

    FilterEvents(ws2,
                 SplitterWorkspace=splitter, RelativeTime=True,
                 FilterByPulseTime=True,
                 OutputWorkspaceBaseName="filtered",
                 GroupWorkspaces=True)
    out = Rebin("filtered", "0,50000,50000", PreserveEvents=False)

    CompareWorkspaces(ws, out, CheckSpectraMap=False, CheckInstrument=False)

**Example - splitting events based on log values**

.. code-block:: python

    # Load only the log we need
    cave_temperature = LoadEventNexus("VULCAN_218062.nxs.h5",
                                      MetaDataOnly=True,
                                      AllowList="CaveTemperature")

    # Use GenerateEventsFilter to create a splitter table based on the log values
    GenerateEventsFilter(cave_temperature,
                         OutputWorkspace='splitter',
                         InformationWorkspace='info',
                         LogName='CaveTemperature',
                         MinimumLogValue=70.10,
                         MaximumLogValue=70.15,
                         LogValueInterval=0.01)

    # Use the splitter table to filter the events during loading and output to workspace group
    ws = AlignAndFocusPowderSlim("VULCAN_218062.nxs.h5", SplitterWorkspace='splitter',
                                 L1=43.755,
                                 L2=[2.296, 2.296, 2.070, 2.070, 2.070, 2.530],
                                 Polar=[90, 90, 120, 150, 157, 65.5],
                                 Azimuthal=[180, 0, 0, 0, 0, 0])
    print(ws.getNumberOfEntries())  # should be 6 workspaces in the group

    # By default the events are split based on pulsetime of the events. If you want to split based on full time (pulsetime + tof), set UseFullTime=False.
    ws2 = AlignAndFocusPowderSlim("VULCAN_218062.nxs.h5",
                                  SplitterWorkspace='splitter',
                                  UseFullTime=True,
                                  L1=43.755,
                                  L2=[2.296, 2.296, 2.070, 2.070, 2.070, 2.530],
                                  Polar=[90, 90, 120, 150, 157, 65.5],
                                  Azimuthal=[180, 0, 0, 0, 0, 0])


**Example - filter bad pulses**

.. code-block:: python

    ws = AlignAndFocusPowderSlim("VULCAN_218062.nxs.h5",
                                 XMin=0, XMax=50000, XDelta=50000,
                                 BinningMode="Linear",
                                 BinningUnits="TOF",
                                 FilterBadPulses=True,
                                 L1=43.755,
                                 L2=[2.296, 2.296, 2.070, 2.070, 2.070, 2.530],
                                 Polar=[90, 90, 120, 150, 157, 65.5],
                                 Azimuthal=[180, 0, 0, 0, 0, 0])

    # This is equivalent to using FilterBadPulses.
    # But note that this example doesn't align the data so put everything in 1 big bin to compare.
    ws2 = LoadEventNexus("VULCAN_218062.nxs.h5")
    grp = CreateGroupingWorkspace(ws2, GroupDetectorsBy='bank')
    ws2 = GroupDetectors(ws2, CopyGroupingFromWorkspace="grp")
    ws2 = FilterBadPulses(ws2)
    ws2 = Rebin(ws2, "0,50000,50000", PreserveEvents=False)

    CompareWorkspaces(ws, ws2, CheckSpectraMap=False, CheckInstrument=False)

.. categories::

.. sourcelink::
