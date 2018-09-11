.. algorithm::

.. summary::

.. relatedalgorithms::

.. properties::

Description
-----------

The algorithm looks at sample logs ("proton\_charge"), finds the mean,
and rejects any events that occurred during a pulse that was below a
certain percentage of that mean. This effectively removes neutrons from
the background that were measured while the accelerator was not actually
producing neutrons, reducing background noise.

Usage
-----

**Example - Using a simple proton charge log**  

.. testcode:: Filter

    ws = CreateSampleWorkspace("Event",BankPixelWidth=1)

    AddTimeSeriesLog(ws, Name="proton_charge", Time="2010-01-01T00:00:00", Value=100) 
    AddTimeSeriesLog(ws, Name="proton_charge", Time="2010-01-01T00:10:00", Value=100)
    AddTimeSeriesLog(ws, Name="proton_charge", Time="2010-01-01T00:20:00", Value=100)
    AddTimeSeriesLog(ws, Name="proton_charge", Time="2010-01-01T00:30:00", Value=100)
    AddTimeSeriesLog(ws, Name="proton_charge", Time="2010-01-01T00:40:00", Value=15)
    AddTimeSeriesLog(ws, Name="proton_charge", Time="2010-01-01T00:50:00", Value=100)
    AddSampleLog(ws,"gd_prtn_chrg", "1e6", "Number")
    wsFiltered = FilterBadPulses(ws)

    print("The number of events that remain: %i" % wsFiltered.getNumberEvents())
    print("compared to the number in the unfiltered workspace: %i" % ws.getNumberEvents())

Output:

.. testoutput:: Filter

    The number of events that remain: 950 
    compared to the number in the unfiltered workspace: 1900


.. categories::

.. sourcelink::
