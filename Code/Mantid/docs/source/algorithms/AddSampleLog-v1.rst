.. algorithm::

.. summary::

.. alias::

.. properties::

Description
-----------

Workspaces contain information in logs. Often these detail what happened
to the sample during the experiment. This algorithm allows one named log
to be entered.

The log can be either a String, a Number, or a Number Series. If you
select Number Series, the workspace start time will be used as the time
of the log entry, and the number in the text used as the (only) value.

If the LogText contains a numeric value, the created log will be of
integer type if an integer is passed and floating point (double)
otherwise. This applies to both the Number & Number Series options.

Usage
-----

**Example - Add Sample Logs in different ways**

.. testcode:: AddSampleLogExample

   # Create a host workspace
   demo_ws = CreateWorkspace(DataX=range(0,3), DataY=(0,2))

   # Add sample logs
   AddSampleLog(Workspace=demo_ws, LogName='x', LogText='hello world', LogType='String')
   AddSampleLog(Workspace=demo_ws, LogName='y', LogText='1', LogType='Number')
   AddSampleLog(Workspace=demo_ws, LogName='z', LogText='2', LogType='Number Series')

   # Fetch the generated logs
   run = demo_ws.getRun()
   log_x = run.getLogData('x')
   log_y = run.getLogData('y')
   log_z = run.getLogData('z')

   # Print the log values
   print log_x.value
   print log_y.value
   print log_z.value

Output:

.. testoutput:: AddSampleLogExample 

  hello world
  1
  [2]

**Example - How to add a Time Series Log**

Strictly, this does not use :ref:`algm-AddSampleLog` at all.

.. testcode:: AddTimeSeriesLog  

   from mantid.kernel import FloatTimeSeriesProperty, DateAndTime

   # Create a host workspace
   ws = CreateWorkspace(DataX=[0,1,2], DataY=[1,1])		
   # Create a time series property. This holds all the data we want
   tsp = FloatTimeSeriesProperty('x')
   # Create an absolute start time
   run_start = DateAndTime("2000-01-01T00:00:00")
   # Add some values to our new log
   for i in range(10):
	   tsp.addValue(run_start + int(i *1e9), float(i))
   # Add the property to the workspace
   ws.mutableRun().addProperty('x', tsp, True)
   # Fetch it off the workspace again just to show that it was correctly attached in the first place
   log = ws.run().getLogData('x')
   # Print the entries
   print log.value
   print log.times

.. testcleanup:: AddTimeSeriesLog
   DeleteWorkspace('ws')
   
Output:

.. testoutput:: AddTimeSeriesLog 

  [ 0.  1.  2.  3.  4.  5.  6.  7.  8.  9.]
  [2000-Jan-01 00:00:00,2000-Jan-01 00:00:01,2000-Jan-01 00:00:02,2000-Jan-01 00:00:03,2000-Jan-01 00:00:04,2000-Jan-01 00:00:05,2000-Jan-01 00:00:06,2000-Jan-01 00:00:07,2000-Jan-01 00:00:08,2000-Jan-01 00:00:09]