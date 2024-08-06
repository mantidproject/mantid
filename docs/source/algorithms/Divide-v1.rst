.. algorithm::

.. summary::

.. relatedalgorithms::

.. properties::

Description
-----------

.. |sym| replace:: \/

.. |verb| replace:: divided

.. |prep| replace:: by

.. include:: BinaryOperation.txt

.. include:: BinaryOperationMultiDivFooter.txt

Usage
-----

**Example - Divide as an Algorithm**

.. testcode:: ExDivideAsAlg

    # create histogram workspaces
    dataX1 = [0,1,2,3,4,5,6,7,8,9] # or use dataX1=range(0,10)
    dataY1 = [0,1,2,3,4,5,6,7,8] # or use dataY1=range(0,9)
    dataE1 = [0,1,2,3,4,5,6,7,8] # or use dataE1=range(0,9)
    dataX2 = [0,1,2,3,4,5,6,7,8,9] #X-values must be identical
    dataY2 = [2,2,2,2,2,2,2,2,2]
    dataE2 = [3,3,3,3,3,3,3,3,3]
    ws1 = CreateWorkspace(dataX1, dataY1, dataE1)
    ws2 = CreateWorkspace(dataX2, dataY2, dataE2)

    # perform the algorithm
    ws = Divide(ws1, ws2)

    print("The X values are: " + str(ws.readX(0)))
    print("The Y values are: " + str(ws.readY(0)))
    print("The updated Error values are: " + str(ws.readE(0)))

Output:

.. testoutput:: ExDivideAsAlg

    The X values are: [0. 1. 2. 3. 4. 5. 6. 7. 8. 9.]
    The Y values are: [0.  0.5 1.  1.5 2.  2.5 3.  3.5 4. ]
    The updated Error values are: [0.         0.90138782 1.80277564 2.70416346 3.60555128 4.50693909
     5.40832691 6.30971473 7.21110255]

**Example - Divide as an operator**

.. testcode:: ExDivideAsOpperand

    # create histogram workspaces
    dataX1 = [0,1,2,3,4,5,6,7,8,9] # or use dataX1=range(0,10)
    dataY1 = [0,1,2,3,4,5,6,7,8] # or use dataY1=range(0,9)
    dataE1 = [0,1,2,3,4,5,6,7,8] # or use dataE1=range(0,9)
    dataX2 = [0,1,2,3,4,5,6,7,8,9] #X-values must be identical
    dataY2 = [2,2,2,2,2,2,2,2,2]
    dataE2 = [3,3,3,3,3,3,3,3,3]
    ws1 = CreateWorkspace(dataX1, dataY1, dataE1)
    ws2 = CreateWorkspace(dataX2, dataY2, dataE2)

    # perform the algorithm
    ws = ws1 / ws2

    print("The X values are: " + str(ws.readX(0)))
    print("The Y values are: " + str(ws.readY(0)))
    print("The updated Error values are: " + str(ws.readE(0)))

Output:

.. testoutput:: ExDivideAsOpperand

    The X values are: [0. 1. 2. 3. 4. 5. 6. 7. 8. 9.]
    The Y values are: [0.  0.5 1.  1.5 2.  2.5 3.  3.5 4. ]
    The updated Error values are: [0.         0.90138782 1.80277564 2.70416346 3.60555128 4.50693909
     5.40832691 6.30971473 7.21110255]

**Example - Divide using in-place operator**

.. testcode:: ExDivideInPlace

    # create histogram workspaces
    dataX1 = [0,1,2,3,4,5,6,7,8,9] # or use dataX1=range(0,10)
    dataY1 = [0,1,2,3,4,5,6,7,8] # or use dataY1=range(0,9)
    dataE1 = [0,1,2,3,4,5,6,7,8] # or use dataE1=range(0,9)
    dataX2 = [0,1,2,3,4,5,6,7,8,9] #X-values must be identical
    dataY2 = [2,2,2,2,2,2,2,2,2]
    dataE2 = [3,3,3,3,3,3,3,3,3]
    ws = CreateWorkspace(dataX1, dataY1, dataE1)
    ws1 = CreateWorkspace(dataX2, dataY2, dataE2)

    # perform the algorithm
    ws /= ws1

    print("The X values are: " + str(ws.readX(0)))
    print("The Y values are: " + str(ws.readY(0)))
    print("The updated Error values are: " + str(ws.readE(0)))

Output:

.. testoutput:: ExDivideInPlace

    The X values are: [0. 1. 2. 3. 4. 5. 6. 7. 8. 9.]
    The Y values are: [0.  0.5 1.  1.5 2.  2.5 3.  3.5 4. ]
    The updated Error values are: [0.         0.90138782 1.80277564 2.70416346 3.60555128 4.50693909
     5.40832691 6.30971473 7.21110255]

**Example - Divide by a scalar**

.. testcode:: ExDivideWithSingleVal

    # create histogram workspaces
    dataX1 = [0,1,2,3,4,5,6,7,8,9] # or use dataX1=range(0,10)
    dataY1 = [0,1,2,3,4,5,6,7,8] # or use dataY1=range(0,9)
    dataE1 = [0,1,2,3,4,5,6,7,8] # or use dataE1=range(0,9)
    ws1 = CreateWorkspace(dataX1, dataY1, dataE1)

    # perform the algorithm
    ws = ws1 / 2.5

    print("The X values are: " + str(ws.readX(0)))
    print("The Y values are: " + str(ws.readY(0)))
    print("The updated Error values are: " + str(ws.readE(0)))

Output:

.. testoutput:: ExDivideWithSingleVal

    The X values are: [0. 1. 2. 3. 4. 5. 6. 7. 8. 9.]
    The Y values are: [0.  0.4 0.8 1.2 1.6 2.  2.4 2.8 3.2]
    The updated Error values are: [0.  0.4 0.8 1.2 1.6 2.  2.4 2.8 3.2]

.. categories::

.. sourcelink::
