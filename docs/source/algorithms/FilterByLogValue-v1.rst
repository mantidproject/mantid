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


Logs that only record changes
#############################

In SNS, most of the sample environment devices record values upon changing.
Therefore, the ``LogBoundary`` value shall set to ``Left`` but not ``Centre``.
And in this case, ``TimeTolerance`` is ignored.

Please check with the instrument scientist to confirm how the sample log values are recorded.

Here is an example how the time splitter works with the a motor's position.

.. figure:: /images/SNAP_motor_filter.png

        For this SNAP run, the user wants to filter events with motor (BL3:Mot:Hexa:MotZ) position
        at value equal to 6.65 with tolerance as 0.1.
        The red curve shows the boundary of the time splitters (i.e., event filters).



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



Time Tolerance, Log Boundary and Pulse Filter
#############################################

How ``TimeTolerance`` is applied to event filtering is highly correlated to
the setup of property ``LogBoundary`` and ``PulseFilter``.

- If ``PulseFilter`` is true, a notch of width ``2 * TimeTolerance`` is centered at each log time.
  Neutron events in this notch will not be used.

- If ``PulseFilter`` is false and ``LogBoundary`` is ``Left``, ``TimeTolerance`` is ignored in the algorithm.

- If ``PulseFilter`` is false and ``LogBoundary`` is set to ``Centre``,
  assuming the log entries are
  ``(t0, v0), (t1, v1), (t2, v2), (t3, v3), ... (t_n, v_n) ...``.

  - If there is a log entry ``(t_i, v_i)`` is between ``MinimumValue`` and ``MaximumValue``,
    while ``v_{i-1}`` and ``v_{i+1}`` are not in the desired log value range,
    all events between ``t_i - TimeTolerance`` and ``t_i + TimeTolerance)`` are kept.

  - If there are several consecutive log entries that have values in the desired log value range,
    such as ``(t_i, v_i), ..., (t_j, v_j)``,
    events between ``t_i - TimeTolerance`` and ``t_j + TimToleranc`` are kept.

  A good value is 1/2 your measurement interval if the intervals are constant.



Comparing with GenerateEventsFilter/FilterEvents
################################################

The combination of :ref:`GenerateEventsFilter <algm-GenerateEventsFilter>` and :ref:`FilterEvents <algm-FilterEvents>` with proper configuration
can produce same result as :ref:`FilterByLogValue <algm-FilterByLogValue>`.

For sample,

.. code-block:: python

   from mantid.simpleapi import *

   # Load data
   LoadEventNexus(Filename='/SNS/SNAP/IPTS-25836/nexus/SNAP_52100.nxs.h5', OutputWorkspace='52100')

   # FilterByLogValue
   FilterByLogValue(InputWorkspace='52100',
                    OutputWorkspace='52100_hexaZ_L',
                    LogName='BL3:Mot:Hexa:MotZ',
                    MinimumValue= 6.55,
                    MaximumValue= 6.75,
                    LogBoundary='Left')

   # Equivalent GenerateEventsFilter/FilterEvents combination
   GenerateEventsFilter(InputWorkspace='52100',
                        OutputWorkspace='MotZSplitter_left',
                        InformationWorkspace='MotZSplitter_left_info',
                        LogName='BL3:Mot:Hexa:MotZ',
                        MinimumLogValue=6.55,
                        MaximumLogValue=6.75,
                        LogBoundary='Left',
                        TitleOfSplitters='Left')
   FilterEvents(InputWorkspace='52100',
                SplitterWorkspace='MotZSplitter_left',
                OutputWorkspaceBaseName='Chop52100',
                InformationWorkspace='MotZSplitter_left_info',
                FilterByPulseTime=True)



The OutputWorkspace with name ``Chop52100_0`` output from :ref:`FilterEvents <algm-FilterEvents>` is equivalent to ``52100_hexaZ_L``
from ``FilterByLogValue``.

Here is the comparison between FilterByLogValue and :ref:`GenerateEventsFilter<algm-GenerateEventsFilter>`/:ref:`FilterEvents<algm-FilterEvents>`.

1. ``FilterByLogValue`` can only filter events at the resolution of neutron pulse.

   - If the start time *t_s* of a splitter is inside a pulse, then all the events inside that pulse but before *t_s*
     will be included in the filtered workspace.
   - If the end time *t_e* of a splitter is inside a pulse, then all the events inside that pulse even before *t_e*
     will be excluded in the filtered workspace.
   - :ref:`FilterEvents<algm-FilterEvents>` is able to do the filtering precisely to include events only within time range [*t_s*, *t_e*).

2. ``FilterByLogValue`` can only filter events around only ``one`` log value, while
   :ref:`GenerateEventsFilter<algm-GenerateEventsFilter>`/:ref:`FilterEvents<algm-FilterEvents>`
   combination can filter events against a series of log values.

3. :ref:`GenerateEventsFilter<algm-GenerateEventsFilter>`/:ref:`FilterEvents<algm-FilterEvents>`
   have more outputs to examine the result.


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
