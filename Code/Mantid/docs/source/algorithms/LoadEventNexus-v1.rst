.. algorithm::

.. summary::

.. alias::

.. properties::

Description
-----------

The LoadEventNeXus algorithm loads data from an EventNexus file into an
`EventWorkspace <http://www.mantidproject.org/EventWorkspace>`_. The default histogram bin
boundaries consist of a single bin able to hold all events (in all
pixels), and will have their `units <http://www.mantidproject.org/units>`_ set to time-of-flight.
Since it is an `EventWorkspace <http://www.mantidproject.org/EventWorkspace>`_, it can be rebinned
to finer bins with no loss of data.

Sample logs, such as motor positions or e.g. temperature vs time, are
also loaded using the :ref:`algm-LoadNexusLogs` child algorithm.

Optional properties
###################

If desired, you can filter out the events at the time of loading, by
specifying minimum and maximum time-of-flight values. This can speed up
loading and reduce memory requirements if you are only interested in a
narrow range of the times-of-flight of your data.

You may also filter out events by providing the start and stop times, in
seconds, relative to the first pulse (the start of the run).

If you wish to load only a single bank, you may enter its name and no
events from other banks will be loaded.

The Precount option will count the number of events in each pixel before
allocating the memory for each event list. Without this option, because
of the way vectors grow and are re-allocated, it is possible for up to
2x too much memory to be allocated for a given event list, meaning that
your EventWorkspace may occupy nearly twice as much memory as needed.
The pre-counting step takes some time but that is normally compensated
by the speed-up in avoid re-allocating, so the net result is smaller
memory footprint and approximately the same loading time.

Veto Pulses
###########

Veto pulses can be filtered out in a separate step using
:ref:`algm-FilterByLogValue`:

``FilterByLogValue(InputWorkspace="ws", OutputWorkspace="ws", LogName="veto_pulse_time", PulseFilter="1")``

.. categories::
