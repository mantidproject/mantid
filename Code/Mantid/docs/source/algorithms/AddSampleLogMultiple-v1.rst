.. algorithm::

.. summary::

.. alias::

.. properties::

Description
-----------

This algorithm provides a way of adding multiple sample log enteries to a
workspace at once by making multiple calls to the :ref:`algm-AddSampleLog`
algorithm.

Typically this is for use in workflow algorithms and scripts.

Usage
-----

**Example - Add multiple sample logs**

.. testcode:: AddSampleLogMultipleExample

   # Create a host workspace
   demo_ws = CreateWorkspace(DataX=range(0,3), DataY=(0,2))

   # Add sample logs
   log_names = ['x', 'y', 'z']
   log_values = ['test', 5, 1.6e-7]
   AddSampleLogMultiple(Workspace=demo_ws,
                        LogNames=log_names,
                        LogValues=log_values)

   # Print the log values
   run = demo_ws.getRun()
   print run.getLogData('x').value
   print run.getLogData('y').value
   print run.getLogData('z').value

Output:

.. testoutput:: AddSampleLogMultipleExample

    test
    5
    1.6e-07

.. categories::
