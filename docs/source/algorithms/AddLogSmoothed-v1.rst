.. algorithm::

.. summary::

.. relatedalgorithms::

.. properties::

Description
-----------

This algorithm performs a smoothing operation over the values in the sample log.

The options available are boxcar (moving average) smoothing, FFT smoothing by zeroing, or FFT smoothing with a Butterworth filter.

Usage
-----

**Example: Smoothing a log with moving average smoothing**

.. testcode:: AddLogSmoothed

    ws = CreateSampleWorkspace()
    AddTimeSeriesLog(ws,"MyLog","2010-01-01T00:00:00",1.0,DeleteExisting=False)
    AddTimeSeriesLog(ws,"MyLog","2010-01-01T00:00:10",2.0,DeleteExisting=False)
    AddTimeSeriesLog(ws,"MyLog","2010-01-01T00:00:20",6.0,DeleteExisting=False)
    AddTimeSeriesLog(ws,"MyLog","2010-01-01T00:00:30",4.0,DeleteExisting=False)

    AddLogSmoothed(ws, "MyLog", "BoxCar", Params = "3")

    for logName in ["MyLog","MyLog_smoothed"]:
        print("Log: {}".format(logName))
        print(ws.getRun().getProperty(logName).valueAsString())

Output:

.. testoutput:: AddLogSmoothed
    :options: +NORMALIZE_WHITESPACE

    Log: MyLog
    2010-Jan-01 00:00:00  1
    2010-Jan-01 00:00:10  2
    2010-Jan-01 00:00:20  6
    2010-Jan-01 00:00:30  4

    Log: MyLog_smoothed
    2010-Jan-01 00:00:00  1.5
    2010-Jan-01 00:00:10  3
    2010-Jan-01 00:00:20  4
    2010-Jan-01 00:00:30  5

.. categories::

.. sourcelink::
