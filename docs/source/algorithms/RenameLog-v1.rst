.. algorithm::

.. summary::

.. relatedalgorithms::

.. properties::

Description
-----------

Rename a specified sample log of type TimeSeriesProperty in a given
Workspace.


Usage
-----

**Example - Rename a sample log:**

.. testcode:: ExRenameLog

  import os
  
  nxsfilename = "HYS_11092_event.nxs"
  wsname = "HYS_11092_event"
  
  Load(Filename = nxsfilename, 
      OutputWorkspace = wsname,
      MetaDataOnly = True,
      LoadLogs = True)
  
  RenameLog(Workspace=wsname, OriginalLogName='a1b', NewLogName='A1B_New')

.. testcleanup:: ExRenameLog


Output:

.. testoutput:: ExRenameLog


.. categories::

.. sourcelink::
