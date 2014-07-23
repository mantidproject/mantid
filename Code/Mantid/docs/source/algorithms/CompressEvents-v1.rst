.. algorithm::

.. summary::

.. alias::

.. properties::

Description
-----------

This algorithm starts by sorting the event lists by TOF; therefore you
may gain speed by calling :ref:`algm-SortEvents` beforehand.
Starting from the smallest TOF, all events within Tolerance are
considered to be identical. Pulse times are ignored. A weighted event
without time information is created; its TOF is the average value of the
summed events; its weight is the sum of the weights of the input events;
its error is the sum of the square of the errors of the input events.

Note that using CompressEvents may introduce errors if you use too large
of a tolerance. Rebinning an event workspace still uses an
all-or-nothing view: if the TOF of the event is in the bin, then the
counts of the bin is increased by the event's weight. If your tolerance
is large enough that the compound event spans more than one bin, then
you will get small differences in the final histogram.

If you are working from the raw events with TOF resolution of 0.100
microseconds, then you can safely use a tolerance of, e.g., 0.05
microseconds to group events together. In this case, histograms
with/without compression are identical. If your workspace has undergone
changes to its X values (unit conversion for example), you have to use
your best judgement for the Tolerance value.


Usage
-----

**Example**  

.. testcode:: CompressEvents

    ws = CreateSampleWorkspace("Event",BankPixelWidth=1)

    print "The unfiltered workspace %s has %i events and a peak value of %.2f" % (ws, ws.getNumberEvents(),ws.readY(0)[50]) 
  
    ws = CompressEvents(ws)

    print "The compressed workspace %s still has %i events and a peak value of %.2f" % (ws, ws.getNumberEvents(),ws.readY(0)[50])     
    print "However it now takes up less memory."
 

Output:

.. testoutput:: CompressEvents
    :options: +NORMALIZE_WHITESPACE

    The unfiltered workspace ws has 8000 events and a peak value of 1030.00
    The compressed workspace ws still has 8000 events and a peak value of 1030.00
    However it now takes up less memory.



.. categories::
