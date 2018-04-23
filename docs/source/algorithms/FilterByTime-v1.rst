.. algorithm::

.. summary::

.. relatedalgorithms::

.. properties::

Description
-----------

Goes through all events in all EventLists and takes out any events with
a PulseTime value not within the range specified.

-  Sample logs consisting of
   :ref:`TimeSeriesProperty`'s are also filtered out
   according to the same time.
-  The integrated proton charge of the run is also re-calculated
   according to the filtered out ProtonCharge pulse log.

You must specify:

-  Both StartTime and Stop time, or
-  Both AbsoluteStartTime and AbsoluteStop time.
-  But not another combination of the four, or the algorithm will abort.

Comparing with other event filtering algorithms
###############################################

Wiki page :ref:`EventFiltering` has a detailed
introduction on event filtering in MantidPlot.

Usage
-----

**Example - Using a relative and absolute times**  

.. testcode:: ExFilter

    ws = CreateSampleWorkspace("Event",BankPixelWidth=1)
    AddTimeSeriesLog(ws, Name="proton_charge", Time="2010-01-01T00:00:00", Value=100) 
    AddTimeSeriesLog(ws, Name="proton_charge", Time="2010-01-01T00:10:00", Value=100)
    AddTimeSeriesLog(ws, Name="proton_charge", Time="2010-01-01T00:20:00", Value=100)
    AddTimeSeriesLog(ws, Name="proton_charge", Time="2010-01-01T00:30:00", Value=100)
    AddTimeSeriesLog(ws, Name="proton_charge", Time="2010-01-01T00:40:00", Value=15)
    AddTimeSeriesLog(ws, Name="proton_charge", Time="2010-01-01T00:50:00", Value=100)

    #Extract the first 30 minutes  * 60 = 1800 seconds (roughly half of the data)
    wsFiltered = FilterByTime(ws,StartTime=0,StopTime=1800)

    #Extract the first 30 minutes  * 60 = 1800 seconds (roughly half of the data)
    wsFilteredAbs = FilterByTime(ws,
        AbsoluteStartTime="2010-01-01T00:10:00",
        AbsoluteStopTime="2010-01-01T00:20:00")

    print("The number of events within the relative Filter: %i" % wsFiltered.getNumberEvents())
    print("The number of events within the Aboslute Filter: %i" % wsFilteredAbs.getNumberEvents())
    print("Compared to the number in the unfiltered workspace: %i" % ws.getNumberEvents())

Output:

.. testoutput:: ExFilter
   :options: +ELLIPSIS +NORMALIZE_WHITESPACE

    The number of events within the relative Filter: 95...
    The number of events within the Aboslute Filter: 3...
    Compared to the number in the unfiltered workspace: 190...


.. categories::

.. sourcelink::
