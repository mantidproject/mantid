.. algorithm::

.. summary::

.. relatedalgorithms::

.. properties::

Description
-----------

Removes all logs from workspace, except those that are specified

Usage
-----
.. testcode:: RemoveLogs

   # create some workspace with an instrument
   ws = CreateSampleWorkspace()
   print("Original logs:  {}".format(ws.run().keys()))
   
   # remove logs, but keep some
   RemoveLogs(ws,KeepLogs="run_start, run_title")
   print("Logs left:  {}".format(ws.run().keys()))
   
   # delete all logs
   RemoveLogs(ws)
   print("Logs left (should be empty):  {}".format(ws.run().keys()))

.. testcleanup:: RemoveLogs

   DeleteWorkspace(ws)
   
Output:

.. testoutput:: RemoveLogs

   Original logs:  ['run_title', 'start_time', 'end_time', 'run_start', 'run_end']
   Logs left:  ['run_title', 'run_start']
   Logs left (should be empty):  []
   
.. categories::

.. sourcelink::
