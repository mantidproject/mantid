.. algorithm::

.. summary::

.. relatedalgorithms::

.. properties::

Description
-----------

This algorithm counts up the events in a workspace against the values of
a log within the workspace. It will most commonly be used as a
sub-algorithm of the `RockingCurve <http://www.mantidproject.org/RockingCurve>`_ algorithm.

The algorithm has two modes:

Table output
############

This option can be used for integer-typed logs and will produce a table
with a row for each integer value between the minimum and maximum
contained in the log, and a further column containing the total events
for which the log had each value. Further columns will be added for:

-  Monitors, if any - this requires an event workspace with the same
   name as the input workspace plus a '\_monitors' suffix (this is what
   :ref:`algm-LoadEventNexus` will give).
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
######################

This option can be used for integer or floating point type logs and
requires that the OutputBinning property is specified. It will produce a
single spectrum workspace where the X values are derived from the
OutputBinning property and the Y values are the total counts in each bin
of the log value.

Usage
-----

**Example - Single-Spectrum Mode**  

.. testcode:: Single-Spectrum

  # a sample workspace with a sample instrument
  ws = CreateSampleWorkspace("Event",BankPixelWidth=1)

  AddTimeSeriesLog(ws, Name="Log2FilterBy", Time="2010-01-01T00:00:00", Value=1) 
  AddTimeSeriesLog(ws, Name="Log2FilterBy", Time="2010-01-01T00:10:00", Value=2)
  AddTimeSeriesLog(ws, Name="Log2FilterBy", Time="2010-01-01T00:20:00", Value=3)
  AddTimeSeriesLog(ws, Name="Log2FilterBy", Time="2010-01-01T00:30:00", Value=1)
  AddTimeSeriesLog(ws, Name="Log2FilterBy", Time="2010-01-01T00:40:00", Value=2)
  AddTimeSeriesLog(ws, Name="Log2FilterBy", Time="2010-01-01T00:50:00", Value=3)

  #split the events by the log value
  wsOut = SumEventsByLogValue(ws,LogName="Log2FilterBy",OutputBinning=[1,1,4])

  #all of the events should be included
  integral = Integration(wsOut)
  print("Events were split into %i sections based on the log 'Log2FilterBy'." % wsOut.blocksize())
  for i in range(0,wsOut.blocksize()):
    print(" section %i: %.2f" % (i+1,wsOut.readY(0)[i]))
  print("Totalling %.0f events, matching the %i events in the input workspace" % (integral.readY(0)[0],ws.getNumberEvents()))

Output:

.. testoutput:: Single-Spectrum

    Events were split into 3 sections based on the log 'Log2FilterBy'.
     section 1: 6...
     section 2: 6...
     section 3: 6...
    Totalling 1900 events, matching the 1900 events in the input workspace


.. categories::

.. sourcelink::
