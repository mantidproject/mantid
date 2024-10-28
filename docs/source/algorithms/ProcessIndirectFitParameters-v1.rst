
.. algorithm::

.. summary::

.. relatedalgorithms::

.. properties::

Description
-----------

An Algorithm designed to allow for the TableWorkspace output of the
PlotPeakByLogValue algorithm to be transformed into a Matrix workspace
based on the desired parameters.

Workflow
--------

.. diagram:: ProcessIndirectFitParameters-v1_wkflw.dot


Usage
-----

**Example - ProcessIndirectFitParameters**

.. testcode:: ProcessIndirectFitParametersExample

   # Create a host workspace
   tws = WorkspaceFactory.createTable()
   tws.addColumn("double", "A")
   tws.addColumn("double", "B")
   tws.addColumn("double", "B_Err")
   tws.addColumn("double", "D")
   tws.addRow([1,2,3,4])
   tws.addRow([5,6,7,8])
   tws.addRow([9,0,1,2])
   tws.addRow([0,0,0,1])

   # Add to Mantid Workspace list
   mtd.addOrReplace("TableWs",tws)
   wsName = "outputWorkspace"

   # "D" is not included in the algorithm params list
   ProcessIndirectFitParameters(tws, 'A', "B", OutputWorkspace=wsName)

   wsOut = mtd[wsName]

   # Print the result
   print("{} is a {} and the Y values are:".format(wsOut, wsOut.id()))
   print(wsOut.readY(0))

Output:

.. testoutput:: ProcessIndirectFitParametersExample
    :options: +NORMALIZE_WHITESPACE

    outputWorkspace is a Workspace2D and the Y values are:
    [2. 6. 0. 0.]

.. categories::

.. sourcelink::
