.. algorithm::

.. summary::

.. relatedalgorithms::

.. properties::

Description
-----------

The algorithm will raise the InputWorkspace to the power of the
Exponent. When acting on an event workspace, the output will be a
Workspace2D, with the default binning from the original workspace.

Errors
######

Defining the power algorithm as: :math:`y = \left ( a^b \right )`, we
can describe the error as: :math:`s_{y} = by\left ( s_{a}/a \right )`,
where :math:`s_{y}` is the error in the result *y* and :math:`s_{a}` is
the error in the input *a*.


Usage
-----

**Example - Square each Y value:**

.. testcode::

   # Create a 2 spectrum workspace with Y values 1->8
   dataX = [0,1,2,3,4,
            0,1,2,3,4]
   dataY = [1,2,3,4,
            5,6,7,8]
   data_ws = CreateWorkspace(dataX, dataY, NSpec=2)
   result_ws = Power(data_ws, 2)

   print("Squared values of first spectrum: {}".format(result_ws.readY(0)))
   print("Squared values of second spectrum: {}".format(result_ws.readY(1)))

Output:

.. testoutput::

   Squared values of first spectrum: [ 1.  4.  9. 16.]
   Squared values of second spectrum: [25. 36. 49. 64.]

.. categories::

.. sourcelink::
