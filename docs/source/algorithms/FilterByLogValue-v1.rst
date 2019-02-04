.. algorithm::

.. summary::

.. relatedalgorithms::

.. properties::

Description
-----------

Filters out events using the entries in the Sample Logs.

Sample logs consist of a series of pairs. The first step in filtering is
to generate a list of start-stop time intervals that will be kept, using
those logs.

-  Each log value is compared to the min/max value filters to determine
   whether it is "good" or not.

   -  For a single log value that satisfies the criteria at time T, all
      events between T+-Tolerance (LogBoundary=Centre), or T and
      T+Tolerance (LogBoundary=Left) are kept.
   -  If there are several consecutive log values matching the filter,
      events between T1-Tolerance and T2+Tolerance, where T2 is the last
      "good" value (LogBoundary=Centre), or T1 and T2, where T2 is the
      first "bad" value (LogBoundary=Left) are kept.

-  The filter is then applied to all events in all spectra. Any events
   with pulse times outside of any "good" time ranges are removed.

There is no interpolation of log values between the discrete sample log
times at this time. However, the log value is assumed to be constant at
times before its first point and after its last. For example, if the
first temperature measurement was at time=10 seconds and a temperature
within the acceptable range, then all events between 0 and 10 seconds
will be included also. If a log has a single point in time, then that
log value is assumed to be constant for all time and if it falls within
the range, then all events will be kept.

.. warning::

   :ref:`FilterByLogValue <algm-FilterByLogValue>` is not suitable for
   fast log filtering.

PulseFilter (e.g. for Veto Pulses)
##################################

If you select PulseFilter, then events will be filtered OUT in notches
around each time in the selected sample log, and the MinValue/MaxValue
parameters are ignored. For example:

-  If you have 3 entries at times:

   -  10, 20, 30 seconds.
   -  A TimeTolerance of 1 second.

-  Then the events at the following times will be EXCLUDED from the
   output:

   -  9-11; 19-21; 29-30 seconds.

The typical use for this is to filter out "veto" pulses from a SNS event
nexus file. Some of these files have a sample log called
"veto\_pulse\_time" that only contains times of the pulses to be
rejected. For example, this call will filter out veto pulses:

.. testsetup:: VetoPulseTime

   ws=CreateSampleWorkspace("Event")
   AddTimeSeriesLog(ws, Name="veto_pulse_time", Time="2010-01-01T00:00:00", Value=1)
   AddTimeSeriesLog(ws, Name="veto_pulse_time", Time="2010-01-01T00:10:00", Value=0)
   AddTimeSeriesLog(ws, Name="veto_pulse_time", Time="2010-01-01T00:20:00", Value=1)
   AddTimeSeriesLog(ws, Name="veto_pulse_time", Time="2010-01-01T00:30:00", Value=0)
   AddTimeSeriesLog(ws, Name="veto_pulse_time", Time="2010-01-01T00:40:00", Value=1)
   AddTimeSeriesLog(ws, Name="veto_pulse_time", Time="2010-01-01T00:50:00", Value=0)

.. testcode:: VetoPulseTime

   ws = FilterByLogValue(ws, LogName="veto_pulse_time", PulseFilter="1")

Comparing with other event filtering algorithms
###############################################

The :ref:`EventFiltering` page has a detailed introduction on event
filtering in mantid.


Usage
-----

**Example - Filtering by a simple time series Log**

.. testcode:: FilterByLogValue

   ws = CreateSampleWorkspace("Event",BankPixelWidth=1)

   AddTimeSeriesLog(ws, Name="proton_charge", Time="2010-01-01T00:00:00", Value=100)
   AddTimeSeriesLog(ws, Name="proton_charge", Time="2010-01-01T00:10:00", Value=100)
   AddTimeSeriesLog(ws, Name="proton_charge", Time="2010-01-01T00:20:00", Value=100)
   AddTimeSeriesLog(ws, Name="proton_charge", Time="2010-01-01T00:30:00", Value=100)
   AddTimeSeriesLog(ws, Name="proton_charge", Time="2010-01-01T00:40:00", Value=15)
   AddTimeSeriesLog(ws, Name="proton_charge", Time="2010-01-01T00:50:00", Value=100)

   print("The unfiltered workspace {} has {} events and a peak value of {:.2f}".format(ws, ws.getNumberEvents(),ws.readY(0)[50]))

   wsOut = FilterByLogValue(ws,"proton_charge",MinimumValue=75, MaximumValue=150)

   print("The filtered workspace {} has {} events and a peak value of {:.2f}".format(wsOut, wsOut.getNumberEvents(),wsOut.readY(0)[50]))


Output:

.. testoutput:: FilterByLogValue

   The unfiltered workspace ws has 1900 events and a peak value of 2...
   The filtered workspace wsOut has 950 events and a peak value of 1...


.. categories::

.. sourcelink::
