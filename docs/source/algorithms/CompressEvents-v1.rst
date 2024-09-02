.. algorithm::

.. summary::

.. relatedalgorithms::

.. properties::

Description
-----------

The algorithm can be run in two modes, with and without pulsetime
resolution. This switch is made by specifying a
``WallClockTolerance``. The ``StartTime`` is ignored unless
``WallClockTolerance`` is specified.


Without pulsetime resolution
############################

This algorithm starts by sorting the event lists by TOF (or whatever
the independent axis is) and ignoring the pulsetime. Therefore you may
gain speed by calling :ref:`algm-SortEvents` beforehand.  Starting
from the smallest TOF, all events within ``Tolerance`` in a spectrum
are considered to be identical. A :py:obj:`weighted event without time
information <mantid.api.EventType.WEIGHTED_NOTIME>` is created; its
TOF is the average value of the summed events; its weight is the sum
of the weights of the input events; its error is the sum of the square
of the errors of the input events.

Note that using ``CompressEvents`` may introduce errors if you use too large
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

Compressing events without sorting
**********************************

If you set the option ``SortFirst`` to ``False`` then a different
method to compress events will be used where the events do not need to
be sorted first. This will be faster when the number of events going
into a single compressed events is large, so when you have lots of
events per spectra. This method works by histogramming the events into
a regular grid that is then converted into events using the bin center
as the x value.

This method will not be used if the events are already sorted as that
will be faster. This option will be ignored when used with
``WallClockTolerance``.

With pulsetime resolution
#########################

Similar to the version without pulsetime resolution with a few key differences:

1. The :py:obj:`events are weighted with time
   <mantid.api.EventType.WEIGHTED>`. As a result, one can still run
   :ref:`algm-FilterEvents` on the results.
2. Similar to the TOF, the resulting pulsetime of an individual
   weighted event is the average of the individual events that
   contribute to it.

While the algorithm can be run with arbitrary values of
``WallClockTolerance``, it is recommended to keep in mind what
resolution is desired for later filtering. The pulsetime is
effectively filtered on a bin of the form:
:math:`{pulsetime[i]} = {StartTime} + {WallClockTolerance} * i`

The ``StartTime`` property is only used in pulsetime resolution
mode. Any events that occur before it in a run are ignored and do not
appear in the ``OutputWorkspace``. If it is not specified, then the
:py:obj:`Run.startTime <mantid.api.Run.startTime>` is used. An example
`ISO8601 <https://www.iso.org/iso-8601-date-and-time-format.html>`_
format for the ``StartTime`` is ``2010-09-14T04:20:12``. Normally this
parameter can be left unset.

Logarithmic binning
###################

If you provide a negative tolerance or select ``Logarithmic`` as the ``BinningMode`` then the events will be combined together in increase large tolerances starting from the smallest TOF value. This follows the same method as the logarithmic binning of :ref:`algm-Rebin`. This mode will fail if any of the TOF values are negative.

Usage
-----

**Example**

.. testcode:: CompressEvents

    ws = CreateSampleWorkspace("Event",BankPixelWidth=1)

    print("The unfiltered workspace {} has {} events and a peak value of {:.2f}".format(ws, ws.getNumberEvents(), ws.readY(0)[50]))

    ws = CompressEvents(ws)

    print("The compressed workspace {} still has {} events and a peak value of {:.2f}".format(ws, ws.getNumberEvents(), ws.readY(0)[50]))
    print("However it now takes up less memory.")


Output:

.. testoutput:: CompressEvents
    :options: +NORMALIZE_WHITESPACE

    The unfiltered workspace ws has 1900 events and a peak value of 257.00
    The compressed workspace ws still has 1900 events and a peak value of 257.00
    However it now takes up less memory.



.. categories::

.. sourcelink::
