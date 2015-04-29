.. algorithm::

.. summary::

.. alias::

.. properties::

Description
-----------

SliceMDHisto extracts a hyperslab of data from a :ref:`MDHistoWorkspace <MDHistoWorkspace>`. Beyond 
the usual input and output workspace parameters, the start and end of the
hyperslabs dimensions are required. Both  as comma separated lists with an 
entry for each dimension of the MDHistoWorkspace. 

Example: consider an input MDHistoWorkspace with dimensions 100,100,100. 
Running SliceMDHisto with parameters Start= 20,20,20 and End= 50,50,100 
will copy all the data between x: 20-50, y: 20-50, z:20-100 into the 
result MDHistoWorkspace with dimensions 30,30,80.

For a more up-to-date way of performing slices on a :ref:`MDHistoWorkspace <MDHistoWorkspace>` this see :ref:`algm-IntegrateMDHistoWorkspace`

Usage
-----

**Example - Taking an MDHisto slice**

.. testcode:: MDHistoSlice

    def outputMDDimensions(ws):
        num_dims = ws.getNumDims()
        print "Name   Bins   Min     Max"
        for dim_index in range(num_dims):
            dim = ws.getDimension(dim_index)
            print "%s      %i      %.2f   %.2f" % (dim.getName(),
                 dim.getNBins(), dim.getMinimum(), dim.getMaximum())   

    #create a test MD event workspace
    mdew = CreateMDWorkspace(Dimensions=3, Extents=[-1,1,-5,5,-9,10], 
        Names='A, B, C', Units='U, U, U')
    FakeMDEventData(mdew, PeakParams=[100000, 0, 0, 0, 1])

    #convert to a MDHisto workspace suing BinMD
    wsHisto = BinMD(mdew,AlignedDim0='A,-1,1,9',
        AlignedDim1='B,-5,5,5',
        AlignedDim2='C,-9,10,9')

    print "The original workspace"
    outputMDDimensions(wsHisto)

    #The values in start and end are the Bin numbers(staring at 0) of the dimensions
    wsOut = SliceMDHisto(wsHisto,Start=[1,2,0],End=[7,4,7])

    print "\nAfter Slicing"
    outputMDDimensions(wsOut)


.. testoutput:: MDHistoSlice

    The original workspace
    Name   Bins   Min     Max
    A      9      -1.00   1.00
    B      5      -5.00   5.00
    C      9      -9.00   10.00

    After Slicing
    Name   Bins   Min     Max
    A      6      -0.78   0.56
    B      2      -1.00   3.00
    C      7      -9.00   5.78

.. categories::
