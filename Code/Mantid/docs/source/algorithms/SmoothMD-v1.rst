
.. algorithm::

.. summary::

.. alias::

.. properties::

Description
-----------

Provides smoothing of :ref:`MDHistoWorkspaces <MDHistoWorkspace>`__ in n-dimensions. The WidthVector relates to the number of pixels to include in the width for each dimension. *WidthVector* **must contain entries that are odd numbers**.


Usage
-----

**Example - SmoothMD**

.. testcode:: SmoothMDExample


   ws = CreateMDWorkspace(Dimensions=2, Extents=[-10,10,-10,10], Names='A,B', Units='U,U')
   FakeMDEventData(InputWorkspace=ws, PeakParams='100000,-5,0,1')
   FakeMDEventData(InputWorkspace=ws, PeakParams='100000,5,0,1')
   histogram = BinMD(InputWorkspace='a', AlignedDim0='A,-10,10,100', AlignedDim1='B,-10,10,100', OutputExtents='-10,10,-10,10,-10,10', OutputBins='10,10,10')
   #plotSlice(histogram)
   smoothed = SmoothMD(InputWorkspace=histogram, WidthVector=5, Function='Hat')
   #plotSlice(smoothed)

   # Create a host workspace
   ws = CreateWorkspace(DataX=range(0,3), DataY=(0,2))
   or
   ws = CreateSampleWorkspace()

   wsOut = SmoothMD()

   # Print the result. Displaying in the SliceViewer would be best
   print "The output workspace has %i points" % smoothed.getNPoints()

Output:

.. testoutput:: SmoothMDExample

  The output workspace has ?? points

.. categories::

