.. algorithm::

.. summary::

.. relatedalgorithms::

.. properties::


Description
-----------

InvertMDDim inverts the dimensions of a MDHistoWorkspace. It copies the
data around to match the new dimensions. This algorithm is useful when
dealing with storage order issues.

Usage
-----

**Example - Inverting the axes of a MD Histo Workspace**

.. testcode:: InvertMDExample

    def outputMDDimensions(ws):
        num_dims = ws.getNumDims()
        print("Name   Bins   Min     Max")
        for dim_index in range(num_dims):
            dim = ws.getDimension(dim_index)
            print("{}      {}      {:.2f}   {:.2f}".format(
                  dim.name, dim.getNBins(), dim.getMinimum(), dim.getMaximum()))

    #create a test MD event workspace
    mdew = CreateMDWorkspace(Dimensions=3, Extents=[-1,1,-5,5,-9,10], 
        Names='A, B, C', Units='U, U, U')
    FakeMDEventData(mdew, PeakParams=[100000, 0, 0, 0, 1])

    #convert to a MDHisto workspace suing BinMD
    wsHisto = BinMD(mdew,AlignedDim0='A,-1,1,9',
        AlignedDim1='B,-5,5,5',
        AlignedDim2='C,-9,10,9')

    print("The original workspace")
    outputMDDimensions(wsHisto)

    wsInverted = InvertMDDim(wsHisto)

    print("\nInvertMDDim reverses the order of the dimensions")
    outputMDDimensions(wsInverted)

.. testoutput:: InvertMDExample

    The original workspace
    Name   Bins   Min     Max
    A      9      -1.00   1.00
    B      5      -5.00   5.00
    C      9      -9.00   10.00

    InvertMDDim reverses the order of the dimensions
    Name   Bins   Min     Max
    C      9      -9.00   10.00
    B      5      -5.00   5.00
    A      9      -1.00   1.00



.. categories::

.. sourcelink::
