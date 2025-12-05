.. algorithm::

.. summary::

.. relatedalgorithms::

.. properties::

Description
-----------

This algorithm performs a cubic spline interpolation over the values in a sample log.

The values used to define the fitting spline must be in the sample log ``LogToInterpolate``.  The interpolated values will be fit to the time-series axis in ``LogToMatch``.

If the time range in ``LogToMatch`` is outside the range of ``LogToInterpolate``, the time axis will simply be truncated to fit, without extrapolation.

Usage
-----

**Example: Interpolating a sample log**

.. testcode:: AddLogInterpolated

    ws = CreateSampleWorkspace()
    # add the time series to interpolate
    AddTimeSeriesLog(ws,"interp","2010-01-01T00:00:00",1.0,DeleteExisting=False)
    AddTimeSeriesLog(ws,"interp","2010-01-01T00:00:10",2.0,DeleteExisting=False)
    AddTimeSeriesLog(ws,"interp","2010-01-01T00:00:20",6.0,DeleteExisting=False)
    AddTimeSeriesLog(ws,"interp","2010-01-01T00:00:30",4.0,DeleteExisting=False)

    # add the time values to match against
    AddTimeSeriesLog(ws,"match","2010-01-01T00:00:03",0.0,DeleteExisting=False)
    AddTimeSeriesLog(ws,"match","2010-01-01T00:00:07",0.0,DeleteExisting=False)
    AddTimeSeriesLog(ws,"match","2010-01-01T00:00:11",0.0,DeleteExisting=False)
    AddTimeSeriesLog(ws,"match","2010-01-01T00:00:15",0.0,DeleteExisting=False)
    AddTimeSeriesLog(ws,"match","2010-01-01T00:00:16",0.0,DeleteExisting=False)
    AddTimeSeriesLog(ws,"match","2010-01-01T00:00:21",0.0,DeleteExisting=False)

    AddLogInterpolated(ws, "interp", "match")

    for logName in ["interp","interp_interpolated"]:
        print("Log: {}".format(logName))
        print(ws.getRun().getProperty(logName).valueAsString())

Output:

.. testoutput:: AddLogInterpolated
    :options: +NORMALIZE_WHITESPACE

    Log: interp
    2010-Jan-01 00:00:00  1
    2010-Jan-01 00:00:10  2
    2010-Jan-01 00:00:20  6
    2010-Jan-01 00:00:30  4

    Log: interp_interpolated
    2010-Jan-01 00:00:03  1
    2010-Jan-01 00:00:07  0.9968
    2010-Jan-01 00:00:11  1.4544
    2010-Jan-01 00:00:15  2.8
    2010-Jan-01 00:00:16  3.263
    2010-Jan-01 00:00:21  5.488

.. categories::

.. sourcelink::
