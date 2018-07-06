.. algorithm::

.. summary::

.. relatedalgorithms::

.. properties::

Description
-----------

The algorithm will use the axes of the input workspace to evaluate a functions on them 
and store the result in the output workspace.

Usage
-----

**Example - 1D function and a matrix workspace.**

.. testcode:: 1DFunction

    # Load a workspace
    ws = Load('MUSR00015189')
    # Evaluate a function
    EvaluateFunction('name=ExpDecay,Height=56', 'ws_1', StartX=0, EndX=30, OutputWorkspace='out')

**Example - 2D function and a MD workspace.**

.. testcode:: 2DFunction

    # Create an empty 2D workspace (100 x 100)
    nx = 100
    ny = 100
    ws = CreateMDHistoWorkspace(Dimensionality=2, Extents=[-10,10,-10,10], Names="x,y", Units="U,V",
        NumberOfBins='%s,%s' % (nx,ny),
        SignalInput=[0] * nx * ny, ErrorInput=[0] * nx * ny)
    # Evaluate a function
    EvaluateFunction('name=UserFunctionMD,Formula=sin(x)*sin(y)','ws', OutputWorkspace='out')

.. categories::

.. sourcelink::
