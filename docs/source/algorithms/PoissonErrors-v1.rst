.. algorithm::

.. summary::

.. relatedalgorithms::

.. properties::

Description
-----------

Takes a Data workspace and an original counts workspace as input and updates the error values in the data workspace to be the same fractionally as the counts workspace. The number of histograms, the binning and units of the two workspaces must match.

Usage
-----

**Example - Perform basic error correction**

.. testcode:: ExPoissonSimple

    # create histogram workspaces
    dataX1 = [0,1,2,3,4,5,6,7,8,9] # or use dataX1=range(0,10)
    dataY1 = [0,1,2,3,4,5,6,7,8] # or use dataY1=range(0,9)
    dataE1 = [1,1,1,1,1,1,1,1,1] # or use dataE1=[1]*9
    dataX2 = [1,1,1,1,1,1,1,1,1,1]
    dataY2 = [2,2,2,2,2,2,2,2,2]
    dataE2 = [3,3,3,3,3,3,3,3,3]
    ws1 = CreateWorkspace(dataX1, dataY1, dataE1)
    ws2 = CreateWorkspace(dataX2, dataY2, dataE2)

    # perform the algorithm
    ws = PoissonErrors(ws1, ws2)

    #X-values aren't touched at all, Y-Values are used but not altered, E-Values are 0 if rhsY is 0 or (rhsE/rhsY)*lshY if they are non-zero
    print("The X values are: " + str(ws.readX(0)))
    print("The Y values are: " + str(ws.readY(0)))
    print("The updated Error values are: " + str(ws.readE(0)))

Output:

.. testoutput:: ExPoissonSimple

    The X values are: [0. 1. 2. 3. 4. 5. 6. 7. 8. 9.]
    The Y values are: [0. 1. 2. 3. 4. 5. 6. 7. 8.]
    The updated Error values are: [ 0.   1.5  3.   4.5  6.   7.5  9.  10.5 12. ]

.. categories::

.. sourcelink::
