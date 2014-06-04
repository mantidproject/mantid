.. algorithm::

.. summary::

.. alias::

.. properties::

Description
-----------

Rebins an EventWorkspace according to the pulse times of each event
rather than the time of flight :ref:`algm-Rebin`. The Params inputs may
be expressed in an identical manner to the :ref:`algm-Rebin` algorithm.
Users may either provide a single value, which is interpreted as the
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

   RebinByPulseTime.png

.. categories::
