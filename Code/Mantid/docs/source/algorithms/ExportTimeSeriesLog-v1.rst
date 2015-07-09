.. algorithm::

.. summary::

.. alias::

.. properties::

Description
-----------

Export a sample log, which is of type TimeSeriesProperty, in a Workspace to a MatrixWorkspace.

Output TimeSeries Log As Events
###############################

The log values can be exported as a set of events.
Each entry in the time series log will be converted to an event, , whose TOF are the log times.
It is helpful if the log series has large number of entries.
User should rebin the EventWorkspace afterwards.


Usage
-----

**Example - export a float series to a MatrixWorkspace of type Workspace2D:**

.. testcode:: ExExpTempWS2D

  # Load data
  dataws = LoadNexusProcessed(Filename="PG3_2538_2k.nxs")

  # Create a new log
  import mantid.kernel as mk
  testprop = mk.FloatTimeSeriesProperty("Temp")

  import random
  random.seed(10)
  for i in xrange(60):
      randsec = random.randint(0, 59)
      randval = random.random()*100.
      timetemp = mk.DateAndTime("2012-01-01T00:%d:%d"%(i, randsec))
      testprop.addValue(timetemp, randval)
  dataws.run().addProperty("Temp", testprop, True)

  # Run algorithm
  propws = ExportTimeSeriesLog(InputWorkspace=dataws, LogName="Temp", IsEventWorkspace=False)

  # Check
  print "Length of X = %d, Length of Y = %d." % (len(propws.readX(0)), len(propws.readY(0)))
  print "X[0]  = %.1f, Y[0]  = %.5f" % (propws.readX(0)[0], propws.readY(0)[0])
  print "X[20] = %.1f, Y[20] = %.5f" % (propws.readX(0)[20], propws.readY(0)[20])
  print "X[40] = %.1f, Y[40] = %.5f" % (propws.readX(0)[40], propws.readY(0)[40])

.. testcleanup:: ExExpTempWS2D

  DeleteWorkspace(dataws)
  DeleteWorkspace(propws)

Output:

.. testoutput:: ExExpTempWS2D

  Length of X = 60, Length of Y = 60.
  X[0]  = 26089826.0, Y[0]  = 42.88891
  X[20] = 26091001.0, Y[20] = 22.42990
  X[40] = 26092226.0, Y[40] = 39.05869


**Example - export a float series to a EventWorkspace:**

.. testcode:: ExExpTempEvent

  # Load data
  import mantid.kernel as mk
  dataws = LoadNexusProcessed(Filename="PG3_2538_2k.nxs")

  # Create a new log
  testprop = mk.FloatTimeSeriesProperty("Temp")

  import random
  random.seed(10)
  for i in xrange(60):
      randsec = random.randint(0, 59)
      randval = random.random()*100.
      timetemp = mk.DateAndTime("2012-01-01T00:%d:%d"%(i, randsec))
      testprop.addValue(timetemp, randval)
  dataws.run().addProperty("Temp", testprop, True)

  # Run algorithm
  propws = ExportTimeSeriesLog(InputWorkspace=dataws, LogName="Temp", NumberEntriesExport=40, IsEventWorkspace=True)

  # Check
  print "Length of X = %d, Length of Y = %d." % (len(propws.readX(0)), len(propws.readY(0)))
  print "X[0]  = %.1f, Y[0]  = %.5f" % (propws.readX(0)[0], propws.readY(0)[0])
  print "Number of events = %d" % (propws.getNumberEvents())

.. testcleanup:: ExExpTempEvent

  DeleteWorkspace(propws)
  DeleteWorkspace(dataws)

Output:

.. testoutput:: ExExpTempEvent

  Length of X = 2, Length of Y = 1.
  X[0]  = 26089826000000.0, Y[0]  = 1702.58055
  Number of events = 40

.. categories::

.. sourcelink::
