.. algorithm::

.. summary::

.. alias::

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
    out = EvaluateFunction('name=ExpDecay,Height=56','ws_1')

**Example - 2D function and a MD workspace.**

.. testcode:: 2DFunction

    # Define a workspace
    nx = 100
    ny = 100
    s = [0] * nx * ny
    e = [0] * nx * ny
    ws = CreateMDHistoWorkspace(Dimensionality=2, Extents=[-10,10,-10,10], Names="x,y", Units="U,V",
        NumberOfBins='%s,%s' % (nx,ny),
        SignalInput=s,ErrorInput=e)
    # Evaluate a function
    out = EvaluateFunction('name=UserFunctionMD,Formula=sin(x)*sin(y)','ws')

.. categories::
