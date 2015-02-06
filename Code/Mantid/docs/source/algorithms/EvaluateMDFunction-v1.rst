.. algorithm::

.. summary::

.. alias::

.. properties::

Description
-----------

The algorithm will use the axes of the input workspace to evaluate a functions on them 
and store the result in the output workspace.

This is a plot of the evaluated function from the usage example below.

.. figure:: /images/Function3D.png
   :alt: Function3D.png


Usage
-----

.. testcode::

    # Create an empty 3D histo workspace.
    n = 50 * 50 * 50
    ws=CreateMDHistoWorkspace(Dimensionality=3, Extents='-1,1,-1,1, -1,1',\
        SignalInput = [0.0] * n, ErrorInput = [1.0] * n,\
        NumberOfBins='50,50,50',Names='Dim1,Dim2,Dim3',Units='MomentumTransfer,MomentumTransfer,MomentumTransfer')

    # Define a function
    function = 'name=UserFunctionMD,Formula=1.0/(1.0 + 100*(0.5 - x^2 - y^2 -z^2)^2)'

    # Evaluate the function on the created workspace
    out = EvaluateMDFunction(ws,function)

    # Check the result workspace
    print out.getNumDims()
    print out.getXDimension().getName()
    print out.getYDimension().getName()
    print out.getZDimension().getName()
    
    
Output
######

.. testoutput::

  3
  Dim1
  Dim2
  Dim3

.. categories::
