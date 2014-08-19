.. algorithm::

.. summary::

.. alias::

.. properties::

Description
-----------

Rebins an EventWorkspace according to the pulse times of each event
rather than the time of flight :ref:`algm-Rebin`. The Params inputs may
be expressed in an identical manner to the :ref:`algm-Rebin` algorithm. Param arguments are
in seconds since run start. Users may either provide a single value, which is interpreted as the
*step* (in seconds), or three comma separated values *start*, *step*,
*end*, where all units are in seconds, and start and end are relative to
the start of the run.

The x-axis is expressed in relative time to the start of the run in
seconds.

This algorithm may be used to diagnose problems with the electronics or
data collection. Typically, detectors should see a uniform distribution
of the events generated between the start and end of the run. This
algorithm allows anomalies to be detected.

Example of Use
--------------

This diagnostic algorithm is particularly useful when coupled with the
Instrument View. In the example below is a real-world usage example
where we were able to highlight issues with data collection on the ISIS
WISH instrument. Some blocks of tubes, where tubes are arranged
vertically, are missing neutrons within large block of pulse time as a
result of data-buffering. After running RebinByPulseTime, we were able
to find both, which banks were affected, as well as the missing pulse
times for each bank. The horizontal slider in the instrument view allows
us to easily integrate over a section of pulse time and see the results
as a colour map.

.. figure:: /images/RebinByPulseTime.png
   :alt: RebinByPulseTime.png

   Instrument view of WISH with data projected after rebinning by pulse times
  
Usage
-----

**Example - Rebinning an EventWorkspace by pulse times**

Input workspace has raw events

.. testcode:: RebinByPulseTimesExample

   # Load RAW event data
   event_ws = Load('CNCS_7860_event.nxs')
   # Start time
   start_time_in_seconds = 0
   # End time 
   end_time_in_seconds = 1e2
   # Step size
   time_step_in_seconds = 1 
   # Perform the rebin
   pulse_t_ws = RebinByPulseTimes(InputWorkspace=event_ws, Params=[start_time_in_seconds, time_step_in_seconds, end_time_in_seconds])

   print "Events are rebinned into: ",  pulse_t_ws.blocksize(), "bins per histogram"
   print "X-axis relative start time in seconds: ", pulse_t_ws.readX(0)[0] 
   print "X-axis relative end time in seconds: ", pulse_t_ws.readX(0)[-1] 

Output:
   
.. testoutput:: RebinByPulseTimesExample

   Events are rebinned into:  100 bins per histogram
   X-axis relative start time in seconds:  0.0
   X-axis relative end time in seconds:  100.0

.. categories::
