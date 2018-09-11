.. algorithm::

.. summary::

.. relatedalgorithms::

.. properties::

Description
-----------

ProjectMD reduces the dimensionality of a MHDistoWorkspace by summing it
along a specified dimension. Example: you have a 3D MDHistoWorkspace
with X,Y,TOF. You sum along Z (TOF) and the result is a 2D workspace X,Y
which gives you a detector image.

Besides the obvious input and output workspaces you have to specify the
dimension along which you wish to sum. The following code is used:

X
    Dimension 0
Y
    Dimension 1
Z
    Dimension 2
K
    Dimension 3

The summation range also has to be specified. This is in indices into
the appropriate axis.

Usage
-----

**Example - Summing a couple of dimensions**

.. testcode:: ProjectMD

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

    wsProjected1 = ProjectMD(wsHisto,ProjectDirection="Z")
    print("\nAfter the workspace has been summed in the Z direction")
    outputMDDimensions(wsProjected1)

    wsProjected2 = ProjectMD(wsProjected1,ProjectDirection="X")
    print("\nAfter the workspace has been also been summed in the X direction")
    outputMDDimensions(wsProjected2)

.. testoutput:: ProjectMD

    The original workspace
    Name   Bins   Min     Max
    A      9      -1.00   1.00
    B      5      -5.00   5.00
    C      9      -9.00   10.00

    After the workspace has been summed in the Z direction
    Name   Bins   Min     Max
    A      9      -1.00   1.00
    B      5      -5.00   5.00

    After the workspace has been also been summed in the X direction
    Name   Bins   Min     Max
    B      5      -5.00   5.00



.. categories::

.. sourcelink::
