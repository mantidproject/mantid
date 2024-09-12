.. algorithm::

.. summary::

.. relatedalgorithms::

.. properties::

Description
-----------

MDHistoToWorkspace2D flattens a MDHistoWorkspace into a Workspace2D. It
can process MDHistoWorkspaces of any dimensionality. The last dimension
of the MDHistoWorkspace becomes the spectra length. Flattening happens
such that the first dimension of the MDHistoWorkspace is the slowest
varying, the second the second slowest varying and so on.

This tool is useful as many algorithms in Mantid only apply to
Workspace2D. After conversion with MDHistoToWorkspace2D such algorithms
can also be applied to MD data.

Usage
-----

**Example - Flatten a small workspace:**

.. testcode:: ExMDHistoToWorkspace2D

   # Create input workspace
   CreateMDWorkspace(Dimensions=3, Extents='-10,10,-10,10,-10,10', Names='A,B,C', Units='U,U,U', OutputWorkspace='demo')
   FakeMDEventData(InputWorkspace='demo', PeakParams='32,0,0,0,1')
   input = BinMD(InputWorkspace='demo', AlignedDim0='A,-2,2,4', AlignedDim1='B,-2,2,4', AlignedDim2='C,-2,2,4')

   # Run the algorithm
   output = MDHistoToWorkspace2D(InputWorkspace='input')

   # print 6th group of 4 bins in both input and output workspaces
   print("part of MD workspace {}".format(input.getSignalArray()[1,1]))
   print("corresponding part of 2D workspace {}".format(output.dataY(5) ))

Output:

.. testoutput:: ExMDHistoToWorkspace2D

   part of MD workspace [0. 5. 4. 0.]
   corresponding part of 2D workspace [0. 5. 4. 0.]

.. categories::

.. sourcelink::
