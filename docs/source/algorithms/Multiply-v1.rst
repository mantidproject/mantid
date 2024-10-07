.. algorithm::

.. summary::

.. relatedalgorithms::

.. properties::

Description
-----------

.. |sym| replace:: \*

.. |verb| replace:: multiplied

.. |prep| replace:: by

.. include:: BinaryOperation.txt

.. include:: BinaryOperationMultiDivFooter.txt

Usage
-----

**Example - Multiply as an algorithm**

.. testcode:: ExMultiplyAsAlg

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
    ws = Multiply(ws1, ws2)

    print("The X values are: {}".format(ws.readX(0)))
    print("The Y values are: {}".format(ws.readY(0)))
    print("The updated Error values are: {}".format(ws.readE(0)))

Output:

.. testoutput:: ExMultiplyAsAlg

    The X values are: [0. 1. 2. 3. 4. 5. 6. 7. 8. 9.]
    The Y values are: [ 0.  2.  4.  6.  8. 10. 12. 14. 16.]
    The updated Error values are: [ 0.          3.60555128  7.21110255 10.81665383 14.4222051  18.02775638
     21.63330765 25.23885893 28.8444102 ]

**Example - Multiply as an operator**

.. testcode:: ExMultiplyAsOpperand

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
    ws = ws1 * ws2

    print("The X values are: {}".format(ws.readX(0)))
    print("The Y values are: {}".format(ws.readY(0)))
    print("The updated Error values are: {}".format(ws.readE(0)))

Output:

.. testoutput:: ExMultiplyAsOpperand

    The X values are: [0. 1. 2. 3. 4. 5. 6. 7. 8. 9.]
    The Y values are: [ 0.  2.  4.  6.  8. 10. 12. 14. 16.]
    The updated Error values are: [ 0.          3.60555128  7.21110255 10.81665383 14.4222051  18.02775638
     21.63330765 25.23885893 28.8444102 ]

**Example - Multiply using in-place operator**

.. testcode:: ExMultiplyInPlace

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
    ws *= ws1

    print("The X values are: {}".format(ws.readX(0)))
    print("The Y values are: {}".format(ws.readY(0)))
    print("The updated Error values are: {}".format(ws.readE(0)))

Output:

.. testoutput:: ExMultiplyInPlace

    The X values are: [0. 1. 2. 3. 4. 5. 6. 7. 8. 9.]
    The Y values are: [ 0.  2.  4.  6.  8. 10. 12. 14. 16.]
    The updated Error values are: [ 0.          3.60555128  7.21110255 10.81665383 14.4222051  18.02775638
     21.63330765 25.23885893 28.8444102 ]

**Example - Multiply with a Scalar**

.. testcode:: ExMultiplyWithSingleVal

    # create histogram workspaces
    dataX1 = [0,1,2,3,4,5,6,7,8,9] # or use dataX1=range(0,10)
    dataY1 = [0,1,2,3,4,5,6,7,8] # or use dataY1=range(0,9)
    dataE1 = [0,1,2,3,4,5,6,7,8] # or use dataE1=range(0,9)
    ws1 = CreateWorkspace(dataX1, dataY1, dataE1)

    # perform the algorithm
    ws = ws1 * 2.5

    print("The X values are: {}".format(ws.readX(0)))
    print("The Y values are: {}".format(ws.readY(0)))
    print("The updated Error values are: {}".format(ws.readE(0)))

Output:

.. testoutput:: ExMultiplyWithSingleVal

    The X values are: [0. 1. 2. 3. 4. 5. 6. 7. 8. 9.]
    The Y values are: [ 0.   2.5  5.   7.5 10.  12.5 15.  17.5 20. ]
    The updated Error values are: [ 0.   2.5  5.   7.5 10.  12.5 15.  17.5 20. ]

.. categories::

.. sourcelink::
