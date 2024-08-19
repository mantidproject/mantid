.. algorithm::

.. summary::

.. relatedalgorithms::

.. properties::


ReflectometrySliceEventWorkspace
--------------------------------

This algorithm slices an input workspace into one or more grouped output workspaces based on input filtering properties.

It uses :ref:`algm-GenerateEventsFilter` to define the way splitting should be done and exposes the relevant input properties for that algorithm. It then performs the filtering using :ref:`algm-FilterEvents`.

The sliced workspaces are then rebinned to histogram data and combined with the given monitor workspace, to produce a workspace suitable for input to :ref:`algm-ReflectometryReductionOneAuto`. The monitors for each slice are scaled according to the percentage of ``proton_charge`` in that slice.

Usage
-------

**Example: slice by time interval**

.. testcode:: ExSliceByTime

    input_ws = CreateSampleWorkspace("Event",BankPixelWidth=1,BinWidth=20000)
    AddTimeSeriesLog(input_ws, Name="proton_charge", Time="2010-01-01T00:00:00", Value=100)
    AddTimeSeriesLog(input_ws, Name="proton_charge", Time="2010-01-01T00:10:00", Value=100)
    AddTimeSeriesLog(input_ws, Name="proton_charge", Time="2010-01-01T00:20:00", Value=80)
    AddTimeSeriesLog(input_ws, Name="proton_charge", Time="2010-01-01T00:30:00", Value=80)
    AddTimeSeriesLog(input_ws, Name="proton_charge", Time="2010-01-01T00:40:00", Value=15)
    AddTimeSeriesLog(input_ws, Name="proton_charge", Time="2010-01-01T00:50:00", Value=100)
    monitor_ws = CreateSampleWorkspace(NumBanks=0, NumMonitors=3, BankPixelWidth=1, NumEvents=10000)

    output = ReflectometrySliceEventWorkspace(InputWorkspace=input_ws, MonitorWorkspace=monitor_ws,
                                              TimeInterval=600, StartTime='1800', StopTime='3300')

    print(str(output.getNumberOfEntries()) + ' slices')
    print(str(output[0].getNumberHistograms()) + ' spectra')
    print('Y values for first bin:')
    for i in range(output.getNumberOfEntries()):
        print('Slice '  + str(i))
        for j in range(output[i].getNumberHistograms()):
            print('{:.8f}'.format(output[i].dataY(j)[0]))

Output:

.. testoutput:: ExSliceByTime

    3 slices
    5 spectra
    Y values for first bin:
    Slice 0
    0.05052632
    0.05052632
    0.05052632
    4.00000000
    4.00000000
    Slice 1
    0.00947368
    0.00947368
    0.00947368
    4.00000000
    4.00000000
    Slice 2
    0.06315789
    0.06315789
    0.06315789
    1.00000000
    1.00000000

.. categories::

.. sourcelink::
