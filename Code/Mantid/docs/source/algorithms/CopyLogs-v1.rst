.. algorithm::

.. summary::

.. alias::

.. properties::

Description
-----------

The algorithm will copy the sample logs in the input workspace to the
the output workspace using one of three merge strategies.

-  MergeReplaceExisting: Default option. Copy logs from the input
   workspace to the output workspace and replace any existing logs with
   the same name.
-  MergeKeepExisting: Keep the existing logs in the output workspace and
   don't modify them, but append any new ones from the input workspace.
   Note that this will not concatenate values or ranges. The algorithm will
   choose to keep the value of any the log already present in the output
   workspace, leaving it unchanged.

-  WipeExisting: Dump any logs that are in the output workspace and
   replace them with the logs from the input workspace.

Usage
-----

**Example - Copy Logs with default merge strategy**

.. testcode:: ExCopyLogsSimple

   # Create two workspaces
   demo_ws1 = CreateWorkspace(DataX=range(0,3), DataY=(0,2))
   demo_ws2 = CreateWorkspace(DataX=range(0,3), DataY=(0,2))

   # Add logs to first workspace 
   AddSampleLog(Workspace=demo_ws1, LogName='x', LogText='hello world', LogType='String')
   AddSampleLog(Workspace=demo_ws1, LogName='y', LogText='1', LogType='Number')
   AddSampleLog(Workspace=demo_ws1, LogName='z', LogText='2', LogType='Number Series')

   # Add logs to second workspace 
   AddSampleLog(Workspace=demo_ws2, LogName='x', LogText='hello universe', LogType='String')
   AddSampleLog(Workspace=demo_ws2, LogName='w', LogText='3', LogType='Number')

   # Fetch the generated logs
   run1 = demo_ws1.getRun()
   log_x1 = run1.getLogData('x')
   log_y = run1.getLogData('y')
   log_z = run1.getLogData('z')
   run2 = demo_ws2.getRun()
   log_x2 = run2.getLogData('x')
   log_w = run2.getLogData('w')

   # Print the log values
   print "Before CopyLog"
   print "1st workspace log values x =",log_x1.value,", y =", log_y.value,", z =", log_z.value 
   print "2nd workspace log values x =",log_x2.value,", w =", log_w.value

   # Copy logs of 1st workspace to 2nd workspace 
   CopyLogs( demo_ws1, demo_ws2)

   # Fetch the new logs
   run1 = demo_ws1.getRun()
   log_x1 = run1.getLogData('x')
   log_y = run1.getLogData('y')
   log_z = run1.getLogData('z')
   run2 = demo_ws2.getRun()
   log_x2 = run2.getLogData('x')
   log_w = run2.getLogData('w')
   log_y2 = run2.getLogData('y')
   log_z2 = run2.getLogData('z')

   # Print the log values
   print "After CopyLog"
   print "1st workspace log values x =",log_x1.value,", y =", log_y.value,", z =", log_z.value 
   print "2nd workspace log values x =",log_x2.value,", w =", log_w.value,", y =", log_y2.value,", z =", log_z2.value 

Output:

.. testoutput:: ExCopyLogsSimple 

   Before CopyLog
   1st workspace log values x = hello world , y = 1 , z = [2]
   2nd workspace log values x = hello universe , w = 3
   After CopyLog
   1st workspace log values x = hello world , y = 1 , z = [2]
   2nd workspace log values x = hello world , w = 3 , y = 1 , z = [2]


**Example - Copy Logs with MergeKeepExisting merge strategy**

.. testcode:: ExCopyLogsKeepExisting

   # Create two workspaces
   demo_ws1 = CreateWorkspace(DataX=range(0,3), DataY=(0,2))
   demo_ws2 = CreateWorkspace(DataX=range(0,3), DataY=(0,2))

   # Add logs to first workspace 
   AddSampleLog(Workspace=demo_ws1, LogName='x', LogText='hello world', LogType='String')
   AddSampleLog(Workspace=demo_ws1, LogName='y', LogText='1', LogType='Number')
   AddSampleLog(Workspace=demo_ws1, LogName='z', LogText='2', LogType='Number Series')

   # Add logs to second workspace 
   AddSampleLog(Workspace=demo_ws2, LogName='x', LogText='hello universe', LogType='String')
   AddSampleLog(Workspace=demo_ws2, LogName='w', LogText='3', LogType='Number')

   # Fetch the generated logs
   run1 = demo_ws1.getRun()
   log_x1 = run1.getLogData('x')
   log_y = run1.getLogData('y')
   log_z = run1.getLogData('z')
   run2 = demo_ws2.getRun()
   log_x2 = run2.getLogData('x')
   log_w = run2.getLogData('w')

   # Print the log values
   print "Before CopyLog"
   print "1st workspace log values x =",log_x1.value,", y =", log_y.value,", z =", log_z.value 
   print "2nd workspace log values x =",log_x2.value,", w =", log_w.value

   # Copy logs of 1st workspace to 2nd workspace 
   CopyLogs( demo_ws1, demo_ws2, MergeStrategy='MergeKeepExisting')

   # Fetch the new logs
   run1 = demo_ws1.getRun()
   log_x1 = run1.getLogData('x')
   log_y = run1.getLogData('y')
   log_z = run1.getLogData('z')
   run2 = demo_ws2.getRun()
   log_x2 = run2.getLogData('x')
   log_w = run2.getLogData('w')
   log_y2 = run2.getLogData('y')
   log_z2 = run2.getLogData('z')

   # Print the log values
   print "After CopyLog"
   print "1st workspace log values x =",log_x1.value,", y =", log_y.value,", z =", log_z.value 
   print "2nd workspace log values x =",log_x2.value,", w =", log_w.value,", y =", log_y2.value,", z =", log_z2.value 

Output:

.. testoutput:: ExCopyLogsKeepExisting 

   Before CopyLog
   1st workspace log values x = hello world , y = 1 , z = [2]
   2nd workspace log values x = hello universe , w = 3
   After CopyLog
   1st workspace log values x = hello world , y = 1 , z = [2]
   2nd workspace log values x = hello universe , w = 3 , y = 1 , z = [2]


**Example - Copy Logs with WipeExisting merge strategy**

.. testcode:: ExCopyLogsWipeExisting

   # Create two workspaces
   demo_ws1 = CreateWorkspace(DataX=range(0,3), DataY=(0,2))
   demo_ws2 = CreateWorkspace(DataX=range(0,3), DataY=(0,2))

   # Add sample logs first workspace
   AddSampleLog(Workspace=demo_ws1, LogName='x', LogText='hello world', LogType='String')
   AddSampleLog(Workspace=demo_ws1, LogName='y', LogText='1', LogType='Number')
   AddSampleLog(Workspace=demo_ws1, LogName='z', LogText='2', LogType='Number Series')

   # Add sample logs second workspace
   AddSampleLog(Workspace=demo_ws2, LogName='x', LogText='hello universe', LogType='String')
   AddSampleLog(Workspace=demo_ws2, LogName='w', LogText='3', LogType='Number')

   # Fetch the generated logs
   run1 = demo_ws1.getRun()
   log_x1 = run1.getLogData('x')
   log_y = run1.getLogData('y')
   log_z = run1.getLogData('z')
   run2 = demo_ws2.getRun()
   log_x2 = run2.getLogData('x')
   log_w = run2.getLogData('w')

   # Print the log values
   print "Before CopyLog"
   print "1st workspace log values x =",log_x1.value,", y =", log_y.value,", z =", log_z.value
   print "2nd workspace log values x =",log_x2.value,", w =", log_w.value

   # Copy logs of 1st workspace to 2nd workspace
   CopyLogs( demo_ws1, demo_ws2, MergeStrategy='WipeExisting')

   # Fetch the new logs
   run1 = demo_ws1.getRun()
   log_x1 = run1.getLogData('x')
   log_y = run1.getLogData('y')
   log_z = run1.getLogData('z')
   run2 = demo_ws2.getRun()
   log_x2 = run2.getLogData('x')
   log_y2 = run2.getLogData('y')
   log_z2 = run2.getLogData('z')

   # Print the log values
   print "After CopyLog"
   print "1st workspace log values x =",log_x1.value,", y =", log_y.value,", z =", log_z.value
   print "2nd workspace log values x =",log_x2.value,", y =", log_y2.value,", z =", log_z2.value


Output:

.. testoutput:: ExCopyLogsWipeExisting

   Before CopyLog
   1st workspace log values x = hello world , y = 1 , z = [2]
   2nd workspace log values x = hello universe , w = 3
   After CopyLog
   1st workspace log values x = hello world , y = 1 , z = [2]
   2nd workspace log values x = hello world , y = 1 , z = [2]

.. categories::
