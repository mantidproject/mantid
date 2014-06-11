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

To add logs that vary over time (Time Series Logs) use :ref:`algm-AddTimeSeriesLog`.

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


.. categories::