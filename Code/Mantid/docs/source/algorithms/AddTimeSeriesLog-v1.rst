.. algorithm::

.. summary::

.. alias::

.. properties::

Description
-----------

Creates/updates a time-series log entry on a chosen workspace. The given
timestamp & value are appended to the named log entry. If the named
entry does not exist then a new log is created. A time stamp must be
given in ISO8601 format, e.g. 2010-09-14T04:20:12.

By default, the given value is interpreted as a double and a double
series is either created or expected. However, if the "Type" is set to
"int" then the value is interpreted as an integer and an integer is
either created or expected.

Usage
-----

**Example**

.. testcode:: AddTimeSeriesLogEx

    ws = CreateSampleWorkspace("Event",BankPixelWidth=1)

    AddTimeSeriesLog(ws, Name="my_log", Time="2010-01-01T00:00:00", Value=100) 
    AddTimeSeriesLog(ws, Name="my_log", Time="2010-01-01T00:30:00", Value=15)
    AddTimeSeriesLog(ws, Name="my_log", Time="2010-01-01T00:50:00", Value=100.2)

    log = ws.getRun().getLogData("my_log")
    print "my_log has %i entries" % log.size()
    for i in range(log.size()):
      print "\t%s\t%f" % (log.times[i], log.value[i])

    AddTimeSeriesLog(ws, Name="my_log", Time="2010-01-01T00:00:00", Value=12, Type="int", DeleteExisting=True)
    AddTimeSeriesLog(ws, Name="my_log", Time="2010-01-01T00:50:00", Value=34, Type="int")

    log = ws.getRun().getLogData("my_log")
    print "my_log now has %i entries" % log.size()
    for i in range(log.size()):
      print "\t%s\t%i" % (log.times[i], log.value[i])

Output:

.. testoutput:: AddTimeSeriesLogEx
    :options: +NORMALIZE_WHITESPACE

    my_log has 3 entries
            2010-01-01T00:00:00     100.000000
            2010-01-01T00:30:00     15.000000
            2010-01-01T00:50:00     100.200000
    my_log now has 2 entries
            2010-01-01T00:00:00     12
            2010-01-01T00:50:00     34

.. categories::

  

