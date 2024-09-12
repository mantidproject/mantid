.. algorithm::

.. summary::

.. relatedalgorithms::

.. properties::

Description
-----------

Clones the input :ref:`Matrix Workspaces <MatrixWorkspace>` and orders the
x-axis in an ascending or descending fashion. Ensures that the y-axis and error data as well as optional Dx data
are sorted in a consistent way with the x-axis.

This algorithm is for use with either point data based workspaces or histogram based workspaces. It is
particularly suitable for reformatting workspaces loaded via
:ref:`LoadAscii <algm-LoadAscii>`.

Usage
-----
.. testcode:: SortXAxis

    #Import requirements
    import numpy as np

    # Create the workspace
    dataX = [2., 1., 3.]
    dataY = [2., 1., 3.]
    dataE = [2., 1., 3.]
    ws = CreateWorkspace(DataX=dataX,DataY=dataY,DataE=dataE,UnitX='TOF',Distribution=True)

    # Print out the "Unordered X Axis"
    print("Unordered Print")
    print(ws.readX(0))
    print(ws.readY(0))
    print(ws.readE(0))

    # Sort the X Axis in a Descending fashion
    ws = SortXAxis(InputWorkspace='ws', Ordering='Descending')
    print("In order print: Descending")
    print(ws.readX(0))
    print(ws.readY(0))
    print(ws.readE(0))

    # Sort the X Axis in a Ascending fashion
    ws = SortXAxis(InputWorkspace='ws', Ordering='Ascending')
    print("In order print: Ascending")
    print(ws.readX(0))
    print(ws.readY(0))
    print(ws.readE(0))

.. testcleanup:: SortXAxis

    DeleteWorkspace(ws)

Output:

.. testoutput:: SortXAxis

    Unordered Print
    [2. 1. 3.]
    [2. 1. 3.]
    [2. 1. 3.]
    In order print: Descending
    [3. 2. 1.]
    [3. 2. 1.]
    [3. 2. 1.]
    In order print: Ascending
    [1. 2. 3.]
    [1. 2. 3.]
    [1. 2. 3.]

.. categories::

.. sourcelink::
