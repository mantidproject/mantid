
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
                                 BinningUnits="TOF")

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
    out = Rebin("filtered_0", "0,50000,50000", PreserveEvents=False)

    CompareWorkspaces(ws, out, CheckUncertainty=False, CheckSpectraMap=False, CheckInstrument=False)

**Example - filter events based on log values**

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
    ws = AlignAndFocusPowderSlim("VULCAN_218062.nxs.h5", SplitterWorkspace='splitter')
    print(ws.getNumberOfEntries())  # should be 6 workspaces in the group


**Example - filter bad pulses**

.. code-block:: python

    ws = AlignAndFocusPowderSlim("VULCAN_218062.nxs.h5",
                                 XMin=0, XMax=50000, XDelta=50000,
                                 BinningMode="Linear",
                                 BinningUnits="TOF",
                                 FilterBadPulses=True)

    # This is equivalent to using FilterBadPulses.
    # But note that this example doesn't align the data so put everything in 1 big bin to compare.
    ws2 = LoadEventNexus("VULCAN_218062.nxs.h5")
    grp = CreateGroupingWorkspace(ws2, GroupDetectorsBy='bank')
    ws2 = GroupDetectors(ws2, CopyGroupingFromWorkspace="grp")
    ws2 = FilterBadPulses(ws2)
    ws2 = Rebin(ws2, "0,50000,50000", PreserveEvents=False)

    CompareWorkspaces(ws, ws2, CheckUncertainty=False, CheckSpectraMap=False, CheckInstrument=False)

.. note::

    We also can currently only filter based on the pulse time, not the event time-of-flight.

.. categories::

.. sourcelink::
