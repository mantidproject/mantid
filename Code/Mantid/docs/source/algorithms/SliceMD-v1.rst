.. algorithm::

.. summary::

.. alias::

.. properties::

Description
-----------

Algorithm that can take a slice out of an original
`MDEventWorkspace <http://www.mantidproject.org/MDEventWorkspace>`_ while preserving all the events
contained therein.

It uses the same parameters as :ref:`algm-BinMD` to determine a
transformation to make from input->output workspace. The difference is
that :ref:`algm-BinMD` sums events in a regular grid whereas SliceMD
moves the events into the output workspace, which boxes itself.

Please see :ref:`algm-BinMD` for a detailed description of the
parameters.

Axis-Aligned Slice
##################

Events outside the range of the slice are dropped. The new output
MDEventWorkspace's dimensions only extend as far as the limit specified.

Non-Axis-Aligned Slice
######################

The coordinates of each event are transformed according to the new basis
vectors, and placed in the output MDEventWorkspace. The dimensions of
the output workspace are along the basis vectors specified.

Splitting Parameters
####################

The **OutputBins** parameter is interpreted as the "SplitInto" parameter
for each dimension. For instance, if you want the output workspace to
split in 2x2x2, you would specify OutputBins="2,2,2".

For 1D slices, it may make sense to specify a SplitInto parameter of 1
in every other dimension - that way, boxes will only be split along the
1D direction.

Slicing a MDHistoWorkspace
##########################

It is possible to slice a :ref:`MDHistoWorkspace <MDHistoWorkspace>`. Each
MDHistoWorkspace holds a reference to the
`MDEventWorkspace <http://www.mantidproject.org/MDEventWorkspace>`_ that created it, as well as the
coordinate transformation that was used.

In this case, the algorithm is executed on the original
MDEventWorkspace, in the proper coordinates. Perhaps surprisingly, the
output of SliceMD on a MDHistoWorkspace is a MDEventWorkspace!

Only the non-axis aligned slice method can be performed on a
MDHistoWorkspace! Of course, your basis vectors can be aligned with the
dimensions, which is equivalent.

Usage
-----

**Example - Axis aligned binning**

.. testcode:: SliceMDExample

   mdew = CreateMDWorkspace(Dimensions=3, Extents=[-10,10,-10,10,-10,10], Names='A, B, C', Units='U, U, U')
   FakeMDEventData(mdew, PeakParams=[100000, 0, 0, 0, 1])

   # Slice out all C > 0
   sliced = SliceMD(InputWorkspace=mdew, AxisAligned=True, AlignedDim0='A,-10,10,10', AlignedDim1='B, -10, 10, 10', AlignedDim2='C,-10, 0, 10',)

   dim0=sliced.getDimension(0)
   dim1=sliced.getDimension(1)
   dim2=sliced.getDimension(2)

   print "A extents", dim0.getMinimum(), dim0.getMaximum()
   print "B extents", dim1.getMinimum(), dim1.getMaximum()
   print "C extents", dim2.getMinimum(), dim2.getMaximum()
   print "Original MDEW should have 2*N events in sliced. We get a factor of : ",  int( mdew.getNEvents() / sliced.getNEvents()  )

Output:

.. testoutput:: SliceMDExample

   A extents -10.0 10.0
   B extents -10.0 10.0
   C extents -10.0 0.0
   Original MDEW should have 2*N events in sliced. We get a factor of :  2

**Example - Non-axis aligned binning**

.. testcode:: SliceMDExampleComplex

   import numpy

   # Create a host workspace
   mdew = CreateMDWorkspace(Dimensions=2, Extents=[-10,10,-10,10], Names='A, B', Units='U, U')
   # Add a peak at -5,-5
   FakeMDEventData(mdew, PeakParams=[100000, -5, -5, 1]) 
   # Add a peak at 5, 5
   FakeMDEventData(mdew, PeakParams=[100000, 5, 5, 1])
   # Slice at 45 degrees. BasisVector0 now runs through both peaks
   sliced = SliceMD(InputWorkspace=mdew, AxisAligned=False, BasisVector0='X, sqrt(2*U^2), 1,1', BasisVector1='Y, sqrt(2*U^2),-1,1',OutputBins=[100,1], OutputExtents=[-10,10,-10,10])

   # Bin it to gather statistics
   binned = BinMD(sliced, AxisAligned=True,  AlignedDim0='X, 0,10, 100', AlignedDim1='Y,-10,10,1')
   signals = binned.getSignalArray()

   dim_x = binned.getDimension(0)
   x_axis= numpy.linspace(dim_x.getMinimum(), dim_x.getMaximum(), dim_x.getNBins())
   x_at_max = x_axis[numpy.argmax(signals)]
   print "Brighest region should be at x  = sqrt( 2*5*5 ) = 7.07. Found to be: ", "{0:.2f}".format(x_at_max)

Output:

.. testoutput:: SliceMDExampleComplex

   Brighest region should be at x  = sqrt( 2*5*5 ) = 7.07. Found to be:  7.07

.. categories::




