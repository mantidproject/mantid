.. algorithm::

.. summary::

.. relatedalgorithms::

.. properties::

Description
-----------

This algorithm takes a continuous block of data from an input workspace and places this data into a destination
workspace, replacing the data in the destination workspace at which the insertion is specified.  The block of
data to be used is specified by entering spectra indices and x indices within the input workspace into the algorithm.
The insertion location is then specified by an InsertionYIndex and InsertionXIndex within the destination workspace.

Note that this algorithm will replace not only the Y values, but also the E values within the destination workspace. The
original input workspace remains unchanged.

Usage
-----

**Example - Copy Data Range within one workspace over to another**

.. testcode:: ExCopyDataRangeSimple

   # Create two workspaces
   demo_ws1 = CreateWorkspace(DataX=[1,2,3], DataY=[1,2,3], DataE=[1,1,1], NSpec=1)
   demo_ws2 = CreateWorkspace(DataX=[1,2,3], DataY=[4,5,6], DataE=[2,2,2], NSpec=1)

   # Copy some of the data from the input workspace over to the destination workspace
   output = CopyDataRange(InputWorkspace=demo_ws1, DestWorkspace=demo_ws2, StartWorkspaceIndex=0, EndWorkspaceIndex=0,
                          XMin=1, XMax=2, InsertionYIndex=0, InsertionXIndex=0)

   # Display the output workpace data
   print("Output x data: {0}".format(output.readX(0)))
   print("Output y data: {0}".format(output.readY(0)))
   print("Output e data: {0}".format(output.readE(0)))

Output:

.. testoutput:: ExCopyDataRangeSimple

   Output x data: [1. 2. 3.]
   Output y data: [1. 2. 6.]
   Output e data: [1. 1. 2.]

.. categories::

.. sourcelink::
