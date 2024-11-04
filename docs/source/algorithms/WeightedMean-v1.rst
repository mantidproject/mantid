.. algorithm::

.. summary::

.. relatedalgorithms::

.. properties::

Description
-----------

The algorithm calculates the weighted mean of two workspaces. This is useful when working with distributions rather than histograms, particularly when counting statistics are poor and it is possible that the value of one data set is statistically insignificant but differs greatly from the other. In such a case simply calculating the average of the two data sets would produce a spurious result.
If each input workspace and the standard deviation are labelled :math:`w_i` and :math:`sigma_i`, respectively, and there
are *N* workspaces then the weighted mean is computed as:

.. math::

   m = \frac{\sum_{i=0}^{N-1}\frac{w_i}{\sigma^{2}_i}}{\sum_{i=0}^{N-1}\frac{1}{\sigma^{2}_i}}

where *m* is the output workspace. The *x* values are copied from the first input workspace.

The input workspaces must be compatible with respect to size, units, and whether they are distributions or not.

Usage
-----

**Example - Perform a simple weighted mean**

.. testcode:: ExWMSimple

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
    ws = WeightedMean(ws1, ws2)

    print("The X values are: " + str(ws.readX(0)))
    print("The Y values are: " + str(ws.readY(0)))
    print("The E values are: " + str(ws.readE(0)))

Output:

.. testoutput:: ExWMSimple

    The X values are: [0. 1. 2. 3. 4. 5. 6. 7. 8. 9.]
    The Y values are: [0.2 1.1 2.  2.9 3.8 4.7 5.6 6.5 7.4]
    The E values are: [0.9486833 0.9486833 0.9486833 0.9486833 0.9486833 0.9486833 0.9486833
     0.9486833 0.9486833]

.. categories::

.. sourcelink::
