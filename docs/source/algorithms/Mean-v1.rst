.. algorithm::

.. summary::

.. relatedalgorithms::

.. properties::

Description
-----------

Calculates the arithmetic mean of the *y* values of the workspaces provided. If each input workspace is labelled :math:`w_i` and there
are *N* workspaces then the mean is computed as:

.. math::

   m = \frac{1}{N} \sum_{i=0}^{N-1} w_i

where *m* is the output workspace. The *x* values are copied from the first input workspace.

Restrictions
############

All input workspaces must have the same shape and the x axis must be in the same order.

Usage:
------

**Example: Simple mean of two workspaces**

.. testcode::

   # Create two  2-spectrum workspaces with Y values 1->8 & 3->10
   dataX = [0,1,2,3,4,
            0,1,2,3,4]
   dataY = [1,2,3,4,
            5,6,7,8]
   ws_1 = CreateWorkspace(dataX, dataY, NSpec=2)
   dataY = [3,4,5,6,
            7,8,9,10]
   ws_2 = CreateWorkspace(dataX, dataY, NSpec=2)

   result = Mean("ws_1, ws_2") # note the comma-separate strings
   print("Mean of y values in first spectrum: {}".format(result.readY(0)))
   print("Mean of y values in second spectrum: {}".format(result.readY(1)))

Output:

.. testoutput::

   Mean of y values in first spectrum: [2. 3. 4. 5.]
   Mean of y values in second spectrum: [6. 7. 8. 9.]

.. categories::

.. sourcelink::
