.. algorithm::

.. summary::

.. alias::

.. properties::

Description
-----------

This algorithm counts up the events in a workspace against the values of
a log within the workspace. It will most commonly be used as a
sub-algorithm of the `RockingCurve <RockingCurve>`__ algorithm.

The algorithm has two modes:

Table output
^^^^^^^^^^^^

This option can be used for integer-typed logs and will produce a table
with a row for each integer value between the minimum and maximum
contained in the log, and a further column containing the total events
for which the log had each value. Further columns will be added for:

-  Monitors, if any - this requires an event workspace with the same
   name as the input workspace plus a '\_monitors' suffix (this is what
   :ref:`_algm-LoadEventNexus` will give).
-  The total time duration, in seconds, during which the log had each
   value.
-  The integrated proton charge during the period(s) for which the log
   had each value.
-  The time-weighted average value of any other number-series logs which
   had more than a single value during the run.

**Warning:** This mode is intended for logs with a small range (e.g.
scan index, period number, status). Be aware that if it is used for a
log with a large range, it will create a table row for every integer
value between the minimum and maximum log value. This might take a long
time!

Single-spectrum option
^^^^^^^^^^^^^^^^^^^^^^

This option can be used for integer or floating point type logs and
requires that the OutputBinning property is specified. It will produce a
single spectrum workspace where the X values are derived from the
OutputBinning property and the Y values are the total counts in each bin
of the log value.

.. categories::
