.. algorithm::

.. summary::

.. relatedalgorithms::

.. properties::


ReflectometrySliceEventWorkspace
--------------------------------

This algorithm slices an input workspace into one or more grouped output workspaces based on input filtering properties.

It uses :ref:`algm-GenerateEventsFilter` to define the way splitting should be done and exposes the relevant input properties for that algorithm. It then performs the filtering using :ref:`algm-FilterEvents`.

The sliced workspaces are then rebinned to histogram data and combined with the given monitor workspace, to produce a workspace suitable for input to :ref:`algm-ReflectometryReductionOneAuto`. The monitors for each slice are scaled according to the percentage of ``proton_charge`` in that slice.

WorkspaceGroup Processing
#########################

This version of the algortithm processes :py:obj:`WorkspaceGroups <mantid.api.WorkspaceGroup>` in a different way to :ref:`algm-ReflectometrySliceEventWorkspace-v1`.
For a given input group :math:`G` with :math:`x` workspaces being split into :math:`n` slices (:math:`S`), the old method would produce a group of groups in the following form:

:math:`G(G_1(S(1_1), ..., S(1_n)), ..., G_x(S(x_1), ..., S(x_n)))`

The new method creates :math:`n` groups in the same "shape" as the input workspace group:

:math:`G_1(S(1_1), ..., S(x_1)), ..., G_n(S(1_n), ..., S(x_n))`

This allows the output groups to be used in the same manner as the input group. For example, when using the group in a polarization correction workflow.

Usage
-----

**Example: slice by time interval**

.. testcode:: ExSliceByTimeV2

    input_ws_1 = CreateSampleWorkspace("Event",BankPixelWidth=1,BinWidth=20000)
    AddTimeSeriesLog(input_ws_1, Name="proton_charge", Time="2010-01-01T00:00:00", Value=100)
    AddTimeSeriesLog(input_ws_1, Name="proton_charge", Time="2010-01-01T00:10:00", Value=100)
    AddTimeSeriesLog(input_ws_1, Name="proton_charge", Time="2010-01-01T00:20:00", Value=80)
    AddTimeSeriesLog(input_ws_1, Name="proton_charge", Time="2010-01-01T00:30:00", Value=80)
    AddTimeSeriesLog(input_ws_1, Name="proton_charge", Time="2010-01-01T00:40:00", Value=15)
    AddTimeSeriesLog(input_ws_1, Name="proton_charge", Time="2010-01-01T00:50:00", Value=100)
    input_ws_2 = CloneWorkspace(input_ws_1)
    input_ws_3 = CloneWorkspace(input_ws_1)

    input_ws = GroupWorkspaces([input_ws_1, input_ws_2, input_ws_3])
    monitor_ws = CreateSampleWorkspace(NumBanks=0, NumMonitors=3, BankPixelWidth=1, NumEvents=10000)

    ReflectometrySliceEventWorkspace(InputWorkspaceName="input_ws", MonitorWorkspaceName="monitor_ws",
                                            TimeInterval=600, StartTime='1800', StopTime='3300', OutputWorkspaceName="output")

    slice_a = mtd["output_1800_2400"]
    print(str(slice_a.getNumberOfEntries()) + ' workspaces')
    print(str(slice_a[0].getNumberHistograms()) + ' spectra')
    print('Y values for first bin:')
    for i in range(slice_a.getNumberOfEntries()):
        print('Workspace '  + str(i+1))
        for j in range(slice_a[i].getNumberHistograms()):
            print('{:.8f}'.format(slice_a[i].y(j)[0]))

Output:

.. testoutput:: ExSliceByTimeV2

    3 workspaces
    5 spectra
    Y values for first bin:
    Workspace 1
    0.05052632
    0.05052632
    0.05052632
    4.00000000
    4.00000000
    Workspace 2
    0.05052632
    0.05052632
    0.05052632
    4.00000000
    4.00000000
    Workspace 3
    0.05052632
    0.05052632
    0.05052632
    4.00000000
    4.00000000

.. categories::

.. sourcelink::
