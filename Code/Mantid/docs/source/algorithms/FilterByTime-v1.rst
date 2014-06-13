.. algorithm::

.. summary::

.. alias::

.. properties::

Description
-----------

Goes through all events in all EventLists and takes out any events with
a PulseTime value not within the range specified.

-  Sample logs consisting of
   `TimeSeriesProperty <TimeSeriesProperty>`__'s are also filtered out
   according to the same time.
-  The integrated proton charge of the run is also re-calculated
   according to the filtered out ProtonCharge pulse log.

You must specify:

-  Both StartTime and Stop time, or
-  Both AbsoluteStartTime and AbsoluteStop time.
-  But not another combination of the four, or the algorithm will abort.

Comparing with other event filtering algorithms
###############################################

Wiki page `EventFiltering <EventFiltering>`__ has a detailed
introduction on event filtering in MantidPlot.

.. categories::
