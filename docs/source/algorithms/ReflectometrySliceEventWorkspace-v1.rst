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

    input_ws = CreateSampleWorkspace("Event",BankPixelWidth=1,BinWidth=20000)
    AddTimeSeriesLog(input_ws, Name="proton_charge", Time="2010-01-01T00:00:00", Value=100)
    AddTimeSeriesLog(input_ws, Name="proton_charge", Time="2010-01-01T00:10:00", Value=100)
    AddTimeSeriesLog(input_ws, Name="proton_charge", Time="2010-01-01T00:20:00", Value=80)
    AddTimeSeriesLog(input_ws, Name="proton_charge", Time="2010-01-01T00:30:00", Value=80)
    AddTimeSeriesLog(input_ws, Name="proton_charge", Time="2010-01-01T00:40:00", Value=15)
    AddTimeSeriesLog(input_ws, Name="proton_charge", Time="2010-01-01T00:50:00", Value=100)
    CreateSampleWorkspace(OutputWorkspace='monitor_ws', NumBanks=0, NumMonitors=3, BankPixelWidth=1, NumEvents=10000, Random=True)

    output = ReflectometrySliceEventWorkspace(InputWorkspace='input_ws', MonitorWorkspace='monitor_ws',OutputWorkspace='sliced',
    TimeInterval=600, StartTime='1800', StopTime='3300')

    print output.getNumberOfEntries(),'slices'
    print output[0].getNumberHistograms(),'spectra'

.. seealso :: Algorithm :ref:`algm-GenerateEventsFilter`, :ref:`algm-FilterEvents` and :ref:`algm-ReflectometryReductionOneAuto`

.. categories::

.. sourcelink::
