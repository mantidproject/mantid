
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
- does not support removing bad pulses

Child algorithms used are

- :ref:`algm-LoadDiffCal`
- :ref:`algm-LoadIDFFromNexus-v1`
- :ref:`algm-EditInstrumentGeometry`
- :ref:`algm-LoadNexusLogs`
- :ref:`algm-ConvertUnits`

Usage
-----

**Example - event filtering**

This algorithm accepts the sample SplitterWorkspace inputs as :ref:`FilterEvents <algm-FilterEvents>`.

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
    ws=AlignAndFocusPowderSlim("VULCAN_218062.nxs.h5",
                               SplitterWorkspace=splitter, RelativeTime=True,
                               XMin=0, XMax=50000, XDelta=50000,
                               BinningMode="Linear",
                               BinningUnits="TOF")

    # This is equilvalent to using FilterEvents with the same splitter table.
    # But note that this example doesn't align the data so put everything in 1 big bin to compare.
    ws2 = LoadEventNexus("VULCAN_218062.nxs.h5", NumberOfBins=1)
    grp = CreateGroupingWorkspace(ws2, GroupDetectorsBy='bank')
    ws2 = GroupDetectors(ws2, CopyGroupingFromWorkspace="grp")

    FilterEvents(ws2,
                 SplitterWorkspace=splitter, RelativeTime=True,
                 FilterByPulseTime=True,
                 OutputWorkspaceBaseName="filtered")

    np.testing.assert_array_equal(mtd["filtered_0"].extractY(), ws.extractY())

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
                         MaximumLogValue=70.15)

    # Use the splitter table to filter the events during loading
    ws=AlignAndFocusPowderSlim("VULCAN_218062.nxs.h5", SplitterWorkspace='splitter')


.. note::

    While we currently only support a single output workspace when filtering events from a splitter table but the output target can be selected with the ``SplitterTarget`` property and you can run the algorithm multiple times with different targets. We also can currently only filter base on the pulse time, not the event time-of-flight.

.. categories::

.. sourcelink::
