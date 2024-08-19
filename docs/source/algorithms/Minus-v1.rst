.. algorithm::

.. summary::

.. relatedalgorithms::

.. properties::

Description
-----------

.. |sym| replace:: \-

.. |verb| replace:: subtracted

.. |prep| replace:: from

.. include:: BinaryOperation.txt

The Minus algorithm will subtract the data values and calculate the
corresponding `error values <Error Values>`__ for two compatible
workspaces.

Usage
-----

**Example - Minus as an Algorithm**

.. testcode:: ExMinusAsAlg

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
    ws = Minus(ws1, ws2)

    print("The X values are: {}".format(ws.readX(0)))
    print("The Y values are: {}".format(ws.readY(0)))
    print("The updated Error values are: {}".format(ws.readE(0)))

Output:

.. testoutput:: ExMinusAsAlg

    The X values are: [0. 1. 2. 3. 4. 5. 6. 7. 8. 9.]
    The Y values are: [-2. -1.  0.  1.  2.  3.  4.  5.  6.]
    The updated Error values are: [3.         3.16227766 3.60555128 4.24264069 5.         5.83095189
     6.70820393 7.61577311 8.54400375]

**Example - Minus as an operator**

.. testcode:: ExMinusAsOpperand

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
    ws = ws1 - ws2

    print("The X values are: {}".format(ws.readX(0)))
    print("The Y values are: {}".format(ws.readY(0)))
    print("The updated Error values are: {}".format(ws.readE(0)))

Output:

.. testoutput:: ExMinusAsOpperand

    The X values are: [0. 1. 2. 3. 4. 5. 6. 7. 8. 9.]
    The Y values are: [-2. -1.  0.  1.  2.  3.  4.  5.  6.]
    The updated Error values are: [3.         3.16227766 3.60555128 4.24264069 5.         5.83095189
     6.70820393 7.61577311 8.54400375]

**Example - Subtract using in-place operator**

.. testcode:: ExMinusInPlace

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
    ws -= ws1

    print("The X values are: {}".format(ws.readX(0)))
    print("The Y values are: {}".format(ws.readY(0)))
    print("The updated Error values are: {}".format(ws.readE(0)))

Output:

.. testoutput:: ExMinusInPlace

    The X values are: [0. 1. 2. 3. 4. 5. 6. 7. 8. 9.]
    The Y values are: [-2. -1.  0.  1.  2.  3.  4.  5.  6.]
    The updated Error values are: [3.         3.16227766 3.60555128 4.24264069 5.         5.83095189
     6.70820393 7.61577311 8.54400375]

**Example - Subtract a scalar**

.. testcode:: ExMinusWithSingleVal

    # create histogram workspaces
    dataX1 = [0,1,2,3,4,5,6,7,8,9] # or use dataX1=range(0,10)
    dataY1 = [0,1,2,3,4,5,6,7,8] # or use dataY1=range(0,9)
    dataE1 = [0,1,2,3,4,5,6,7,8] # or use dataE1=range(0,9)
    ws1 = CreateWorkspace(dataX1, dataY1, dataE1)

    # perform the algorithm
    ws = ws1 - 2.5

    print("The X values are: {}".format(ws.readX(0)))
    print("The Y values are: {}".format(ws.readY(0)))
    print("The updated Error values are: {}".format(ws.readE(0)))

Output:

.. testoutput:: ExMinusWithSingleVal

    The X values are: [0. 1. 2. 3. 4. 5. 6. 7. 8. 9.]
    The Y values are: [-2.5 -1.5 -0.5  0.5  1.5  2.5  3.5  4.5  5.5]
    The updated Error values are: [0. 1. 2. 3. 4. 5. 6. 7. 8.]

.. categories::

.. sourcelink::
