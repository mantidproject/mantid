.. algorithm::

.. summary::

.. relatedalgorithms::

.. properties::

Description
-----------

This algorithm performs a simple numerical derivative of the values in a
sample log.

The 1st order derivative is simply: dy = (y1-y0) / (t1-t0), which is
placed in the log at t=(t0+t1)/2

Higher order derivatives are obtained by performing the equation above N
times. Since this is a simple numerical derivative, you can expect the
result to quickly get noisy at higher derivatives.

If any of the times in the logs are repeated, then those repeated time
values will be skipped, and the output derivative log will have fewer
points than the input.

Usage
-----

**Example: Taking the derivative of logs**

.. testcode:: AddLogDerivative
    
    ws = CreateSampleWorkspace()
    AddTimeSeriesLog(ws,"MyLog","2010-01-01T00:00:00",1.0,DeleteExisting=False)
    AddTimeSeriesLog(ws,"MyLog","2010-01-01T00:00:10",2.0,DeleteExisting=False)
    AddTimeSeriesLog(ws,"MyLog","2010-01-01T00:00:20",0.0,DeleteExisting=False)
    AddTimeSeriesLog(ws,"MyLog","2010-01-01T00:00:30",5.0,DeleteExisting=False)

    AddLogDerivative(ws,"MyLog",derivative=1,NewLogName="Derivative1")
    AddLogDerivative(ws,"MyLog",derivative=2,NewLogName="Derivative2")
    AddLogDerivative(ws,"MyLog",derivative=3,NewLogName="Derivative3")

    for logName in ["MyLog","Derivative1","Derivative2","Derivative3"]:
        print("Log: {}".format(logName))
        print(ws.getRun().getProperty(logName).valueAsString())


Output:

.. testoutput:: AddLogDerivative
    :options: +NORMALIZE_WHITESPACE

    Log: MyLog
    2010-Jan-01 00:00:00  1
    2010-Jan-01 00:00:10  2
    2010-Jan-01 00:00:20  0
    2010-Jan-01 00:00:30  5

    Log: Derivative1
    2010-Jan-01 00:00:05  0.1
    2010-Jan-01 00:00:15  -0.2
    2010-Jan-01 00:00:25  0.5

    Log: Derivative2
    2010-Jan-01 00:00:10  -0.03
    2010-Jan-01 00:00:20  0.07

    Log: Derivative3
    2010-Jan-01 00:00:15  0.01


.. categories::

.. sourcelink::
