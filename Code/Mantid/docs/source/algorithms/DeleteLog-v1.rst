.. algorithm::

.. summary::

.. alias::

.. properties::

Description
-----------

Removes a named log from the run attached to the input workspace. If the
log does not exist then the algorithm simply emits a warning and does
not fail.


Usage
-----

**Example - Deleting a log**

.. testcode:: DeleteLogExample

   # Create a host workspace
   demo_ws = CreateWorkspace(DataX=range(0,3), DataY=(0,2))

   # Add  a sample log
   AddSampleLog(Workspace=demo_ws, LogName='my_log', LogText='1', LogType='Number')
   print 'Log is present before deletion: ', demo_ws.getRun().hasProperty('my_log')

   # Now delete it
   DeleteLog(demo_ws,'my_log')
   print 'Log is present after deletion: ', demo_ws.getRun().hasProperty('my_log')


Output:

.. testoutput:: DeleteLogExample 

    Log is present before deletion:  True
    Log is present after deletion:  False


.. categories::
